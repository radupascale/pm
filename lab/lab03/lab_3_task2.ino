#include <Servo.h>

Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int pos = 0;    // variable to store the servo position
int up = 1;
int step = 10;

int count_s = 0;
ISR(TIMER0_COMPA_vect) {
  count_s++;
  if (count_s % 100 == 0) {
    Serial.println("ba");
    if (up) {
      pos += step;
      if (pos > 180) {
        up = 0;
        pos = 180;
      }
    }
    else {
      pos -= step;
      if (pos < 0) {
        up = 1;
        pos = 0;
      }
    }
  }
}

void init_timer0(void) {
  TCCR0A = 0;
  TCCR0B = 0;
  TCNT0 = 0;
  OCR0A = 63;
  TCCR0B |= (1 << WGM10);   // PMW mode
  TCCR0B |= (1 << CS02);    // 256 prescaler 
  TIMSK0 |= (1 << OCIE0A);

  // Setup led

}

void setup() {
  Serial.begin(9600);
  cli();
  init_timer0();
  sei();

  myservo.attach(3);  // attaches the servo on pin 3 to the servo object
  // test led
  DDRD |= (1 << PD10);
  PORTD &= ~(1 << PD10);
}

void loop() {
  myservo.write(pos);
  delay(15);
}