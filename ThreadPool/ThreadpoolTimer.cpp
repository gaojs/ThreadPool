#include "stdafx.h"

void __stdcall Func(PTP_CALLBACK_INSTANCE, 
	void* pVoid, PTP_TIMER Timer)
{
	printf("%s\n", static_cast<const char*>(pVoid));
}

int main()
{
	const char* pStrC = "Hello";
	PTP_TIMER pTimer = CreateThreadpoolTimer(Func,
		const_cast<char*>(pStrC), NULL);
	assert(pTimer);

	ULARGE_INTEGER nTem;
	FILETIME FileTime;
	nTem.QuadPart = -1 * 20000000;
	FileTime.dwHighDateTime = nTem.HighPart;
	FileTime.dwLowDateTime = nTem.LowPart;
	/*
	FileTime.dwHighDateTime = 4294967295
	FileTime.dwLowDateTime = 4274967296
	*/
	SetThreadpoolTimer(pTimer, &FileTime, 1000, NULL);
	Sleep(5000);
	WaitForThreadpoolTimerCallbacks(pTimer, FALSE);
	CloseThreadpoolTimer(pTimer);
	return 0;
	/*
	在程序执行2秒后，输出Hello，
		然后每隔1秒输出Hello，
	一共输出3个Hello，程序结束
	*/
}