#pragma once
#include <dht.h>
#include <iarduino_RTC.h>
#include <HCSR04.h>
#include "BaseSensorScanner.h"

class SensorScanner : public BaseSensorScanner
{
private:
	dht dHT;
	HCSR04 distSensor;
	int pinDHT;
	int pinRain;
public:
	double humidity;
	double temperature;
	double waterDistance;
	int tankLevel;
	int rainPercent;
	iarduino_RTC clock;

	SensorScanner(int pinDistOut, int pinDistEcho) : clock(RTC_DS3231), distSensor(pinDistOut, pinDistEcho), BaseSensorScanner() {
		humidity = 0;
		temperature = 0;
		waterDistance = 0;
		tankLevel = 0;
	}

	void init(int pinDHT, int pinRain) {
		//clock.settime(30, 1, 7, 6, 5, 2021, 4);
		pinMode(pinDHT, INPUT);
		this->pinDHT = pinDHT;
		this->pinRain = pinRain;
		clock.begin();
	}

	void scan() {
		BaseSensorScanner::scan();
		int dhtResult = dHT.read22(pinDHT);
		if (dhtResult == DHTLIB_OK) {
			humidity = dHT.humidity;
			temperature = dHT.temperature;
		}
		clock.gettime();
		waterDistance = distSensor.dist();
		if (waterDistance <= 100)
			tankLevel = (100 - waterDistance) * 10;
		else tankLevel = 0;
		rainPercent = analogRead(pinRain);
		if (rainPercent <= 1023)
			rainPercent = 1023 - rainPercent;
		else rainPercent = 0;
		rainPercent = map(rainPercent, 0, 1023, 0, 100);

	}

};

