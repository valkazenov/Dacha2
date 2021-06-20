#include <SPI.h>
#include "Packets.h"
#include "RadioExchange.h"
#include "MillisTimer.h"
#include "Rebooter.h"
#include "Constants.h"

#define RADIO_TIMEOUT 1000
#define ESP_TIMEOUT 20000
#define PIN_ROUTER 8
#define PIN_HC_SET 50
#define PIN_HC_RX 51
#define PIN_HC_TX 52
#define RADIO_SELF_ID 1
#define RADIO_BASEMENT_ID 2
#define RADIO_WATERING_ID 3
#define START_REBOOT_DELAY 600000;

const String debugPrefix = "debug:";
const String answerPrefix = "answer:";
const String serverUrl = "~192.168.1.104~45457~";

RadioExchange radioExchange(PIN_HC_RX, PIN_HC_TX);
Rebooter rebooter;
MillisTimer timer(10000);
unsigned long lastSuccessfulTime;
unsigned long rebootDelay;

void serialDebug(const String &message1) {
	Serial.print(debugPrefix);
	Serial.println(message1);
	Serial.flush();
}

void serialDebug(const String &message1, const String &message2) {
	Serial.print(debugPrefix);
	Serial.print(message1);
	Serial.println(message2);
	Serial.flush();
}

void serialDebug(const String &message1, const String &message2, const String &message3) {
	Serial.print(debugPrefix);
	Serial.print(message1);
	Serial.print(message2);
	Serial.println(message3);
	Serial.flush();
}

void serialDebug(const String &message1, const String &message2, const String &message3, const String &message4) {
	Serial.print(debugPrefix);
	Serial.print(message1);
	Serial.print(message2);
	Serial.print(message3);
	Serial.println(message4);
	Serial.flush();
}

void setup() {
	Serial.begin(115200);
	Serial3.begin(115200);
	radioExchange.init(RADIO_SELF_ID, RADIO_BASEMENT_ID, RADIO_WATERING_ID, RADIO_TIMEOUT);
	rebooter.init(RADIO_TIMEOUT * 4);
	pinMode(PIN_ROUTER, OUTPUT);
	lastSuccessfulTime = 0;
	rebootDelay = START_REBOOT_DELAY;
	delay(1000);
	serialDebug("Started");
}

bool waitServerAnswer(String &answer) {
	answer = "";
	unsigned long startTime = millis();
	while (true) {
		if (Serial3.available()) {
			while (Serial3.available())
				answer += (char)Serial3.read();
			if (answer.indexOf(answerPrefix) < 0) continue;
			return true;
		}
		else {
			if (millis() - startTime > ESP_TIMEOUT) break;
			delay(100);
			rebooter.alive();
		}
	}
	return false;
}

int parseESPAnswer(String &answer) {
	int pos = answer.indexOf(answerPrefix);
	answer.remove(0, pos + answerPrefix.length());
	pos = answer.indexOf(";");
	int status = answer.substring(0, pos).toInt();
	answer.remove(0, pos + 1);
	answer.trim();
	return status;
}

int sendESPRequest(int commandType, String request, String &answer) {
	String text = (String)commandType;
	if (commandType == ESP_COMMAND_HTTP)
		text += serverUrl + request;
	Serial3.println(text);
	if (waitServerAnswer(answer)) 
		return parseESPAnswer(answer);
	else return 408;
}

int sendHttpRequest(String command, String params, String &answer) {
	String request = command;
	if (params != "")
		request += "?q=" + params;
	int status = sendESPRequest(ESP_COMMAND_HTTP, request, answer);
	if (status != 408)
		lastSuccessfulTime = millis();
	return status;
}

int sendHttpRequest(String command, String params) {
	String answer;
	return sendHttpRequest(command, params, answer);
}

bool checkESPState() {
	String answer;
	int status = sendESPRequest(ESP_COMMAND_STATE, "", answer);
	serialDebug("Check ESP ", (String)status, " " + answer);
	return status == 200 && answer.toInt() == ESP_STATE_CONNECTED;
}

String encodeParams(String list[], int length) {
	String result = "";
	for (int i = 0; i < length; i++)
		result += list[i] + (String)";";
	return result;
}

String getParamValue(String param, int index) {
	int strIndex = 0;
	for (int i = 0; i < index; i++) {
		strIndex = param.indexOf(';', strIndex);
		if (strIndex >= 0) strIndex++;
	}
	if (strIndex >= 0) {
		int newIndex = param.indexOf(';', strIndex);
		if (newIndex >= 0)
			return param.substring(strIndex, newIndex);
		else return param.substring(strIndex);
	}
	else return "";
}

void checkRouterReboot() {
	unsigned long time = millis();
	if (lastSuccessfulTime > time)
		lastSuccessfulTime = time;
	unsigned long delta = time - lastSuccessfulTime;
	serialDebug("Delta: ", (String)delta, " ", (String)rebootDelay);
	if (delta >= rebootDelay) {
		if (checkESPState()) {
			digitalWrite(PIN_ROUTER, HIGH);
			delay(2000);
			digitalWrite(PIN_ROUTER, LOW);
			unsigned long day = (unsigned long)24 * 3600 * 1000;
			if (rebootDelay < day)
				rebootDelay = rebootDelay * 2;
			if (rebootDelay > day)
				rebootDelay = day;
			serialDebug("Router was rebooted");
		}
		else serialDebug("ESP is not available. No router reboot");
	}
}

