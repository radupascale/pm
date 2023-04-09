int LED = PB5;
int BUTTON = PD2;
long ts = 0;
char buf[20];
int index = 0;
bool blink = false;

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  pinMode(BUTTON, INPUT_PULLUP);
  Serial.begin(9600);
}

void readCommand(char buf[]) {
  if (strcmp(buf, "on") == 0) {
    blink = false;
    Serial.print("Received on\n");
    PORTB |= (1 << LED);
  }
  else if (strcmp(buf, "off") == 0) {
    blink = false;
    Serial.print("Received off\n");
    digitalWrite(LED, LOW);
  }
  else if (strcmp(buf, "blink") == 0) {
    blink = true;
    Serial.print("Received blink\n");
  }
  else if (strcmp(buf, "get") == 0) {
    int button = digitalRead(BUTTON);
    Serial.print("Received get\n");
    Serial.print("Starea butonului: ");
    Serial.print(button);
    Serial.print("\n");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (blink && (millis() - ts) >= 100) {
    ts = millis();
    PORTB ^= (1 << LED);
  }

  if (Serial.available()) {
    char a = Serial.read();
    if (a == '\n') {
      buf[index] = '\0';
      index = 0;
      readCommand(buf);
    }
    else {
      sprintf(buf+index, "%c", a);
      index++;
    }
  }

}
