
#include "MyScanner.h"
PFLT_FILTER FilterHandle = NULL;
PFLT_PORT ServerPort = NULL;
PFLT_PORT cP = NULL;

const UNICODE_STRING ScannerExtensionsToScan[] =
{ RTL_CONSTANT_STRING(L"doc"),
  RTL_CONSTANT_STRING(L"txt"),
  RTL_CONSTANT_STRING(L"bat"),
  RTL_CONSTANT_STRING(L"cmd"),
  RTL_CONSTANT_STRING(L"inf"),
	/*RTL_CONSTANT_STRING( L"ini"),   Removed, to much usage*/
	{0, 0, NULL}
};
BOOLEAN
ScannerpCheckExtension(
	__in PUNICODE_STRING Extension
)
{
	const UNICODE_STRING* ext;

	if (Extension->Length == 0) {

		return FALSE;
	}

	

	ext = ScannerExtensionsToScan;

	while (ext->Buffer != NULL) {

		if (RtlCompareUnicodeString(Extension, ext, TRUE) == 0) {

			return TRUE;
		}
		ext++;
	}

	return FALSE;
}
FLT_PREOP_CALLBACK_STATUS ScanPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID *CompletionContext)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	PAGED_CODE();

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}
NTSTATUS
ScanFileInUserMode(
	__in PWCHAR pwFilePath,
	__in ULONG cbFilePath,
	__in PFLT_INSTANCE Instance,
	__in PFILE_OBJECT FileObject,
	__out PBOOLEAN SafeToOpen
)
{
	NTSTATUS status;
	PFLT_VOLUME volume;
	ULONG length;
	FLT_VOLUME_PROPERTIES volumeProps;
	PVOID buffer=NULL;
	LARGE_INTEGER offset;
	ULONG bytesRead;
	ULONG replyLength;
	ScannerMsg* msg;
	UNREFERENCED_PARAMETER(SafeToOpen);
	UNREFERENCED_PARAMETER(pwFilePath);
	UNREFERENCED_PARAMETER(cbFilePath);
	*SafeToOpen = true;
	if (cP == NULL)
	{
		
		return STATUS_SUCCESS;
	}

	status = FltGetVolumeFromInstance(Instance, &volume);
	if (!NT_SUCCESS(status))
	{
		return STATUS_SUCCESS;
	}

	status = FltGetVolumeProperties(volume,
		&volumeProps,
		sizeof(volumeProps),
		&length);


	if (NT_ERROR(status)) {
		return STATUS_SUCCESS;
	}

	length = max(SCANNER_BUFFER, volumeProps.SectorSize);

	buffer = FltAllocatePoolAlignedWithTag(Instance, NonPagedPool, length, 'nacS');
	if (NULL == buffer)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	msg =(ScannerMsg*) ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(ScannerMsg), 'nacS');
	if (msg == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(msg, sizeof(ScannerMsg));
	offset.QuadPart = bytesRead = 0;
	status = FltReadFile(Instance, FileObject, &offset, length, buffer, FLTFL_IO_OPERATION_NON_CACHED | FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET
		, &bytesRead, NULL, NULL
	);

	if (!(NT_SUCCESS(status) && (0 != bytesRead)))
	{
		return STATUS_SUCCESS;
	}
	msg->size = bytesRead;
	RtlCopyMemory(msg->buffer, buffer, min(msg->size,SCANNER_BUFFER));
	replyLength = sizeof(ReplyMsg);

	status = FltSendMessage(FilterHandle, &cP, msg, sizeof(ScannerMsg), msg, &replyLength, NULL);
	
	if (status == STATUS_SUCCESS)
	{
		DbgPrint("Success send Msg\r\n");
		DbgBreakPoint();
	}
	else
	{
		DbgPrint("Failed send Msg %x\r\n",status);
	}
	ExFreePool(msg);

	return status;

}
FLT_POSTOP_CALLBACK_STATUS
ScanPostCreate(
	__inout PFLT_CALLBACK_DATA Data,
	__in PCFLT_RELATED_OBJECTS FltObjects,
	__in_opt PVOID CompletionContext,
	__in FLT_POST_OPERATION_FLAGS Flags
){
	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION nameInfo;
	BOOLEAN scanFile = NULL;
	BOOLEAN SafeToOpen = TRUE;
	ULONG cbFilePath = 0;
	WCHAR wFilePath[512] = { 0, };
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(Flags);

	if (!NT_SUCCESS(Data->IoStatus.Status) ||
		(STATUS_REPARSE == Data->IoStatus.Status)) {

		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &nameInfo);
	if (!NT_SUCCESS(status))
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	FltParseFileNameInformation(nameInfo);
	scanFile = ScannerpCheckExtension(&nameInfo->Extension);
	if (scanFile)
	{
		cbFilePath = min((512 - 1) * sizeof(WCHAR), nameInfo->Name.Length);
		RtlCopyMemory(wFilePath, nameInfo->Name.Buffer, cbFilePath);
	}
	FltReleaseFileNameInformation(nameInfo);
	if (scanFile) {
		DbgPrint("PostCreate %ws \r\n", wFilePath);
		ScanFileInUserMode(wFilePath,cbFilePath,FltObjects->Instance,FltObjects->FileObject,&SafeToOpen);
	}


	return FLT_POSTOP_FINISHED_PROCESSING;
}
NTSTATUS ScanUnload(FLT_FILTER_UNLOAD_FLAGS flags)
{
	UNREFERENCED_PARAMETER(flags);
	FltUnregisterFilter(FilterHandle);

	return STATUS_SUCCESS;
}
const FLT_OPERATION_REGISTRATION Callbacks[] = {
	{IRP_MJ_CREATE, 0, ScanPreCreate,(PFLT_POST_OPERATION_CALLBACK)ScanPostCreate},
	{IRP_MJ_OPERATION_END}
};

