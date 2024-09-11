#include "libtftp.h"
#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

TftpClient::tftp_error_e TftpClient::read(const char *ip, const char *filename, const char *destination_path, std::string &error_message)
{
	if (!initialize_sockets())
		return return_error(error_message);

	std::unique_ptr<FILE, int (*)(FILE *)> destination_file(nullptr, &fclose);

	destination_file.reset(fopen(destination_path, "wb"));
	if (!destination_file)
	{
		error_message = std::string("failed to open destination file: ") + destination_path;
		return TFTP_ERROR_INTERNAL_ERROR;
	}

	std::vector<uint8_t> buffer;
	size_t last_data_packet_block = 0;
	size_t timeout_count = 0;
	bool end_of_transfer = false;
	while (true)
	{
		bool is_timeout;

		// Send ACK for previous packet
		if (last_data_packet_block == 0)
		{
			if (!send_request(ip, filename))
				return return_error(error_message);
		}
		else
		{
			send_ack(last_data_packet_block);
		}

		if (end_of_transfer)
		{
			break;
		}

		if (!read_packet(buffer, is_timeout))
		{
			if (is_timeout)
			{
				timeout_count++;
				if (timeout_count < 10)
					continue;
				error_message = "timeout waiting for tftp reply";
			}
			return return_error(error_message);
		}

		timeout_count = 0;

		int opcode = (buffer[0] << 8) | buffer[1];
		switch (opcode)
		{
		case 3: // DATA packet
		{
			if (buffer.size() < 4)
			{
				error_message = "failed parse data packet";
				return TFTP_ERROR_ILLEGAL_TFTP_OPERATION;
			}

			uint16_t current_block = (buffer[2] << 8) | buffer[3];
			if (current_block == last_data_packet_block)
			{
				// Already processed
				break;
			}
			last_data_packet_block = current_block;

			if (buffer.size() > 4)
			{
				if (fwrite(&buffer[4], 1, buffer.size() - 4, destination_file.get()) < 0)
				{
					error_message = std::string("failed to write destination file: ") + destination_path;
					return TFTP_ERROR_INTERNAL_ERROR;
				}
			}
			if (buffer.size() < 516)
			{
				// DATA of less than 512 bytes => Last packet
				end_of_transfer = true;
			}
			break;
		}
		case 5: // ERROR packet
			error_message = std::string("transfer error: ");
			tftp_error = (enum tftp_error_e)((buffer[2] << 8) | buffer[3]);

			buffer[buffer.size() - 1] = 0;

			switch (tftp_error)
			{
			case TFTP_SUCCESS:
				error_message += std::string((const char *)&buffer[4]);
				break;
			case TFTP_ERROR_FILE_NOT_FOUND:
				error_message += "File Not Found";
				break;
			case TFTP_ERROR_ACCESS_VIOLATION:
				error_message += "Access Violation";
				break;
			case TFTP_ERROR_DISK_FULL_OR_ALLOCATION_EXCEEDED:
				error_message += "Disk Full Or Allocation Exceeded";
				break;
			case TFTP_ERROR_ILLEGAL_TFTP_OPERATION:
				error_message += "Illegal Tftp Operation";
				break;
			case TFTP_ERROR_UNKNOWN_TRANSFER_ID:
				error_message += "Unknown Transfer Id";
				break;
			case TFTP_ERROR_FILE_ALREADY_EXISTS:
				error_message += "File Already Exists";
				break;
			case TFTP_ERROR_NO_SUCH_USER:
				error_message += "No Such User";
				break;
			case TFTP_ERROR_INTERNAL_ERROR:
				error_message += "Internal Error";
				break;
			}
			return tftp_error;
		default: // Unexpected packet
			error_message = std::string("invalid opcode received: ") + std::to_string(opcode);
			return TFTP_ERROR_ILLEGAL_TFTP_OPERATION;
			break;
		}
	}

	destination_file.reset();

	return TFTP_SUCCESS;
}

TftpClient::tftp_error_e TftpClient::return_error(std::string &error_message)
{
	error_message = this->error_message;
	return tftp_error;
}

bool TftpClient::initialize_sockets()
{
	local_sockfd.reset(socket(AF_INET, SOCK_DGRAM, 0));
	if (local_sockfd.get() < 0)
	{
		error_message = "socket creation failed";
		tftp_error = TFTP_ERROR_INTERNAL_ERROR;
		return false;
	}

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));

	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(0); // Use random available port

	// Bind the socket with the server address
	if (bind(local_sockfd, (const struct sockaddr *)&servaddr,
			 sizeof(servaddr)) < 0)
	{
		error_message = "bind failed";
		tftp_error = TFTP_ERROR_INTERNAL_ERROR;
		return false;
	}

	// Set timeout to 1s
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(local_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
	{
		error_message = "set socket timeout failed";
		tftp_error = TFTP_ERROR_INTERNAL_ERROR;
		return false;
	}

	return true;
}

bool TftpClient::read_packet(std::vector<uint8_t> &buffer, bool &is_timeout)
{
	int ret;
	buffer.resize(512 + 4);
	do
	{
		ret = recvfrom(local_sockfd, (char *)buffer.data(), buffer.size(),
					   0, (struct sockaddr *)&remote_addr,
					   &remote_addr_len);
	} while (ret < 0 && errno == EINTR);

	if (ret >= 0)
	{
		buffer.resize(ret);
		return true;
	}

	is_timeout = errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS;

	return false;
}

bool TftpClient::send_packet(const std::vector<uint8_t> &buffer)
{
	int ret;
	do
	{
		ret = sendto(local_sockfd, (const char *)buffer.data(), buffer.size(),
					 0, (const struct sockaddr *)&remote_addr,
					 remote_addr_len);
	} while (ret < 0 && errno == EINTR);

	if (ret >= 0)
	{
		return true;
	}

	error_message = strerror(errno);

	return false;
}

bool TftpClient::send_request(const char *ip, const char *filename)
{
	static const char mode[] = "octet";
	std::vector<uint8_t> buffer;

	// Opcode: RRQ = 1
	buffer.push_back(0);
	buffer.push_back(1);

	buffer.insert(buffer.end(), filename, filename + strlen(filename));
	buffer.push_back(0);

	buffer.insert(buffer.end(), mode, mode + sizeof(mode));

	memset(&remote_addr, 0, sizeof(remote_addr));

	// Initialize server information
	remote_addr.sin_family = AF_INET; // IPv4
	inet_pton(AF_INET, ip, &remote_addr.sin_addr);
	remote_addr.sin_port = htons(69);
	remote_addr_len = sizeof(remote_addr);

	return send_packet(buffer);
}

bool TftpClient::send_ack(size_t last_data_packet_block)
{
	std::vector<uint8_t> buffer;

	// Opcode: ACK = 4
	buffer.push_back(0);
	buffer.push_back(4);

	// Block #
	buffer.push_back((last_data_packet_block >> 8) & 0xFF);
	buffer.push_back(last_data_packet_block & 0xFF);

	return send_packet(buffer);
}