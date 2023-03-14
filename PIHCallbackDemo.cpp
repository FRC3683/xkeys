// PIHCallbackDemo.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include <stdlib.h>
#include "PIEHid32.h"
#include "GameInfo.h"

#include <string.h>
#include <stdio.h>


#define MAXDEVICES  4   //max allowed array size for enumeratepie =128 devices*4 bytes per device


// function declares 
int CALLBACK DialogProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
);
void FindAndStart(HWND hDialog);
void AddEventMsg(HWND hDialog, char *msg);
void AddDevices(HWND hDialog, char *msg);
DWORD __stdcall HandleDataEvent(UCHAR *pData, DWORD deviceID, DWORD error);
DWORD __stdcall HandleErrorEvent(DWORD deviceID, DWORD status);

void ClearLEDs();
void SendToDS();
void StateLEDs();
void SlowWrite(long hnd, UCHAR* data);

BYTE buffer[80];  //used for writing to device
BYTE lastpData[80];  //stores the data of the previous read
int readlength=0; 

HWND hDialog;
long hDevice = -1;
int combotodevice[MAXDEVICES];
bool flash_cone = false;
bool flash_cube = false;
bool flash_waste_of_button = false;
int scoring_level = HIGH;
int scoring_offset = CENTER;
int scoring_slot = ONE;

bool autominus = false;
bool autoplus = false;
bool score = false;
bool conebeam = false;
bool cubebeam = false;
bool stopEE = false;
bool zero = false;

bool escu = false;
bool escd = false;
bool lnxu = false;
bool lnxd = false;
bool wrsu = false;
bool wrsd = false;
bool intu = false;
bool intd = false;


void Reset() {
	ClearLEDs();
	flash_cone = false;
	flash_cube = false;
	flash_waste_of_button = false;
	scoring_level = HIGH;
	scoring_offset = CENTER;
	scoring_slot = ONE;
	autominus = false;
	autoplus = false;
	score = false;
	conebeam = false;
	cubebeam = false;
	stopEE = false;
	zero = false;

	escu = false;
	escd = false;
	lnxu = false;
	lnxd = false;
	wrsu = false;
	wrsd = false;
	intu = false;
	intd = false;
	SendToDS();
	StateLEDs();
}


//---------------------------------------------------------------------

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    DWORD result;
	MSG   msg;


	hDialog = CreateDialog(hInstance, (LPCTSTR)IDD_MAIN, NULL, DialogProc);

	ShowWindow(hDialog, SW_NORMAL);

	//put numbers in edit boxes
	HWND hList;

	hList = GetDlgItem(hDialog, IDC_TXTBL); // Key ID
	if (hList == NULL) return 0;
	SendMessage(hList, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)"1");

	result = GetMessage( &msg, NULL, 0, 0 );
	while (result != 0)    { 
		if (result == -1)    {
			return 1;
			// handle the error and possibly exit
		}
		else    {
			TranslateMessage(&msg); 
			DispatchMessage(&msg); 
		}
		
		result = GetMessage( &msg, NULL, 0, 0 );
	}

    if (hDevice != -1) CleanupInterface(hDevice);

	


	return 0;
}
//---------------------------------------------------------------------

