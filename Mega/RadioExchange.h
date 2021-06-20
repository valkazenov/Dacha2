#pragma once
#include <SoftwareSerial.h>
#include <WirelessUART.h>
#include "Packets.h"

#define MINSENDTIME 300

class RadioExchange
{
private:
	SoftwareSerial radioSerial;
	WirelessUART radio;
	int selfId;
	int basementId;
	int wateringId;
	int timeout;

	int sendRequest(int targetId, int requestId, int answerId) {
		unsigned long totalTime = 0;
		while (true) {
			bool success = true;
			while (radioSerial.available()) 
				radioSerial.read();
			radio.sendData(requestId, targetId);
			unsigned long time = millis();
			while (!radio.receiveData()) {
				delay(10);
				if (millis() - time > MINSENDTIME) {
					success = false;
					break;
				}
			}
			if (success) {
				if (radio.sender_id != targetId) return 500;
				if (radio.struct_id != answerId) return 500;
				return 200;
			}
			else {
				totalTime += MINSENDTIME;
				if (totalTime > timeout)
					return 408;
			}
		}
	}

public:
	RadioExchange(uint8_t receivePin, uint8_t transmitPin) : radioSerial(receivePin, transmitPin) {
		selfId = 0;
		basementId = 0;
	}
	
	void init(int selfId, int basementId, int wateringId, int timeout) {
		radioSerial.begin(9600);
		radio.begin(&radioSerial, selfId);
		radio.setStructs(details(emptyPacket), GETBASEMENTDATAREQUEST_ID);
		radio.setStructs(details(getBasementDataAnswer), GETBASEMENTDATAANSWER_ID);
		radio.setStructs(details(emptyPacket), GETBASEMENTSETTINGSREQUEST_ID);
		radio.setStructs(details(getBasementSettingsAnswer), GETBASEMENTSETTINGSANSWER_ID);
		radio.setStructs(details(setBasementSettingsRequest), SETBASEMENTSETTINGSREQUEST_ID);
		radio.setStructs(details(emptyPacket), SETBASEMENTSETTINGSANSWER_ID);
		radio.setStructs(details(emptyPacket), GETWATERINGDATAREQUEST_ID);
		radio.setStructs(details(getWateringDataAnswer), GETWATERINGDATAANSWER_ID);
		radio.setStructs(details(emptyPacket), GETWATERINGSETTINGSREQUEST_ID);
		radio.setStructs(details(getWateringSettingsAnswer), GETWATERINGSETTINGSANSWER_ID);
		radio.setStructs(details(setWateringSettingsRequest), SETWATERINGSETTINGSREQUEST_ID);
		radio.setStructs(details(emptyPacket), SETWATERINGSETTINGSANSWER_ID);
		radio.setStructs(details(getWateringTuneSettingsRequest), GETWATERINGTUNESETTINGSREQUEST_ID);
		radio.setStructs(details(getWateringTuneSettingsAnswer), GETWATERINGTUNESETTINGSANSWER_ID);
		radio.setStructs(details(setWateringTuneSettingsRequest), SETWATERINGTUNESETTINGSREQUEST_ID);
		radio.setStructs(details(emptyPacket), SETWATERINGTUNESETTINGSANSWER_ID);
		this->selfId = selfId;
		this->basementId = basementId;
		this->wateringId = wateringId;
		this->timeout = timeout;
	}

	int getBasementData() {
		return sendRequest(basementId, GETBASEMENTDATAREQUEST_ID, GETBASEMENTDATAANSWER_ID);
	}

	int getBasementSettings() {
		return sendRequest(basementId, GETBASEMENTSETTINGSREQUEST_ID, GETBASEMENTSETTINGSANSWER_ID);
	}

	int setBasementSettings() {
		return sendRequest(basementId, SETBASEMENTSETTINGSREQUEST_ID, SETBASEMENTSETTINGSANSWER_ID);
	}

	int getWateringData() {
		return sendRequest(wateringId, GETWATERINGDATAREQUEST_ID, GETWATERINGDATAANSWER_ID);
	}

	int getWateringSettings() {
		return sendRequest(wateringId, GETWATERINGSETTINGSREQUEST_ID, GETWATERINGSETTINGSANSWER_ID);
	}

	int setWateringSettings() {
		return sendRequest(wateringId, SETWATERINGSETTINGSREQUEST_ID, SETWATERINGSETTINGSANSWER_ID);
	}

	int getWateringTuneSettings() {
		return sendRequest(wateringId, GETWATERINGTUNESETTINGSREQUEST_ID, GETWATERINGTUNESETTINGSANSWER_ID);
	}

	int setWateringTuneSettings() {
		return sendRequest(wateringId, SETWATERINGTUNESETTINGSREQUEST_ID, SETWATERINGTUNESETTINGSANSWER_ID);
	}

};

