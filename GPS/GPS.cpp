
#include "GPS.h"

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

int GPS::connect(String^ hostName, int portNumber)
{
	// Create TcpClient and connect to it
	Client = gcnew TcpClient(hostName, portNumber);
	// Configure connection settings
	Client->NoDelay = true;
	Client->ReceiveTimeout = 5000;//ms - longer for auth
	Client->SendTimeout = 5000;//ms
	Client->ReceiveBufferSize = 1024;
	Client->SendBufferSize = 1024;

	// unsigned char arrays of 16 bytes each are created on managed heap
	SendData = gcnew array<unsigned char>(16);
	// Reading double+ the size of a GPS data pack so one full packet of data is almost granteed to be in there
	ReadData = gcnew array<unsigned char>(sizeof(GPSStruct) * 2);

	// Get the network stream object associated with client so we can use it to read and write
	Stream = Client->GetStream();

	return 1;
}
int GPS::setupSharedMemory()
{
	// Setting up shared memory objects, providing create/access and pointers to smstructs
	// ProcessManagement
	ProcessManagementData = new SMObject(_TEXT("PMObj"), sizeof(ProcessManagement));
	ProcessManagementData->SMCreate();
	ProcessManagementData->SMAccess();
	PMData = (ProcessManagement*)ProcessManagementData->pData;

	// GPS
	SensorData = new SMObject(_TEXT("GPS"), sizeof(SM_GPS));
	SensorData->SMCreate();
	SensorData->SMAccess();
	GPSData = (SM_GPS*)SensorData->pData;

	// Local shutdown setup
	PMData->Shutdown.Flags.GPS = 0;

	return 1;
}
int GPS::getData()
{
	if (Stream->DataAvailable) {
		System::Threading::Thread::Sleep(10);
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

	}
	return 1;
}
int GPS::checkData()
{
	// Compare CRC values
	unsigned char* bytePtr = (unsigned char*)&NovatelGPS;
	unsigned int GeneratedCRC = CalculateBlockCRC32(sizeof(NovatelGPS) - 4, bytePtr);
	// Print the CRC values
	Console::WriteLine("Calculated CRC: {0}", GeneratedCRC);
	Console::WriteLine("Server CRC: {0}", NovatelGPS.CRC);
	Console::WriteLine("Are the CRC Values Equal: {0}", GeneratedCRC == NovatelGPS.CRC);

	if (GeneratedCRC == NovatelGPS.CRC)
	{
		return 1;
	}
	else
	{
		Console::WriteLine("CRC Values did not match");
		return 0;
	}
}
int GPS::sendDataToSharedMemory()
{
	// Store in SM
	GPSData->Easting = NovatelGPS.Easting;
	GPSData->Northing = NovatelGPS.Northing;
	GPSData->Height = NovatelGPS.Height;

	System::Threading::Thread::Sleep(50);

	// Print northing, easting, height from SM
	Console::Write("Northing: {0,10:F6}\n", GPSData->Northing);
	Console::Write("Easting: {0,10:F6}\n", GPSData->Easting);
	Console::Write("Height: {0,10:F6}\n\n", GPSData->Height);

	return 1;
}
bool GPS::getShutdownFlag()
{
	// Return shutdown flag for GPS
	return PMData->Shutdown.Flags.GPS;
}
int GPS::setHeartbeat(bool heartbeat)
{
	// Set heartbeat flag for GPS
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

	return 1;
}
GPS::~GPS()
{
	Stream->Close();
	Client->Close();
	delete ProcessManagementData;
	delete SensorData;
}



unsigned long CRC32Value(int i)
{
	int j;
	unsigned long ulCRC;
	ulCRC = i;
	for (j = 8; j > 0; j--)
	{
		if (ulCRC & 1)
			ulCRC = (ulCRC >> 1) ^ CRC32_POLYNOMIAL;
		else
			ulCRC >>= 1;
	}
	return ulCRC;
}

unsigned long CalculateBlockCRC32(unsigned long ulCount, /* Number of bytes in the data block */
	unsigned char* ucBuffer) /* Data block */
{
	unsigned long ulTemp1;
	unsigned long ulTemp2;
	unsigned long ulCRC = 0;
	while (ulCount-- != 0)
	{
		ulTemp1 = (ulCRC >> 8) & 0x00FFFFFFL;
		ulTemp2 = CRC32Value(((int)ulCRC ^ *ucBuffer++) & 0xff);
		ulCRC = ulTemp1 ^ ulTemp2;
	}
	return(ulCRC);
}