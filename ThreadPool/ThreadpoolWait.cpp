#include "stdafx.h"

void __stdcall Func(PTP_CALLBACK_INSTANCE, void* pVoid,
	PTP_WAIT pWait, TP_WAIT_RESULT nWaitResult)
{
	if (WAIT_OBJECT_0 == nWaitResult)
	{
		printf("%s\n", static_cast<const char*>(pVoid));
	}
	else if (WAIT_TIMEOUT == nWaitResult)
	{
		printf("Timeout\n");
	}
	else if (WAIT_ABANDONED_0 == nWaitResult)
	{
		printf("Abandoned\n");
	}
	else
	{
		assert(false);
	}
}

int main()
{
	const char* pStrC = "Hello";
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	PTP_WAIT pWait = CreateThreadpoolWait(Func,
		const_cast<char*>(pStrC), NULL);
	assert(pWait && hEvent);

	ULARGE_INTEGER nTem;
	nTem.QuadPart = -10000000 * 2;
	FILETIME FileTime;
	FileTime.dwLowDateTime = nTem.LowPart;
	FileTime.dwHighDateTime = nTem.HighPart;

	SetThreadpoolWait(pWait, hEvent, &FileTime);
	Sleep(3000);
	SetThreadpoolWait(pWait, hEvent, &FileTime);
	Sleep(1000);
	SetEvent(hEvent);
	Sleep(2000);

	WaitForThreadpoolWaitCallbacks(pWait, FALSE);
	CloseHandle(hEvent);
	CloseThreadpoolWait(pWait);

	/*
	运行结果:
	运行2s后输出"Timeout"
	运行4s后输出"Hello"
	运行6s后退出
	*/
}