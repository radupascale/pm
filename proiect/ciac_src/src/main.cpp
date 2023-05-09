#include <Arduino.h>
/*

  Shift Register Example

  Turning on the outputs of a 74HC595 using an array

 Hardware:

 * 74HC595 shift register

 * LEDs attached to each of the outputs of the shift register

 */
// Pin connected to ST_CP of 74HC595
int latchPin = 8;
// Pin connected to SH_CP of 74HC595
int clockPin = 12;
////Pin connected to DS of 74HC595
int dataPin = 11;
// Pin connected to MR of 74HC595
int resetPin = 2;

// holders for information you're going to pass to shifting function
byte data;
byte dataArray[10];
int COL_COUNT = 8;
int LINE_COUNT = 8;

// MUX variables
const int selectPins[3] = {3, 4, 5};  // S0~3, S1~4, S2~5
const int zInput = 10;  // Connect common (Z) to A0 (analog input)
int chessboard[8][8];

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
    delay(100);
    digitalWrite(resetPin, 1);
}

void light_column(int column) {
	// Set clear bit to 0 to clear the registers
    digitalWrite(latchPin, 0);
    data = (1 << column);
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
    // Print header
    for (int i = 0; i < 3; i++) {
        if (pin & (1 << i))
            digitalWrite(selectPins[i], HIGH);
        else
            digitalWrite(selectPins[i], LOW);
    }
}

void read_mux(int col) {
    // Loop through all eight pins.
    for (byte line = 0; line < LINE_COUNT; line++) {
        selectMuxPin(line);                    // Select one at a time
        int inputValue = digitalRead(zInput);  // and read Z
        chessboard[line][col] = inputValue;
        // Serial.print(String(inputValue) + "\t");
    }
    Serial.println();
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
    Serial.println("\tY0\tY1\tY2\tY3\tY4\tY5\tY6\tY7");
    for (int i = 0; i < LINE_COUNT; i++) {
        Serial.print("COL: " + String(i) + "\t");
        for (int j = 0; j < COL_COUNT; j++) {
            Serial.print(String(chessboard[j][i]) + "\t");
        }
        Serial.println();
    }
}

void light_and_print(int col) {
    light_column(col);
    read_mux(col);
    /* Header */
    Serial.println("Y0\tY1\tY2\tY3\tY4\tY5\tY6\tY7");
    for (int i = 0; i < LINE_COUNT; i++) {
        Serial.print(String(chessboard[i][col]) + "\t");
    }
    Serial.println();
}

void setup() {
    Serial.begin(9600);
    setup_shift_register();
	setup_mux();
}

void loop() {
    if (Serial.available()) {
        char a = Serial.read();
        /* Parse the int stored in a and light the column */
        int col = a - '0';
        if (col >= 0 && col < COL_COUNT) {
            Serial.println("Lighting column " + String(col));
            light_and_print(col);
        }
        else if (a == 'p') {
            for (int i = 0; i < COL_COUNT; i++) {
                // Serial.print("Column: " + String(i) + "\t");
                light_column(i);
                read_mux(i);
            }
            print_chessboard();
        }
        else {
            Serial.println("Invalid input");
        }

    }

}
