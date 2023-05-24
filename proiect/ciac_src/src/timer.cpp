#include "utils.h"
void setup_timers(void)
{
	cli();
	// TIMER 1 - used for updating the display time
	TCCR1A = 0; // set entire TCCR1A register to 0
	TCCR1B = 0; // same for TCCR1B
	TCNT1 = 0;	// initialize counter value to 0
	// set compare match register for 1 Hz increments
	OCR1A = 62499; // = 16000000 / (256 * 1) - 1 (must be <65536)
	// turn on CTC mode
	TCCR1B |= (1 << WGM12);
	// Set CS12, CS11 and CS10 bits for 256 prescaler
	TCCR1B |= (1 << CS12) | (0 << CS11) | (0 << CS10);
	// enable timer compare interrupt
	TIMSK1 |= (1 << OCIE1A);

	sei(); // allow interrupts
}

void update_time(const char *time_packet)
{
	/*  TIME PACKET FORMAT:
		1 byte: Has the clock started
		1 byte: Current turn
		4 bytes: White time (big endian)
		4 bytes: Black time (big endian)
		1 byte: Newline character */

	clock_started = time_packet[0] != 0;
	turn = time_packet[1] == WHITE;
	white_time = 0;
	black_time = 0;

	/* TODO: REPLACE MAGIC NUMBERS */
    for (int i = 0; i < 4; i++) {
        white_time = (white_time << 8) | (time_packet[2 + i]);
        black_time = (black_time << 8) | (time_packet[6 + i]);
    }
}
ISR(TIMER1_COMPA_vect)
{
	if (clock_started) {
		if (turn == WHITE) {
			white_time--;
		} else {
			black_time--;
		}
		second_passed = true;
	}
}