int CALLBACK DialogProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)    {

	DWORD result;
//	BYTE buffer[80];  //this globally declared only to keep track of LEDs	
	
	HWND hList;
	
	int K0;    
    int K1;   
    int K2;
	int K3;
	int countout=0;
	int wlen=GetWriteLength(hDevice);
	char *p;
	char errordescription[100]; //used with the GetErrorString call to describe the error


	switch (uMsg)    {
	case WM_INITDIALOG:
		SendMessage(GetDlgItem(hwndDlg, CHK_NONE), BM_SETCHECK, BST_CHECKED, 0);
		// Indicate that events are off to start.
		return FALSE;   // not choosing keyboard focus
	
    case WM_COMMAND:
		switch (LOWORD(wParam))
         {
			 case IDC_LIST1:
             {
				 switch (HIWORD(wParam)) 
                 { 
					case LBN_SELCHANGE:
                    {
                        //user selecting different device
						hList = GetDlgItem(hDialog, IDC_LIST1);
						if (hList == NULL) return TRUE;
						hDevice=combotodevice[SendMessage(hList, LB_GETCURSEL, 0, 0)];
                        return TRUE; 
                    } 
					return TRUE;
                 }
			 }
		 }

		switch (wParam)    {
		case IDCANCEL:
			PostQuitMessage(0);
			return TRUE;

		case IDSTART:
		    if (hDevice != -1) CloseInterface(hDevice);
			// only one at a time in the demo, please

			hDevice = -1;
			SendMessage(GetDlgItem(hwndDlg, CHK_NONE), BM_CLICK, 0, 0);
			FindAndStart(hwndDlg);
			return TRUE;

        case IDC_CALLBACK:
			if (hDevice == -1) return TRUE;
			//Turn on the data callback
			result = SetDataCallback(hDevice, HandleDataEvent);
			result = SetErrorCallback(hDevice, HandleErrorEvent);
			if (result != 0)    {
				AddEventMsg(hwndDlg, "Error:");
				GetErrorString(result, errordescription, 100);
				AddEventMsg(hwndDlg, errordescription);
			}
			SuppressDuplicateReports(hDevice, true);
			DisableDataCallback(hDevice, false); //turn on callback in the case it was turned off by some other command
			
			Reset();
			return TRUE;
        
		case IDC_CLEAR:
		    
			hList = GetDlgItem(hDialog, ID_EVENTS);
			if (hList == NULL) return TRUE;
			SendMessage(hList, LB_RESETCONTENT, 0, 0);
			return TRUE;


		case IDC_CHECK1: //Green LED
			
			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[1]=179; //0xb3
			buffer[2]=6; //6=green, 7=red
			//get checked state
			hList = GetDlgItem(hDialog, IDC_CHECK1);
			if (hList == NULL) return TRUE;
			if (SendMessage(hList, BM_GETCHECK, 0, 0)==BST_CHECKED) buffer[3]=1; //0=off, 1=on, 2=flash
			
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			return TRUE;

		case IDC_CHECK2: //Red LED
			
			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[1]=179; //0xb3
			buffer[2]=7; //6=green, 7=red
			//get checked state
			hList = GetDlgItem(hDialog, IDC_CHECK2);
			if (hList == NULL) return TRUE;
			if (SendMessage(hList, BM_GETCHECK, 0, 0)==BST_CHECKED) buffer[3]=1; //0=off, 1=on, 2=flash
			
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			return TRUE;
		case IDC_CHKBLONOFF:
			//Turn on/off the backlight of the entered key in IDC_TXTBL
			//Use the Set Flash Freq to control frequency of blink
            //Key Index (in decimal)
            //Bank 1
			//Columns-->
            //  0   8   16  24  32  40  48  56  64  72
            //  1   9   17  25  33  41  49  57  65  73
            //  2   10  18  26  34  42  50  58  66  74
            //  3   11  19  27  35  43  51  59  67  75
            //  4   12  20  28  36  44  52  60  68  76
            //  5   13  21  29  37  45  53  61  69  77
            //  6   14  22  30  38  46  54  62  70  78
            //  7   15  23  31  39  47  55  63  71  79

			//Bank 2
			//Columns-->
            //  80	88	96	104	112	120	128	136	144	152
            //  81	89	97	105	113	121	129	137	145	153
            //  82	90	98	106	114	122	130	138	146	154
            //  83	91	99	107	115	123	131	139	147	155
            //  84	92	100	108	116	124	132	140	148	156
            //  85	93	101	109	117	125	133	141	149	157
			//	86	94	102	110	118	126	134	142	150	158
			//	87	95	103	111	119	127	135	143	151	159

			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[1]=181; //0xb5
			//get key index
			//get text box text
			hList = GetDlgItem(hDialog, IDC_TXTBL);
			if (hList == NULL) return TRUE;
			char keyid[10];
			SendMessage(hList, WM_GETTEXT, 8, (LPARAM)keyid);
			buffer[2]= atoi(keyid);
			
			//get checked state
			hList = GetDlgItem(hDialog, IDC_CHKBLONOFF);
			if (hList == NULL) return TRUE;
			if (SendMessage(hList, BM_GETCHECK, 0, 0)==BST_CHECKED) 
			{
				hList = GetDlgItem(hDialog, IDC_CHKBLFLASH);
				if (hList == NULL) return TRUE;
				if (SendMessage(hList, BM_GETCHECK, 0, 0)==BST_CHECKED) buffer[3]=2; ////0=off, 1=on, 2=flash
				else buffer[3]=1;
			}
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			return TRUE;
		case IDC_CHKBLFLASH:
            //Turn on/off the backlight of the entered key in IDC_TXTBL
			//Use the Set Flash Freq to control frequency of blink
            //Key Index (in decimal)
			//Bank 1
            //Columns-->
            //Columns-->
            //  0   8   16  24  32  40  48  56  64  72
            //  1   9   17  25  33  41  49  57  65  73
            //  2   10  18  26  34  42  50  58  66  74
            //  3   11  19  27  35  43  51  59  67  75
            //  4   12  20  28  36  44  52  60  68  76
            //  5   13  21  29  37  45  53  61  69  77
            //  6   14  22  30  38  46  54  62  70  78
            //  7   15  23  31  39  47  55  63  71  79

			//Bank 2
			//Columns-->
            //  80	88	96	104	112	120	128	136	144	152
            //  81	89	97	105	113	121	129	137	145	153
            //  82	90	98	106	114	122	130	138	146	154
            //  83	91	99	107	115	123	131	139	147	155
            //  84	92	100	108	116	124	132	140	148	156
            //  85	93	101	109	117	125	133	141	149	157
			//	86	94	102	110	118	126	134	142	150	158
			//  87  95  103 111 119 127 135 143 151 159

			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[1]=181; //0xb5
			//get key index
			//get text box text
			hList = GetDlgItem(hDialog, IDC_TXTBL);
			if (hList == NULL) return TRUE;
			char keyidf[10];
			SendMessage(hList, WM_GETTEXT, 8, (LPARAM)keyidf);
			buffer[2]= atoi(keyidf);
			
			//get checked state
			hList = GetDlgItem(hDialog, IDC_CHKBLFLASH);
			if (hList == NULL) return TRUE;
			if (SendMessage(hList, BM_GETCHECK, 0, 0)==BST_CHECKED) 
			{
				buffer[3]=2;
			}
			else
			{
				hList = GetDlgItem(hDialog, IDC_CHKBLONOFF);
				if (hList == NULL) return TRUE;
				if (SendMessage(hList, BM_GETCHECK, 0, 0)==BST_CHECKED) buffer[3]=1; //0=off, 1=on, 2=flash
			}
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			return TRUE;
        case IDC_CHKBANK1:
			//Turns on or off ALL bank 1 BLs using current intensity
			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[0]=0;
			buffer[1]=182;
			buffer[2]=0; //0 for bank 1, 1 for bank 2
			//get checked state
			hList = GetDlgItem(hDialog, IDC_CHKBANK1);
			if (hList == NULL) return TRUE;
			if (SendMessage(hList, BM_GETCHECK, 0, 0)==BST_CHECKED) buffer[3]=255;
			else buffer[3]=0;  //0=off, 255=on OR use individual bits to turn on rows, bit 1=row 1, bit 2= row 2, etc
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			if (result != 0)    {
				AddEventMsg(hwndDlg, "Error:");
				GetErrorString(result, errordescription, 100);
				AddEventMsg(hwndDlg, errordescription);
			}
			return TRUE;

		case IDC_CHKBANK2:
			//Turns on or off ALL bank 2 BLs using current intensity
			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[0]=0;
			buffer[1]=182;
			buffer[2]=1; //0 for bank 1, 1 for bank 2
			//get checked state
			hList = GetDlgItem(hDialog, IDC_CHKBANK2);
			if (hList == NULL) return TRUE;
			if (SendMessage(hList, BM_GETCHECK, 0, 0)==BST_CHECKED) buffer[3]=255;
			else buffer[3]=0;  //0=off, 255=on OR use individual bits to turn on rows, bit 1=row 1, bit 2= row 2, etc
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			if (result != 0)    {
				AddEventMsg(hwndDlg, "Error:");
				GetErrorString(result, errordescription, 100);
				AddEventMsg(hwndDlg, errordescription);
			}
			return TRUE;
		case IDC_BLToggle:
			//Toggle the backlights
			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[0]=0;
			buffer[1]=184;
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			if (result != 0)    {
				AddEventMsg(hwndDlg, "Error:");
				GetErrorString(result, errordescription, 100);
				AddEventMsg(hwndDlg, errordescription);
			}
			return TRUE;
		case IDC_BLIntensity:
			//Sets the bank 1 and bank 2 backlighting intensity, one value for all bank 1 or bank 2
			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}

			buffer[0]=0;
			buffer[1]=187;
			buffer[2]=127; //0-255 bank 1 intensity
			buffer[3]=64; //0-255 bank 2 intensity
			
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			if (result != 0)    {
				AddEventMsg(hwndDlg, "Error:");
				GetErrorString(result, errordescription, 100);
				AddEventMsg(hwndDlg, errordescription);
			}
			return TRUE;
		case IDC_FREQ:
			//Set the frequency of the flashing, same one for LEDs and backlights
			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[1]=180;
			//get text box text
			hList = GetDlgItem(hDialog, IDC_EDIT3);
			if (hList == NULL) return TRUE;
			char Freq[10];
			SendMessage(hList, WM_GETTEXT, 8, (LPARAM)Freq);
			buffer[2]= atoi(Freq);
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			if (result != 0)    {
				AddEventMsg(hwndDlg, "Error:");
				GetErrorString(result, errordescription, 100);
				AddEventMsg(hwndDlg, errordescription);
			}
			return TRUE;
		case IDC_SAVEBACKLIGHTS: //Makes the current backlighting the default
			
			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[1]=199; //0xc7
			buffer[2]=1; //anything other than 0 will save bl state to eeprom
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			return TRUE;
		case IDC_TOPID3:
			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[0]=0;
			buffer[1]=204; //0xcc
			buffer[2]=2;   //0=PID 1, 1=PID 2, 2=PID 3, 3=PID 4
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			return TRUE;
		case IDC_TIMESTAMP:
			//Sending this command will turn off the 4 bytes of data which assembled give the time in ms from the start of the computer
			
            for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[0]=0;
			buffer[1]=210;
			buffer[2]=0; //0 to turn off time stamp, 1 to turn on time stamp
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			return TRUE;

		case IDC_TIMESTAMP2:
			//Sending this command will turn on the 4 bytes of data which assembled give the time in ms from the start of the computer
		
            for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[0]=0;
			buffer[1]=210;
			buffer[2]=1; //0 to turn off time stamp, 1 to turn on time stamp
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			return TRUE;
		case IDC_SETKEY:
			//for users of the dongle feature only, set the dongle key here REMEMBER there 4 numbers, they are needed to check the dongle key
			 //This routine is done once per unit by the developer prior to sale.
			if (hDevice == -1) return TRUE;
			//Pick 4 numbers between 1 and 254.
            K0 = 7;    //pick any number between 1 and 254, 0 and 255 not allowed
            K1 = 58;   //pick any number between 1 and 254, 0 and 255 not allowed
            K2 = 33;   //pick any number between 1 and 254, 0 and 255 not allowed
            K3 = 243;  //pick any number between 1 and 254, 0 and 255 not allowed
			for (int i=0;i<wlen;i++)
			{
				buffer[i]=0;
			}
			buffer[0]=0;
			buffer[1]=192; //0xc0 set dongle key command
			buffer[2]=K0;
			buffer[3]=K1;
			buffer[4]=K2;
			buffer[5]=K3;
			result=404;
			while (result==404)
			{
				result = WriteData(hDevice, buffer);
			}
			return TRUE;
		

		
		default:
			return FALSE;
		} 

		break;

	}

	return FALSE;
}

