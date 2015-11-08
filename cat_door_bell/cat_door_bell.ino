#include <RCSwitch.h> // Library for sending/receiving RF signals
#include <buttons.h> // Button library

// Programming button
Button programButton;
const int PRG_PIN= 3;
const int STATUS_LED = 8;

// PIR sensor pin
const int PIR_PIN = 4;

// FOR DEBUG READ FURTHER DOWN
// This is because the PIR sends a repeated continous signal on activity 
long TRIGGER_THRESHOLD = 10000; //10 seconds

// TRIGGER_THRESHOLD sets how long we should wait before we trigger the door bell
// A human (with good intensions:) would probably have pushed the button by now
//long TRIGGER_THRESHOLD = 90000; // 1.5 minutes

// timer variables
long last_trigger = 0;
long time_buffer = 0;

// Library for sending/receiving RF signals
RCSwitch mySwitch = RCSwitch();
const int TRANSMIT_PIN = 12;
const int RECEIVE_PIN = 2;

// RC-Switch data
struct BellData {
  unsigned int bell_bit_length = 0;
  unsigned long bell_code = 0;
  unsigned int pulse_length = 0;
  unsigned int protocol = 0;
};

BellData bellData;

void setup() {

  Serial.begin(9600);

  // Setup PINs for RF
  pinMode(TRANSMIT_PIN, OUTPUT);
  pinMode(RECEIVE_PIN, INPUT);

  // Setup program button behavior
  pinMode(PRG_PIN, INPUT);
  programButton.assign(PRG_PIN);
  programButton.setMode(OneShotTimer);
  programButton.setTimer(1500);
  
  // Make sure we READ from the sensor
  pinMode(PIR_PIN, INPUT);

  // Avoid false positive trigger  
  digitalWrite(PIR_PIN, LOW);
}


void loop() {
  checkButtons();
  checkTrigger();
}

void checkTrigger()
{
  static long last_trigger = millis();
  static long time_buffer = 0;
  
  if (digitalRead(PIR_PIN)) {

    // Check if new trigger is outside of given time frame (TRIGGER_THRESHOLD) thus a new incindent
    if ((millis() - last_trigger) >= TRIGGER_THRESHOLD)
    {
      //Serial.println("Trigger timed out. resetting buildup");
      time_buffer = 0;
    }
    
    // Check if we have activity over given time frame assuming a cat wants in 
    if (time_buffer >= TRIGGER_THRESHOLD)
    {
      Serial.println("DING DONG!");

      Serial.print("bell code ");
      Serial.println(bellData.bell_code);
      Serial.print("bit length ");
      Serial.println(bellData.bell_bit_length);
      Serial.print("pulse length ");
      Serial.println(bellData.pulse_length);
      Serial.print("protocol ");
      Serial.println(bellData.protocol);
      
      // Ring door bell with code caught by copyKeys()
      digitalWrite(STATUS_LED, HIGH);
      mySwitch.send(bellData.bell_code, bellData.bell_bit_length);
      
      // Reset timers
      time_buffer = 0;
      digitalWrite(STATUS_LED, LOW);
    }
    // Increment time_buffer
    else {
      time_buffer += (millis() - last_trigger);
    }

    // Update last_trigger
    last_trigger = millis();
  }
}

bool copyKeys()
{
  bool result = false;
  long counter = millis();
  while ((millis() - counter) <= 5000)
  {
    mySwitch.disableTransmit();
    mySwitch.enableReceive(0);  // Receiver on inerrupt 0 => that is pin #2
    
    if (mySwitch.available())
    {
      // Set protocol
      bellData.protocol = mySwitch.getReceivedProtocol();
      mySwitch.setProtocol(bellData.protocol);

      // Bell code
      bellData.bell_code = mySwitch.getReceivedValue();

      // Bell bit length
      bellData.bell_bit_length = mySwitch.getReceivedBitlength();
      
      // Optional set pulse length.
      bellData.pulse_length = mySwitch.getReceivedDelay();
      mySwitch.setPulseLength(bellData.pulse_length);

      mySwitch.resetAvailable();
      
      result = true;
      break;
      }
    }
  mySwitch.disableReceive();
  return result;
  }
  
void checkButtons(){
 switch (programButton.check()) {
   case Hold:
     digitalWrite(STATUS_LED, HIGH);
     delay(200);
     digitalWrite(STATUS_LED, LOW);
     
     if (!copyKeys())
     {
       for (int i=0; i < 5; i++)
        {
          digitalWrite(STATUS_LED, HIGH);
          delay(150);
          digitalWrite(STATUS_LED, LOW);
          delay(150);
          }
     }
     else
     {
       digitalWrite(STATUS_LED, HIGH);
       delay(1500);
       digitalWrite(STATUS_LED, LOW);
       mySwitch.enableTransmit(TRANSMIT_PIN);
       time_buffer = 0;
     }
     break;
   default:
     break;
 }
}
