#include <SPI.h>
#include <avr/eeprom.h>
#include "Packets.h"
#include "RadioListener.h"
#include "SensorScanner.h"
#include "MillisTimer.h"
#include "Rebooter.h"

#define PIN_DHT 2
#define PIN_FAN 3
#define PIN_BOILER 4
#define PIN_AMPERAGE 1
#define EEPROM_KEY 193
#define PIN_HC_SET 5
#define PIN_HC_RX 6
#define PIN_HC_TX 7
#define RADIO_SELF_ID 2
#define RADIO_MEGA_ID 1

RadioListener radioListener(PIN_HC_RX, PIN_HC_TX);
SensorScanner sensorScanner;
MillisTimer timer(1000);
Rebooter rebooter;
byte fanWorkMode;
int fanDisableInterval;
int fanEnableInterval;
bool fanWorking;
unsigned long fanChangeTime;
bool boilerWorking;
double autoFanHumidity;
unsigned long autoFanStartTime;

void initVariables() {
	fanWorkMode = 0;
	fanDisableInterval = 0;
	fanEnableInterval = 0;
	fanWorking = false;
	fanChangeTime = millis();
	boilerWorking = false;
	autoFanHumidity = 0;
	autoFanStartTime = 0;
}

void saveSettings() {
	eeprom_write_byte((uint8_t*)0, EEPROM_KEY);
	eeprom_write_byte((uint8_t*)1, (byte)boilerWorking);
	eeprom_write_byte((uint8_t*)2, fanWorkMode);
	eeprom_write_byte((uint8_t*)3, (byte)fanDisableInterval);
	eeprom_write_byte((uint8_t*)4, (byte)fanEnableInterval);
}

void loadSettings() {
	if (eeprom_read_byte((uint8_t*)0) == EEPROM_KEY) {
		boilerWorking = (bool)eeprom_read_byte((uint8_t*)1);
		fanWorkMode = eeprom_read_byte((uint8_t*)2);
		fanDisableInterval = (int)eeprom_read_byte((uint8_t*)3);
		fanEnableInterval = (int)eeprom_read_byte((uint8_t*)4);
	}
}

void setup() {
	Serial.begin(115200);
	rebooter.init(4000);
	radioListener.init(RADIO_SELF_ID, RADIO_MEGA_ID);
	sensorScanner.init(PIN_DHT, PIN_AMPERAGE);
	pinMode(PIN_FAN, OUTPUT);
	pinMode(PIN_BOILER, OUTPUT);
	initVariables();
	loadSettings();
	sensorScanner.scan();
	applySettings();
	delay(1000);
}

void checkAutoFan() {
	unsigned long time = millis();
	double deltaHumidity = sensorScanner.humidity - autoFanHumidity;
	if (fanWorking) {
		//Serial.print(time); Serial.print(" "); Serial.println(autoFanStartTime + (unsigned long)fanDisableInterval * 1000 * 60);
		if (deltaHumidity < -0.5) {
			Serial.println((String)"Humidity go down. Enabling continue. Humidity: " + sensorScanner.humidity + " " + autoFanHumidity);
			autoFanHumidity = sensorScanner.humidity;
			autoFanStartTime = millis();
		}
		else {
			bool doKey = false;
			if (deltaHumidity > 0.5) {
				Serial.println((String)"Fan disabled. Humidity is growing. Humidity: " + sensorScanner.humidity + " " + autoFanHumidity);
				doKey = true;
			}
			else if (time < autoFanStartTime || time > autoFanStartTime + (unsigned long)fanDisableInterval * 1000 * 60 * 60) {
				Serial.println((String)"Fan disabled. No changes for interval. Humidity: " + sensorScanner.humidity + " " + autoFanHumidity);
				doKey = true;
			}
			if (doKey) {
				fanWorking = false;
				digitalWrite(PIN_FAN, LOW);
				autoFanHumidity = sensorScanner.humidity;
				autoFanStartTime = millis();
				fanChangeTime = autoFanStartTime;
			}
		}
	}
	else {
		if (deltaHumidity < -0.5) {
			Serial.println((String)"Humidity go down. Disabling continue. Humidity: " + sensorScanner.humidity + " " + autoFanHumidity);
			autoFanHumidity = sensorScanner.humidity;
			autoFanStartTime = millis();
		}
		else if (time < autoFanStartTime || time > autoFanStartTime + (unsigned long)fanEnableInterval * 1000 * 60 * 60) {
			Serial.println((String)"Fan enabled. Interval expired. Humidity: " + sensorScanner.humidity + " " + autoFanHumidity);
			fanWorking = true;
			digitalWrite(PIN_FAN, HIGH);
			autoFanHumidity = sensorScanner.humidity;
			autoFanStartTime = millis();
			fanChangeTime = autoFanStartTime;
		}
	}
}

