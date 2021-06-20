#include <avr/eeprom.h>
#include "Packets.h"
#include "RadioListener.h"
#include "MillisTimer.h"
#include "SensorScanner.h"
#include "TuneSettingsStorage.h"
#include "WateringCalculator.h"
#include "Rebooter.h"

#define EEPROM_KEY 193
#define PIN_DHT 2
#define PIN_DIST_OUT 4
#define PIN_DIST_ECHO 3
#define PIN_RAIN 2
#define PIN_HC_SET 7
#define PIN_HC_RX 9
#define PIN_HC_TX 8
#define PIN_ZONE0 6
#define PIN_ZONE1 10
#define PIN_ZONE2 11
#define PIN_ZONE3 12
#define RADIO_SELF_ID 3
#define RADIO_MEGA_ID 1

RadioListener radioListener(PIN_HC_RX, PIN_HC_TX);
SensorScanner sensorScanner(PIN_DIST_OUT, PIN_DIST_ECHO);
MillisTimer timer(1000);
TuneSettingsStorage tuneSettingsStorage;
Rebooter rebooter;
byte zoneModes[4];
byte minRainPercent;
unsigned long settingsTime;
WateringCalculator wateringCalculator;

void initVariables() {
	zoneModes[0] = 0;
	zoneModes[1] = 0;
	zoneModes[2] = 0;
	zoneModes[3] = 0;
	minRainPercent = 0;
	settingsTime = 0;
}

void saveSettings() {
	eeprom_write_byte((uint8_t*)0, EEPROM_KEY);
	eeprom_write_byte((uint8_t*)1, zoneModes[0]);
	eeprom_write_byte((uint8_t*)2, zoneModes[1]);
	eeprom_write_byte((uint8_t*)3, zoneModes[2]);
	eeprom_write_byte((uint8_t*)4, zoneModes[3]);
	eeprom_write_byte((uint8_t*)5, minRainPercent);
	eeprom_write_dword((uint32_t*)6, settingsTime);
}

void loadSettings() {
	if (eeprom_read_byte((uint8_t*)0) == EEPROM_KEY) {
		zoneModes[0] = eeprom_read_byte((uint8_t*)1);
		zoneModes[1] = eeprom_read_byte((uint8_t*)2);
		zoneModes[2] = eeprom_read_byte((uint8_t*)3);
		zoneModes[3] = eeprom_read_byte((uint8_t*)4);
		minRainPercent = eeprom_read_byte((uint8_t*)5);
		settingsTime = eeprom_read_dword((uint32_t*)6);
	}
}

void setup() {
	Serial.begin(115200);
	resetRelays();
	pinMode(PIN_ZONE0, OUTPUT);
	pinMode(PIN_ZONE1, OUTPUT);
	pinMode(PIN_ZONE2, OUTPUT);
	pinMode(PIN_ZONE3, OUTPUT);
	rebooter.init(4000);
	radioListener.init(RADIO_SELF_ID, RADIO_MEGA_ID);
	sensorScanner.init(PIN_DHT, PIN_RAIN);
	tuneSettingsStorage.init(10);
	initVariables();
	loadSettings();
	tuneSettingsStorage.loadSettings();
	sensorScanner.scan();
	applySettings();
	delay(1000);
}

bool applySettings() {
	return true;
}

bool processGetDataRequest() {
	getWateringDataAnswer.working = wateringCalculator.working;
	getWateringDataAnswer.zone = wateringCalculator.zoneNumber;
	getWateringDataAnswer.startTime = wateringCalculator.startTime;
	getWateringDataAnswer.stopTime = wateringCalculator.stopTime;
	getWateringDataAnswer.tankLevel = sensorScanner.tankLevel;
	getWateringDataAnswer.outsideTemperature = sensorScanner.temperature;
	getWateringDataAnswer.outsideHumidity = sensorScanner.humidity;
	getWateringDataAnswer.rainPercent = sensorScanner.rainPercent;
	getWateringDataAnswer.currentTime = sensorScanner.clock.Unix;
	getWateringDataAnswer.supplyVoltage = sensorScanner.supplyVoltage;
	return true;
}

