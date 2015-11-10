#include <RCSwitch.h> // Library for sending/receiving RF signals
#include <buttons.h> // Button library
#include <EEPROM.h> // For saving settings
#include "LowPower.h" // Sleep between battles

// Programming button
Button programButton;
const int PRG_PIN= 4;
const int STATUS_LED = 8;
const int POWER_PIN = 10;

// PIR sensor pins
const int PIR_PIN = 3;
unsigned short interruptPin = digitalPinToInterrupt(PIR_PIN);

// TRIGGER_THRESHOLD sets how long we should wait before we trigger the door bell
// A human (with good intensions:) would probably have pushed the button by now
//const long TRIGGER_THRESHOLD = 30000; //30 seconds for development
const long TRIGGER_THRESHOLD = 90000; // 1.5 minutes

// timer variables
long last_trigger = 0;
long time_buffer = 0;

// Library for sending/receiving RF signals
RCSwitch mySwitch = RCSwitch();
const int TRANSMIT_PIN = 12;
const int RECEIVE_PIN = 2; // Don't change. There doesn't seem to be a way to set this in rc-switch

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
  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW);

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
  static bool new_trigger = false;
  
  // Check if new trigger is outside of given time frame (TRIGGER_THRESHOLD) thus a new incindent
  if ((millis() - last_trigger) >= TRIGGER_THRESHOLD)
  {
    Serial.print("Good night!");
    sleep();
    detachInterrupt(interruptPin);
    statusBlink(5, 30);
    time_buffer = 0;
    reset_timer = true;
    }
    
  if (digitalRead(PIR_PIN)) {
    new_trigger = true;

    // Check if we have activity over given time frame assuming a cat wants in 
    if (time_buffer >= TRIGGER_THRESHOLD)
    {
      // Give power to transmitter
      digitalWrite(POWER_PIN, HIGH);
  
      Serial.println("DING DONG!");

      // Ring door bell with code caught by copyKeys()
      // Status LED is mostly for debugging to indicate transmission of doorbell
      digitalWrite(STATUS_LED, HIGH);
      mySwitch.send(settings.bell_code, settings.bell_bit_length);
      
      // Reset timer
      time_buffer = 0;
      digitalWrite(STATUS_LED, LOW);

      // Turn of power to transmitter/receiver
      digitalWrite(POWER_PIN, LOW);
    }
    // Increment time_buffer if reset_timer is false and we're sure we have a new trigger
    else {
      if (!reset_timer && new_trigger)
        time_buffer += (millis() - last_trigger);
      else if (reset_timer)
        reset_timer = false;
    }

    // Update last_trigger
    last_trigger = millis();
  }
  // if PIR_PIN is LOW
  else
    new_trigger = false;
}

bool copyKeys()
{
  bool result = false;
  long counter = millis();
  
  mySwitch.enableReceive(0);  // Receiver on inerrupt 0 => that is pin #2
  digitalWrite(POWER_PIN, HIGH);

  statusBlink(2, 200); // To indicate that we're ready for programming
     
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
  digitalWrite(POWER_PIN, LOW);

  return result;
  }
  
void checkButtons(){
 switch (programButton.check()) {
   case Hold:
     mySwitch.disableTransmit();

     if (!copyKeys())
       statusBlink(5, 50);
     else
     {
       digitalWrite(STATUS_LED, HIGH);
       got_settings = true;
       saveData();
       mySwitch.enableTransmit(TRANSMIT_PIN);
       digitalWrite(STATUS_LED, LOW);
       time_buffer = 0;
     }
     break;
   default:
     break;
 }
}
