#include <RCSwitch.h>

int PIR_TOP_PIN = 2;
int PIR_BOTTOM_PIN = 4;

RCSwitch mySwitch = RCSwitch();

void setup() {

 Serial.begin(9600);

 // Transmitter is connected to Arduino Pin #10 
 mySwitch.enableTransmit(10);

 // Optional set pulse length.
 mySwitch.setPulseLength(213);

 // set protocol (default is 1, will work for most outlets)
 // mySwitch.setProtocol(1);

 // Optional set number of transmission repetitions.
 // mySwitch.setRepeatTransmit(1);

  pinMode(PIR_TOP_PIN, INPUT);
  pinMode(PIR_BOTTOM_PIN, INPUT);
  pinMode(13,OUTPUT);
 
  digitalWrite(PIR_TOP_PIN, LOW);
  digitalWrite(PIR_BOTTOM_PIN, LOW);

}

void loop() {
  if (digitalRead(PIR_BOTTOM_PIN) && !digitalRead(PIR_TOP_PIN)) {
    digitalWrite(13, HIGH);
    mySwitch.send("010101010101111101010100");
  }
  else {
    digitalWrite(13, LOW);
  }
}
