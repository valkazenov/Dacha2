#pragma once

#include <limits.h>
#include <TimeLib.h>
#include <Time.h>

class WateringCalculator
{
private:
	bool isEnabledZone(byte zone, byte enabledZones[4], byte enabledZoneCount) {
		for (byte i = 0; i < enabledZoneCount; i++)
			if (enabledZones[i] == zone)
				return true;
		return false;
	}

	int compareBytes(byte byte1, byte byte2) {
		if (byte1 > byte2)
			return 1;
		else if (byte1 < byte2)
			return -1;
		else return 0;
	}

	int compareTimes(byte weekday1, byte hour1, byte minute1, byte weekday2, byte hour2, byte minute2) {
		int result = compareBytes(weekday1, weekday2);
		if (result == 0)
			result = compareBytes(hour1, hour2);
		if (result == 0)
			result = compareBytes(minute1, minute2);
		return result;
	}

	int compareTuneTimes(WateringTuneTime time, byte weekday, byte stopHour, byte stopMinute) {
		return compareTimes(time.weekday, time.stopHour, time.stopMinute, weekday, stopHour, stopMinute);
	}

	int compareTuneTimes(WateringTuneTime time1, WateringTuneTime time2) {
		return compareTuneTimes(time1, time2.weekday, time2.stopHour, time2.stopMinute);
	}

	String unixTimeToString(unsigned long value) {
		return String(day(value)) + "." + String(month(value)) + "." + String(year(value)) + " " + String(hour(value)) + ":" + String(minute(value));
	}

	unsigned long encodeTime(byte clockWeekday, iarduino_RTC &clock, byte weekday, byte hour, byte minute, bool pastToFuture) {
		bool greater = compareTimes(weekday, hour, minute, clockWeekday, clock.Hours, clock.minutes) > 0;
		if (!greater && pastToFuture) weekday += 7;
		unsigned long result = clock.gettimeUnix();
		result += (unsigned long)(weekday - clockWeekday) * 3600 * 24;
		result += (unsigned long)(hour - clock.Hours) * 3600;
		result += (unsigned long)(minute - clock.minutes) * 60;
		result -= clock.seconds;
		return result;
	}

	bool findWateringTime(TuneSettingsStorage &tuneSettingsStorage, byte enabledZones[4], byte enabledZoneCount,
		byte clockWeekday, iarduino_RTC &clock, bool greater, bool skipCurrent, byte &zone, WateringTuneTime &foundTime) {

		bool result = false;  bool firstKey = true;
		for (byte i = 0; i < 4; i++) {
			if (isEnabledZone(i, enabledZones, enabledZoneCount)) {
				for (byte j = 0; j < tuneSettingsStorage.zones[i].count; j++) {
					WateringTuneTime time = tuneSettingsStorage.zones[i].times[j];
					bool doKey;
					if (greater) {
						if (skipCurrent)
							doKey = compareTimes(time.weekday, time.startHour, time.startMinute, clockWeekday, clock.Hours, clock.minutes);
						else doKey = compareTuneTimes(time, clockWeekday, clock.Hours, clock.minutes) > 0;
					} else doKey = true;
					if (doKey) {
						if (firstKey || compareTuneTimes(time, foundTime) < 0) {
							foundTime = time;
							zone = i;
							firstKey = false;
						}
						result = true;
					}
				}
			}
		}
		return result;
	}

	void loadClosestWateringTime(SensorScanner &sensorScanner, TuneSettingsStorage &tuneSettingsStorage,
		byte enabledZones[4], byte enabledZoneCount, byte minRainPercent) {

		//Serial.print("CurrentTime: "); Serial.println(unixTimeToString(sensorScanner.clock.gettimeUnix()));
		iarduino_RTC clock = sensorScanner.clock;
		byte weekday = clock.weekday;
		if (weekday == 0) weekday = 7;
		WateringTuneTime foundTime; byte zone; bool findKey;
		bool skipCurrent = sensorScanner.rainPercent >= minRainPercent;
		if (findWateringTime(tuneSettingsStorage, enabledZones, enabledZoneCount,
			weekday, sensorScanner.clock, true, skipCurrent, zone, foundTime)) {

			zoneNumber = zone;
			working = foundTime.weekday == weekday && (foundTime.startHour < clock.Hours || (foundTime.startHour == clock.Hours && foundTime.startMinute <= clock.minutes));
			if (working) {
				startTime = encodeTime(weekday, sensorScanner.clock, foundTime.weekday, foundTime.startHour, foundTime.startMinute, false);
				stopTime = encodeTime(weekday, sensorScanner.clock, foundTime.weekday, foundTime.stopHour, foundTime.stopMinute, false);
			}
			else startTime = encodeTime(weekday, sensorScanner.clock, foundTime.weekday, foundTime.startHour, foundTime.startMinute, false);
		}
		else if (findWateringTime(tuneSettingsStorage, enabledZones, enabledZoneCount,
			weekday, sensorScanner.clock, false, skipCurrent, zone, foundTime)) {

			zoneNumber = zone;
			startTime = encodeTime(weekday, sensorScanner.clock, foundTime.weekday, foundTime.startHour, foundTime.startMinute, true);
		}
	}

public:
	bool working;
	int8_t zoneNumber;
	unsigned long startTime;
	unsigned long stopTime;

	WateringCalculator() {
		working = false;
		zoneNumber = 0;
		startTime = 0;
		stopTime = 0;
	}

	void calculate(SensorScanner &sensorScanner, byte zoneModes[4], byte minRainPercent,
		unsigned long settingsTime, TuneSettingsStorage &tuneSettingsStorage) {

		working = false; zoneNumber = -1; startTime = 0; stopTime = 0;
		byte enabledZones[4]; byte enabledZoneCount = 0; int zone = -1;
		for (int i = 0; i < 4; i++) {
			if (zoneModes[i] == 1) {  // enabled
				zone = i;
				break;
			}
			if (zoneModes[i] != 0) {  // not disabled
				enabledZones[enabledZoneCount] = i;
				enabledZoneCount++;
			}
		}
		if (zone >= 0) {
			working = true;
			zoneNumber = zone;
			startTime = settingsTime;
			stopTime = ULONG_MAX;
		}
		else if (enabledZoneCount > 0) {
			loadClosestWateringTime(sensorScanner, tuneSettingsStorage, enabledZones, enabledZoneCount, minRainPercent);
		}
	}

};

