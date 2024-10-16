#include "LoggerTester.h"

VOID Write()
{
    DWORD Tid = GetCurrentThreadId();
    KernLogger& logger = KernLogger::GetLogger();

    std::ostringstream ss;

    ss << "Writing from thread: " << Tid;

    logger.LogInfo(ss.str().c_str());
}

VOID LoggerTester()
{
    std::cout << "Will test now the logger synchronization\n";
    HANDLE Threads[10];

    for (int i = 0; i < 10; i++)
    {
        DWORD ThreadId;
        Threads[i] = CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE)Write,
            NULL,
            0,
            &ThreadId);

        //printf("Created thread tid: %u\n", ThreadId);
    }

    WaitForMultipleObjects(10, Threads, TRUE, INFINITE);

    for (int i = 0; i < 10; i++)
    {
        CloseHandle(Threads[i]);
    }

}