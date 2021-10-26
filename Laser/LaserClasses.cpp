
#include "LaserClasses.h"

int Laser::connect(String^ hostName, int portNumber)
{
	// String command to ask for Channel 1 analogue voltage from the PLC
	// These command are available on Galil RIO47122 command reference manual
	// available online
	String^ AskScan = gcnew String("sRN LMDscandata");
	// String to store received data for display
	String^ ResponseData;

	// Create TcpClient and connect to it
	Client = gcnew TcpClient(hostName, portNumber);
	// Configure connection settings
	Client->NoDelay = true;
	Client->ReceiveTimeout = 1500;//ms
	Client->SendTimeout = 1500;//ms
	Client->ReceiveBufferSize = 2048;
	Client->SendBufferSize = 1024;

	// unsigned char arrays of 16 bytes each are created on managed heap
	SendData = gcnew array<unsigned char>(16);
	ReadData = gcnew array<unsigned char>(2500);

	// Get the network streab object associated with clien so we 
	// can use it to read and write
	Stream = Client->GetStream();

	// Authenticate user
	System::String^ Auth = gcnew String("5260252\n");
	// Convert string command to an array of unsigned char
	SendData = System::Text::Encoding::ASCII->GetBytes(Auth);
	Stream->Write(SendData, 0, SendData->Length);
	// Wait for the server to prepare the data, 1 ms would be sufficient, but used 10 ms
	System::Threading::Thread::Sleep(10);
	// Read the incoming data
	Stream->Read(ReadData, 0, ReadData->Length);
	// Convert incoming data from an array of unsigned char bytes to an ASCII string
	ResponseData = System::Text::Encoding::ASCII->GetString(ReadData);
	// Print the received string on the screen - AUTH CHECK - OK
	Console::WriteLine(ResponseData);

	SendData = System::Text::Encoding::ASCII->GetBytes(AskScan);

	return 1;
}
int Laser::setupSharedMemory()
{
	// Setting up shared memory objects, providing create/access and pointers to smstructs
	// ProcessManagement
	ProcessManagementData = new SMObject(_TEXT("PMObj"), sizeof(ProcessManagement));
	ProcessManagementData->SMCreate();
	ProcessManagementData->SMAccess();
	PMData = (ProcessManagement*)ProcessManagementData->pData;

	// Laser
	SensorData = new SMObject(_TEXT("Laser"), sizeof(SM_Laser));
	SensorData->SMCreate();
	SensorData->SMAccess();
	LaserData = (SM_Laser*)SensorData->pData;

	// Local shutdown setup
	PMData->Shutdown.Flags.Laser = 0;

	return 1;
}
int Laser::getData()
{
	String^ ResponseData;
	// Write command asking for data
	Stream->WriteByte(0x02);
	Stream->Write(SendData, 0, SendData->Length);
	Stream->WriteByte(0x03);
	// Wait for the server to prepare the data, 1 ms would be sufficient, but used 10 ms
	System::Threading::Thread::Sleep(10);
	// Read the incoming data
	Stream->Read(ReadData, 0, ReadData->Length);
	// Convert incoming data from an array of unsigned char bytes to an ASCII string
	ResponseData = System::Text::Encoding::ASCII->GetString(ReadData);

	// Print the received string on the screen - RAW DATA
	//Console::WriteLine(ResponseData);

	// Split laser data into individual sub strings
	StringArray = ResponseData->Split(' ');

	return 1;
}
int Laser::checkData()
{
	// Check if the data packet recieved is a complete packet
	if (StringArray[1] != "LMDscandata" && System::Convert::ToInt32(StringArray[25], 16) != 361)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
int Laser::sendDataToSharedMemory()
{
	// 16 is the hex format
	StartAngle = System::Convert::ToInt32(StringArray[23], 16);
	Resolution = System::Convert::ToInt32(StringArray[24], 16) / 10000.0;
	NumRanges = System::Convert::ToInt32(StringArray[25], 16);

	LaserData->NumRanges = NumRanges;

	Range = gcnew array<double>(NumRanges);
	RangeX = gcnew array<double>(NumRanges);
	RangeY = gcnew array<double>(NumRanges);

	for (int i = 0; i < NumRanges; i++) {
		// Convert raw to X and Y and from rads to degs
		Range[i] = System::Convert::ToInt32(StringArray[26 + i], 16);
		RangeX[i] = Range[i] * Math::Sin((i * Resolution) * (Math::PI / 180.0));
		RangeY[i] = -Range[i] * Math::Cos((i * Resolution) * (Math::PI / 180.0));
		// Store in SM
		LaserData->x[i] = RangeX[i];
		LaserData->y[i] = RangeY[i];
		Console::WriteLine("{0} \tx: {1,10:F6} \ty: {2,10:F6}", i + 1, LaserData->x[i], LaserData->y[i]);
	}
	std::cout << "" << std::endl;
	return 1;
}
bool Laser::getShutdownFlag()
{
	// Return shutdown flag for laser
	return PMData->Shutdown.Flags.Laser;
}
int Laser::setHeartbeat(bool heartbeat)
{
	// Set heartbeat flag for laser
	PMData->Heartbeat.Flags.Laser = 1;
	// Check PM heartbeat
	if (PMData->PMHeartbeat.Flags.Laser == 1)
	{
		PMData->PMHeartbeat.Flags.Laser = 0;
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
Laser::~Laser()
{
	Stream->Close();
	Client->Close();
	delete ProcessManagementData;
	delete SensorData;
}
