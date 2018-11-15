#include <iostream>
#include <windows.h>
#include <process.h>

/*** https://docs.microsoft.com/zh-cn/windows/desktop/sync/using-semaphore-objects ***/

typedef void (CALLBACK* OPERATIONFUNC)(); //函数做参数

volatile long g_value = 0;


//createthread 参数
unsigned long WINAPI ThreadFun(PVOID operfun)
{
	OPERATIONFUNC op = (OPERATIONFUNC)operfun;
	op();
	return 0;
}
//封装createthread
void MyThreads(unsigned int threadCounts, PVOID operationFun)
{
	HANDLE* pThreads = new HANDLE[threadCounts];
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	for (unsigned int kk = 0; kk < threadCounts; kk++)
	{
		pThreads[kk] = CreateThread(NULL, 0, ThreadFun, operationFun, 0, NULL);
	}
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
	WaitForMultipleObjects(threadCounts, pThreads, TRUE, INFINITE);
	for (unsigned int kk = 0; kk < threadCounts; kk++)
	{
		CloseHandle(pThreads[kk]);
	}
	delete pThreads;
}
//原子操作函数
void WINAPI InterLockedIncrementCallBack()
{
	InterlockedIncrement(&g_value);
}
//关键代码段, 需要使用该锁的线程都要看到它 critical_section
CRITICAL_SECTION g_cs;
void WINAPI CriticalSectionCallBack()
{
	EnterCriticalSection(&g_cs);
	g_value++;
	LeaveCriticalSection(&g_cs);
}

//互斥对象 mutex
HANDLE g_mutex;
void WINAPI MutexCallBack()
{
	WaitForSingleObject(g_mutex,INFINITE);
	g_value++;
	ReleaseMutex(g_mutex);
}

//读写锁 SRWlock
SRWLOCK g_srwlock;

//读锁 
void WINAPI SRWLockReadCallBack()
{
	AcquireSRWLockShared(&g_srwlock);
	std::cout << g_value << std::endl;
	ReleaseSRWLockShared(&g_srwlock);
}
//写锁
void WINAPI SRQLockWriteCallBack()
{
	AcquireSRWLockExclusive(&g_srwlock);
	g_value++;
	ReleaseSRWLockExclusive(&g_srwlock);
}

//事件 event  回调函数 做线程函数， 回调函数带参数，将回调函数及其参数一起封装
//利用event 循环顺序打印A B C  5次
HANDLE g_event[3];
void WINAPI EventPrintACallBack(unsigned int kk)
{
	unsigned int  count = 0;
	while (true)
	{
		WaitForSingleObject(g_event[0], INFINITE);
		std::cout << GetCurrentThreadId() << ":" << "A" << std::endl;
		Sleep(50);
		SetEvent(g_event[1]);
		count++;
		if (count > 5)
		{
			break;
		}
	}
}
void WINAPI EventPrintBCallBack(unsigned int kk)
{
	unsigned int  count = 0;
	while (true)
	{
		WaitForSingleObject(g_event[1], INFINITE);
		std::cout << GetCurrentThreadId() << ":" << "B" << std::endl;
		Sleep(500);
		SetEvent(g_event[2]);
		count++;
		if (count > 5)
		{
			break;
		}
	}
}
void WINAPI EventPrintCCallBack(unsigned int kk)
{
	unsigned int  count = 0;
	while (true)
	{
		WaitForSingleObject(g_event[2], INFINITE);
		std::cout << GetCurrentThreadId() << ":" << "C" << std::endl;
		Sleep(500);
		SetEvent(g_event[0]);
		count++;
		if (count > 5)
		{
			break;
		}
	}
}
typedef void (CALLBACK* OPERFUNWITHPARA)(unsigned int kk);
typedef struct THREADFUNPARA
{
	OPERFUNWITHPARA opfun;
	unsigned int kk;
}THREADFUNPARA,*PTHREADFUNPARA;

unsigned long WINAPI ThreadFunWithPara(PVOID opfunandpara)
{
	PTHREADFUNPARA op = (PTHREADFUNPARA)opfunandpara;
	unsigned int temp = op->kk;
	op->opfun(temp);
	return 0;
}
void MyThreadsPara(unsigned int threadCounts)
{
	HANDLE* pThreads = new HANDLE[threadCounts];
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);



	THREADFUNPARA funandpara0;
	OPERFUNWITHPARA fun0 = EventPrintACallBack;
	funandpara0.opfun = fun0;
	funandpara0.kk = 0;
	pThreads[0] = CreateThread(NULL, 0, ThreadFunWithPara, (void*)&funandpara0, CREATE_SUSPENDED, NULL);
	

	THREADFUNPARA funandpara1;
	OPERFUNWITHPARA fun1 = EventPrintBCallBack;
	funandpara1.opfun = fun1;
	funandpara1.kk = 1;
	pThreads[1] = CreateThread(NULL, 0, ThreadFunWithPara, (void*)&funandpara1, CREATE_SUSPENDED, NULL);
	

	THREADFUNPARA funandpara2;
	OPERFUNWITHPARA fun2 = EventPrintCCallBack;
	funandpara2.opfun = fun2;
	funandpara2.kk = 2;
	pThreads[2] = CreateThread(NULL, 0, ThreadFunWithPara, (void*)&funandpara2, CREATE_SUSPENDED, NULL);
	

	
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

	SetEvent(g_event[0]);
	ResumeThread(pThreads[0]);
	ResumeThread(pThreads[1]);
	ResumeThread(pThreads[2]);
	

	WaitForMultipleObjects(threadCounts, pThreads, TRUE, INFINITE);

	CloseHandle(pThreads[0]);
	CloseHandle(pThreads[1]);
	CloseHandle(pThreads[2]);
	delete pThreads;
}