//---------------------------------------------------------------------

void FindAndStart(HWND hDialog)
{
	DWORD result;
	//long  deviceData[MAXDEVICES];  
	TEnumHIDInfo info[MAXDEVICES];
	long  hnd;
	long  count;
	int pid;

	//clear out listbox
	HWND hList;
	hList = GetDlgItem(hDialog, ID_EVENTS);
	if (hList == NULL) return;
	SendMessage(hList, LB_RESETCONTENT, 0, 0);

	result = EnumeratePIE(0x5F3, info, count);

	if (result != 0)    
	{
		if (result==102){
			AddDevices(hDialog, "No PI Engineering Devices Found");
		}
		else{
		AddDevices(hDialog, "Error finding PI Engineering Devices");
		}
		return;
	} 
	else if (count == 0)    {
		AddDevices(hDialog, "No PI Engineering Devices Found");
		return;
	}

	for (int i=0;i< MAXDEVICES;i++)combotodevice[i]=-1;
	int cbocount=0;

	for (long i=0; i<count; i++)    {
		pid=info[i].PID; //get the device pid
		
		int hidusagepage=info[i].UP; //hid usage page
		int version=info[i].Version;
		int writelen=GetWriteLength(info[i].Handle);
		
		if ((hidusagepage == 0xC && writelen==36))    
		{	
			hnd = info[i].Handle; //handle required for piehid.dll calls
			result = SetupInterfaceEx(hnd);
			if (result != 0)    
			{
				AddDevices(hDialog, "Error setting up PI Engineering Device");
			}
			else    
			{
				if (pid==1089)
				{
					AddDevices(hDialog, "Found Device: XK-80, PID=1089 (PID #1)");
					combotodevice[cbocount] = i; //this is the handle
					cbocount++;
				}
				else if (pid==1090)
				{
					AddDevices(hDialog, "Found Device: XK-80, PID=1090 (PID #2)");
					combotodevice[cbocount] = i; //this is the handle
					cbocount++;
				}
				else if (pid==1091)
				{
					AddDevices(hDialog, "Found Device: XK-80, PID=1091 (PID #3)");
					combotodevice[cbocount] = i; //this is the handle
					cbocount++;
				}
				else if (pid==1250)
				{
					AddDevices(hDialog, "Found Device: XK-80, PID=1250 (PID #4)");
					combotodevice[cbocount] = i; //this is the handle
					cbocount++;
				}
				else if (pid==1121)
				{
					AddDevices(hDialog, "Found Device: XK-60, PID=1121 (PID #1)");
					combotodevice[cbocount] = i; //this is the handle
					cbocount++;
				}
				else if (pid==1122)
				{
					AddDevices(hDialog, "Found Device: XK-60, PID=1122 (PID #2)");
					combotodevice[cbocount] = i; //this is the handle
					cbocount++;
				}
				else if (pid==1123)
				{
					AddDevices(hDialog, "Found Device: XK-60, PID=1123 (PID #3)");
					combotodevice[cbocount] = i; //this is the handle
					cbocount++;
				}
				else if (pid==1254)
				{
					AddDevices(hDialog, "Found Device: XK-60, PID=1254 (PID #4)");
					combotodevice[cbocount] = i; //this is the handle
					cbocount++;
				}
				else
				{
					AddDevices(hDialog, "Unknown device found");
					combotodevice[cbocount] = i; //this is the handle
					cbocount++;
				}
				
			}
			
		}
	}

	if (cbocount>0)
	{
		hDevice=combotodevice[0]; //which is same as hDevice=info[combotodevice[0]].Handle
		readlength=info[combotodevice[0]].readSize;
		//print version, this is NOT firmware version which is given in the descriptor
		int version=info[combotodevice[0]].Version;
		char dataStr[256];
		_itoa_s(version,dataStr,10);
		
		hList = GetDlgItem(hDialog, IDC_LblVersion);
		SendMessage(hList, WM_SETTEXT,0, (LPARAM)(LPCSTR)dataStr);
		//show first device as selected
		hList = GetDlgItem(hDialog, IDC_LIST1);
		if (hList == NULL) return;
		SendMessage(hList, LB_SETCURSEL, 0, 0);
	}
	else
	{
		AddDevices(hDialog, "No X-keys devices found");
	}
}

