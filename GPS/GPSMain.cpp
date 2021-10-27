
#using <System.dll>
#include <Windows.h>
#include <conio.h>

#include "GPSClasses.h"

#include <SMStructs.h>
#include <SMObject.h>

using namespace System;
using namespace System::Threading;

using namespace System::Net::Sockets;
using namespace System::Net;
using namespace System::Text;

using namespace System::IO::Ports;

int main()
{
    GPS GPS;

    // Setup shared memory
    GPS.setupSharedMemory();

    // Connect to client with 24000 port for GPS
    int PortNumber = 24000;
    GPS.connect("192.168.1.200", PortNumber);

    // While not shutdown
    while (!GPS.getShutdownFlag())
    {
        // Set and check heatbeats of GPS and PM
        bool heartbeat = true;
        GPS.setHeartbeat(heartbeat);

        // Read/trap header and fill struct
        GPS.getData();

        // Compare CRC values
        if (GPS.checkData())
        {
            // If CRC values match, share to SM
            GPS.sendDataToSharedMemory();
        }

        if (_kbhit())
        {
            break;
        }

        Thread::Sleep(20);
    }

    return 0;
}