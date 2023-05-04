#pragma once
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#define SCANNER_BUFFER 1024
#pragma push(1)
class ScannerMsg
{
public:
	ULONG size;
	UCHAR buffer[SCANNER_BUFFER];
	
};
class ReplyMsg
{
public:
	FILTER_REPLY_HEADER header;
	BOOLEAN safetoOpen;
};