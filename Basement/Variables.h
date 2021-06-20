#pragma once
class Variables
{
public:
	double humidity;
	double temperature;
	byte fanWorkMode;
	int fanDisableInterval;
	int fanEnableInterval;
	bool fanWorking;
	unsigned long fanChangeTime;
	bool boilerWorking;
	double boilerAmperage;
	int supplyVoltage;
};

