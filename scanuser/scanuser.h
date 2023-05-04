#pragma once
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include <iostream>
#include <Windows.h>
#include <vector>
#include <thread>
#include <functional>
#include "fltuser.h"
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"fltlib.lib")
#define SCANNER_BUFFER 1024
using namespace std;
#pragma push(1)
class WorkerContext {
public:
	HANDLE port, IocpCore = NULL;
};
class ScannerMsg
{
public:
	ULONG size;
	UCHAR buffer[SCANNER_BUFFER];

};
class ScannerGetMsg {
public:
	FILTER_MESSAGE_HEADER header;
	ScannerMsg msg;
	OVERLAPPED ovlp;
	
};
class ReplyMsg
{
public:

	FILTER_REPLY_HEADER header;
	BOOLEAN safetoOpen;
};