bool applySettings() {
	if (boilerWorking)
		digitalWrite(PIN_BOILER, HIGH);
	else digitalWrite(PIN_BOILER, LOW);
	bool oldFanWorking = fanWorking;
	if (fanWorkMode == 0) {
		fanWorking = false;
		digitalWrite(PIN_FAN, LOW);
	}
	else if (fanWorkMode == 1) {
		fanWorking = true;
		digitalWrite(PIN_FAN, HIGH);
	}
	else {
		fanWorking = true;
		digitalWrite(PIN_FAN, HIGH);
		autoFanHumidity = sensorScanner.humidity;
		autoFanStartTime = millis();
	}
	if (oldFanWorking != fanWorking)
		fanChangeTime = millis();
	return true;
}

bool processGetDataRequest() {
	getBasementDataAnswer.temperature = sensorScanner.temperature;
	getBasementDataAnswer.humidity = sensorScanner.humidity;
	getBasementDataAnswer.boilerWorking = boilerWorking;
	getBasementDataAnswer.boilerAmperage = sensorScanner.boilerAmperage;
	getBasementDataAnswer.fanWorking = fanWorking;
	getBasementDataAnswer.fanSeconds = (millis() - fanChangeTime) / 1000;
	getBasementDataAnswer.supplyVoltage = sensorScanner.supplyVoltage;
	return true;
}

bool processGetSettingsRequest() {
	getBasementSettingsAnswer.boilerEnabled = boilerWorking;
	getBasementSettingsAnswer.fanWorkMode = fanWorkMode;
	getBasementSettingsAnswer.fanDisableInterval = fanDisableInterval;
	getBasementSettingsAnswer.fanEnableInterval = fanEnableInterval;
	return true;
}

bool processSetSettingsRequest() {
	boilerWorking = setBasementSettingsRequest.boilerEnabled;
	fanWorkMode = setBasementSettingsRequest.fanWorkMode;
	fanDisableInterval = setBasementSettingsRequest.fanDisableInterval;
	fanEnableInterval = setBasementSettingsRequest.fanEnableInterval;
	if (applySettings()) {
		saveSettings();
		return true;
	}
	else return false;
}

bool processRequest(int requestId, int &answerId) {
	if (requestId == GETBASEMENTDATAREQUEST_ID) {
		answerId = GETBASEMENTDATAANSWER_ID;
		return processGetDataRequest();
	}
	else if (requestId == GETBASEMENTSETTINGSREQUEST_ID) {
		answerId = GETBASEMENTSETTINGSANSWER_ID;
		return processGetSettingsRequest();
	}
	else if (requestId == SETBASEMENTSETTINGSREQUEST_ID) {
		answerId = SETBASEMENTSETTINGSANSWER_ID;
		return processSetSettingsRequest();
	}
	else return false;
}

void checkRequest() {
	int requestId;
	if (radioListener.checkRequest(requestId)) {
		int answerId;
		if (processRequest(requestId, answerId)) {
			Serial.println((String)"Received commmand: " + requestId);
			radioListener.sendAnswer(answerId);
		}
		else Serial.println((String)"Unrecognized command " + requestId);
	}
}

void processTimer() {
	if (timer.check()) {
		sensorScanner.scan();
		if (fanWorkMode == 2)
			checkAutoFan();
	}
}

void loop() {
	checkRequest();
	processTimer();
	rebooter.alive();
}

ISR(TIMER1_COMPA_vect)
{
	rebooter.isrInterrupt();
}