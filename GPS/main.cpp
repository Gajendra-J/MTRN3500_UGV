
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

// 112 bytes
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
            if (++WaitAndSeeTime > 50)
            {
                // If no response from PM, request shutdown all
                PMData->Shutdown.Status = 0xFF;
            }
        }

        Thread::Sleep(10);
        if (Stream->DataAvailable) {
            Stream->Read(ReadData, 0, ReadData->Length);
        }

        // Read/Trapping the Header
        unsigned int Header = 0;
        int i = 0;
        int Start; //Start of data
        unsigned char Data;
        do
        {
            Data = ReadData[i++];
            Header = ((Header << 8) | Data);
        }
        while (Header != 0xaa44121c && i < ReadData->Length);
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
        Console::WriteLine("CalcCRC: {0}, ServerCRC: {1}, Equal {2}", GeneratedCRC, NovatelGPS.CRC, GeneratedCRC == NovatelGPS.CRC);
        
        // Store in SM if matching
        if (GeneratedCRC == NovatelGPS.CRC)
        {
            GPSData->Easting = NovatelGPS.Easting;
            GPSData->Northing = NovatelGPS.Northing;
            GPSData->Height = NovatelGPS.Height;

            Thread::Sleep(50);
            // Print northing, easting, height from SM
            Console::Write("Northing: {0}, \tEasting: {1}, \tHeight: {2}\n\n", GPSData->Northing, GPSData->Easting, GPSData->Height);
        }
        else
        {
            Console::WriteLine("CRC Values did not match");
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