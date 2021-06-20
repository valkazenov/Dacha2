#pragma once
#include <dht.h>
#include "BaseSensorScanner.h"

class SensorScanner: public BaseSensorScanner
{
private:
	dht dHT;
	int pinDHT;
	int pinAmperage;

public:
	double humidity;
	double temperature;
	double boilerAmperage;

	SensorScanner() : BaseSensorScanner() {
		humidity = 0;
		temperature = 0;
		boilerAmperage = 0;
	}

	void init(int pinDHT, int pinAmperage) {
		pinMode(pinDHT, INPUT);
		this->pinDHT = pinDHT;
		this->pinAmperage = pinAmperage;
	}

	void scan() {
		BaseSensorScanner::scan();
		int dhtResult = dHT.read22(pinDHT);
		if (dhtResult == DHTLIB_OK) {
			humidity = dHT.humidity;
			temperature = dHT.temperature;
		}
		int amperage = abs(analogRead(pinAmperage) - 512);
		if (amperage < 20) amperage = 0;
		boilerAmperage = (double)amperage / 1024 * supplyVoltage / 100;
		boilerAmperage = boilerAmperage / 1.414;
	}
};

