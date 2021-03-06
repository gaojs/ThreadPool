#include "stdafx.h"

void CALLBACK Func(PTP_CALLBACK_INSTANCE pInstance, void* pVoid, PTP_WORK)
{
	printf("%s\n", static_cast<const char*>(pVoid));

	while (true)
	{
		Sleep(1);
	}
}

int main()
{
	const char* pStrC = "Hello World";

	PTP_POOL pPool = CreateThreadpool(NULL);
	assert(pPool);

	SetThreadpoolThreadMaximum(pPool, 1);
	SetThreadpoolThreadMinimum(pPool, 1);

	TP_CALLBACK_ENVIRON CallbackEnviron;
	InitializeThreadpoolEnvironment(&CallbackEnviron);
	SetThreadpoolCallbackPool(&CallbackEnviron, pPool);

	PTP_WORK pWork = CreateThreadpoolWork(Func, const_cast<char*>(pStrC),
		&CallbackEnviron);
	assert(pWork);

	SubmitThreadpoolWork(pWork);
	SubmitThreadpoolWork(pWork);
	WaitForThreadpoolWorkCallbacks(pWork, FALSE);

	CloseThreadpoolWork(pWork);
	CloseThreadpool(pPool);
	DestroyThreadpoolEnvironment(&CallbackEnviron);

	system("pause");
	/*
	程序运行结果:
	输出"Hello World"后无反应，进程开启的线程总数为2
	*/
}