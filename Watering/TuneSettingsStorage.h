#pragma once
#include <avr/eeprom.h>

#define STORAGE_EEPROM_KEY 143

struct TuneSettingsZone {
	byte count;
	WateringTuneTime* times;
};
class TuneSettingsStorage
{
private:
	int startAddress;
public:
	TuneSettingsZone zones[4];

	TuneSettingsStorage()
	{
		startAddress = 0;
		for (int i = 0; i < 4; i++) {
			zones[i].count = 0;
			zones[i].times = 0;
		}
	}

	~TuneSettingsStorage()
	{
		for (int i = 0; i < 4; i++)
			if (zones[i].times != 0)
				delete[] zones[i].times;
	}

	void init(int startAddress) {
		this->startAddress = startAddress;
	}

	void loadSettings() {
		if (eeprom_read_byte((uint8_t*)startAddress) == STORAGE_EEPROM_KEY) {
			int address = startAddress + 1;
			for (int i = 0; i < 4; i++) {
				byte count = eeprom_read_byte((uint8_t*)address);
				address++;
				if (count > 0) {
					zones[i].count = count;
					zones[i].times = new WateringTuneTime[count];
					int size = sizeof(WateringTuneTime) * zones[i].count;
					eeprom_read_block(zones[i].times, (uint8_t*)address, size);
					address += size;
				}
				else {
					delete zones[i].times;
					zones[i].times = 0;
					zones[i].count = 0;
				}
			}
		}
	}

	void saveSettings() {
		eeprom_write_byte((uint8_t*)startAddress, STORAGE_EEPROM_KEY);
		int address = startAddress + 1;
		for (int i = 0; i < 4; i++) {
			eeprom_write_byte((uint8_t*)address, zones[i].count);
			address++;
			if (zones[i].count > 0) {
				int size = sizeof(WateringTuneTime) * zones[i].count;
				eeprom_write_block(zones[i].times, (uint8_t*)address, size);
				address += size;
			}
		}
	}

	void updateZone() {
		byte zoneNumber = setWateringTuneSettingsRequest.zoneNumber;
		int count = setWateringTuneSettingsRequest.count;
		zones[zoneNumber].count = count;
		if (zones[zoneNumber].times != 0)
			delete[] zones[zoneNumber].times;
		zones[zoneNumber].times = new WateringTuneTime[count];
		for (int i = 0; i < count; i++)
			zones[zoneNumber].times[i] = setWateringTuneSettingsRequest.times[i];
		saveSettings();
	}
};

