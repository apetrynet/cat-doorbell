#include <RCSwitch.h> // Library for sending/receiving RF signals
#include <buttons.h> // Button library
#include <EEPROM.h> // For saving settings

// Programming button
Button programButton;
const int PRG_PIN= 3;
const int STATUS_LED = 8;
const int BOOST_POWER_PIN = 10;

// PIR sensor pin
const int PIR_PIN = 4;

// FOR DEBUG READ FURTHER DOWN
// This is because the PIR sends a repeated continous signal on activity 
long TRIGGER_THRESHOLD = 30000; //30 seconds

// TRIGGER_THRESHOLD sets how long we should wait before we trigger the door bell
// A human (with good intensions:) would probably have pushed the button by now
//long TRIGGER_THRESHOLD = 90000; // 1.5 minutes

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

// Status of settings
bool got_settings = false;

void setup() {

  Serial.begin(9600);

  // Setup PINs for RF
  pinMode(TRANSMIT_PIN, OUTPUT);
  pinMode(RECEIVE_PIN, INPUT);

  // Set pinmode and make sure transmitter doesn't get unnecessary power.
  pinMode(BOOST_POWER_PIN, OUTPUT);
  digitalWrite(BOOST_POWER_PIN, LOW);

  // Setup program button behavior
  pinMode(PRG_PIN, INPUT);
  programButton.assign(PRG_PIN);
  programButton.setMode(OneShotTimer);
  programButton.setTimer(1500);
  
  // Make sure we READ from the sensor
  pinMode(PIR_PIN, INPUT);

  // Avoid false positive trigger  
  digitalWrite(PIR_PIN, LOW);

  // Check and see if we find settings
  if (!checkSettings())
      digitalWrite(STATUS_LED, HIGH);
    else
      got_settings = true;
      
}


void loop() {
  checkButtons();

  if (got_settings)
    checkTrigger();
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

  return true;
  }
  
void checkTrigger()
{
  static long last_trigger = millis();
  static long time_buffer = 0;
  static bool reset_timer = false;
  
  if (digitalRead(PIR_PIN)) {

    // Check if new trigger is outside of given time frame (TRIGGER_THRESHOLD) thus a new incindent
    if ((millis() - last_trigger) >= TRIGGER_THRESHOLD)
    {
      Serial.print("Trigger timed out. resetting buildup: ");
      Serial.println(time_buffer);
      time_buffer = 0;
      reset_timer = true;
      Serial.println(time_buffer);
      }
    
    // Check if we have activity over given time frame assuming a cat wants in 
    if (time_buffer >= TRIGGER_THRESHOLD)
    {
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
  digitalWrite(BOOST_POWER_PIN, HIGH);
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
  digitalWrite(BOOST_POWER_PIN, LOW);
  return result;
  }
  
void checkButtons(){
 switch (programButton.check()) {
   case Hold:
     digitalWrite(STATUS_LED, LOW);
     delay(200);
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