bool processGetSettingsRequest() {
	getWateringSettingsAnswer.zoneModes[0] = zoneModes[0];
	getWateringSettingsAnswer.zoneModes[1] = zoneModes[1];
	getWateringSettingsAnswer.zoneModes[2] = zoneModes[2];
	getWateringSettingsAnswer.zoneModes[3] = zoneModes[3];
	getWateringSettingsAnswer.minRainPercent = minRainPercent;
	return true;
}

bool processSetSettingsRequest() {
	zoneModes[0] = setWateringSettingsRequest.zoneModes[0];
	zoneModes[1] = setWateringSettingsRequest.zoneModes[1];
	zoneModes[2] = setWateringSettingsRequest.zoneModes[2];
	zoneModes[3] = setWateringSettingsRequest.zoneModes[3];
	sensorScanner.clock.settimeUnix(setWateringSettingsRequest.currentTime);
	minRainPercent = setWateringSettingsRequest.minRainPercent;
	settingsTime = setWateringSettingsRequest.currentTime;
	if (applySettings()) {
		saveSettings();
		return true;
	}
	else return false;
}

bool processGetTuneSettingsRequest() {
	byte zoneNumber = getWateringTuneSettingsRequest.zoneNumber;
	getWateringTuneSettingsAnswer.count = tuneSettingsStorage.zones[zoneNumber].count;
	for (int i = 0; i < getWateringTuneSettingsAnswer.count; i++)
		getWateringTuneSettingsAnswer.times[i] = tuneSettingsStorage.zones[zoneNumber].times[i];
	return true;
}

bool processSetTuneSettingsRequest() {
	tuneSettingsStorage.updateZone();
	return true;
}

bool processRequest(int requestId, int &answerId) {
	if (requestId == GETWATERINGDATAREQUEST_ID) {
		answerId = GETWATERINGDATAANSWER_ID;
		return processGetDataRequest();
	}
	else if (requestId == GETWATERINGSETTINGSREQUEST_ID) {
		answerId = GETWATERINGSETTINGSANSWER_ID;
		return processGetSettingsRequest();
	}
	else if (requestId == SETWATERINGSETTINGSREQUEST_ID) {
		answerId = SETWATERINGSETTINGSANSWER_ID;
		return processSetSettingsRequest();
	}
	else if (requestId == GETWATERINGTUNESETTINGSREQUEST_ID) {
		answerId = GETWATERINGTUNESETTINGSANSWER_ID;
		return processGetTuneSettingsRequest();
	}
	else if (requestId == SETWATERINGTUNESETTINGSREQUEST_ID) {
		answerId = SETWATERINGTUNESETTINGSANSWER_ID;
		return processSetTuneSettingsRequest();
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

void applyRelay(byte zone, byte activeZone, int pin) {
	if (zone == activeZone)
		digitalWrite(pin, LOW);
	else digitalWrite(pin, HIGH);
}

void resetRelays() {
	digitalWrite(PIN_ZONE0, HIGH);
	digitalWrite(PIN_ZONE1, HIGH);
	digitalWrite(PIN_ZONE2, HIGH);
	digitalWrite(PIN_ZONE3, HIGH);
}

void applyRelays() {
	if (wateringCalculator.working) {
		applyRelay(0, wateringCalculator.zoneNumber, PIN_ZONE0);
		applyRelay(1, wateringCalculator.zoneNumber, PIN_ZONE1);
		applyRelay(2, wateringCalculator.zoneNumber, PIN_ZONE2);
		applyRelay(3, wateringCalculator.zoneNumber, PIN_ZONE3);
	}
	else resetRelays();
}

void processTimer() {
	if (timer.check()) {
		sensorScanner.scan();
		wateringCalculator.calculate(sensorScanner, zoneModes, minRainPercent, settingsTime, tuneSettingsStorage);
		applyRelays();
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