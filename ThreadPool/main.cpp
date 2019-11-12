//https://www.cnblogs.com/phpzhou/p/5941492.html
#include "stdafx.h"
#include <windows.h>
#include <iostream>
using namespace std;

void NTAPI poolThreadCallback(
	_Inout_ PTP_CALLBACK_INSTANCE Instance,
	_Inout_opt_ PVOID Context)
{
	int i = 10;
	while (i > 0)
	{
		printf("%d :%d\n", (DWORD)Context, i);
		Sleep(100);
		i--;
	}
}

void NTAPI poolThreadWork(
	_Inout_ PTP_CALLBACK_INSTANCE Instance,
	_Inout_opt_ PVOID Context,
	_Inout_ PTP_WORK Work)
{
	int i = 10;
	while (i > 0)
	{
		printf("%d :%d\n", (DWORD)Context, i);
		Sleep(100);
		i--;
	}
}

int main()
{
	//创建线程池
	PTP_POOL threadPool = CreateThreadpool(NULL);
	BOOL bRet = SetThreadpoolThreadMinimum(threadPool, 2);
	SetThreadpoolThreadMaximum(threadPool, 10);
	//初始化环境
	TP_CALLBACK_ENVIRON te;
	InitializeThreadpoolEnvironment(&te);
	SetThreadpoolCallbackPool(&te, threadPool);
	//创建线程
	//单次工作提交，以异步的方式运行函数，一次性任务
	for (size_t i = 1; i < 30; i++)
	{
		bRet = TrySubmitThreadpoolCallback(poolThreadCallback, 
			(PVOID)i, &te);
		if (!bRet)
		{
			bRet = GetLastError();
		}
	} 

	Sleep(5000); //只能等？
	//清理线程池的环境变量
	DestroyThreadpoolEnvironment(&te);
	//关闭线程池
	CloseThreadpool(threadPool);

	//SuspendThread();   //更改线程状态为悬挂
	//ResumeThread();    //恢复线程状态运行

	/*
	创建工作项
	*/
	//PTP_WORK pwk;
	//TP_CALLBACK_ENVIRON te;
	//InitializeThreadpoolEnvironment(&te);
	//pwk = CreateThreadpoolWork(poolThreadWork, NULL, &te);
	////提交工作项，可以提交多次
	//SubmitThreadpoolWork(pwk);
	//SubmitThreadpoolWork(pwk);
	////等待工作结束
	//WaitForThreadpoolWorkCallbacks(pwk, false);
	////关闭工作对象
	//CloseThreadpoolWork(pwk);

	system("pause");
	return 0;
}