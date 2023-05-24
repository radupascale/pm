#include "utils.h"
#define BUTTON_DEBOUNCE_TIME 300

// Button variables
int buttonPin = PD2;
volatile int buttonState = 0;
unsigned int t = 0;

// GLOBAL VARIABLES
char buff[BUFF_SIZE];
int latchPin = PD4;
int clockPin = PD6;
int dataPin = PD5;
int resetPin = A5;
byte data;
byte dataArray[10];
const int selectPins[3] = {A4, A3, A2}; // S0~3, S1~4, S2~5
const int zInput = PD7;					// Connect common (Z) to A0 (analog input)
byte chessboard[64];					// Could be 8 bytes, but 64 bytes is easier to work with

Ucglib_ILI9341_18x240x320_HWSPI ucg(/*cd=*/9, /*cs=*/10, /*reset=*/8);
// XPT2046 touch(/*cs=*/23, /*irq=*/ 3);
TFT tft;
lichess_time_t white_time = 0;
lichess_time_t black_time = 0;
volatile bool turn = WHITE; /* Compared against WHITE or BLACK */
volatile bool clock_started = false;
volatile bool second_passed = true;

void setup_button()
{
	cli();
	pinMode(buttonPin, INPUT_PULLUP);
	EIMSK |= (1 << INT0); // Enable INT0

	// Trigger INT0 on falling edge
	EICRA |= (1 << ISC01);

	sei();
}

ISR(INT0_vect)
{
	if (millis() - t > BUTTON_DEBOUNCE_TIME && buttonState == 0) {
		buttonState = 1;
		t = millis();
	}
}

void setup()
{
	Serial.begin(9600);
	setup_shift_register();
	setup_mux();
	tft.setup_tft();
	tft.draw_game();
	setup_button();
	// setup_timers();
}

void loop()
{
	if (Serial.available() > 0) {
		char serial_buff[BUFF_SIZE] = {0};
		while (!readline(Serial.read(), serial_buff, BUFF_SIZE))
			;

		switch (serial_buff[0]) {
		case BOARD_STATE:
			/* This means we read a new board state sent by the python script */
			for (int i = 0; i < RANKS * FILES; i++) {
				tft.update_pieces(serial_buff + 1);
			}
			break;
		case BOARD_MOVE:
			tft.draw_last_move(serial_buff + 1);
			break;
		case BOARD_TIME:
			update_time(serial_buff + 1);
			break;
		}
	}

	if (buttonState) {
		for (int i = 0; i < RANKS; i++) {
			light_rank(i);
			read_mux(i);
		}
		buttonState = 0;
		send_chessboard();
	}
}