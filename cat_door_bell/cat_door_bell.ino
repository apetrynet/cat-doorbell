// Library for sending/receiving RF signals
#include <RCSwitch.h>

// PIR sensor pin
int PIR_PIN = 4;

// FOR DEBUG READ FURTHER DOWN
// This is because the PIR sends a repeated continous signal on activity 
//int TRIGGER_DURATION = 3000; //3 seconds

// TRIGGER_THRESHOLD sets how long we should wait before we trigger the door bell
// A human (with good intensions:) would probably have pushed the button by now
long TRIGGER_THRESHOLD = 90000; // 1.5 minutes

// timer variables
long last_trigger = 0;
long time_buffer = 0;

// Library for sending/receiving RF signals
RCSwitch mySwitch = RCSwitch();


void setup() {

  //Serial.begin(9600);

  // Transmitter is connected to Arduino Pin #10 
  mySwitch.enableTransmit(10);

  // Optional set pulse length.
  mySwitch.setPulseLength(213);

  // set protocol (default is 1, will work for most outlets)
  //mySwitch.setProtocol(1);

  // Optional set number of transmission repetitions.
  //mySwitch.setRepeatTransmit(1);

  // Make sure we READ from the sensor
  pinMode(PIR_PIN, INPUT);

  // Avoid false positive trigger  
  digitalWrite(PIR_PIN, LOW);
}


void loop() {
  if (digitalRead(PIR_PIN)) {
    
    // Avoid false positive on initial run
    if (last_trigger == 0) {
      last_trigger = millis();
    }
      
    // Check if new trigger is outside of given time frame (TRIGGER_THRESHOLD) thus a new incindent
    if ((millis() - last_trigger) >= TRIGGER_THRESHOLD)
    {
      //Serial.println("Trigger timed out. resetting buildup");
      time_buffer = 0;
    }
    
    // Check if we have activity over given time frame assuming a cat wants in 
    if (time_buffer >= TRIGGER_THRESHOLD)
    {
      //Serial.println("DING DONG!");

      // Ring door bell with code caught by rc-switch ReceiveDemoAdvanced
      mySwitch.send("010101010101111101010100");
      
      // Reset timers
      time_buffer = 0;
    }
    // Increment time_buffer
    else {
      time_buffer += (millis() - last_trigger);
    }

    // Update last_trigger
    last_trigger = millis();
/*
    // FOR DEBUGGING THIS MIGHT BE BETTER THEN THE "else" ABOVE
    // Filter out repeated triggers from the PIR and update time_buffer
    else if ((millis() - last_trigger) >= TRIGGER_DURATION)
    {
      time_buffer += (millis() - last_trigger);
      Serial.print("time_buffer: ");
      Serial.println(time_buffer);
      last_trigger = millis();
    }
*/

  }
}
