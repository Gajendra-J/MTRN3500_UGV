
#include "VehicleControlClasses.h"

int VehicleControl::connect(String^ hostName, int portNumber)
{
	// Create TcpClient and connect to it
	Client = gcnew TcpClient(hostName, portNumber);
	// Configure connection settings
	Client->NoDelay = true;
	Client->ReceiveTimeout = 2000;//ms - longer for auth
	Client->SendTimeout = 500;//ms
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;

	// Unsigned char arrays of 16 bytes each are created on managed heap
	SendData = gcnew array<unsigned char>(50);
	// Get the network stream object associated with client so we can use it to read and write
	Stream = Client->GetStream();

	// Authenticate user
	System::String^ Auth = gcnew System::String("5260252\n");
	// Convert string command to an array of unsigned char
	SendData = System::Text::Encoding::ASCII->GetBytes(Auth);
	Stream->Write(SendData, 0, SendData->Length);
	// Resets buffer
	SendData = gcnew array<unsigned char>(50);

	return 1;
}
int VehicleControl::setupSharedMemory()
{
	// Setting up shared memory objects, providing create/access and pointers to smstructs
	// ProcessManagement
	ProcessManagementData = new SMObject(_TEXT("PMObj"), sizeof(ProcessManagement));
	ProcessManagementData->SMCreate();
	ProcessManagementData->SMAccess();
	PMData = (ProcessManagement*)ProcessManagementData->pData;

	// VehicleControl
	SensorData = new SMObject(_TEXT("VehicleControl"), sizeof(SM_VehicleControl));
	SensorData->SMCreate();
	SensorData->SMAccess();
	VehicleControlData = (SM_VehicleControl*)SensorData->pData;

	// Local shutdown setup
	PMData->Shutdown.Flags.VehicleControl = 0;

	return 1;
}
int VehicleControl::getData()
{
	// Store VC data from display locally
	Speed = VehicleControlData->Speed;
	Steer = VehicleControlData->Steering;
	// Needs to be inverse based of weeders response in camera
	Steer = -Steer;

	// String format: # <steer> <speed> <flag> #
	String^ Controls = gcnew String("# " + Steer.ToString("f2") + " " + Speed.ToString("f2") + " " + flag + " #");
	Console::WriteLine(Controls);
	// Send controls
	SendData = System::Text::Encoding::ASCII->GetBytes(Controls);
	Stream->WriteByte(0x02);
	Stream->Write(SendData, 0, SendData->Length);
	Stream->WriteByte(0x03);

	System::Threading::Thread::Sleep(100);

	// Flip flag to indicate active control - 0/1
	flag = !flag;

	return 1;
}
int VehicleControl::checkData()
{
	// YOUR CODE HERE
	return 1;
}
int VehicleControl::sendDataToSharedMemory()
{
	// YOUR CODE HERE
	return 1;
}
bool VehicleControl::getShutdownFlag()
{
	// Return shutdown flag for VC
	return PMData->Shutdown.Flags.VehicleControl;
}
int VehicleControl::setHeartbeat(bool heartbeat)
{
	// Set heartbeat flag for VC
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

	return 1;
}
VehicleControl::~VehicleControl()
{
	Stream->Close();
	Client->Close();
	delete ProcessManagementData;
	delete SensorData;
}
