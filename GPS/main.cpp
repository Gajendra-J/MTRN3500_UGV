
#using <System.dll>
#include <Windows.h>
#include <conio.h>

#include "GPS.h"

#include <SMStructs.h>
#include <SMObject.h>

using namespace System;
using namespace System::Threading;

int main()
{
    // Setting up shared Memory Objects and providing Create/Access
    SMObject PMObj(_TEXT("PMObj"), sizeof(ProcessManagement));
    SMObject GPSObj(_TEXT("GPS"), sizeof(SM_GPS));

    PMObj.SMCreate();
    PMObj.SMAccess();
    GPSObj.SMCreate();
    GPSObj.SMAccess();

    // Pointers to smstruct
    ProcessManagement* PMData = (ProcessManagement*)PMObj.pData;
    SM_GPS* GPSData = (SM_GPS*)GPSObj.pData;

    // Local Setup
    PMData->Shutdown.Flags.GPS = 0;
    int WaitAndSeeTime = 0;

    while (!PMData->Shutdown.Flags.GPS)
    {
        // Set heartbeat flag
        PMData->Heartbeat.Flags.GPS = 1;
        // Check PM heartbeat
        if (PMData->PMHeartbeat.Flags.GPS == 1)
        {
            PMData->PMHeartbeat.Flags.GPS = 0;
            WaitAndSeeTime = 0;
        }
        else
        {
            if (++WaitAndSeeTime > 50)
            {
                // If no response from PM, request shutdown all
                PMData->Shutdown.Status = 0xFF;
            }
        }

        // Put GPS Module Code Below

        if (_kbhit())
        {
            break;
        }

        Thread::Sleep(20);
    }

    return 0;
}