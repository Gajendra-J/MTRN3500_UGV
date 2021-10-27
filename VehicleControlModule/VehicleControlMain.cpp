
#using <System.dll>
#include <Windows.h>
#include <conio.h>
#include <math.h>

#include <SMStructs.h>
#include <SMObject.h>
#include "VehicleControlClasses.h"

using namespace System;
using namespace System::Threading;

int main()
{
    VehicleControl VC;

    // Setup shared memory
    VC.setupSharedMemory();

    // Connect to client with 25000 port for VC
    int PortNumber = 25000;
    VC.connect("192.168.1.200", PortNumber);

    // While not shutdown
    while (!VC.getShutdownFlag())
    {
        // Set and check heatbeats of VC and PM
        bool heartbeat = true;
        VC.setHeartbeat(heartbeat);

        // Set controls and send
        VC.getData();

        if (_kbhit())
        {
            break;
        }

        Thread::Sleep(20);
    }

    return 0;
}