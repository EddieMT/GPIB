
/*
* setup cpp
*
*
*
*
*
*
*/

#include "stdafx.h"
#include "Header.h"
#include "ni4882.h"             // Communications medium/interface
#include <string>
#include <Windows.h>

static HANDLE SOT = NULL;
static HANDLE EOT = NULL;
static HANDLE sotThread = NULL;
static BOOL g_listeningForSOT = FALSE;
static BOOL g_listeningForEOT = FALSE;
static BOOL g_readyForSOT = FALSE;
static UINT activateSite = 0x0000000F;

int gpibBoard = 0;
int gpibDevice = 0;
int gpibPAD = 1;  //primary address
int gpibSAD = 0;  //second  address
short listen = 0;
bool getHandler = TRUE;
//------ for debug--------------------------------
int hardBins[4] = { 4,9,10,11 };
char *Fullsites = "Fullsites 0000000E";
static BOOL ProcessActiveSites(const char* fullSites)
{
	activateSite = 0x0000000F;    // Default to maximum available sites.
	BOOL validResponse = FALSE;
	char fullSiteStr[20];
	sscanf_s(fullSites, "%s%x", fullSiteStr, sizeof(fullSiteStr), &activateSite);
	if (_stricmp("Fullsites", fullSiteStr) == 0)
	{
		validResponse = TRUE;
	}
	else
	{
		//message: Fullsites don't macth
	}
	return validResponse;
}
DWORD WINAPI SotMonitor(LPVOID nothing)//static DWORD WINAPI SotMonitor(LPVOID nothing)
{
	BOOL waitingForResponse;
	int  gpibRC;
	char rsp;
	char buffer[64];
	int sts;
	while (g_listeningForSOT)
	{
		if (EOT)
		{
			WaitForSingleObject(EOT, INFINITE);
			waitingForResponse = TRUE;
		}
		while (g_listeningForSOT && waitingForResponse)
		{
			gpibRC = ibwait(gpibDevice, TIMO | RQS);
			sts = ThreadIbsta();
			if (sts & RQS)//debug sts & RQS
			{
				waitingForResponse = FALSE;
			}
		}
		if(g_listeningForSOT && !waitingForResponse)
		{
			ibrsp(gpibDevice, &rsp);
			//rsp = 0x41;//debug
			//printf("0x41\n");//debug
			
			if(rsp == 0x41)
			{
				ibwrt(gpibDevice,"FULLSITES?\r\n",sizeof("FULLSITES?\r\n"));
				printf("FULLSITES?\n");//debug
				ibrd(gpibDevice, buffer, sizeof(buffer));
				printf("%s\n", buffer);//debug
				//buffer[ThreadIbcnt()] = '\0';
				if (ProcessActiveSites(buffer))//
				{
					SetEvent(SOT);
					//g_listeningForSOT = FALSE;
				}
			}
			else
			{
			  //g_listeningForSOT = FALSE;
			  //message: Cannot find the SRQ 0x41
			}
		}
	}
	return 0;
}

extern "C" HANDLER_API int Eot(int* hardBins)
{
	char sortString[128];
	char echoString[128];
	char buffer[128];
	char binchar[4];
	int rc = -1;

		for (int i = 0; i < 4; i++)
		{
			//if (*(hardBins + i) > 9)
			//	binchar[i] = *(hardBins + i) + '0' - 10;
			if (*(hardBins + i) <= 9)
			{
				binchar[i] = *(hardBins + i) + '0';
			}
			else if (*(hardBins + i) == 10)
			{
				binchar[i] = 'A';
			}
			else if (*(hardBins + i) == 11)
			{
				binchar[i] = 'B';
			}
			else if (*(hardBins + i) == 12)
			{
				binchar[i] = 'C';
			}
			else if (*(hardBins + i) == 13)
			{
				binchar[i] = 'D';
			}
			else if (*(hardBins + i) == 14)
			{
				binchar[i] = 'E';
			}
			else if (*(hardBins + i) == 15)
			{
				binchar[i] = 'F';
			}
			else
			{
				binchar[i] = '0';
			}

			if ((activateSite & 1 << i) == 0)
				binchar[i] = '0';
		}
		//sprintf_s(sortString, sizeof(sortString), "BINON:00000000,00000000,00000000,0000%c%c%c%c;\r\n", binchar[3], binchar[2], binchar[1], binchar[0]);
		sprintf_s(sortString, sizeof(sortString), "BINON:00000000,00000000,00000000,0000%c%c%c%c;\r\n", binchar[3], binchar[2], binchar[1], binchar[0]);
		ibwrt(gpibDevice, sortString, sizeof(sortString));
		printf("%s", sortString);//debug
		sprintf_s(echoString, sizeof(echoString), "ECHO:00000000,00000000,00000000,0000%c%c%c%c;\r\n", binchar[3], binchar[2], binchar[1], binchar[0]);
		printf("%s", echoString);//debug
		ibrd(gpibDevice, buffer, sizeof(buffer));
		//strcmp("A", "A");
		//if (strcmp(buffer, echoString) == 0)
		if (memcmp(buffer, echoString,42) == 0)
		{
			//message:ECHOOK
			ibwrt(gpibDevice, "ECHOOK\r\n", sizeof("ECHOOK\r\n"));
			printf("ECHOOK\n");//debug
			//g_listeningForEOT = FALSE;
			SetEvent(EOT);
			return 0;
		}
		else
		{
			//message:ECHOOG
			ibwrt(gpibDevice, "ECHONG\r\n", sizeof("ECHONG\r\n"));
			printf("ECHONG\n");//debug
			printf("\n");//debug
			printf("\n");//debug
		}

	return rc;
}

extern "C" HANDLER_API int Setup()
{
	int err = -1;
	int rc = -1;
	gpibBoard = ibfindA("GPIB0");
	if(gpibBoard > 1)//debug gpibBoard > 1
	{
		if (!EOT)
			EOT = CreateEventA(NULL, FALSE, TRUE, NULL);
		SOT = CreateEventA(NULL, FALSE, FALSE, "SOT");
		if (SOT)
		{
			err = ibrsc(gpibBoard, 1); //request control
			err = ibtmo(gpibBoard, T1s); //adjust to timeout
			err = ibsic(gpibBoard);  // perform interface clear
			err = ibtmo(gpibBoard, T300ms); //
			err = ibln(gpibBoard, gpibPAD, gpibSAD, &listen); // check listen
			if (listen)
			{
				gpibDevice = ibdev(0, gpibPAD, gpibSAD, T1s, 1, 0);
				if (gpibDevice > 0)
				{
					getHandler = TRUE;
					return 0;
				}

			}
			else
			{
				//message: Not find handler
			}
		}
	}
	else
	{
		//message: Not find GPIB0
	}
	return rc;
}

extern "C" HANDLER_API int Start()
{
	int err = -1;
	int rc = -1;
	//SotMonitor
	if (getHandler)
	{
		SetEvent(EOT);
		g_listeningForSOT = TRUE;
		sotThread = CreateThread(NULL, 0, SotMonitor, NULL, 0, NULL);
		return 0;
	}
	return rc;
}
extern "C" HANDLER_API int Stop()
{
	int err = -1;
	int rc = -1;
	//SotMonitor
	g_listeningForSOT = FALSE;
	return 0;
}

int main()
{
	int rc = -1;
	rc = Setup();
	getHandler = TRUE;
	int i = 0;
	rc = Start();
	while (i < 10)
	{		
		WaitForSingleObject(SOT, INFINITE);
		rc = Eot(hardBins);		
		i++;
	}
	rc = Stop();
	return rc;
}
