
#using <System.dll>
#include <Windows.h>
#include <conio.h>

#include "GPS.h"

#include <SMStructs.h>
#include <SMObject.h>

using namespace System;
using namespace System::Threading;

using namespace System::Net::Sockets;
using namespace System::Net;
using namespace System::Text;

using namespace System::IO::Ports;

// MAIN AFTER CLASS IMPLEMENTATIION
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

/*

// MAIN BEFORE CLASS IMPLEMENTATIION

// 112 bytes
#pragma pack(1)
struct GPSStruct
{
    unsigned int Header;
    unsigned char Discards1[40];
    double Northing;
    double Easting;
    double Height;
    unsigned char Discards2[40];
    unsigned int CRC;
}NovatelGPS;

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

    // GPS
    // GPS port number must be 24000
    int PortNumber = 24000;
    // Pointer to TcpClent type object on managed heap
    TcpClient^ Client;
    // arrays of unsigned chars to send and receive data
    array<unsigned char>^ SendData;
    array<unsigned char>^ ReadData;

    // Creat TcpClient object and connect to it
    Client = gcnew TcpClient("192.168.1.200", PortNumber);
    // Configure connection
    Client->NoDelay = true;
    Client->ReceiveTimeout = 5000;//ms
    Client->SendTimeout = 5000;//ms
    Client->ReceiveBufferSize = 1024;
    Client->SendBufferSize = 1024;

    // unsigned char arrays of 16 bytes each are created on managed heap
    SendData = gcnew array<unsigned char>(16);
    // Reading double+ the size of a GPS data pack so 
    // one full packet of data is almost granteed to be in there
    ReadData = gcnew array<unsigned char>(sizeof(GPSStruct) * 2);

    // Get the network streab object associated with clien so we 
    // can use it to read and write
    NetworkStream^ Stream = Client->GetStream();

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
            if (++WaitAndSeeTime > 200)
            {
                // If no response from PM, request shutdown all
                PMData->Shutdown.Status = 0xFF;
            }
        }

        
        if (Stream->DataAvailable) {
            Thread::Sleep(10);
            Stream->Read(ReadData, 0, ReadData->Length);

            // Read/Trapping the Header
            unsigned int Header = 0;
            int i = 0;
            int Start; //Start of data
            unsigned char Data;
            do
            {
                Data = ReadData[i++];
                Header = ((Header << 8) | Data);
            } while (Header != 0xaa44121c && i < ReadData->Length);
            Start = i - 4;

            unsigned char* BytePtr = nullptr;
            // Filling data
            BytePtr = (unsigned char*)&NovatelGPS;
            for (int i = Start; i < Start + sizeof(NovatelGPS); i++)
            {
                *(BytePtr++) = ReadData[i];
            }

            // Compare CRC values
            unsigned char* bytePtr = (unsigned char*)&NovatelGPS;
            unsigned int GeneratedCRC = CalculateBlockCRC32(sizeof(NovatelGPS) - 4, bytePtr);
            // Print the CRC values
            Console::WriteLine("Calculated CRC: {0}", GeneratedCRC);
            Console::WriteLine("Server CRC: {0}", NovatelGPS.CRC);
            Console::WriteLine("Are the CRC Values Equal: {0}", GeneratedCRC == NovatelGPS.CRC);

            // Store in SM if matching
            if (GeneratedCRC == NovatelGPS.CRC)
            {
                GPSData->Easting = NovatelGPS.Easting;
                GPSData->Northing = NovatelGPS.Northing;
                GPSData->Height = NovatelGPS.Height;

                Thread::Sleep(50);
                
                // Print northing, easting, height from SM
                Console::Write("Northing: {0,10:F6}\n", GPSData->Northing);
                Console::Write("Easting: {0,10:F6}\n", GPSData->Easting);
                Console::Write("Height: {0,10:F6}\n\n", GPSData->Height);
            }
            else
            {
                Console::WriteLine("CRC Values did not match");
            }
        }

        if (_kbhit())
        {
            break;
        }

        Thread::Sleep(20);
    }

    Stream->Close();
    Client->Close();

    return 0;
}
*/