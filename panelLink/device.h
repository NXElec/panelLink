//
// Define below GUIDs
//
#include <initguid.h>

//
// Device Interface GUID.
// Used by all WinUsb devices that this application talks to.
// Must match "DeviceInterfaceGUIDs" registry value specified in the INF file.
// b5178612-9b1b-4787-b10d-ffdd4751e1c3
//
DEFINE_GUID(GUID_DEVINTERFACE_panelLink,
	//	0xb5178612, 0x9b1b, 0x4787, 0xb1, 0x0d, 0xff, 0xdd, 0x47, 0x51, 0xe1, 0xc3);
	//0x88bae032,0x5a81,0x49f0,0xbc, 0x3d,0xa4, 0xff, 0x13, 0x82, 0x16, 0xd6);
//	0x58D07210, 0x27C1, 0x11DD, 0xBD, 0x0B, 0x08, 0x00, 0x20, 0x0C, 0x9a, 0x66);
0x8E41214B, 0x6785, 0x4CFE, 0xB9, 0x92, 0x03, 0x7D, 0x68, 0x94, 0x9A, 0x14);


typedef struct _DEVICE_DATA {

    BOOL                    HandlesOpen;
    WINUSB_INTERFACE_HANDLE WinusbHandle;
    HANDLE                  DeviceHandle;
    TCHAR                   DevicePath[MAX_PATH];

} DEVICE_DATA, *PDEVICE_DATA;

HRESULT
OpenDevice(
    _Out_     PDEVICE_DATA DeviceData,
    _Out_opt_ PBOOL        FailureDeviceNotFound
    );

VOID
CloseDevice(
    _Inout_ PDEVICE_DATA DeviceData
    );
