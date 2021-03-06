#include "stdafx.h"

#pragma comment(lib, "ws2_32")

struct SIoContext
{
	SIoContext() : sock(INVALID_SOCKET)
	{
		memset(&OverLapped, 0, sizeof(OverLapped));
		wsBuff.buf = buff;
		wsBuff.len = sizeof(buff);
	}

	SOCKET sock;
	OVERLAPPED OverLapped;
	WSABUF wsBuff;
	char buff[65536];
};

void InitNetEnvironment()
{
	WSAData wsaData;
	if (0 == WSAStartup(MAKEWORD(2, 2), &wsaData)
		&& wsaData.wVersion == 0x0202)
	{
		return;
	}
	assert(false);
}

VOID _stdcall Func(PTP_CALLBACK_INSTANCE Instance,
	PVOID Context, PVOID Overlapped, ULONG IoResult,
	ULONG_PTR NumberOfBytesTransferred, PTP_IO Io)
{
	assert(NO_ERROR == IoResult);
	SIoContext* pIoContext = (SIoContext*)Context;
	if (NumberOfBytesTransferred > 0)
	{
		pIoContext->buff[NumberOfBytesTransferred] = 0;
		printf("%s\n", pIoContext->buff);

		StartThreadpoolIo(Io);
		DWORD nFlag = 0;
		if (!WSARecv(pIoContext->sock, &pIoContext->wsBuff,
			1, nullptr, &nFlag, &pIoContext->OverLapped, NULL))
		{
			CancelThreadpoolIo(Io);
		}
	}
	else
	{
		printf("连接中断\n");
	}
}

int main()
{
	InitNetEnvironment();

	SIoContext IoContext;
	IoContext.sock = socket(AF_INET, SOCK_STREAM, 0);
	assert(INVALID_SOCKET != IoContext.sock);

	sockaddr_in sockAddr = {};
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(8888);
	if (1 != inet_pton(AF_INET, "127.0.0.1",
		&sockAddr.sin_addr.s_addr))
	{
		assert(false);
	}

	int nRet = connect(IoContext.sock,
		reinterpret_cast<sockaddr*>(&sockAddr), sizeof(sockaddr));
		//(sockaddr*)(&sockAddr), sizeof(sockaddr));
	assert(SOCKET_ERROR != nRet);

	//PTP_IO pIo = CreateThreadpoolIo((HANDLE)IoContext.sock,
	PTP_IO pIo = CreateThreadpoolIo(
		reinterpret_cast<HANDLE>(IoContext.sock),
		Func, &IoContext, NULL);
	assert(pIo);

	StartThreadpoolIo(pIo);

	DWORD nFlag = 0;
	if (!WSARecv(IoContext.sock, &IoContext.wsBuff,
		1, nullptr, &nFlag, &IoContext.OverLapped, NULL))
	{
		CancelThreadpoolIo(pIo);
	}

	system("pause");

	WaitForThreadpoolIoCallbacks(pIo, false);
	CloseThreadpoolIo(pIo);
	closesocket(IoContext.sock);
	WSACleanup();
	/*
	运行结果:
	先启动服务器，然后服务器先发送"Hello"，
		然后再发送"World"，然后断开连接
	本软件输出:
	"Hello"
	"World"
	"连接中断"
	*/
}