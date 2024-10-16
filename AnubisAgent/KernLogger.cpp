#include "KernLogger.h"



KernLogger& KernLogger::GetLogger()
{
	static KernLogger Logger;
	return Logger;
}


VOID KernLogger::RawLogUnsafe(const char* line)
{
	std::cout << line << std::endl;
}

VOID KernLogger::LogInfo(const char* line)
{
	WaitForSingleObject(hMutex_, INFINITE);
	std::cout << "<Info> ";
	RawLogUnsafe(line);
	ReleaseMutex(hMutex_);
}

VOID KernLogger::LogError(const char* line)
{
	WaitForSingleObject(hMutex_, INFINITE);
	std::cout << "<Erro> ";
	RawLogUnsafe(line);
	ReleaseMutex(hMutex_);
}


KernLogger::KernLogger()
{
	std::cerr << "Constructor " << std::endl;
	hMutex_ = CreateMutex(
		NULL,			// Default security attributes
		FALSE,			// Initially not owned
		NULL			// Mutex name
	);

	if (hMutex_ == NULL)
	{
		std::cerr << "Failed to create mutex. Error: " << GetLastError() << std::endl;
		exit(1); // Exit if we cannot create a mutex
	}
}


KernLogger::~KernLogger() { CloseHandle(hMutex_); }
