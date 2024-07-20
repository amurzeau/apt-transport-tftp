// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   Copy URI - This method takes a uri like a file: uri and copies it
   to the destination file.

   ##################################################################### */
/*}}}*/
// Include Files							/*{{{*/
#include <apt-pkg/acquire-method.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/error.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/hashes.h>
#include <apt-pkg/strutl.h>

#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

class aptMethod : public pkgAcqMethod
{
	protected:
	std::string const Binary;
	unsigned long SeccompFlags;
	enum Seccomp
	{
		BASE = (1 << 1),
		NETWORK = (1 << 2),
		DIRECTORY = (1 << 3),
	};

	public:
	virtual bool Configuration(std::string Message) APT_OVERRIDE
	{
		if (pkgAcqMethod::Configuration(Message) == false)
			return false;

		std::string const conf = std::string("Binary::") + Binary;
		_config->MoveSubTree(conf.c_str(), NULL);

		DropPrivsOrDie();

		return true;
	}

	bool CalculateHashes(FetchItem const *const Itm, FetchResult &Res) const APT_NONNULL(2)
	{
		Hashes Hash(Itm->ExpectedHashes);
		FileFd Fd;
		if (Fd.Open(Res.Filename, FileFd::ReadOnly) == false || Hash.AddFD(Fd) == false)
			return false;
		Res.TakeHashes(Hash);
		return true;
	}

	void Warning(std::string &&msg)
	{
		std::unordered_map<std::string, std::string> fields;
		if (Queue != 0)
			fields.emplace("URI", Queue->Uri);
		else
			fields.emplace("URI", "<UNKNOWN>");
		if (not UsedMirror.empty())
			fields.emplace("UsedMirror", UsedMirror);
		fields.emplace("Message", std::move(msg));
		SendMessage("104 Warning", std::move(fields));
	}

	bool TransferModificationTimes(char const *const From, char const *const To, time_t &LastModified) APT_NONNULL(2, 3)
	{
		if (strcmp(To, "/dev/null") == 0)
			return true;

		struct stat Buf2;
		if (lstat(To, &Buf2) != 0 || S_ISLNK(Buf2.st_mode))
			return true;

		struct stat Buf;
		if (stat(From, &Buf) != 0)
			return _error->Errno("stat", "Failed to stat");

		// we don't use utimensat here for compatibility reasons: #738567
		struct timeval times[2];
		times[0].tv_sec = Buf.st_atime;
		LastModified = times[1].tv_sec = Buf.st_mtime;
		times[0].tv_usec = times[1].tv_usec = 0;
		if (utimes(To, times) != 0)
			return _error->Errno("utimes", "Failed to set modification time");
		return true;
	}

	// This is a copy of #pkgAcqMethod::Dequeue which is private & hidden
	void Dequeue()
	{
		FetchItem const *const Tmp = Queue;
		Queue = Queue->Next;
		if (Tmp == QueueBack)
			QueueBack = Queue;
		delete Tmp;
	}
	static std::string URIEncode(std::string const &part)
	{
		// The "+" is encoded as a workaround for an S3 bug (LP#1003633 and LP#1086997)
		return QuoteString(part, _config->Find("Acquire::URIEncode", "+~ ").c_str());
	}

	static std::string DecodeSendURI(std::string const &part)
	{
		if (_config->FindB("Acquire::Send-URI-Encoded", false))
			return DeQuoteString(part);
		return part;
	}

	aptMethod(std::string &&Binary, char const *const Ver, unsigned long const Flags) APT_NONNULL(3)
		: pkgAcqMethod(Ver, Flags), Binary(std::move(Binary)), SeccompFlags(0)
	{
		try
		{
			std::locale::global(std::locale(""));
		}
		catch (...)
		{
			setlocale(LC_ALL, "");
		}
	}
};
/*}}}*/

class TftpMethod : public aptMethod
{
	virtual bool Fetch(FetchItem *Itm) APT_OVERRIDE;

	public:
	TftpMethod() : aptMethod("tftp", "1.0", SendURIEncoded)
	{
		SeccompFlags = aptMethod::BASE;
	}
};

// TftpMethod::Fetch - Fetch a file					/*{{{*/
// ---------------------------------------------------------------------
/* */
bool TftpMethod::Fetch(FetchItem *Itm)
{
	URI Get(Itm->Uri);
	auto const File = DecodeSendURI(Get.Path);

	// Formulate a result and send a start message
	FetchResult Res;
	Res.Filename = Itm->DestFile;
	URIStart(Res);

	_error->Debug("Downloading %s", Res.Filename.c_str());

	pid_t Process = ExecFork();
	if (Process == 0)
	{
		if (Get.Port == 0)
			Get.Port = 69; // Default TFTP port
		std::string Port = std::to_string(Get.Port);

		// Invoke tftp to download the file
		const char *Args[16];
		unsigned int i = 0;
		Args[i++] = "atftp";
		Args[i++] = "--get";
		Args[i++] = "--local-file";
		Args[i++] = Itm->DestFile.c_str();
		Args[i++] = "--remote-file";
		Args[i++] = File.c_str();
		Args[i++] = Get.Host.c_str();
		Args[i++] = Port.c_str();
		Args[i++] = NULL;
		execvp(Args[0], (char **)Args);
		exit(127);
	}

	if (!ExecWait(Process, "atftp", false))
	{
		Fail(true);
		return true;
	}

	CalculateHashes(Itm, Res);
	URIDone(Res);

	return true;
}
/*}}}*/

int main()
{
	return TftpMethod().Run();
}