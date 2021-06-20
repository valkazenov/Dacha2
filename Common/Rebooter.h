#pragma once

void(*resetFunc) (void) = 0;

class Rebooter
{
private:
	int timeout;
	unsigned long aliveTime;
public:
	Rebooter() {
		timeout = 0;
	}

	void init(int timeout) {
		cli();  // отключить глобальные прерывания
		TCCR1A = 0;   // установить регистры в 0
		OCR1A = 15624; // установка регистра совпадения
		TCCR1B = 0;
		TCCR1B |= (1 << WGM12);  // включить CTC режим 
		TCCR1B |= (1 << CS10); // Установить биты на коэффициент деления 1024
		TCCR1B |= (1 << CS12);
		TIMSK1 |= (1 << OCIE1A);  // включить прерывание по совпадению таймера 
		sei(); // включить глобальные прерывания
		this->timeout = timeout;
		aliveTime = 0;
	}

	void alive() {
		aliveTime = millis();
	}

	void isrInterrupt() {
		unsigned long time = millis();
		if (aliveTime != 0 && time > aliveTime && time - aliveTime > timeout)
			resetFunc();
	}
};

