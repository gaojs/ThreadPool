/*==========================================================================
* 类ThreadPool是本代码的核心类，类中自动维护线程池的创建和任务队列的派送
* 其中的TaskFun是任务函数
* 其中的TaskCallbackFun是回调函数
*用法：定义一个ThreadPool变量，TaskFun函数和TaskCallbackFun回调函数，
 然后调用ThreadPool的QueueTaskItem()函数即可
Author: TTGuoying
Date: 2018/02/19 23:15
Revised by GaoJS,at 2019/11/11
==========================================================================*/
#pragma once
#include <list>
#include <queue>
#include <memory>
using namespace std; 
#include "stdafx.h"

#define THRESHOLE_OF_WAIT_TASK 20

typedef int(*TaskFunc)(PVOID param); // 任务函数
typedef void(*TaskCallbackFunc)(int result); // 回调函数

class ThreadPool
{
private:
	// 线程类(内部类)
	class Thread
	{
	public:
		ThreadPool* threadPool; // 所属线程池
		TaskCallbackFunc taskCb; // 回调的任务
		TaskFunc taskFunc; // 要执行的任务
		PVOID param; // 任务参数
		HANDLE hThread; // 线程句柄
		BOOL bBusy; // 是否有任务在执行
		BOOL bExit; // 是否退出

	public:
		Thread(ThreadPool* threadPool);
		~Thread();

		BOOL isBusy(); // 是否有任务在执行
		void ExecuteTask(TaskFunc task, PVOID param,
			TaskCallbackFunc taskCallback); // 执行任务
		// 线程函数
		static unsigned int __stdcall ThreadProc(PVOID pMgr);
	};

	//线程临界区锁
	class CriticalSectionLock
	{
	private:
		CRITICAL_SECTION cs;//临界区
	public:
		CriticalSectionLock() { InitializeCriticalSection(&cs); }
		~CriticalSectionLock() { DeleteCriticalSection(&cs); }
		void Lock() { EnterCriticalSection(&cs); }
		void UnLock() { LeaveCriticalSection(&cs); }
	};

	// IOCP的通知种类
	enum class WAIT_OPERATION_TYPE
	{
		GET_TASK,
		EXIT,
	};

	// 待执行的任务类
	class WaitTask
	{
	public:
		TaskCallbackFunc taskCb; // 回调的任务
		TaskFunc taskFunc; // 要执行的任务
		PVOID param; // 任务参数
		BOOL bLongTask; // 是否为长任务

	public:
		WaitTask(TaskFunc taskFunc, PVOID param,
			TaskCallbackFunc taskCb, BOOL bLongTask)
		{
			this->taskCb = taskCb;
			this->taskFunc = taskFunc;
			this->param = param;
			this->bLongTask = bLongTask;
		}
		~WaitTask()
		{
			taskCb = NULL;
			taskFunc = NULL;
			param = NULL;
			bLongTask = FALSE;
		}
	};

	// 从任务列表取任务的线程函数
	static unsigned int __stdcall GetTaskThreadProc(PVOID pMgr);

public:
	ThreadPool(size_t minNumOfThread = 2, size_t maxNumOfThread = 10);
	~ThreadPool();

	BOOL QueueTaskItem(TaskFunc taskFunc, PVOID param,
		TaskCallbackFunc taskCb = NULL,
		BOOL longTask = FALSE); // 任务入队

private:
	size_t GetCurNumOfThread();// 获取线程池中的当前线程数
	size_t GetMaxNumOfThread(); // 获取线程池中的最大线程数
	size_t GetMinNumOfThread();// 获取线程池中的最小线程数
	size_t GetIdleThreadNum(); // 获取线程池中的空闲线程数
	size_t GetBusyThreadNum();// 获取线程池中的繁忙线程数
	void CreateIdleThread(size_t size); // 创建空闲线程
	void DeleteIdleThread(size_t size); // 删除空闲线程
	void SetMaxNumOfThread(size_t size); // 设置线程池中的最大线程数
	void SetMinNumOfThread(size_t size); // 设置线程池中的最小线程数
	void MoveThreadToIdleList(Thread* busyThread); // 移入空闲列表	
	void MoveThreadToBusyList(Thread* thread); // 移入忙碌列表
	void GetTaskExecute(); // 从任务队列中取任务执行
	Thread* GetIdleThread(); // 获取空闲线程
	WaitTask* GetTask(); // 从任务队列中取任务

	size_t maxNumOfThread; // 线程池中最大的线程数
	size_t minNumOfThread; // 线程池中最小的线程数
	size_t numOfLongTask; // 线程池中的长任务数
	HANDLE hCompletionPort; // 完成端口
	HANDLE hDispatchThread; // 分发任务线程
	HANDLE hStopEvent; // 通知线程退出的事件
	CriticalSectionLock idleThreadLock; // 空闲线程列表锁
	list<Thread*> idleThreadList; // 空闲线程列表
	CriticalSectionLock busyThreadLock; // 忙碌线程列表锁
	list<Thread*> busyThreadList; // 忙碌线程列表
	CriticalSectionLock waitTaskLock;
	list<WaitTask*> waitTaskList; // 任务列表
};