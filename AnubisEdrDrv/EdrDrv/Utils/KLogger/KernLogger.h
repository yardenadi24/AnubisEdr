#pragma once
#include <Windows.h>

#define LOGGER_MUTEX L"LoggerMutex"

class KernLogger
{
public:
	static KernLogger& GetLogger();

	// Delete all constructors
	KernLogger(const KernLogger& other) = delete;
	KernLogger(KernLogger&& other) = delete;

	KernLogger& operator=(const KernLogger& other) = delete;
	KernLogger& operator=(KernLogger&& other) = delete;

	VOID RawLogUnsafe(const char* line);
	VOID LogInfo(const char* line);
	VOID LogError(const char* line);

private:
	KernLogger();
	~KernLogger();


	HANDLE hMutex_;
};