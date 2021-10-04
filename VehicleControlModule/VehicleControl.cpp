
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
    SMObject VehicleControlObj(_TEXT("VehicleControl"), sizeof(SM_VehicleControl));

    PMObj.SMCreate();
    PMObj.SMAccess();
    VehicleControlObj.SMCreate();
    VehicleControlObj.SMAccess();

    // Pointers to smstruct
    ProcessManagement* PMData = (ProcessManagement*)PMObj.pData;
    SM_VehicleControl* VehicleControlData = (SM_VehicleControl*)VehicleControlObj.pData;

    // Local Setup
    PMData->Shutdown.Flags.VehicleControl = 0;
    int WaitAndSeeTime = 0;

    while (!PMData->Shutdown.Flags.VehicleControl)
    {
        // Set heartbeat flag
        PMData->Heartbeat.Flags.VehicleControl = 1;
        // Check PM heartbeat
        if (PMData->PMHeartbeat.Flags.VehicleControl == 1)
        {
            PMData->PMHeartbeat.Flags.VehicleControl = 0;
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

        // Put Vehicle Control Module Code Below

        if (_kbhit())
        {
            break;
        }

        Thread::Sleep(20);
    }

    return 0;
}