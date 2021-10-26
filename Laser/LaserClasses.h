#pragma once
#include <UGV_module.h>
#include <smstructs.h>

ref class Laser : public UGV_module
{

public:
	int connect(String^ hostName, int portNumber) override;
	int setupSharedMemory() override;
	int getData() override;
	int checkData() override;
	int sendDataToSharedMemory() override;
	bool getShutdownFlag() override;
	int setHeartbeat(bool heartbeat) override;
	~Laser();

protected:
	ProcessManagement* PMData;
	SM_VehicleControl* VehicleControlData;

	array<unsigned char>^ SendData; // arrays of unsigned chars to send data
	int WaitAndSeeTime = 0; // timer for heartbeats
	unsigned int flag = 0; // field required for UGV control
	double Speed; // to store speed controls
	double Steer; // to store steering controls
};