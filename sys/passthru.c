/*++
Copyright (c) 1992  Microsoft Corporation
 
Module Name:
 
	passthru.c

Abstract:

	Ndis Intermediate Miniport driver sample. This is a passthru driver.

Author:

Environment:

Revision History:
--*/

#include "precomp.h"
#pragma hdrstop

#pragma NDIS_INIT_FUNCTION(DriverEntry)

NDIS_PHYSICAL_ADDRESS			HighestAcceptableMax = NDIS_PHYSICAL_ADDRESS_CONST(-1, -1);
NDIS_HANDLE						ProtHandle = NULL;
NDIS_HANDLE						DriverHandle = NULL;
NDIS_MEDIUM						MediumArray[3] =
									{
										NdisMedium802_3,	// Ethernet
										NdisMedium802_5,	// Token-ring
										NdisMediumFddi		// Fddi
									};

PADAPT  pAdaptList = NULL;

static NTSTATUS MydrvDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp); 
static NTSTATUS MydrvDispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp); 

NDIS_HANDLE NdisDeviceHandle;
PVOID gpEventObject = NULL;			// ��Ӧ�ó���ͨ�ŵ� Event ����
extern UINT Monitor_flag;			// ���ӱ�־��1->���ӣ�0->������
extern UINT Filt_flag;				// ���˱�־��1->���ˣ�0->������
UCHAR filt[13];						// �������飬��Ź�������

// �����ڴ�ָ��
PVOID SystemVirtualAddress, UserVirtualAddress;
PMDL Mdl;

NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT		DriverObject,
	IN	PUNICODE_STRING		RegistryPath
	)
{
	NDIS_STATUS						Status;
	NDIS_PROTOCOL_CHARACTERISTICS	PChars;
	NDIS_MINIPORT_CHARACTERISTICS	MChars;
	PNDIS_CONFIGURATION_PARAMETER	Param;
	NDIS_STRING						Name;
	NDIS_HANDLE						WrapperHandle;

	UNICODE_STRING nameString, linkString; 
	UINT FuncIndex;
	PDEVICE_OBJECT MyDeviceObject; 
	PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];

	//
	// Register the miniport with NDIS. Note that it is the miniport
	// which was started as a driver and not the protocol. Also the miniport
	// must be registered prior to the protocol since the protocol's BindAdapter
	// handler can be initiated anytime and when it is, it must be ready to
	// start driver instances.
	//
	NdisMInitializeWrapper(&WrapperHandle, DriverObject, RegistryPath, NULL);

	NdisZeroMemory(&MChars, sizeof(NDIS_MINIPORT_CHARACTERISTICS));

	MChars.MajorNdisVersion = 4;
	MChars.MinorNdisVersion = 0;

	MChars.InitializeHandler = MPInitialize;
	MChars.QueryInformationHandler = MPQueryInformation;
	MChars.SetInformationHandler = MPSetInformation;
	MChars.ResetHandler = MPReset;
	MChars.TransferDataHandler = MPTransferData;
	MChars.HaltHandler = MPHalt;

	//
	// We will disable the check for hang timeout so we do not
	// need a check for hang handler!
	//
	MChars.CheckForHangHandler = NULL;
	MChars.SendHandler = MPSend;
	MChars.ReturnPacketHandler = MPReturnPacket;

	//
	// Either the Send or the SendPackets handler should be specified.
	// If SendPackets handler is specified, SendHandler is ignored
	//
	// MChars.SendPacketsHandler = MPSendPackets;

	Status = NdisIMRegisterLayeredMiniport(WrapperHandle,
										   &MChars,
										   sizeof(MChars),
										   &DriverHandle);
	ASSERT(Status == NDIS_STATUS_SUCCESS);

	NdisMRegisterUnloadHandler(WrapperHandle, PtUnload);

	//
	// Now register the protocol.
	//
	NdisZeroMemory(&PChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
	PChars.MajorNdisVersion = 4;
	PChars.MinorNdisVersion = 0;

	//
	// Make sure the protocol-name matches the service-name under which this protocol is installed.
	// This is needed to ensure that NDIS can correctly determine the binding and call us to bind
	// to miniports below.
	//
	NdisInitUnicodeString(&Name, L"SFilter");	// Protocol name
	PChars.Name = Name;
	PChars.OpenAdapterCompleteHandler = PtOpenAdapterComplete;
	PChars.CloseAdapterCompleteHandler = PtCloseAdapterComplete;
	PChars.SendCompleteHandler = PtSendComplete;
	PChars.TransferDataCompleteHandler = PtTransferDataComplete;
	
	PChars.ResetCompleteHandler = PtResetComplete;
	PChars.RequestCompleteHandler = PtRequestComplete;
	PChars.ReceiveHandler = PtReceive;
	PChars.ReceiveCompleteHandler = PtReceiveComplete;
	PChars.StatusHandler = PtStatus;
	PChars.StatusCompleteHandler = PtStatusComplete;
	PChars.BindAdapterHandler = PtBindAdapter;
	PChars.UnbindAdapterHandler = PtUnbindAdapter;
	PChars.UnloadHandler = NULL;
	PChars.ReceivePacketHandler = PtReceivePacket;
	PChars.PnPEventHandler= PtPNPHandler;

	NdisRegisterProtocol(&Status,
						 &ProtHandle,
						 &PChars,
						 sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

	ASSERT(Status == NDIS_STATUS_SUCCESS);

	NdisIMAssociateMiniport(DriverHandle, ProtHandle);

	// �����豸�������������
	RtlInitUnicodeString(&nameString, L"\\Device\\IMDKernel" ); 
	RtlInitUnicodeString(&linkString, L"\\??\\IMDKernel"); 

	for(FuncIndex = 0; FuncIndex <=IRP_MJ_MAXIMUM_FUNCTION; FuncIndex++)
	{
		MajorFunction[FuncIndex] = NULL;
	}
	    
	MajorFunction[IRP_MJ_CREATE]         = MydrvDispatch;
	MajorFunction[IRP_MJ_CLOSE]          = MydrvDispatch;
	MajorFunction[IRP_MJ_DEVICE_CONTROL] = MydrvDispatchIoctl;

	Status = NdisMRegisterDevice(
                    WrapperHandle, 
                    &nameString,
                    &linkString,
                    MajorFunction,
                    &MyDeviceObject,
                    &NdisDeviceHandle
                    );

	if(Status != STATUS_SUCCESS)
	{
		DbgPrint("NdisMRegisterDevice failed!\n");
	}

	// �����ڴ����
	SystemVirtualAddress = ExAllocatePool(NonPagedPool, MAX_PACKET_SIZE);
	Mdl = IoAllocateMdl(SystemVirtualAddress, MAX_PACKET_SIZE, FALSE, FALSE, NULL);
	MmBuildMdlForNonPagedPool(Mdl);

	return NDIS_STATUS_SUCCESS; 
}

static NTSTATUS MydrvDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) 
{ 
	NTSTATUS status; 
	PIO_STACK_LOCATION irpSp; 

	irpSp = IoGetCurrentIrpStackLocation(Irp); 

	switch (irpSp->MajorFunction) 
	{ 
		case IRP_MJ_CREATE: 
			Irp->IoStatus.Status = STATUS_SUCCESS; 
			Irp->IoStatus.Information = 0L; 
			break; 

		case IRP_MJ_CLOSE: 
			Irp->IoStatus.Status = STATUS_SUCCESS; 
			Irp->IoStatus.Information = 0L; 
			MmUnmapLockedPages(UserVirtualAddress, Mdl);
			break; 
	} 

	IoCompleteRequest(Irp, IO_NO_INCREMENT); 
	return STATUS_SUCCESS; 

} 

