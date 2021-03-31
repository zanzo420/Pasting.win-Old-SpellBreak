#pragma once
#pragma warning( disable : 4099 )

#include <ntdef.h>
#include <ntifs.h>

#include <ntifs.h>
#include <ntddk.h>
#include <windef.h>
#include <wdf.h>
#include <ntdef.h>
#include <ntimage.h>
#include <ntifs.h>
#include <intrin.h>


#include <ntdef.h>
#include <ntifs.h>
#include <ntifs.h>
#include <ntddk.h>
#include <windef.h>
#include <wdf.h>
#include <ntdef.h>

#include <ntimage.h>
#include <ntifs.h>
#include <intrin.h>

#include "Utils.h"


//DRIVER_INITIALIZE DriverInitialize;
//extern "C" DRIVER_INITIALIZE DriverEntry;
//#pragma alloc_text(INIT, DriverEntry)


#define IOCTL_DISK_BASE                 FILE_DEVICE_DISK
#define IOCTL_DISK_GET_DRIVE_GEOMETRY   CTL_CODE(IOCTL_DISK_BASE, 0x0000, METHOD_BUFFERED, FILE_ANY_ACCESS)


#define READ_PROCESS_MEMORY_IOCTL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2331, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define WRITE_PROCESS_MEMORY_IOCTL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2332, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define GET_PROCESS_BASE_IOCTL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2333, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define GET_PROCESS_GDI32_MODULE_IOCTL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2334, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define GET_PROCESS_PEB_IOCTL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2335, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define GET_MODULE_GAMEASSEMBLY_IOCTL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2336, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define GET_MODULE_UNITY_IOCTL CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2337, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)


struct CUSTOM_IOCTL_CALL
{
	ULONG64 Filter;
	ULONG ControlCode;

};

struct READ_PROCESS_MEMORY : CUSTOM_IOCTL_CALL
{
	unsigned __int64 ProcessId;
	unsigned __int64 ProcessAddress;
	unsigned __int64 Length;
	unsigned __int64 OutBuffer;
};

struct WRITE_PROCESS_MEMORY : CUSTOM_IOCTL_CALL
{
	unsigned __int64 ProcessId;
	unsigned __int64 ProcessAddress;
	unsigned __int64 Length;
	unsigned __int64 InBuffer;
};

struct GET_PROCESS_BASE : CUSTOM_IOCTL_CALL
{
	unsigned __int64 ProcessId;
	unsigned __int64 ProcessBaseAddres;
};

struct GET_PROCESS_PEB : CUSTOM_IOCTL_CALL
{
	unsigned __int64 ProcessId;
	unsigned __int64 ProcessBaseAddres;
};

typedef struct _PEB_LDR_DATA {
	ULONG Length;
	BOOLEAN Initialized;
	PVOID SsHandle;
	LIST_ENTRY ModuleListLoadOrder;
	LIST_ENTRY ModuleListMemoryOrder;
	LIST_ENTRY ModuleListInitOrder;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

extern "C" NTKERNELAPI
PPEB
PsGetProcessPeb(
	IN PEPROCESS Process
);

typedef void(__stdcall* PPS_POST_PROCESS_INIT_ROUTINE)(void); // not exported

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE Reserved1[16];
	PVOID Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, * PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Reserved3[2];
	PPEB_LDR_DATA Ldr;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
	PVOID Reserved4[3];
	PVOID AtlThunkSListPtr;
	PVOID Reserved5;
	ULONG Reserved6;
	PVOID Reserved7;
	ULONG Reserved8;
	ULONG AtlThunkSListPtr32;
	PVOID Reserved9[45];
	BYTE Reserved10[96];
	PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
	BYTE Reserved11[128];
	PVOID Reserved12[1];
	ULONG SessionId;
} PEB, * PPEB;

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;  // in bytes
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;  // LDR_*
	USHORT LoadCount;
	USHORT TlsIndex;
	LIST_ENTRY HashLinks;
	PVOID SectionPointer;
	ULONG CheckSum;
	ULONG TimeDateStamp;
	//    PVOID			LoadedImports;
	//    // seems they are exist only on XP !!! PVOID
	//    EntryPointActivationContext;	// -same-
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

#define Printf(...) DbgPrintEx( DPFLTR_SYSTEM_ID, DPFLTR_ERROR_LEVEL, "[zDriver] " __VA_ARGS__ )