int getBasementSettings() {
	String answer;
	int status = sendHttpRequest("getbsmtsettings", "", answer);
	if (status == 200) {
		bool applied = getParamValue(answer, 4).toInt() == 1;
		if (!applied) {
			setBasementSettingsRequest.boilerEnabled = getParamValue(answer, 0).toInt() == 1;
			setBasementSettingsRequest.fanWorkMode = getParamValue(answer, 1).toInt();
			setBasementSettingsRequest.fanDisableInterval = getParamValue(answer, 2).toInt();
			setBasementSettingsRequest.fanEnableInterval = getParamValue(answer, 3).toInt();
			status = radioExchange.setBasementSettings();
			if (status == 200)
				status = sendHttpRequest("SetBsmtApplied", "");
		}
	}
	return status;
}

int getWateringSettings() {
	String answer;
	int status = sendHttpRequest("getwtrnsettings", "", answer);
	if (status == 200) {
		bool applied = getParamValue(answer, 6).toInt() == 1;
		if (!applied) {
			setWateringSettingsRequest.zoneModes[0] = getParamValue(answer, 0).toInt();
			setWateringSettingsRequest.zoneModes[1] = getParamValue(answer, 1).toInt();
			setWateringSettingsRequest.zoneModes[2] = getParamValue(answer, 2).toInt();
			setWateringSettingsRequest.zoneModes[3] = getParamValue(answer, 3).toInt();
			setWateringSettingsRequest.currentTime = getParamValue(answer, 4).toInt();
			setWateringSettingsRequest.minRainPercent = getParamValue(answer, 5).toInt();
			status = radioExchange.setWateringSettings();
			if (status == 200)
				status = sendHttpRequest("SetWtrnApplied", "");
		}
	}
	return status;
}

int getWateringTuneSettings(int zoneNumber) {
	String answer;
	int status = sendHttpRequest("getwtrntunesettings", (String)zoneNumber, answer);
	if (status == 200) {
		setWateringTuneSettingsRequest.zoneNumber = zoneNumber;
		setWateringTuneSettingsRequest.count = getParamValue(answer, 0).toInt();
		int index = 1;
		for (byte i = 0; i < setWateringTuneSettingsRequest.count; i++) {
			WateringTuneTime time;
			time.weekday = getParamValue(answer, index).toInt(); index++;
			time.startHour = getParamValue(answer, index).toInt(); index++;
			time.startMinute = getParamValue(answer, index).toInt(); index++;
			time.stopHour = getParamValue(answer, index).toInt(); index++;
			time.stopMinute = getParamValue(answer, index).toInt(); index++;
			setWateringTuneSettingsRequest.times[i] = time;
		}
		bool applied = getParamValue(answer, index).toInt() == 1;
		if (!applied) {
			status = radioExchange.setWateringTuneSettings();;
			if (status == 200)
				status = sendHttpRequest("SetWtrnApplied", (String)zoneNumber);
		}
	}
	return status;
}

void getSettings() {
	serialDebug("Get start");
	int basementStatus = getBasementSettings();
	int wateringStatus = getWateringSettings();
	int tuneStatus = 200;
	for (int i = 0; i < 4; i++) {
		int status = getWateringTuneSettings(i);
		if (status != 200) tuneStatus = status;
	}
	serialDebug("Get. Statuses: ", (String)basementStatus + " " + (String)wateringStatus + " " + (String)tuneStatus);
}

int sendBasementData() {
	int status = radioExchange.getBasementData();
	if (status == 200) {
		String result[7];
		result[0] = getBasementDataAnswer.temperature;
		result[1] = getBasementDataAnswer.humidity;
		result[2] = getBasementDataAnswer.boilerWorking;
		result[3] = getBasementDataAnswer.boilerAmperage;
		result[4] = getBasementDataAnswer.fanWorking;
		result[5] = getBasementDataAnswer.fanSeconds;
		result[6] = getBasementDataAnswer.supplyVoltage;
		status = sendHttpRequest("setbsmtdata", encodeParams(result, 7));
	}
	return status;
}

int sendWateringData() {
	int status = radioExchange.getWateringData();
	if (status == 200) {
		String result[10];
		result[0] = getWateringDataAnswer.working;
		result[1] = getWateringDataAnswer.zone;
		result[2] = getWateringDataAnswer.startTime;
		result[3] = getWateringDataAnswer.stopTime;
		result[4] = getWateringDataAnswer.tankLevel;
		result[5] = getWateringDataAnswer.outsideTemperature;
		result[6] = getWateringDataAnswer.outsideHumidity;
		result[7] = getWateringDataAnswer.rainPercent;
		result[8] = getWateringDataAnswer.currentTime;
		result[9] = getWateringDataAnswer.supplyVoltage;
		status = sendHttpRequest("setwtrndata", encodeParams(result, 10));
	}
	return status;
}

void sendData() {
	serialDebug("Send start");
	int basementStatus = sendBasementData();
	int wateringStatus = sendWateringData();
	serialDebug("Send. BsmtSts: ", (String)basementStatus , ", WtrnSts: ", (String)wateringStatus);
}

void loop() {
	if (timer.check()) {
		checkRouterReboot();
		getSettings();
		sendData();
	}
	rebooter.alive();
}

ISR(TIMER1_COMPA_vect)
{
	rebooter.isrInterrupt();
}