static NTSTATUS MydrvDispatchIoctl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) 
{ 
	PIO_STACK_LOCATION IrpStack; 
	NTSTATUS status; 
	ULONG ControlCode; 
	ULONG InputLength, OutputLength; 
	OBJECT_HANDLE_INFORMATION	objHandleInfo;
	HANDLE	hEvent = NULL;
	PVOID pIoBuffer = Irp->AssociatedIrp.SystemBuffer;

	IrpStack = IoGetCurrentIrpStackLocation(Irp); 

	// �õ�DeviceIoControl�����Ŀ�����
	ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode; 

	switch (ControlCode) 
	{ 
		//��ù����ڴ��ַ
		case IO_GET_SHAREDMEMORY_ADDR:
			// ��������ڴ�ӳ�䵽�û����̵�ַ�ռ䣬�����ص�ַ��
			try
			{
				UserVirtualAddress = MmMapLockedPages(Mdl, UserMode); 
				*((PVOID *)Irp->AssociatedIrp.SystemBuffer) = UserVirtualAddress;
				Irp->IoStatus.Status = STATUS_SUCCESS;
				Irp->IoStatus.Information = sizeof(PVOID);
			}
			except(EXCEPTION_EXECUTE_HANDLER){}
			break;
			
		//����Ӧ�ò��¼�
		case IO_REFERENCE_EVENT: 
			hEvent = (HANDLE)IrpStack->Parameters.DeviceIoControl.Type3InputBuffer;
			status = ObReferenceObjectByHandle(hEvent,
												GENERIC_ALL,
												NULL,
												KernelMode,
												&gpEventObject,
												&objHandleInfo);
			if(status != STATUS_SUCCESS)
			{
				DbgPrint("ObReferenceObjectByHandle failed! status = %x\n", status);
				break;
			}
			else DbgPrint("Reference object successfully!\n");
			Monitor_flag = 1;	// ���ü��ӱ�ʶΪ1
			Irp->IoStatus.Status = STATUS_SUCCESS;
			Irp->IoStatus.Information = 0L;
			break;
			
		//ֹͣ����	
		case IO_STOP_MONITOR_EVENT:
			Monitor_flag = 0;	// ���ü��ӱ�־Ϊ0
			Filt_flag = 0;		// ���ù��˱�ʶΪ0
	
			if(gpEventObject)
			{
				ObDereferenceObject(gpEventObject);
				DbgPrint("ObDereferenceObject successfully!\n");
			}
			else{}
			Irp->IoStatus.Status = STATUS_SUCCESS;
			Irp->IoStatus.Information = 0L;
			break;
		
		//�¼�����
		case IO_RESET_EVENT:
			KeClearEvent(gpEventObject);
			DbgPrint("KeClearEvent successfully!\n");
			Irp->IoStatus.Status = STATUS_SUCCESS;
			Irp->IoStatus.Information = 0L;
			break;

		//������������
		case IO_SET_FILTER:
			Filt_flag = 1;
			memset(filt,0, 13);
			memcpy(filt, pIoBuffer, 13);		
			Irp->IoStatus.Status = STATUS_SUCCESS;
			Irp->IoStatus.Information = 0L;	
			break;

		default:
			break;
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status; 
} 


