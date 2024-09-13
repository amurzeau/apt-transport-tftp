#pragma once

#include <memory>
#include <string>
#include <vector>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

class unique_fd
{
	public:
	constexpr unique_fd() noexcept = default;
	explicit unique_fd(int fd) noexcept : fd_(fd) {}
	unique_fd(unique_fd &&u) noexcept : fd_(u.fd_) { u.fd_ = -1; }

	~unique_fd()
	{
		if (-1 != fd_)
			::close(fd_);
	}

	unique_fd &operator=(unique_fd &&u) noexcept
	{
		reset(u.release());
		return *this;
	}

	int get() const noexcept { return fd_; }
	operator int() const noexcept { return fd_; }

	int release() noexcept
	{
		int fd = fd_;
		fd_ = -1;
		return fd;
	}
	void reset(int fd = -1) noexcept { unique_fd(fd).swap(*this); }
	void swap(unique_fd &u) noexcept { std::swap(fd_, u.fd_); }

	unique_fd(const unique_fd &) = delete;
	unique_fd &operator=(const unique_fd &) = delete;

	// in the global namespace to override ::close(int)
	friend int close(unique_fd &u) noexcept
	{
		int closed = ::close(u.fd_);
		u.fd_ = -1;
		return closed;
	}

	private:
	int fd_ = -1;
};

class TftpClient
{
	public:
	enum tftp_error_e
	{
		TFTP_SUCCESS,
		TFTP_ERROR_FILE_NOT_FOUND,
		TFTP_ERROR_ACCESS_VIOLATION,
		TFTP_ERROR_DISK_FULL_OR_ALLOCATION_EXCEEDED,
		TFTP_ERROR_ILLEGAL_TFTP_OPERATION,
		TFTP_ERROR_UNKNOWN_TRANSFER_ID,
		TFTP_ERROR_FILE_ALREADY_EXISTS,
		TFTP_ERROR_NO_SUCH_USER,

		TFTP_ERROR_INTERNAL_ERROR
	};

	tftp_error_e read(const char *ip, const char *filename, const char *destination_path, std::string &error_message);

	protected:
	tftp_error_e return_error(std::string &error_message);

	bool initialize_sockets();
	bool read_packet(std::vector<uint8_t> &buffer, bool &is_timeout);
	bool send_packet(const std::vector<uint8_t> &buffer);
	bool send_request(const char *ip, const char *filename);
	bool send_ack(size_t last_data_packet_block);
	void parse_oack(const std::vector<uint8_t> &buffer);

	private:
	unique_fd local_sockfd;
	struct sockaddr_in remote_addr;
	unsigned int remote_addr_len;
	size_t block_size = 512;

	enum tftp_error_e tftp_error;
	std::string error_message;
};
