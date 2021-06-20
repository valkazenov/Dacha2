#pragma once
#include <SoftwareSerial.h>
#include <WirelessUART.h>
#include "Packets.h"

class RadioListener
{
private:
	SoftwareSerial radioSerial;
	WirelessUART radio;
	int selfId;
	int megaId;

public:
	RadioListener(uint8_t receivePin, uint8_t transmitPin) : radioSerial(receivePin, transmitPin) {
		selfId = 0;
		megaId = 0;
	}
	
	void init(int selfId, int megaId) {
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
		radio.receiveFromAll = false;
		radio.receiveFrom[0] = megaId;
		this->selfId = selfId;
		this->megaId = megaId;
	}

	bool checkRequest(int &requestId) {
		requestId = 0;
		if (radio.receiveData()) {
			if (radio.sender_id == megaId) {
				requestId = radio.struct_id;
				return true;
			}
			else Serial.println((String)"Invalid sender " + radio.sender_id);
		}
		return false;
	}

	void sendAnswer(int answerId) {
		radio.sendData(answerId, megaId);
	}

};

