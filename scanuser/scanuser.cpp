#include "scanuser.h"

DWORD WorkerThread(WorkerContext * Context)
{
	HANDLE port = Context->port;
	HANDLE IocpCore = Context->IocpCore;
	DWORD dwTrBytes = 0;
	ULONG_PTR ulKey = 0;
	LPOVERLAPPED pOvlp;
	BOOL result;
	ScannerGetMsg* msg;
	ReplyMsg repmsg;
	while (true) {
		result = GetQueuedCompletionStatus(IocpCore, &dwTrBytes, &ulKey, &pOvlp, INFINITE);
		msg = CONTAINING_RECORD(pOvlp, ScannerGetMsg, ovlp); //ovlp에서 [ Header |  ScannerMsg  | ovlp   ] povlp에서 구조체내의 ovlp 와 맞춰서 상위 구조체를 찾는 매크로 
		printf("Recevied message size %d\n", pOvlp->InternalHigh);

		printf("%s\n", msg->msg.buffer);

		repmsg.header.MessageId = msg->header.MessageId;
		repmsg.header.Status = 0;
		repmsg.safetoOpen = true;
		HRESULT hr = FilterReplyMessage(port, (PFILTER_REPLY_HEADER)&repmsg, sizeof(ReplyMsg));
		if (SUCCEEDED(hr))
		{
			printf("Replied Msg\n");
		}
		else {
			printf("UnReplied Msg\n");
		}

		memset(&msg->ovlp, 0, sizeof(OVERLAPPED));
		hr = FilterGetMessage(port, &msg->header, FIELD_OFFSET(ScannerGetMsg, ovlp), &msg->ovlp);
		if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
			msg = NULL;
			cout << "Failed FilterGetMessage\n";
		}
			
	}

	return 0;
}

int main()
{
	HANDLE port, IocpCore = NULL;
	const DWORD threadCounts = 1;
	HANDLE threads[threadCounts];
	HRESULT hr;
	WorkerContext context;
	DWORD threadId;
	shared_ptr<ScannerGetMsg> msg[threadCounts];
	hr = FilterConnectCommunicationPort(L"\\Scanner", 0, NULL, 0, NULL, &port);
	if(IS_ERROR(hr))
	{
		printf("error\n");
		MessageBoxA(0, 0, 0, 0);

	}
	context.port = port;
	IocpCore = CreateIoCompletionPort(port, NULL, 0, threadCounts);
	context.IocpCore = IocpCore;
	if (IocpCore == NULL)
	{
		std::cout << "Failed Creating IOCP Core\n";
	}
	std::cout << "Successed Creating IOCP Core\n";

	for (int i = 0; i < threadCounts; i++)
	{
		threads[i]=CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)WorkerThread, &context, 0, &threadId);
		if (threads[i] == NULL)
		{
			printf("Error : Couldn't create thread:%d \n", GetLastError());
		}

	}
	for (int i = 0; i < threadCounts; i++)
	{
		msg[i] = make_shared<ScannerGetMsg>();
		memset(&msg[i]->ovlp, 0, sizeof(OVERLAPPED));
		HRESULT hr=FilterGetMessage(port, &msg[i]->header, FIELD_OFFSET(ScannerGetMsg, ovlp), &msg[i]->ovlp);

		if (hr != HRESULT_FROM_WIN32(ERROR_IO_PENDING)) {
			msg[i] = NULL;
		}
		cout << "Registration GetMessage "<< i<<endl;
	}
	WaitForMultipleObjectsEx(threadCounts, threads, TRUE, INFINITE, FALSE);

	return 0;
}