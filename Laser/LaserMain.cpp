
#using <System.dll>
#include <Windows.h>
#include <conio.h>

#include <SMStructs.h>
#include <SMObject.h>
#include "LaserClasses.h"


using namespace System;
using namespace System::Threading;

using namespace System::Net::Sockets;
using namespace System::Net;
using namespace System::Text;

int main()
{
    Laser Laser;

    // Setup shared memory
    Laser.setupSharedMemory();

    // Connect to client with 23000 port for LMS151
    int PortNumber = 23000;
    Laser.connect("192.168.1.200", PortNumber);

    // While not shutdown
    while (!Laser.getShutdownFlag())
    {
        // Set and check heatbeats of Laser and PM
        bool heartbeat = true;
        Laser.setHeartbeat(heartbeat);

        // Ask, get and convert data to ASCII string
        Laser.getData();

        // Check data packet recieved is a complete packet
        if (Laser.checkData())
        {
            // Process data and share to memory
            Laser.sendDataToSharedMemory();
        }

        if (_kbhit())
        {
            break;
        }

        Thread::Sleep(50);
    }

    return 0;
}