//------------------------------------------------------------------------

void AddDevices(HWND hDialog, char *msg)
{
	HWND hList;
	int cnt=-1;
	hList = GetDlgItem(hDialog, IDC_LIST1);
	if (hList == NULL) return;
    cnt=SendMessage(hList, LB_GETCOUNT, 0, 0);
	SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)msg);
}
//------------------------------------------------------------------------

void AddEventMsg(HWND hDialog, char *msg)
{
	HWND hList;
 
	int cnt=-1;
 
	hList = GetDlgItem(hDialog, ID_EVENTS);
	if (hList == NULL) return;
    cnt=SendMessage(hList, LB_GETCOUNT, 0, 0);
	SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)msg);
   
	SendMessage(hList, LB_SETCURSEL, cnt, 0);
}
//------------------------------------------------------------------------

DWORD __stdcall HandleDataEvent(UCHAR *pData, DWORD deviceID, DWORD error)
{
	
	//Read Unit ID
	HWND hList = GetDlgItem(hDialog, IDC_UNITID3);
	if (hList == NULL) return TRUE;
	char dataStr2[256];
	_itoa_s(pData[1],dataStr2,10);
	SendMessage(hList, WM_SETTEXT,NULL , (LPARAM)dataStr2);
	


	int wlen = GetWriteLength(hDevice);
	DWORD result;

	for (int i = 0; i < wlen; i++)
	{
		buffer[i] = 0;
	}
	buffer[0] = 0;
	buffer[1] = 202;

	bool entered = false;

	char key = -1;
	char last_key = -1;

	//Buttons
	int maxcols=10;
	int maxrows=8;
	for (int i=0;i<maxcols;i++) //loop for each column of button data (Max Cols)
	{
		for (int j = 0; j < maxrows; j++) //loop for each row of button data (Max Rows)
		{
			int temp1 = 1 << j;
			int keynum = maxrows * i + j; //0 based index

			int state = 0; //0=was up and is up, 1=was up and is down, 2= was down and is down, 3=was down and is up 
			if (((pData[i + 3] & temp1) != 0) && ((lastpData[i + 3] & temp1) == 0))
				state = 1;
			else if (((pData[i + 3] & temp1) != 0) && ((lastpData[i + 3] & temp1) != 0))
				state = 2;
			else if (((pData[i + 3] & temp1) == 0) && ((lastpData[i + 3] & temp1) != 0))
				state = 3;





			//Perform action based on key number, consult P.I. Engineering SDK documentation for the key numbers

			switch (keynum) {
			case K_CONE:
				if (state == 1) {
					flash_cone = true;
					key = keynum;
				}
				else if (state == 3) {
					flash_cone = false;
					last_key = keynum;
				}
				break;
			case K_CUBE:
				if (state == 1) {
					flash_cube = true;
					key = keynum;
				}
				else if (state == 3) {
					flash_cube = false;
					last_key = keynum;
				}
				break;
			case K_BS_FLASH:
				if (state == 1) {
					flash_waste_of_button = true;
					key = keynum;
				}
				else if (state == 3) {
					flash_waste_of_button = false;
					last_key = keynum;
				}
				break;

			case K_LOW:
				if (state == 1) {
					scoring_level = LOW;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_MID:
				if (state == 1) {
					scoring_level = MID;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_HIGH:
				if (state == 1) {
					scoring_level = HIGH;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;

			case K_CENTER:
				if (state == 1) {
					scoring_offset = CENTER;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_LEFT:
				if (state == 1) {
					scoring_offset = LEFT;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_RIGHT:
				if (state == 1) {
					scoring_offset = RIGHT;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_FAR_LEFT:
				if (state == 1) {
					scoring_offset = FAR_LEFT;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_FAR_RIGHT:
				if (state == 1) {
					scoring_offset = FAR_RIGHT;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;

			case K_ONE:
				if (state == 1) {
					scoring_slot = ONE;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_TWO:
				if (state == 1) {
					scoring_slot = TWO;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_THREE:
				if (state == 1) {
					scoring_slot = THREE;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_FOUR:
				if (state == 1) {
					scoring_slot = FOUR;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_FIVE:
				if (state == 1) {
					scoring_slot = FIVE;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_SIX:
				if (state == 1) {
					scoring_slot = SIX;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_SEVEN:
				if (state == 1) {
					scoring_slot = SEVEN;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_EIGHT:
				if (state == 1) {
					scoring_slot = EIGHT;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_NINE:
				if (state == 1) {
					scoring_slot = NINE;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;

			case K_AUTOPLUS:
				if (state == 1) {
					autoplus = true;
					key = keynum;
				}
				else if (state == 3) {
					autoplus = false;
					last_key = keynum;
				}
				break;
			case K_AUTOMINUS:
				if (state == 1) {
					autominus = true;
					key = keynum;
				}
				else if (state == 3) {
					autominus = false;
					last_key = keynum;
				}
				break;
			case K_SCORE:
				if (state == 1) {
					score = true;
					key = keynum;
				}
				else if (state == 3) {
					score = false;
					last_key = keynum;
				}
				break;
			case K_CONEBEAM:
				if (state == 1) {
					conebeam = !conebeam;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_CUBEBEAM:
				if (state == 1) {
					cubebeam = !cubebeam;
					key = keynum;
				}
				else if (state == 3) {
					last_key = keynum;
				}
				break;
			case K_STOPEE:
				if (state == 1) {
					stopEE = true;
				}
				else if (state == 3) {
				}
				break;
			case K_ZERO:
				if (state == 1) {
					zero = true;
					stopEE = false;
					key = keynum;
				}
				else if (state == 3) {
					zero = false;
					last_key = keynum;
				}
				break;


			case K_ESCU:
				if (state == 1) {
					escu = true;
					key = keynum;
				}
				else if (state == 3) {
					escu = false;
					last_key = keynum;
				}
				break;
			case K_ESCD:
				if (state == 1) {
					escd = true;
					key = keynum;
				}
				else if (state == 3) {
					escd = false;
					last_key = keynum;
				}
				break;
			case K_LNXU:
				if (state == 1) {
					lnxu = true;
					key = keynum;
				}
				else if (state == 3) {
					lnxu = false;
					last_key = keynum;
				}
				break;
			case K_LNXD:
				if (state == 1) {
					lnxd = true;
					key = keynum;
				}
				else if (state == 3) {
					lnxd = false;
					last_key = keynum;
				}
				break;
			case K_WRSU:
				if (state == 1) {
					wrsu = true;
					key = keynum;
				}
				else if (state == 3) {
					wrsu = false;
					last_key = keynum;
				}
				break;
			case K_WRSD:
				if (state == 1) {
					wrsd = true;
					key = keynum;
				}
				else if (state == 3) {
					wrsd = false;
					last_key = keynum;
				}
				break;
			case K_INTU:
				if (state == 1) {
					intu = true;
					key = keynum;
				}
				else if (state == 3) {
					intu = false;
					last_key = keynum;
				}
				break;
			case K_INTD:
				if (state == 1) {
					intd = true;
					key = keynum;
				}
				else if (state == 3) {
					intd = false;
					last_key = keynum;
				}
				break;

			default:
				break;
			};
		}
	}

	SendToDS();
	


	if (last_key != -1) {
		buffer[1] = 181;
		buffer[2] = last_key;
		buffer[3] = 0;
		buffer[4] = 0;
		FastWrite(hDevice, buffer);
	}
	if (key != -1) {
		buffer[1] = 181;
		buffer[2] = key;
		buffer[3] = 1;
		buffer[4] = 0;
		SlowWrite(hDevice, buffer);
	}


	StateLEDs();

	for (int i=0;i<readlength;i++)
	{
		lastpData[i]=pData[i];  //save it for comparison on next read
	}
	//end Buttons

	
	//error handling
	if (error==307)
	{
		CleanupInterface(hDevice);
		MessageBeep(MB_ICONHAND);
		AddEventMsg(hDialog, "Device Disconnected");
	}
	
	return TRUE;
}
//------------------------------------------------------------------------

DWORD __stdcall HandleErrorEvent(DWORD deviceID, DWORD status)
{
	MessageBeep(MB_ICONHAND);
	AddEventMsg(hDialog, "Error from error callback");
	return TRUE;
}
//------------------------------------------------------------------------


void SendToDS() {
	buffer[1] = 202;
	buffer[2] = (127 * (1 + escd - escu))^127;
	buffer[3] = (127 * (1 + lnxd - lnxu))^127;
	buffer[4] = (127 * (1 + wrsd - wrsu))^127;
	buffer[7] = flash_cone | flash_cube << 1 | ((scoring_offset * 3 + scoring_level) << 2) | conebeam << 6 | cubebeam << 7;
	buffer[8] = score | stopEE << 1 | zero << 2 | autoplus << 3 | autominus << 4 | intu << 5 | intd << 6 | flash_waste_of_button << 7;
	buffer[12] = scoring_slot;

	//buffer[7] = game_piece | 1 << (1 + scoring_level) | 1 << (4 + scoring_slot);
	//buffer[8] = 1 << (scoring_slot - 4);
	//buffer[7] = game_piece | ((scoring_slot * 3 + scoring_level) << 1);
	//buffer[8] = score | conebeam << 1 | cubebeam << 2 | stopEE << 3 | zero << 4 | autominus << 5 | autoplus << 6;
	//buffer[9] = escu | escd << 1 | lnxu << 2 | lnxd << 3 | wrsu << 4 | wrsd << 5 | intu << 6 | intd << 7;

	SlowWrite(hDevice, buffer);
}


void ClearLEDs() {
	for (int i = 0; i < 80; i++) {
		buffer[1] = 181;
		buffer[2] = i;
		buffer[3] = 0;
		buffer[4] = 0;
		SlowWrite(hDevice, buffer);
		buffer[2] = i + 80;
		SlowWrite(hDevice, buffer);
	}
}

void StateLEDs() {
	buffer[1] = 181;
	buffer[2] = K_CONE + 80;
	buffer[3] = flash_cone * 2;
	buffer[4] = 0;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_CUBE + 80;
	buffer[3] = flash_cube * 2;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_BS_FLASH + 80;
	buffer[3] = flash_waste_of_button * 2;
	SlowWrite(hDevice, buffer);


	buffer[2] = K_HIGH + 80;
	buffer[3] = scoring_level == HIGH;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_MID + 80;
	buffer[3] = scoring_level == MID;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_LOW + 80;
	buffer[3] = scoring_level == LOW;
	SlowWrite(hDevice, buffer);

	buffer[2] = K_CENTER + 80;
	buffer[3] = scoring_offset == CENTER;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_LEFT + 80;
	buffer[3] = scoring_offset == LEFT;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_RIGHT + 80;
	buffer[3] = scoring_offset == RIGHT;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_FAR_LEFT + 80;
	buffer[3] = scoring_offset == FAR_LEFT;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_FAR_RIGHT + 80;
	buffer[3] = scoring_offset == FAR_RIGHT;
	SlowWrite(hDevice, buffer);

	buffer[2] = K_ONE + 80;
	buffer[3] = scoring_slot == ONE;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_TWO + 80;
	buffer[3] = scoring_slot == TWO;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_THREE + 80;
	buffer[3] = scoring_slot == THREE;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_FOUR + 80;
	buffer[3] = scoring_slot == FOUR;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_FIVE + 80;
	buffer[3] = scoring_slot == FIVE;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_SIX + 80;
	buffer[3] = scoring_slot == SIX;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_SEVEN + 80;
	buffer[3] = scoring_slot == SEVEN;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_EIGHT + 80;
	buffer[3] = scoring_slot == EIGHT;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_NINE + 80;
	buffer[3] = scoring_slot == NINE;
	SlowWrite(hDevice, buffer);


	buffer[2] = K_CONEBEAM + 80;
	buffer[3] = conebeam;
	SlowWrite(hDevice, buffer); 
	buffer[2] = K_CUBEBEAM + 80;
	buffer[3] = cubebeam;
	SlowWrite(hDevice, buffer);

	buffer[2] = K_STOPEE;
	buffer[3] = stopEE * 2;
	SlowWrite(hDevice, buffer);
	buffer[2] = K_STOPEE + 80;
	buffer[3] = stopEE*2;
	SlowWrite(hDevice, buffer);
}


void SlowWrite(long hnd, UCHAR* data) {
	DWORD result = 404;
	while (result == 404)
	{
		result = WriteData(hnd, data);
	}
}
