
#using <System.dll>
#include <Windows.h>
#include <conio.h>

#include <SMStructs.h>
#include <SMObject.h>

using namespace System;
using namespace System::Threading;

int main()
{
    // Setting up shared Memory Objects and providing Create/Access
    SMObject PMObj(_TEXT("PMObj"), sizeof(ProcessManagement));
    SMObject LaserObj(_TEXT("Laser"), sizeof(SM_Laser));

    PMObj.SMCreate();
    PMObj.SMAccess();
    LaserObj.SMCreate();
    LaserObj.SMAccess();

    // Pointers to smstruct
    ProcessManagement* PMData = (ProcessManagement*)PMObj.pData;
    SM_Laser* LaserData = (SM_Laser*)LaserObj.pData;

    // Local Setup
    PMData->Shutdown.Flags.Laser = 0;
    int WaitAndSeeTime = 0;

    while (!PMData->Shutdown.Flags.Laser)
    {
        // Set heartbeat flag
        PMData->Heartbeat.Flags.Laser = 1;
        // Check PM heartbeat
        if (PMData->PMHeartbeat.Flags.Laser == 1)
        {
            PMData->PMHeartbeat.Flags.Laser = 0;
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

        // Put Laser Module Code Below

        if (_kbhit())
        {
            break;
        }

        Thread::Sleep(20);
    }

	return 0;
}