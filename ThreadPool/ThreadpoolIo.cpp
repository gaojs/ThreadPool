#include "stdafx.h"

VOID _stdcall Func(PTP_CALLBACK_INSTANCE Instance,
	PVOID Context, PVOID Overlapped, ULONG IoResult,
	ULONG_PTR NumberOfBytesTransferred, PTP_IO Io)
{
	assert(NO_ERROR == IoResult);
	printf("I/O操作完成，传输字节数:%d\n", NumberOfBytesTransferred);
}

int main()
{
	HANDLE hFile = CreateFileA("1.txt", GENERIC_READ | GENERIC_WRITE, 
		0, NULL, OPEN_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	assert(INVALID_HANDLE_VALUE != hFile);
	PTP_IO pIo = CreateThreadpoolIo(hFile, Func, NULL, NULL);
	assert(pIo);
	StartThreadpoolIo(pIo);
	const char* pStrC = "Hello World";
	OVERLAPPED ov = {};
	BOOL nRet = WriteFile(hFile, pStrC,
		static_cast<DWORD>(strlen(pStrC)), NULL, &ov);
	if (!(!nRet && ERROR_IO_PENDING == GetLastError()))
	{
		CancelThreadpoolIo(pIo);
	}
	WaitForThreadpoolIoCallbacks(pIo, false);
	CloseThreadpoolIo(pIo);
	CloseHandle(hFile);

	system("pause");
	//程序输出:I/O操作完成，传输字节数:11
}