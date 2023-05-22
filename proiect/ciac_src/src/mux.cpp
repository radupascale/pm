#include "utils.h"

void setup_mux() {
	// Hardware Hookup:
	// Mux Breakout ----------- Arduino
	//  S0 ------------------- 3
	//  S1 ------------------- 4
	//  S2 ------------------- 5
	//  Z -------------------- A0
	// VCC ------------------- 5V
	// GND ------------------- GND
	// (VEE should be connected to GND)

	// Set up the select pins as outputs:
	for (int i = 0; i < 3; i++) {
		pinMode(selectPins[i], OUTPUT);
		digitalWrite(selectPins[i], HIGH);
	}
	pinMode(zInput, INPUT_PULLUP);
}

void selectMuxPin(byte pin) {
	for (int i = 0; i < 3; i++) {
		if (pin & (1 << i))
			digitalWrite(selectPins[i], HIGH);
		else
			digitalWrite(selectPins[i], LOW);
	}
}

void read_mux(int rank) {
	/* Rank0 = Q0, Rank1 = Q1, etc.
	 * A = Y7, B = Y6, etc. 
	 */
	for (int file = FILES-1; file >= 0; file--) {
		selectMuxPin((byte)file);
		int inputValue = digitalRead(zInput);
		// chessboard[RANKS - 1 - rank][file] = inputValue;
        chessboard[(RANKS - 1 - rank) * RANKS + file] = inputValue;
    }
}