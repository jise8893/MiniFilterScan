#pragma once
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <bcrypt.h>
#define SCANNER_BUFFER 1024

#pragma push(1)
class ReplyMsg
{
public:
	FILTER_REPLY_HEADER header;
	BOOLEAN safetoOpen;
};

class ScannerMsg
{
public:
	ULONG size;
	UCHAR buffer[SCANNER_BUFFER];
	
};
