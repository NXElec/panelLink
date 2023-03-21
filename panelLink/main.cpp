
#include "pch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define for pack type, per PanelLink protocol. */
#define TYPE_START 1
#define TYPE_END 2
#define TYPE_RESET 3
#define TYPE_CLEAR 4

#pragma pack(push) //�������״̬
#pragma pack(1)   // 1 bytes����

const char protocol_str[] = "PANEL-LINK";
const char raw_video_str[] = "video/x-raw, format=BGR, height=480, width=800, framerate=0/1";

typedef struct _PANELLINK_STREAM_TAG {
	unsigned char protocol_name[10];
	unsigned char version;
	unsigned char type;
	unsigned char fmtstr[256];
	unsigned short checksum16;
} PANELLINK_STREAM_TAG;

#pragma pack(pop)//�ָ�����״̬

#define MIN_Buffer_Size 512

BOOL GetUSBDeviceSpeed(WINUSB_INTERFACE_HANDLE hDeviceHandle, UCHAR* pDeviceSpeed)
{
	if (!pDeviceSpeed || hDeviceHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	BOOL bResult = TRUE;

	ULONG length = sizeof(UCHAR);

	bResult = WinUsb_QueryDeviceInformation(hDeviceHandle, DEVICE_SPEED, &length, pDeviceSpeed);
	if (!bResult)
	{
		printf("Error getting device speed: %d.\n", GetLastError());
		goto done;
	}

	if (*pDeviceSpeed == LowSpeed)
	{
		printf("Device speed: %d (Low speed).\n", *pDeviceSpeed);
		goto done;
	}
	if (*pDeviceSpeed == FullSpeed)
	{
		printf("Device speed: %d (Full speed).\n", *pDeviceSpeed);
		goto done;
	}
	if (*pDeviceSpeed == HighSpeed)
	{
		printf("Device speed: %d (High speed).\n", *pDeviceSpeed);
		goto done;
	}

done:
	return bResult;
}

struct PIPE_ID
{
	UCHAR  PipeInId;
	UCHAR  PipeOutId;
};

BOOL QueryDeviceEndpoints(WINUSB_INTERFACE_HANDLE hDeviceHandle, PIPE_ID* pipeid)
{
	if (hDeviceHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	BOOL bResult = TRUE;

	USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
	ZeroMemory(&InterfaceDescriptor, sizeof(USB_INTERFACE_DESCRIPTOR));

	WINUSB_PIPE_INFORMATION  Pipe;
	ZeroMemory(&Pipe, sizeof(WINUSB_PIPE_INFORMATION));


	bResult = WinUsb_QueryInterfaceSettings(hDeviceHandle, 0, &InterfaceDescriptor);

	if (bResult)
	{
		for (int index = 0; index < InterfaceDescriptor.bNumEndpoints; index++)
		{
			bResult = WinUsb_QueryPipe(hDeviceHandle, 0, index, &Pipe);

			if (bResult)
			{
				if (Pipe.PipeType == UsbdPipeTypeControl)
				{
					printf("Endpoint index: %d Pipe type: %d Control Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
				}
				else if (Pipe.PipeType == UsbdPipeTypeIsochronous)
				{
					printf("Endpoint index: %d Pipe type: %d Isochronous Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
				}
				else if (Pipe.PipeType == UsbdPipeTypeBulk)
				{
					if (USB_ENDPOINT_DIRECTION_IN(Pipe.PipeId))
					{
						printf("Endpoint index: %d Pipe type: %d Bulk In Pipe ID: %x.\n", index, Pipe.PipeType, Pipe.PipeId);
						pipeid->PipeInId = Pipe.PipeId;
					}
					else if (USB_ENDPOINT_DIRECTION_OUT(Pipe.PipeId))
					{
						printf("Endpoint index: %d Pipe type: %d Bulk Out Pipe ID: %x.\n", index, Pipe.PipeType, Pipe.PipeId);
						pipeid->PipeOutId = Pipe.PipeId;
						break;
					}

				}
				else if (Pipe.PipeType == UsbdPipeTypeInterrupt)
				{
					printf("Endpoint index: %d Pipe type: %d Interrupt Pipe ID: %d.\n", index, Pipe.PipeType, Pipe.PipeId);
				}
			}
			else
			{
				continue;
			}
		}
	}

done:
	return bResult;
}

BOOL WriteToBulkEndpoint(WINUSB_INTERFACE_HANDLE hDeviceHandle, UCHAR* pID, ULONG* pcbWritten)
{
	if (hDeviceHandle == INVALID_HANDLE_VALUE || !pID || !pcbWritten)
	{
		return FALSE;
	}

	BOOL bResult = TRUE;

	UCHAR szBuffer[] = "Hello World";
	ULONG cbSize = strlen((const char*)szBuffer);
	ULONG cbSent = 0;

	bResult = WinUsb_WritePipe(hDeviceHandle, *pID, szBuffer, cbSize, &cbSent, 0);
	if (!bResult)
	{
		goto done;
	}

	printf("Wrote to pipe %d: %s \nActual data transferred: %d.\n", *pID, szBuffer, cbSent);
	*pcbWritten = cbSent;


done:
	return bResult;

}

BOOL ReadFromBulkEndpoint(WINUSB_INTERFACE_HANDLE hDeviceHandle, UCHAR* pID, ULONG cbSize)
{
	if (hDeviceHandle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	BOOL bResult = TRUE;

	UCHAR* szBuffer = (UCHAR*)LocalAlloc(LPTR, sizeof(UCHAR)*cbSize);

	ULONG cbRead = 0;

	bResult = WinUsb_ReadPipe(hDeviceHandle, *pID, szBuffer, cbSize, &cbRead, 0);
	if (!bResult)
	{
		goto done;
	}

	printf("Read from pipe %d: %s \nActual data read: %d.\n", *pID, szBuffer, cbRead);


done:
	LocalFree(szBuffer);
	return bResult;

}

unsigned short checksum16(unsigned short *buf, int nword)
{
	unsigned long sum;

	for (sum = 0; nword > 0; nword--)
		sum += *buf++;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);

	return ~sum;
}

LONG __cdecl
_tmain(
	LONG     Argc,
	LPTSTR * Argv
)
/*++

Routine description:

	Sample program that communicates with a USB device using WinUSB

--*/
{
	DEVICE_DATA           deviceData;
	HRESULT               hr;
	USB_DEVICE_DESCRIPTOR deviceDesc;
	BOOL                  bResult;
	BOOL                  noDevice;
	ULONG                 lengthReceived;
	PIPE_ID               pipeID;
	UCHAR                 speed;
	PANELLINK_STREAM_TAG * pTemp;
	int loop = 25;
	UCHAR Fmt[256];
	int isize;

	//
	// Find a device connected to the system that has WinUSB installed using our
	// INF
	//
	hr = OpenDevice(&deviceData, &noDevice);

	if (FAILED(hr)) {

		if (noDevice) {

			wprintf(L"Device not connected or driver not installed\n");

		}
		else {

			wprintf(L"Failed looking for device, HRESULT 0x%x\n", hr);
		}

		return 0;
	}

	//
	// Get device descriptor
	//
	bResult = WinUsb_GetDescriptor(deviceData.WinusbHandle,
		USB_DEVICE_DESCRIPTOR_TYPE,
		0,
		0,
		(PBYTE)&deviceDesc,
		sizeof(deviceDesc),
		&lengthReceived);

	if (FALSE == bResult || lengthReceived != sizeof(deviceDesc)) {

		wprintf(L"Error among LastError %d or lengthReceived %d\n",
			FALSE == bResult ? GetLastError() : 0,
			lengthReceived);
		CloseDevice(&deviceData);
		return 0;
	}

	//
	// Print a few parts of the device descriptor
	//
	wprintf(L"Device found: VID_%04X&PID_%04X; bcdUsb %04X\n",
		deviceDesc.idVendor,
		deviceDesc.idProduct,
		deviceDesc.bcdUSB);

	GetUSBDeviceSpeed(deviceData.WinusbHandle, &speed);
	QueryDeviceEndpoints(deviceData.WinusbHandle, &pipeID);

	if (Argc < 2) {
		CloseDevice(&deviceData);
		return 0;
	}
	else if (Argc == 3) {
		isize = WideCharToMultiByte(CP_ACP, 0, Argv[2], -1, NULL, 0, NULL, NULL);
		if (isize <= 256) {
			WideCharToMultiByte(CP_ACP, 0, Argv[2], -1, (LPSTR)Fmt, isize, NULL, NULL);

			//	pFmt = (UCHAR *)Argv[2];
			printf("format string %s %d\n",
				Fmt, strlen((CONST char*)Fmt));
		}
	}


	//		loop = _wtoi(Argv[2]);

	//	while (1) {
	HANDLE hFile = CreateFile(Argv[1],
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
	);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		wprintf(L"File open failure. %s\n", Argv[1]);
		CloseDevice(&deviceData);
		return 0;
	}



	// Todo:
	// Should we implement a file mapping here to improve I/O performance?? 
	//
	DWORD dwFileSize = GetFileSize(hFile, NULL);
	UCHAR* szBuffer = (UCHAR*)LocalAlloc(LPTR, MIN_Buffer_Size*loop);

	{
		SetFilePointer(hFile,
			0,
			NULL,
			FILE_BEGIN);

		pTemp = (PANELLINK_STREAM_TAG*)szBuffer;
		pTemp->type = TYPE_CLEAR;
		pTemp->version = 1;
		memcpy(pTemp->protocol_name, protocol_str, strlen(protocol_str));
		memset(pTemp->fmtstr, 0, 256);
		pTemp->checksum16 = checksum16((unsigned short*)szBuffer, (sizeof(PANELLINK_STREAM_TAG) - 2) / 2);
		DWORD cbSent = 0;

		bResult = WinUsb_WritePipe(deviceData.WinusbHandle, pipeID.PipeOutId, (UCHAR*)szBuffer, sizeof(PANELLINK_STREAM_TAG), &cbSent, 0);
		if (!bResult)
		{
			wprintf(L"WinUsb_WritePipe failure - reset.\n");
			LocalFree(szBuffer);
			CloseHandle(hFile);
			CloseDevice(&deviceData);
			return 0;
		}
		else {
			wprintf(L"WinUsb_WritePipe success - %d bytes reset.\n", cbSent);
		}

		pTemp->type = TYPE_START;
		pTemp->version = 1;
		memcpy(pTemp->protocol_name, protocol_str, strlen(protocol_str));
		memset(pTemp->fmtstr, 0, 256);
		memcpy(pTemp->fmtstr, Fmt, strlen((CONST char*)Fmt));
		pTemp->checksum16 = checksum16((unsigned short *)szBuffer, (sizeof(PANELLINK_STREAM_TAG) - 2) / 2);
		cbSent = 0;

		bResult = WinUsb_WritePipe(deviceData.WinusbHandle, pipeID.PipeOutId, (UCHAR *)szBuffer, sizeof(PANELLINK_STREAM_TAG), &cbSent, 0);
		if (!bResult)
		{
			wprintf(L"WinUsb_WritePipe failure - header.\n");
			LocalFree(szBuffer);
			CloseHandle(hFile);
			CloseDevice(&deviceData);
			return 0;
		}
		else {
			wprintf(L"WinUsb_WritePipe success - %d bytes header.\n", cbSent);
		}

		DWORD sections = dwFileSize / (MIN_Buffer_Size * loop);
		DWORD reminders = dwFileSize % (MIN_Buffer_Size * loop);
		if (reminders)
			sections += 1;

		unsigned long lpNumber = 0;
		for (int i = 0; i < sections; i++) {
			ReadFile(hFile,
				szBuffer,
				MIN_Buffer_Size * loop,//��ȡ�ļ��ж�������
				&lpNumber,
				NULL
			);

			bResult = WinUsb_WritePipe(deviceData.WinusbHandle, pipeID.PipeOutId, szBuffer, lpNumber, &cbSent, 0);
			if (!bResult)
			{
				wprintf(L"WinUsb_WritePipe failure.\n");
				LocalFree(szBuffer);
				CloseHandle(hFile);
				CloseDevice(&deviceData);
				return 0;
			}


			//	printf("Wrote to pipe %d: \nActual data transferred: %d.\n", pipeID.PipeOutId, cbSent);
		}

		pTemp->type = TYPE_END;
		pTemp->version = 1;
		memcpy(pTemp->protocol_name, protocol_str, strlen(protocol_str));
		memset(pTemp->fmtstr, 0, 256);
		memcpy(pTemp->fmtstr, Fmt, strlen((CONST char *)Fmt));
		pTemp->checksum16 = checksum16((unsigned short *)szBuffer, (sizeof(PANELLINK_STREAM_TAG) - 2) / 2);
		cbSent = 0;

		bResult = WinUsb_WritePipe(deviceData.WinusbHandle, pipeID.PipeOutId, (UCHAR *)szBuffer, sizeof(PANELLINK_STREAM_TAG), &cbSent, 0);
		if (!bResult)
		{
			wprintf(L"WinUsb_WritePipe failure - header.\n");
			LocalFree(szBuffer);
			CloseHandle(hFile);
			CloseDevice(&deviceData);
			return 0;
		}
		else {
			wprintf(L"WinUsb_WritePipe success - %d bytes header.\n", cbSent);
		}

		wprintf(L"File content dump done.\n");
	}

 
 LocalFree(szBuffer);
		CloseHandle(hFile);
	//	Sleep(5000);
	
    CloseDevice(&deviceData);
    return 0;
}