const FLT_REGISTRATION FilterRegistration = {

	sizeof(FLT_REGISTRATION),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags
	NULL,                //  Context Registration.
	Callbacks,                          //  Operation callbacks
	ScanUnload,                      //  FilterUnload
	NULL,
	NULL,
	NULL,                               //  InstanceTeardownStart
	NULL,                               //  InstanceTeardownComplete
	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent
};
NTSTATUS
ScannerPortConnect(
	__in PFLT_PORT ClientPort,
	__in_opt PVOID ServerPortCookie,
	__in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
	__in ULONG SizeOfContext,
	__deref_out_opt PVOID* ConnectionCookie
)
{
	PAGED_CODE();

	cP = ClientPort;
	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie);
	
	return STATUS_SUCCESS;
}

VOID
ScannerPortDisconnect(
	__in_opt PVOID ConnectionCookie
)
{

	UNREFERENCED_PARAMETER(ConnectionCookie);
	FltCloseClientPort(FilterHandle, &cP);
}
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;
	UNICODE_STRING uniString;
	OBJECT_ATTRIBUTES ObjAtt;
	PSECURITY_DESCRIPTOR SecDes;
	UNREFERENCED_PARAMETER(pRegistryPath);
	
	status = FltRegisterFilter(DriverObject, &FilterRegistration, &FilterHandle);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	RtlInitUnicodeString(&uniString, L"\\Scanner");
	status = FltBuildDefaultSecurityDescriptor(&SecDes, FLT_PORT_ALL_ACCESS);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	InitializeObjectAttributes(&ObjAtt, &uniString, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, SecDes);
	status = FltCreateCommunicationPort(FilterHandle, &ServerPort, &ObjAtt, NULL, ScannerPortConnect, ScannerPortDisconnect, NULL, 1);
	
	FltFreeSecurityDescriptor(SecDes);
	
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	status = FltStartFiltering(FilterHandle);
	if (!NT_SUCCESS(status))
	{
		FltCloseCommunicationPort(ServerPort);
		FltUnregisterFilter(FilterHandle);
		return status;

	}

	return status;


}