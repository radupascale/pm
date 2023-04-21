#include "SPI.h"

byte data[100] = "buna ziua\n";

void setup_spi() {
  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);

  SPI.begin();

  SPI.setClockDivider(SPI_CLOCK_DIV32);

}

void send_vector(byte data[]) {
  digitalWrite(SS, LOW);
  for (int i = 0; i < strlen(data); i++) {
    byte masterRecv = SPI.transfer(data[i]);
  }
  digitalWrite(SS, HIGH);
  delay(250);
}

void send_data(byte data) {
  /* Wait for transmission complete */
  digitalWrite(SS, LOW);
  byte masterRecv = SPI.transfer(data);
  digitalWrite(SS, HIGH);
}

void setup() {
  Serial.begin(9600);
  setup_spi();
}

void loop() {
  // put your main code here, to run repeatedly:
  // send_data('a');
  send_vector(data);
}
