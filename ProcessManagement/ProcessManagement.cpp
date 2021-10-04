
#using <System.dll>
#include <Windows.h>
#include <tchar.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <iostream>
#include <conio.h>

#include <SMStructs.h>
#include <SMObject.h>

using namespace System;
using namespace System::Net::Sockets;
using namespace System::Net;
using namespace System::Text;

#define NUM_UNITS 5
#define WAIT_TIME 10

bool IsProcessRunning(const char* processName);
void StartProcesses();

// Defining logical start up sequence for tele operation of UGV
TCHAR Units[10][20] = //
{
	TEXT("LASER.exe"),
	TEXT("Display.exe"),
	TEXT("VehicleControl.exe"),
	TEXT("GPS.exe"),
	TEXT("Camera.exe")
};

// Manage wait and see timers
struct waitCount
{
	unsigned int LASER = 0;
	unsigned int GPS = 0;
	unsigned int CAMERA = 0;
	unsigned int VehicleControl = 0;
	unsigned int Display = 0;
}WaitAndSeeTime;

int main()
{
	// Setting up shared Memory Objects and providing Create/Access
	SMObject PMObj(_TEXT("PMObj"), sizeof(ProcessManagement));
	SMObject LaserObj(_TEXT("Laser"), sizeof(SM_Laser));
	SMObject GPSObj(_TEXT("GPS"), sizeof(SM_GPS));
	SMObject VehicleControlObj(_TEXT("VehicleControl"), sizeof(SM_VehicleControl));

	PMObj.SMCreate();
	PMObj.SMAccess();
	LaserObj.SMCreate();
	LaserObj.SMAccess();
	GPSObj.SMCreate();
	GPSObj.SMAccess();
	VehicleControlObj.SMCreate();
	VehicleControlObj.SMAccess();

	// Pointers to smstruct
	ProcessManagement* PMData = (ProcessManagement*)PMObj.pData;
	SM_Laser* LaserData = (SM_Laser*)LaserObj.pData;
	SM_GPS* GPSData = (SM_GPS*)GPSObj.pData;
	SM_VehicleControl* VehicleControlData = (SM_VehicleControl*)VehicleControlObj.pData;

	// Set state of shutdown and heartbeat flags
	PMData->Shutdown.Status = 0x00;
	PMData->Heartbeat.Status = 0x00;

	//start all 5 modules
	StartProcesses();

	// Check heartbeats, updates PM heartbeat and
	// if a critical process is dead after wait time - request shutdown all,
	// if a non-critical process is dead after wait time - shutdown non-critical process and attempt restart
	while (!PMData->Shutdown.Flags.ProcessManagement)
	{
		Sleep(200);
		// Set PMs heartbeat as alive
		PMData->PMHeartbeat.Status = 0xFF;
		
		// CRITICAL PROCESSES
		if (PMData->Heartbeat.Flags.Laser == 1)
		{
			PMData->Heartbeat.Flags.Laser = 0;
			WaitAndSeeTime.LASER = 0;
		}
		else
		{
			WaitAndSeeTime.LASER++;
			if (WaitAndSeeTime.LASER > WAIT_TIME)
			{
				PMData->Shutdown.Status = 0xFF;
			}
		}

		if (PMData->Heartbeat.Flags.VehicleControl == 1)
		{
			PMData->Heartbeat.Flags.VehicleControl = 0;
			WaitAndSeeTime.VehicleControl = 0;
		}
		else
		{
			WaitAndSeeTime.VehicleControl++;
			if (WaitAndSeeTime.VehicleControl > WAIT_TIME)
			{
				PMData->Shutdown.Status = 0xFF;
			}
		}


		/*
		// NON-CRITICAL PRCESSES
		if (PMData->Heartbeat.Flags.GPS == 1)
		{
			PMData->Heartbeat.Flags.GPS = 0;
			WaitAndSeeTime.GPS = 0;
		}
		else
		{
			WaitAndSeeTime.GPS++;
			if (WaitAndSeeTime.GPS > WAIT_TIME)
			{
				PMData->Shutdown.Flags.GPS = 1;
				System::Threading::Thread::Sleep(100);
				// Restart - Should only startup GPS
				//StartProcesses();
				Console::WriteLine("Bro GPS IS FKED");
			}
		}
		*/
		// Routine shutdown by "Pressing a key"
		if (_kbhit())
		{
			break;
		}
	}

	return 0;
}


//Is process running function
bool IsProcessRunning(const char* processName)
{
	bool exists = false;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry))
		while (Process32Next(snapshot, &entry))
			if (!_stricmp((const char *)entry.szExeFile, processName))
				exists = true;

	CloseHandle(snapshot);
	return exists;
}


void StartProcesses()
{
	STARTUPINFO s[10];
	PROCESS_INFORMATION p[10];

	for (int i = 0; i < NUM_UNITS; i++)
	{
		if (!IsProcessRunning((const char *)Units[i]))
		{
			ZeroMemory(&s[i], sizeof(s[i]));
			s[i].cb = sizeof(s[i]);
			ZeroMemory(&p[i], sizeof(p[i]));

			if (!CreateProcess(NULL, Units[i], NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &s[i], &p[i]))
			{
				printf("%s failed (%d).\n", Units[i], GetLastError());
				_getch();
			}
			std::cout << "Started: " << Units[i] << std::endl;
			Sleep(100);
		}
	}
}

