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
		printf("%zd :%d\n", (size_t)Context, i);
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
		printf("%zd :%d\n", (size_t)Context, i);
		Sleep(100);
		i--;
	}
}

int main()
{
	//初始化环境
	TP_CALLBACK_ENVIRON te;
	InitializeThreadpoolEnvironment(&te);
	//创建线程池
	PTP_POOL threadPool = CreateThreadpool(NULL);
	BOOL bRet = SetThreadpoolThreadMinimum(threadPool, 2);
	SetThreadpoolThreadMaximum(threadPool, 2);
	SetThreadpoolCallbackPool(&te, threadPool);
	//创建线程
	//单次工作提交，以异步的方式运行函数，一次性任务
	//for (size_t i = 1; i < 30; i++)
	//{
	//	bRet = TrySubmitThreadpoolCallback(poolThreadCallback, 
	//		(PVOID)i, &te);
	//	if (!bRet)
	//	{
	//		bRet = GetLastError();
	//	}
	//} 
	//Sleep(5000); //只能等？

	PTP_CLEANUP_GROUP cleanupGroup = NULL;
	cleanupGroup = CreateThreadpoolCleanupGroup();
	SetThreadpoolCallbackCleanupGroup(&te, cleanupGroup, NULL);
	//创建工作项
	const size_t N = 3; //数量
	PTP_WORK pWork[N] = { 0 };
	for (size_t i = 0; i < N; i++)
	{
		printf("CreateThreadpoolWork(%zd)\n", i);
		pWork[i] = CreateThreadpoolWork(poolThreadWork, (PVOID)i, &te);
		if (pWork[i] != NULL)
		{
			//提交工作项，可以提交多次
			printf("SubmitThreadpoolWork(%zd)\n", i);
			SubmitThreadpoolWork(pWork[i]);
		}
		else
		{
			printf("CreateThreadpoolWork(%zd) FAIL!\n", i);
		}
	}
	//size_t n = N - 1;
	////中途取消某项，在执行了，无效果
	//if (pWork[n] != NULL)
	//{
	//	//Releases the specified work object.
	//	//The work object is freed immediately 
	//	//	if there are no outstanding(未完成的) callbacks; 
	//	//otherwise, the work object is freed asynchronously 
	//	//	after the outstanding callbacks complete.
	//	printf("CloseThreadpoolWork(%zd)\n", n);
	//	CloseThreadpoolWork(pWork[n]);
	//	pWork[n] = NULL;
	//}
	//n = N - 2;
	////等待工作结束
	/*if (pWork[n] != NULL)
	{
		printf("WaitForThreadpoolWorkCallbacks(%zd)\n", n);
		WaitForThreadpoolWorkCallbacks(pWork[n], false);
	}*/
	////关闭工作对象
	/*for (size_t i = 0; i < N; i++)
	{
		if (pWork[i] != NULL)
		{
			printf("CloseThreadpoolWork(%zd)\n", i);
			CloseThreadpoolWork(pWork[i]);
			pWork[i] = NULL;
		}
	}*/
	printf("CloseThreadpoolCleanupGroupMembers()\n");
	//CloseThreadpoolCleanupGroupMembers(cleanupGroup, FALSE, NULL);
	CloseThreadpoolCleanupGroupMembers(cleanupGroup, TRUE, NULL);
	//清理线程池的环境变量
	printf("DestroyThreadpoolEnvironment()\n");
	DestroyThreadpoolEnvironment(&te);
	//关闭线程池
	printf("CloseThreadpool()\n");
	CloseThreadpool(threadPool);

	system("pause");
	return 0;
}