//信号 semaphore

#define MAX_SEM_COUNT 10
#define THREADCOUNT 12

HANDLE ghSemaphore;

DWORD WINAPI ThreadProc(LPVOID);
DWORD WINAPI ThreadProc(LPVOID lpParam)
{

	// lpParam not used in this example
	UNREFERENCED_PARAMETER(lpParam);

	DWORD dwWaitResult;
	BOOL bContinue = TRUE;

	while (bContinue)
	{
		// Try to enter the semaphore gate.

		dwWaitResult = WaitForSingleObject(
			ghSemaphore,   // handle to semaphore
			0L);           // zero-second time-out interval  OR infinite all acquire

		switch (dwWaitResult)
		{
			// The semaphore object was signaled.
		case WAIT_OBJECT_0:
			// TODO: Perform task
			printf("Thread %d: wait succeeded\n", GetCurrentThreadId());
			bContinue = FALSE;

			// Simulate thread spending time on task
			Sleep(5);

			// Release the semaphore when task is finished

			if (!ReleaseSemaphore(
				ghSemaphore,  // handle to semaphore
				1,            // increase count by one
				NULL))       // not interested in previous count
			{
				printf("ReleaseSemaphore error: %d\n", GetLastError());
			}
			break;

			// The semaphore was nonsignaled, so a time-out occurred.
		case WAIT_TIMEOUT:
			printf("Thread %d: wait timed out\n", GetCurrentThreadId());
			break;
		}
	}
	return TRUE;
}



int main(int argc, char** argv)
{
	/*******************/
	std::cout << "test atom fun" << std::endl;
	MyThreads(10, InterLockedIncrementCallBack);
	
	/*******************/
	std::cout << "test critical section" << std::endl;
	InitializeCriticalSection(&g_cs);
	MyThreads(10, CriticalSectionCallBack);
	DeleteCriticalSection(&g_cs);

	/*****************/
	std::cout << "test mutex" << std::endl;
	g_mutex = CreateMutex(NULL,false,NULL);
	MyThreads(10, MutexCallBack);
	CloseHandle(g_mutex);


	/*****************/
	std::cout << "test SRWlock" << std::endl;
	InitializeSRWLock(&g_srwlock);
	MyThreads(10, SRWLockReadCallBack);
	MyThreads(10, SRQLockWriteCallBack);
	
	
	/***********************/
	std::cout << "test event" << std::endl;
	for (unsigned int kk = 0; kk < 3; kk++)
	{
		g_event[kk]  = CreateEvent(NULL,FALSE,FALSE,NULL);
	}
	MyThreadsPara(3);

	/**************************/
	std::cout << "test semaphore" << std::endl;
	HANDLE aThread[THREADCOUNT];
	DWORD ThreadID;
	int i;

	// Create a semaphore with initial and max counts of MAX_SEM_COUNT

	ghSemaphore = CreateSemaphore(
		NULL,           // default security attributes
		MAX_SEM_COUNT,  // initial count
		MAX_SEM_COUNT,  // maximum count
		NULL);          // unnamed semaphore

	if (ghSemaphore == NULL)
	{
		printf("CreateSemaphore error: %d\n", GetLastError());
		return 1;
	}

	// Create worker threads

	for (i = 0; i < THREADCOUNT; i++)
	{
		aThread[i] = CreateThread(
			NULL,       // default security attributes
			0,          // default stack size
			(LPTHREAD_START_ROUTINE)ThreadProc,
			NULL,       // no thread function arguments
			0,          // default creation flags
			&ThreadID); // receive thread identifier

		if (aThread[i] == NULL)
		{
			printf("CreateThread error: %d\n", GetLastError());
			return 1;
		}
	}

	// Wait for all threads to terminate

	WaitForMultipleObjects(THREADCOUNT, aThread, TRUE, INFINITE);

	// Close thread and semaphore handles

	for (i = 0; i < THREADCOUNT; i++)
		CloseHandle(aThread[i]);

	CloseHandle(ghSemaphore);

	system("pause");
	return 0;
}