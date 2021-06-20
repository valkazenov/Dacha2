#pragma once
class MillisTimer
{
private:
	unsigned long ms;
	unsigned long lastTime;
public:
	MillisTimer(unsigned long ms) {
		this->ms = ms;
		lastTime = 0;
	}

	bool check() {
		unsigned long time = millis();
		if (time < lastTime || time - lastTime > ms) {
			lastTime = time;
			return true;
		}
		else return false;
	}
};

