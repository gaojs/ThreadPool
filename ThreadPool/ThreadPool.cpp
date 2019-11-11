#include "stdafx.h"
#include "ThreadPool.h"

// 从任务列表取任务的线程函数
unsigned int __stdcall ThreadPool::GetTaskThreadProc(PVOID pThiz)
{
	printf("GetTaskThreadProc()\n");
	ThreadPool* threadPool = (ThreadPool*)pThiz;	
	//没有退出，则循环取任务并执行
	while (!threadPool->bExit)
	{
		DWORD dwBytes = 0;
		OVERLAPPED* pOverLapped = NULL;
		WAIT_OPERATION_TYPE opType = WAIT_OPERATION_TYPE::GET_TASK;
		BOOL bRet = GetQueuedCompletionStatus(threadPool->hCompletionPort,
			&dwBytes, (PULONG_PTR)&opType, &pOverLapped, INFINITE);
		// 收到退出标志
		if (WAIT_OPERATION_TYPE::EXIT == opType)
		{
			printf("GetTaskThreadProc() break\n");
			break;
		}
		else if (WAIT_OPERATION_TYPE::GET_TASK == opType)
		{
			threadPool->GetTaskExecute();
		}
	}
	printf("_endthreadex()\n");
	_endthreadex(0);
	return 0;
}

ThreadPool::ThreadPool(size_t minNumOfThread, size_t maxNumOfThread)
{
	printf("ThreadPool()\n"); 
	this->minNumOfThread = minNumOfThread < 2 ? 2 : minNumOfThread;
	this->maxNumOfThread = (this->minNumOfThread * 2 > maxNumOfThread)
		? this->minNumOfThread * 2 : maxNumOfThread;
	this->numOfLongTask = 0;
	this->hCompletionPort = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE, NULL, 0, 1);
	this->hDispatchThread = (HANDLE)_beginthreadex(0, 0,
		GetTaskThreadProc, this, 0, 0);
	this->idleThreadList.clear();
	this->busyThreadList.clear();
	this->waitTaskList.clear();
	this->bExit = FALSE;
	CreateIdleThread(this->minNumOfThread);
}

ThreadPool::~ThreadPool()
{
	printf("~ThreadPool()\n");
	this->bExit = TRUE;
	BOOL bRet = CloseHandle(this->hDispatchThread);
	bRet = PostQueuedCompletionStatus(hCompletionPort,
		0, (DWORD)WAIT_OPERATION_TYPE::EXIT, NULL);
	bRet = CloseHandle(this->hCompletionPort);
	this->DeleteIdleThreadAll();
	this->DeleteBusyThreadAll();
	this->DeleteWaitTaskAll();
}

BOOL ThreadPool::QueueTaskItem(TaskFunc task, PVOID param,
	TaskCallbackFunc taskCb, BOOL bLongTask)
{
	waitTaskLock.Lock();
	WaitTask* waitTask = new WaitTask(task, param, taskCb, bLongTask);
	waitTaskList.push_back(waitTask);
	waitTaskLock.UnLock();
	BOOL bRet = PostQueuedCompletionStatus(hCompletionPort,
		0, (DWORD)WAIT_OPERATION_TYPE::GET_TASK, NULL);
	return TRUE;
}

void ThreadPool::CreateIdleThread(size_t size)
{
	idleThreadLock.Lock();
	for (size_t i = 0; i < size; i++)
	{
		idleThreadList.push_back(new Thread(this));
	}
	idleThreadLock.UnLock();
}

void ThreadPool::DeleteIdleThread(size_t size)
{
	idleThreadLock.Lock();
	size_t t = idleThreadList.size();
	t = (t < size) ? t : size;
	for (size_t i = 0; i < t; i++)
	{
		auto thread = idleThreadList.back();
		delete thread;
		idleThreadList.pop_back();
	}
	idleThreadLock.UnLock();
}

void ThreadPool::DeleteIdleThreadAll()
{
	idleThreadLock.Lock();
	size_t t = idleThreadList.size();
	for (size_t i = 0; i < t; i++)
	{
		auto thread = idleThreadList.back();
		delete thread;
		idleThreadList.pop_back();
	}
	idleThreadLock.UnLock();
}

void ThreadPool::DeleteBusyThreadAll()
{
	busyThreadLock.Lock();
	size_t t = busyThreadList.size();
	for (size_t i = 0; i < t; i++)
	{
		auto thread = busyThreadList.back();
		delete thread;
		busyThreadList.pop_back();
	}
	busyThreadLock.UnLock();
}

void ThreadPool::DeleteWaitTaskAll()
{
	waitTaskLock.Lock();
	size_t t = waitTaskList.size();
	for (size_t i = 0; i < t; i++)
	{
		auto thread = waitTaskList.back();
		delete thread;
		waitTaskList.pop_back();
	}
	waitTaskLock.UnLock();
}

ThreadPool::Thread* ThreadPool::GetIdleThread()
{
	Thread* thread = NULL;
	idleThreadLock.Lock();
	if (idleThreadList.size() > 0)
	{
		thread = idleThreadList.front();
		idleThreadList.pop_front();
	}
	idleThreadLock.UnLock();

	if (thread == NULL && GetCurNumOfThread() < maxNumOfThread)
	{
		thread = new Thread(this);
	}
	if (thread == NULL && waitTaskList.size() > THRESHOLE_OF_WAIT_TASK)
	{
		thread = new Thread(this);
		InterlockedIncrement(&maxNumOfThread);
	}
	return thread;
}

