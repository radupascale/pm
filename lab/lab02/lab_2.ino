// void setup() {
//   /* NOTES: */
//   // sei() -> activeaza intreruperile
//   // cli() -> dezactiveaza intreruperile
//   // registrul EICRA: cu el setam modul (F,R,C,L)
//   // ISC11, ISC10,(INT1 - PIN3) ISC01, ISC00(INT0 - PIN2)
//   // EIMSK - INT1, INT0 (seteaza intreruperea)
//   // PCICR - registru care activeaza vectorii de intreruperi (PCMSK0, PCMSK1, PCMSK2)
//   // PCINT0 ~ PCMSK0
//   // ISR(vector) - apelata de intreruperi

// }

// void loop() {
//   // put your main code here, to run repeatedly:

// }

volatile int dt = 500;

unsigned long lastPress = 0;

ISR(INT0_vect)
{
  // cod întrerupere externă
  if ((millis() - lastPress >= 100)) {
    dt += 100;
    lastPress = millis();
  }

}
 
ISR(PCINT2_vect) {
  // cod întrerupere de tip pin change
  // TODO
  // citesc val de pe PIN4 ca sa vad daca e 0(buton apasat)
  if ((millis() - lastPress >= 500) && !(PIND & (1 << PD4)) && dt > 0) {
    dt -= 100;
    lastPress = millis();
  }
}
 
void setup_interrupts() {
  // buton 1: PD2 / INT0
  // buton 2: PD4 / PCINT20
  cli();
 
  // input
  DDRD &= ~(1 << PD2) & ~(1 << PD4);
  // input pullup
  PORTD |= (1 << PD2) | (1 << PD4);
 
  // configurare intreruperi
  // intreruperi externe
  // TODO
  EICRA |= (1 << ISC11);
 
  // întreruperi de tip pin change (activare vector de întreruperi)
  // TODO
  PCICR |= (1 << PCIE2);
 
  // activare intreruperi
  // intrerupere externa
  // TODO
  EIMSK |= (1 << INT0);
  
  // întrerupere de tip pin change
  // TODO
  PCMSK2 |= (1 << PCINT20);
 
  sei();
}
 
void setup() {
  setup_interrupts();
  DDRD |= (1 << PD7);
  PORTD &= ~(1 << PD7);
}
 
void loop() {  
  // increase or decrease blinking time on button press
  PORTD |= (1 << PD7);
  delay(dt);
  PORTD &= ~(1 << PD7);
  delay(dt);
}
