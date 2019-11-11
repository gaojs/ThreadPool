/*==========================================================================
* ��ThreadPool�Ǳ�����ĺ����࣬�����Զ�ά���̳߳صĴ�����������е�����
* ���е�TaskFun��������
* ���е�TaskCallbackFun�ǻص�����
*�÷�������һ��ThreadPool������TaskFun������TaskCallbackFun�ص�������
 Ȼ�����ThreadPool��QueueTaskItem()��������
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

typedef int(*TaskFunc)(PVOID param); // ������
typedef void(*TaskCallbackFunc)(int result); // �ص�����

class ThreadPool
{
private:
	// �߳���(�ڲ���)
	class Thread
	{
	public:
		ThreadPool* threadPool; // �����̳߳�
		TaskCallbackFunc taskCb; // �ص�������
		TaskFunc taskFunc; // Ҫִ�е�����
		PVOID param; // �������
		HANDLE hThread; // �߳̾��
		BOOL bBusy; // �Ƿ���������ִ��
		BOOL bExit; // �Ƿ��˳�

	public:
		Thread(ThreadPool* threadPool);
		~Thread();

		BOOL isBusy(); // �Ƿ���������ִ��
		void ExecuteTask(TaskFunc task, PVOID param,
			TaskCallbackFunc taskCallback); // ִ������
		// �̺߳���
		static unsigned int __stdcall ThreadProc(PVOID pMgr);
	};

	//�߳��ٽ�����
	class CriticalSectionLock
	{
	private:
		CRITICAL_SECTION cs;//�ٽ���
	public:
		CriticalSectionLock() { InitializeCriticalSection(&cs); }
		~CriticalSectionLock() { DeleteCriticalSection(&cs); }
		void Lock() { EnterCriticalSection(&cs); }
		void UnLock() { LeaveCriticalSection(&cs); }
	};

	// IOCP��֪ͨ����
	enum class WAIT_OPERATION_TYPE
	{
		GET_TASK,
		EXIT,
	};

	// ��ִ�е�������
	class WaitTask
	{
	public:
		TaskCallbackFunc taskCb; // �ص�������
		TaskFunc taskFunc; // Ҫִ�е�����
		PVOID param; // �������
		BOOL bLongTask; // �Ƿ�Ϊ������

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

	// �������б�ȡ������̺߳���
	static unsigned int __stdcall GetTaskThreadProc(PVOID pMgr);

public:
	ThreadPool(size_t minNumOfThread = 2, size_t maxNumOfThread = 10);
	~ThreadPool();

	BOOL QueueTaskItem(TaskFunc taskFunc, PVOID param,
		TaskCallbackFunc taskCb = NULL,
		BOOL longTask = FALSE); // �������

private:
	size_t GetCurNumOfThread();// ��ȡ�̳߳��еĵ�ǰ�߳���
	size_t GetMaxNumOfThread(); // ��ȡ�̳߳��е�����߳���
	size_t GetMinNumOfThread();// ��ȡ�̳߳��е���С�߳���
	size_t GetIdleThreadNum(); // ��ȡ�̳߳��еĿ����߳���
	size_t GetBusyThreadNum();// ��ȡ�̳߳��еķ�æ�߳���
	void CreateIdleThread(size_t size); // ���������߳�
	void DeleteIdleThread(size_t size); // ɾ�������߳�
	void SetMaxNumOfThread(size_t size); // �����̳߳��е�����߳���
	void SetMinNumOfThread(size_t size); // �����̳߳��е���С�߳���
	void MoveThreadToIdleList(Thread* busyThread); // ��������б�	
	void MoveThreadToBusyList(Thread* thread); // ����æµ�б�
	void GetTaskExecute(); // �����������ȡ����ִ��
	Thread* GetIdleThread(); // ��ȡ�����߳�
	WaitTask* GetTask(); // �����������ȡ����

	size_t maxNumOfThread; // �̳߳��������߳���
	size_t minNumOfThread; // �̳߳�����С���߳���
	size_t numOfLongTask; // �̳߳��еĳ�������
	HANDLE hCompletionPort; // ��ɶ˿�
	HANDLE hDispatchThread; // �ַ������߳�
	HANDLE hStopEvent; // ֪ͨ�߳��˳����¼�
	CriticalSectionLock idleThreadLock; // �����߳��б���
	list<Thread*> idleThreadList; // �����߳��б�
	CriticalSectionLock busyThreadLock; // æµ�߳��б���
	list<Thread*> busyThreadList; // æµ�߳��б�
	CriticalSectionLock waitTaskLock;
	list<WaitTask*> waitTaskList; // �����б�
};