void ThreadPool::MoveThreadToIdleList(Thread* busyThread)
{
	idleThreadLock.Lock();
	idleThreadList.push_back(busyThread);
	idleThreadLock.UnLock();

	busyThreadLock.Lock();
	for (auto it = busyThreadList.begin();
		it != busyThreadList.end(); it++)
	{
		if (*it == busyThread)
		{
			busyThreadList.erase(it);
			break;
		}
	}
	busyThreadLock.UnLock();

	if (maxNumOfThread != 0
		&& idleThreadList.size() > maxNumOfThread * 0.8)
	{
		DeleteIdleThread(idleThreadList.size() / 2);
	}

	BOOL bRet = PostQueuedCompletionStatus(hCompletionPort,
		0, (DWORD)WAIT_OPERATION_TYPE::GET_TASK, NULL);
}

void ThreadPool::MoveThreadToBusyList(Thread* thread)
{
	busyThreadLock.Lock();
	busyThreadList.push_back(thread);
	busyThreadLock.UnLock();
}

void ThreadPool::GetTaskExecute()
{
	Thread* thread = NULL;
	WaitTask* waitTask = NULL;

	waitTask = GetTask();
	if (waitTask == NULL)
	{
		return;
	}

	if (waitTask->bLongTask)
	{
		if (idleThreadList.size() > minNumOfThread)
		{
			thread = GetIdleThread();
		}
		else
		{
			thread = new Thread(this);
			InterlockedIncrement(&numOfLongTask);
			InterlockedIncrement(&maxNumOfThread);
		}
	}
	else
	{
		thread = GetIdleThread();
	}

	if (thread != NULL)
	{
		thread->ExecuteTask(waitTask->taskFunc,
			waitTask->param, waitTask->taskCb);
		delete waitTask;
		MoveThreadToBusyList(thread);
	}
	else
	{
		waitTaskLock.Lock();
		waitTaskList.push_front(waitTask);
		waitTaskLock.UnLock();
	}
}

ThreadPool::WaitTask* ThreadPool::GetTask()
{
	ThreadPool::WaitTask* waitTask = NULL;
	waitTaskLock.Lock();
	if (waitTaskList.size() > 0)
	{
		waitTask = waitTaskList.front();
		waitTaskList.pop_front();
	}
	waitTaskLock.UnLock();
	return waitTask;
}

ThreadPool::Thread::Thread(ThreadPool* threadPool):
	threadPool(threadPool), hThread(INVALID_HANDLE_VALUE),
	taskCb(NULL), taskFunc(NULL), param(0),
	bBusy(FALSE), bExit(FALSE)
{
	printf("Thread()\n");
	hThread = (HANDLE)_beginthreadex(0, 0,
		ThreadProc, this, CREATE_SUSPENDED, 0);
}

ThreadPool::Thread::~Thread()
{
	printf("~Thread()\n");
	bExit = TRUE;
	bBusy = FALSE;
	taskCb = NULL;
	taskFunc = NULL;
	threadPool = NULL;
	DWORD dwRet = ResumeThread(hThread);
	dwRet = WaitForSingleObject(hThread, INFINITE);
	BOOL bRet = CloseHandle(hThread);
	hThread = NULL;
}

BOOL ThreadPool::Thread::isBusy()
{
	return bBusy;
}

void ThreadPool::Thread::ExecuteTask(TaskFunc taskFunc,
	PVOID param, TaskCallbackFunc taskCallback)
{
	this->taskCb = taskCallback;
	this->taskFunc = taskFunc;
	this->param = param;
	this->bBusy = TRUE;
	DWORD dwRet = ResumeThread(hThread);
}

unsigned int ThreadPool::Thread::ThreadProc(PVOID pThiz)
{
	Thread* pThread = (Thread*)pThiz;
	while (true)
	{
		if (pThread->bExit)
		{
			break; //线程退出
		}
		if (pThread->taskFunc == NULL)
		{
			pThread->bBusy = FALSE;
			pThread->threadPool->MoveThreadToIdleList(pThread);
			SuspendThread(pThread->hThread);
			continue;
		}

		int result = pThread->taskFunc(pThread->param);
		if (pThread->taskCb)
		{
			pThread->taskCb(pThread->param, result);
		}
		WaitTask* waitTask = pThread->threadPool->GetTask();
		if (waitTask != NULL)
		{
			pThread->taskCb = waitTask->taskCb;
			pThread->taskFunc = waitTask->taskFunc;
			pThread->param = waitTask->param;
			delete waitTask;
			continue;
		}
		else
		{
			pThread->taskCb = NULL;
			pThread->taskFunc = NULL;
			pThread->param = NULL;
			pThread->bBusy = FALSE;
			pThread->threadPool->MoveThreadToIdleList(pThread);
			DWORD dwRet = SuspendThread(pThread->hThread);
		}
	}
	return 0;
}

size_t ThreadPool::GetCurNumOfThread()
{ // 获取线程池中的当前线程数
	return GetIdleThreadNum() + GetBusyThreadNum();
}

size_t ThreadPool::GetMaxNumOfThread()
{ // 获取线程池中的最大线程数
	return maxNumOfThread - numOfLongTask;
}

void ThreadPool::SetMaxNumOfThread(size_t size)
{ // 设置线程池中的最大线程数
	if (size < numOfLongTask)
	{
		maxNumOfThread = size + numOfLongTask;
	}
	else
	{
		maxNumOfThread = size;
	}
}

size_t ThreadPool::GetMinNumOfThread()
{ // 获取线程池中的最小线程数
	return minNumOfThread;
}

void ThreadPool::SetMinNumOfThread(size_t size)
{ // 设置线程池中的最小线程数
	minNumOfThread = size;
}

size_t ThreadPool::GetIdleThreadNum()
{ // 获取线程池中的空闲线程数
	return idleThreadList.size();
}

size_t ThreadPool::GetBusyThreadNum()
{ // 获取线程池中的繁忙线程数
	return busyThreadList.size();
}