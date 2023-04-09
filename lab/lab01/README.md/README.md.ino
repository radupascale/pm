void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  DDRD &= ~_BV(PD2);
  PORTD |= _BV(PD2);

  DDRB |= _BV(PB5);
}

void loop() {
  // put your main code here, to run repeatedly:
  int value = PIND & _BV(PD2);

  if (value == 0) {
    int state = PINB & _BV(PB5);
    if (state == 0) {
      PORTB |= (1 << PB5);
      delay(500);
    }
    else {
      PORTB &= ~(1 << PB5);
      delay(500);
    }
  }
}
