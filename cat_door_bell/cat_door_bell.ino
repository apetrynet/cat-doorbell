#include <RCSwitch.h> // Library for sending/receiving RF signals
#include <buttons.h> // Button library
#include <EEPROM.h> // For saving settings
#include "LowPower.h" // Sleep between battles

// Programming button
Button programButton;
const int PRG_PIN= 4;
const int STATUS_LED = 8;
const int RECEIVER_POWER_PIN = 10;

// PIR sensor pins
const int PIR_PIN = 3;
unsigned short interruptPin = digitalPinToInterrupt(PIR_PIN);

// TRIGGER_THRESHOLD sets how long we should wait before we trigger the door bell
// A human (with good intensions:) would probably have pushed the button by now
const long TRIGGER_THRESHOLD = 30000; //30 seconds for development
//const long TRIGGER_THRESHOLD = 90000; // 1.5 minutes

// timer variables
long last_trigger = 0;
long time_buffer = 0;

// Library for sending/receiving RF signals
RCSwitch mySwitch = RCSwitch();
const int TRANSMIT_PIN = 12;
const int RECEIVE_PIN = 2; // Don't change. There doesn't seem to be a way to set this in rc-switch
const int TRANSMITTER_POWER_PIN = 11;

// RC-Switch data
struct BellData {
  unsigned int bell_bit_length = 0;
  unsigned long bell_code = 0;
  unsigned int pulse_length = 0;
  unsigned int protocol = 0;
};

// Storage of data
BellData settings;

// Settings toggle
bool got_settings = false;
  
void setup() {

  Serial.begin(9600);

  // Setup PINs for RF
  pinMode(TRANSMIT_PIN, OUTPUT);
  pinMode(RECEIVE_PIN, INPUT);

  // Set pinmode and make sure transmitter/receiver don't get unnecessary power.
  pinMode(RECEIVER_POWER_PIN, OUTPUT);
  digitalWrite(RECEIVER_POWER_PIN, LOW);
  pinMode(TRANSMITTER_POWER_PIN, OUTPUT);
  digitalWrite(TRANSMITTER_POWER_PIN, LOW);

  // Setup program button behavior
  pinMode(PRG_PIN, INPUT);
  programButton.assign(PRG_PIN);
  programButton.setMode(OneShotTimer);
  programButton.setTimer(1500);
  
  // Make sure we READ from the sensor
  pinMode(PIR_PIN, INPUT);

  // Avoid false positive trigger  
  digitalWrite(PIR_PIN, LOW);

  // Setup Status led
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  // Attempt to restore settings
  if (!checkSettings())
    {
      Serial.println("no settings found");
      digitalWrite(STATUS_LED, HIGH);
    }
  
}


void loop() {
  checkButtons();

  if (got_settings)
    checkTrigger();
}

void statusBlink(int num, int hold)
{
  for (int i=0; i < num; i++)
  {
    digitalWrite(STATUS_LED, HIGH);
    delay(hold);
    digitalWrite(STATUS_LED, LOW);
    delay(hold);
    }
  }

void sleep()
{
  attachInterrupt(interruptPin, wake, HIGH);
  statusBlink(3, 30);
  //Serial.println("Good night!");
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  }

void wake()
{
  //Serial.println("Good morning!");
  }

void saveData()
{
  EEPROM.put(0, settings);
  }

void loadData()
{
  EEPROM.get(0, settings);
  }

bool checkSettings()
{
  loadData();
  
  if (settings.bell_bit_length == 0)
    return false;

  if (settings.bell_code == 0)
    return false;

  if (settings.pulse_length == 0)
    return false;

  if (settings.protocol == 0)
    return false;

  got_settings = true;
  return true;
  }
  
void checkTrigger()
{
  static long last_trigger = millis();
  static long time_buffer = 0;
  static bool reset_timer = false;
  
  // Check if new trigger is outside of given time frame (TRIGGER_THRESHOLD) thus a new incindent
  if ((millis() - last_trigger) >= TRIGGER_THRESHOLD)
  {
    Serial.print("Timeout");
    sleep();
    detachInterrupt(interruptPin);
    statusBlink(5, 30);
    time_buffer = 0;
    reset_timer = true;
    }
    
  if (digitalRead(PIR_PIN)) {

    // Check if we have activity over given time frame assuming a cat wants in 
    if (time_buffer >= TRIGGER_THRESHOLD)
    {
      // Give power to transmitter
      digitalWrite(TRANSMITTER_POWER_PIN, HIGH);
  
      Serial.println("DING DONG!");

      Serial.print("bell code ");
      Serial.println(settings.bell_code);
      Serial.print("bit length ");
      Serial.println(settings.bell_bit_length);
      Serial.print("pulse length ");
      Serial.println(settings.pulse_length);
      Serial.print("protocol ");
      Serial.println(settings.protocol);
      
      // Ring door bell with code caught by copyKeys()
      digitalWrite(STATUS_LED, HIGH);
      mySwitch.send(settings.bell_code, settings.bell_bit_length);
      
      // Reset timers
      time_buffer = 0;
      digitalWrite(STATUS_LED, LOW);

      // Turn of power to transmitter
      digitalWrite(TRANSMITTER_POWER_PIN, LOW);
    }
    // Increment time_buffer if just didn't reset timer
    else {
      if (!reset_timer)
        time_buffer += (millis() - last_trigger);
      else
        reset_timer = false;
    }

    // Update last_trigger
    last_trigger = millis();
  }

}

bool copyKeys()
{
  bool result = false;
  long counter = millis();
  mySwitch.disableTransmit();
  digitalWrite(RECEIVER_POWER_PIN, HIGH);
  mySwitch.enableReceive(0);  // Receiver on inerrupt 0 => that is pin #2

  while ((millis() - counter) <= 5000)
  {
    if (mySwitch.available())
    {
      // Set protocol
      settings.protocol = mySwitch.getReceivedProtocol();
      mySwitch.setProtocol(settings.protocol);

      // Bell code
      settings.bell_code = mySwitch.getReceivedValue();

      // Bell bit length
      settings.bell_bit_length = mySwitch.getReceivedBitlength();
      
      // Optional set pulse length.
      settings.pulse_length = mySwitch.getReceivedDelay();
      mySwitch.setPulseLength(settings.pulse_length);

      mySwitch.resetAvailable();
      
      result = true;
      break;
      }
    }
  mySwitch.disableReceive();
  digitalWrite(RECEIVER_POWER_PIN, LOW);
  return result;
  }
  
void checkButtons(){
 switch (programButton.check()) {
   case Hold:
     statusBlink(2, 200);
     
     if (!copyKeys())
       statusBlink(5, 50);
     else
     {
       digitalWrite(STATUS_LED, HIGH);
       got_settings = true;
       saveData();
       digitalWrite(STATUS_LED, LOW);
       mySwitch.enableTransmit(TRANSMIT_PIN);
       time_buffer = 0;
     }
     break;
   default:
     break;
 }
}
