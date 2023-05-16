#include <Arduino.h>
#define BUFF_SIZE 100
#define BUTTON_DEBOUNCE_TIME 200

// Constants used for sending information to the serial port
#define BOARD_STATE_BYTE 'B'

char buff[BUFF_SIZE];
// Pin connected to ST_CP of 74HC595
int latchPin = PD4;
// Pin connected to SH_CP of 74HC595
int clockPin = PD6;
////Pin connected to DS of 74HC595
int dataPin = PD5;
// Pin connected to MR of 74HC595
int resetPin = A5;

// holders for information you're going to pass to shifting function
byte data;
byte dataArray[10];
int RANKS = 8;
int FILES = 8;

// MUX variables
const int selectPins[3] = {A4, A3, A2};  // S0~3, S1~4, S2~5
const int zInput = PD7;  // Connect common (Z) to A0 (analog input)
byte chessboard[64];

// Button variables
int buttonPin = PD2;
volatile int buttonState = 0;
unsigned int t = 0;

// blinks the whole register based on the number of times you want to
// blink "n" and the pause between them "d"
// starts with a moment of darkness to make sure the first blink
// has its full visual effect.
// the heart of the program
void shiftOut(int myDataPin, int myClockPin, byte myDataOut) {
	
	// This shifts 8 bits out MSB first,
	// on the rising edge of the clock,
	// clock idles low
	// internal function setup

	int i = 0;
	int pinState;
	pinMode(myClockPin, OUTPUT);
	pinMode(myDataPin, OUTPUT);

	// clear everything out just in case to
	// prepare shift register for bit shifting
	digitalWrite(myDataPin, 0);
	digitalWrite(myClockPin, 0);

	// for each bit in the byte myDataOut&#xFFFD;
	// NOTICE THAT WE ARE COUNTING DOWN in our for loop
	// This means that %00000001 or "1" will go through such
	// that it will be pin Q0 that lights.

	for (i = 7; i >= 0; i--) {
		digitalWrite(myClockPin, 0);
		// if the value passed to myDataOut and a bitmask result
		// true then... so if we are at i=6 and our value is
		// %11010100 it would the code compares it to %01000000
		// and proceeds to set pinState to 1.
		if (myDataOut & (1 << i)) {
			pinState = 1;
		} else {
			pinState = 0;
		}
		// Sets the pin to HIGH or LOW depending on pinState
		digitalWrite(myDataPin, pinState);
		// register shifts bits on upstroke of clock pin
		digitalWrite(myClockPin, 1);
		// zero the data pin after shift to prevent bleed through
		digitalWrite(myDataPin, 0);
	}
	// stop shifting
	digitalWrite(myClockPin, 0);
}

void clear_registers() {
	digitalWrite(resetPin, 0);
	digitalWrite(resetPin, 1);
}

void light_rank(int rank) {
	digitalWrite(latchPin, 0);
	data = (1 << rank);
	shiftOut(dataPin, clockPin, data);
	digitalWrite(latchPin, 1);
}

void setup_shift_register() {
	pinMode(resetPin, OUTPUT);
	pinMode(dataPin, OUTPUT);
	pinMode(clockPin, OUTPUT);
	clear_registers();
	pinMode(latchPin, OUTPUT);
	digitalWrite(latchPin, 0);
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

ISR(INT0_vect) {
    if (millis() - t > BUTTON_DEBOUNCE_TIME) {
        buttonState = 1;
        t = millis();
    }
}

void setup_button() {
    cli();
    pinMode(buttonPin, INPUT_PULLUP);
    EIMSK |= (1 << INT0);  // Enable INT0

    // Trigger INT0 on falling edge
    EICRA |= (1 << ISC01);

    // Enable global interrupts
    sei();
}

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

void print_chessboard() {
	/* Header */
	Serial.println("\tA\tB\tC\tD\tE\tF\tG\tH");
	for (int i = 0; i < RANKS; i++) {
		Serial.print(String(RANKS-i) + "\t");
		for (int j = 0; j < FILES; j++) {
			Serial.print(String(chessboard[i * RANKS + j]) + "\t");
		}
		Serial.println(String(RANKS-i));
	}
	Serial.println("\tA\tB\tC\tD\tE\tF\tG\tH");
}

void send_chessboard() {
    /* Send the chessboard to the serial port from the eight rank to the first rank
     * and from the first file to the eighth file.
    */
    Serial.write(BOARD_STATE_BYTE);  // B for board
    Serial.write(chessboard, RANKS * FILES);
    Serial.write('\n');
}

void setup() {
	Serial.begin(9600);
	setup_shift_register();
	setup_mux();
    setup_button();
}

int readline(int readch, char *buffer, int len) {
	static int pos = 0;
	int rpos;

	if (readch > 0) {
		switch (readch) {
			case '\r': // Ignore CR
				break;
			case '\n': // Return on new-line
				rpos = pos;
				pos = 0;  // Reset position index ready for next time
				return rpos;
			default:
				if (pos < len-1) {
					buffer[pos++] = readch;
					buffer[pos] = 0;
				}
		}
	}
	return 0;   
}

void loop() {
    if (buttonState) {
        for (int i = 0; i < RANKS; i++) {
            light_rank(i);
            read_mux(i);
        }
        send_chessboard();
        buttonState = 0;
    }
}
