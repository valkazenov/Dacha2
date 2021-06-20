#include <ESP8266WiFi.h>
#include "Constants.h"

#define TIMEOUT 10000

const char* ssid = "KVE3_Network";
const char* password = "dfkbr1979";

WiFiClient client;
int currentState;
unsigned long stateLastTime;
unsigned long stateDelay;
String inputString;
bool inputComplete;

void serialDebug(String message) {
	Serial.println("debug:" + message);
}

void serialAnswer(int status, String answer) {
	Serial.println((String)"answer:" + status + (String)";" + answer);
}

void setup() {
	Serial.begin(115200);
	delay(1000);
	currentState = ESP_STATE_INIT;
	stateLastTime = 0;
	stateDelay = 0;
	inputString = "";
	inputComplete = false;
	serialDebug("Setup finished");
}

void processLoopState() {
	unsigned long currentTime = millis();
	if (stateLastTime > currentTime || currentTime - stateLastTime > stateDelay) {
		if (currentState == ESP_STATE_INIT || currentState == ESP_STATE_CONNECT_ERROR) {
			WiFi.begin(ssid, password);
			serialDebug("Connecting to " + (String)ssid);
			currentState = ESP_STATE_WAITCONNECT;
			stateDelay = 1000;
		}
		else if (currentState == ESP_STATE_WAITCONNECT) {
			wl_status_t status = WiFi.status();
			if (status == WL_CONNECTED) {
				serialDebug((String)"Connected. IP address " + WiFi.localIP().toString());
				currentState = ESP_STATE_CONNECTED;
				stateDelay = 1000;
			}
			else if (status == WL_NO_SSID_AVAIL) {
				serialDebug("Incorrect SSID");
				currentState = ESP_STATE_CONNECT_ERROR;
				stateDelay = 60000;
			}
		}
		else if (currentState == ESP_STATE_CONNECTED) {
			if (WiFi.status() != WL_CONNECTED) {
				serialDebug("Connection lost");
				currentState = ESP_STATE_WAITCONNECT;
				stateDelay = 1000;
			}
		}
		stateLastTime = currentTime;
	}
}

String getStringWord(String source, char separator, int index) {
	int strIndex = 0;
	for (int i = 0; i < index; i++) {
		strIndex = source.indexOf(separator, strIndex);
		if (strIndex >= 0)
			strIndex++;
		else return "";
	}
	if (strIndex >= 0) {
		int newIndex = source.indexOf(separator, strIndex);
		if (newIndex >= 0)
			return source.substring(strIndex, newIndex);
		else return source.substring(strIndex);
	}
	else return "";
}

String getCommandPart(String source, int index) {
	return getStringWord(source, '~', index);
}

bool parseCommand(String command, int &commandType, String &hostName, int &port, String &request) {
	commandType = getCommandPart(command, 0).toInt();
	hostName = getCommandPart(command, 1);
	port = getCommandPart(command, 2).toInt();
	request = getCommandPart(command, 3);
	if (commandType == ESP_COMMAND_HTTP)
		return hostName != "" && port > 0;
	else return commandType == ESP_COMMAND_STATE;
}

bool clientConnect(String hostName, int port) {
	if (client.connected()) client.stop();
	if (!client.connect(hostName, port)) return false;
	return client.connected();
}

void clientRequest(String hostName, String request) {
	client.println("GET /" + request + " HTTP/1.1");
	client.println("Host: " + hostName);
	client.println("Connection: close");
	client.println();
}

bool waitServerAnswer() {
	unsigned long startTime = millis();
	while (!client.available()) {
		if (millis() - startTime > TIMEOUT) return false;
		delay(100);
	}
	return true;
}

void parseAnswerLine(String line, int &expectType, int &status, String &answer) {
	if (expectType == 0) {
		status = getStringWord(line, ' ', 1).toInt();
		if (status <= 0) status = 400;
		if (status == 200)
			expectType = 1;
		else expectType = 2;
	}
	else if (expectType == 1) {
		if (line.indexOf(' ') < 0) {
			answer = line;
			expectType = 2;
		}
	}
}

void processLoopCommand() {
	int commandType; String hostName; int port; String request;
	if (parseCommand(inputString, commandType, hostName, port, request)) {
		if (commandType == ESP_COMMAND_HTTP) {
			serialDebug((String)"Request. Host: " + hostName + (String)", port: " + port + (String)", request: " + request);
			if (clientConnect(hostName, port)) {
				clientRequest(hostName, request);
				if (waitServerAnswer()) {
					String line = ""; int expectType = 0; int status = 0; String answer = "";
					while (client.available()) {
						char ch = client.read();
						if (ch == '\r' || ch == '\n') {
							if (line != "") {
								parseAnswerLine(line, expectType, status, answer);
								line = "";
							}
						}
						else line += ch;
					}
					if (line != "")
						parseAnswerLine(line, expectType, status, answer);
					serialAnswer(status, answer);
				}
				else serialAnswer(408, ""); // timeout
			}
			else serialAnswer(503, "");  // unavailable
		}
		else serialAnswer(200, (String)currentState);
	}
	else serialAnswer(400, "");  // bad request
}

void processSerial() {
	while (Serial.available()) {
		char inChar = (char)Serial.read();
		if (inChar == '\n' || inChar == '\r')
			inputComplete = true;
		else inputString += inChar;
	}
}

void loop() {
	processLoopState();
	processSerial();
	if (inputComplete) {
		if (inputString != "")
			processLoopCommand();
		inputString = "";
		inputComplete = false;
	}
}

