#include "libtftp.h"
#include <string>

int main()
{
	const char *ip = "127.0.0.1";
	const char *filename = "Release";
	const char *destination = "file.Release";

	TftpClient TftpClient;

	std::string ErrorMessage;
	TftpClient::tftp_error_e Result = TftpClient.read(ip, filename, destination, ErrorMessage);

	printf("Downloaded %s: result %d, message: %s\n", filename, (int)Result, ErrorMessage.c_str());

	return (int)Result;
}