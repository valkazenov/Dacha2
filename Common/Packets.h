#pragma once

struct WateringTuneTime {
	byte weekday;
	byte startHour;
	byte startMinute;
	byte stopHour;
	byte stopMinute;
};

struct EmptyPacket {
};
#define GETBASEMENTDATAREQUEST_ID 0
#define GETBASEMENTSETTINGSREQUEST_ID 1
#define SETBASEMENTSETTINGSANSWER_ID 12
#define GETWATERINGDATAREQUEST_ID 3
#define GETWATERINGSETTINGSREQUEST_ID 4
#define SETWATERINGSETTINGSANSWER_ID 15
#define SETWATERINGTUNESETTINGSANSWER_ID 17

struct SetBasementSettingsRequest {
	bool boilerEnabled;
	int fanWorkMode;
	int fanDisableInterval;
	int fanEnableInterval;
};
#define SETBASEMENTSETTINGSREQUEST_ID 2

struct SetWateringSettingsRequest {
	byte zoneModes[4];
	unsigned long currentTime;
	byte minRainPercent;
};
#define SETWATERINGSETTINGSREQUEST_ID 5

struct GetWateringTuneSettingsRequest {
	byte zoneNumber;
};
#define GETWATERINGTUNESETTINGSREQUEST_ID 6

struct SetWateringTuneSettingsRequest {
	byte zoneNumber;
	byte count;
	WateringTuneTime times[7];
};
#define SETWATERINGTUNESETTINGSREQUEST_ID 7

struct GetBasementDataAnswer {
	double temperature;
	double humidity;
	bool boilerWorking;
	double boilerAmperage;
	bool fanWorking;
	unsigned long fanSeconds;
	int supplyVoltage;
};
#define GETBASEMENTDATAANSWER_ID 10

struct GetBasementSettingsAnswer {
	bool boilerEnabled;
	int fanWorkMode;
	int fanDisableInterval;
	int fanEnableInterval;
};
#define GETBASEMENTSETTINGSANSWER_ID 11

struct GetWateringDataAnswer {
	bool working;
	int8_t zone;
	unsigned long startTime;
	unsigned long stopTime;
	int tankLevel;
	double outsideTemperature;
	double outsideHumidity;
	byte rainPercent;
	unsigned long currentTime;
	int supplyVoltage;
};
#define GETWATERINGDATAANSWER_ID 13

struct GetWateringSettingsAnswer {
	byte zoneModes[4];
	byte minRainPercent;
};
#define GETWATERINGSETTINGSANSWER_ID 14

struct GetWateringTuneSettingsAnswer {
	byte count;
	WateringTuneTime times[7];
};
#define GETWATERINGTUNESETTINGSANSWER_ID 15

EmptyPacket emptyPacket;
GetBasementDataAnswer getBasementDataAnswer;
GetBasementSettingsAnswer getBasementSettingsAnswer;
SetBasementSettingsRequest setBasementSettingsRequest;
GetWateringDataAnswer getWateringDataAnswer;
GetWateringSettingsAnswer getWateringSettingsAnswer;
SetWateringSettingsRequest setWateringSettingsRequest;
GetWateringTuneSettingsRequest getWateringTuneSettingsRequest;
GetWateringTuneSettingsAnswer getWateringTuneSettingsAnswer;
SetWateringTuneSettingsRequest setWateringTuneSettingsRequest;
