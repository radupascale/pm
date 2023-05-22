#include "utils.h"
void setup_timers(void) {
	cli();
	// TIMER 0 - used for reading the board every 500ms
	TCCR0A = 0; // set entire TCCR0A register to 0
	TCCR0B = 0; // same for TCCR0B
	TCNT0  = 0; // initialize counter value to 0
	// set compare match register for 500 Hz increments
	OCR0A = 124; // = 16000000 / (256 * 500) - 1 (must be <256)
	// turn on CTC mode
	TCCR0A |= (1 << WGM01);
	// Set CS02, CS01 and CS00 bits for 256 prescaler
	TCCR0B |= (1 << CS02) | (0 << CS01) | (0 << CS00);
	// enable timer compare interrupt
	TIMSK0 |= (1 << OCIE0A);
	sei(); // allow interrupts

	// // TIMER 1 - used for updating the display time
	// TCCR1A = 0; // set entire TCCR1A register to 0
	// TCCR1B = 0; // same for TCCR1B
	// TCNT1  = 0; // initialize counter value to 0
	// // set compare match register for 1 Hz increments
	// OCR1A = 62499; // = 16000000 / (256 * 1) - 1 (must be <65536)
	// // turn on CTC mode
	// TCCR1B |= (1 << WGM12);
	// // Set CS12, CS11 and CS10 bits for 256 prescaler
	// TCCR1B |= (1 << CS12) | (0 << CS11) | (0 << CS10);
	// // enable timer compare interrupt
	// TIMSK1 |= (1 << OCIE1A);

	// sei(); // allow interrupts
}

ISR(TIMER0_COMPA_vect){
   //interrupt commands for TIMER 0 here
}

ISR(TIMER1_COMPA_vect){
   //interrupt commands for TIMER 1 here

}