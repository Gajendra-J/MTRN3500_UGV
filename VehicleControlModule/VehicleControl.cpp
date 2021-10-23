
#using <System.dll>
#include <Windows.h>
#include <conio.h>
#include <math.h>

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
    unsigned int flag = 0;
    double Speed;
    double Steer;

    // VEHICLE
    // Vehicle port number must be 25000
    int PortNumber = 25000;
    // Pointer to TcpClent type object on managed heap
    TcpClient^ Client;
    // arrays of unsigned chars to send data
    array<unsigned char>^ SendData;

    String^ Auth = gcnew String("5260252\n");

    // Creat TcpClient object and connect to it
    Client = gcnew TcpClient("192.168.1.200", PortNumber);
    // Configure connection
    Client->NoDelay = true;
    Client->ReceiveTimeout = 2000;//ms LONGER FOR AUTH
    Client->SendTimeout = 500;//ms
    Client->ReceiveBufferSize = 1024;
    Client->SendBufferSize = 1024;

    // unsigned char arrays of 16 bytes each are created on managed heap
    SendData = gcnew array<unsigned char>(50);

    // Get the network streab object associated with clien so we 
    // can use it to read and write
    NetworkStream^ Stream = Client->GetStream();

    // Authenticate user
    // Convert string command to an array of unsigned char
    SendData = System::Text::Encoding::ASCII->GetBytes(Auth);
    Stream->Write(SendData, 0, SendData->Length);
    // Resets buffer
    SendData = gcnew array<unsigned char>(50);

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
            if (++WaitAndSeeTime > 200)
            {
                // If no response from PM, request shutdown all
                PMData->Shutdown.Status = 0xFF;
            }
        }

        // Vehicle things
        // random testing values
        Speed = VehicleControlData->Speed;
        Steer = VehicleControlData->Steering;
        // Needs to be inverse based of weeders response in camera
        Steer = -Steer;

        // # <steer> <speed> <flag> # format
        String^ Controls = gcnew String("# " + Steer.ToString("f2") + " " + Speed.ToString("f2") + " " + flag + " #");
        Console::WriteLine(Controls);
        SendData = System::Text::Encoding::ASCII->GetBytes(Controls);

        Stream->WriteByte(0x02);
        Stream->Write(SendData, 0, SendData->Length);
        Stream->WriteByte(0x03);

        System::Threading::Thread::Sleep(100);

        flag = !flag;

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