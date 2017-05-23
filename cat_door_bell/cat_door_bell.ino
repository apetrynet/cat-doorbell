#include <RCSwitch.h> // Library for sending/receiving RF signals
#include <buttons.h> // Button library
#include <EEPROM.h> // For saving settings
#include "LowPower.h" // Sleep between battles

// Programming button
Button programButton;
const int PRG_PIN= 4;
const int STATUS_LED = 8;
const int RCV_POWER_PIN = 9;
const int TR_POWER_PIN = 10;

// PIR sensor pins
const int PIR_PIN = 3; // Don't change. Interrupt pin
const unsigned short interruptPin = digitalPinToInterrupt(PIR_PIN);

// TRIGGER_THRESHOLD sets how long we should wait before we trigger the door bell
// A human (with good intensions:) would probably have pushed the button by now
const long TRIGGER_THRESHOLD = 20000; // 20 seconds

// Amount of censecutive triggers before snooze. For trigger happy PIR sensors
// reacting to the sun or for when your cat needs some time to settle.
// Set ANNOYANCE_LEVEL to -1 if you don't want to use the snooze feature
const int ANNOYANCE_LEVEL = 3;
const long SNOOZE_TIME = 60000 * 15; // 15 minutes
bool snoozing = false;
long snooze_counter;
int number_of_calls = 0;

// Library for sending/receiving RF signals
RCSwitch mySwitch = RCSwitch();
const int TRANSMIT_PIN = 12;
const int RECEIVE_PIN = 2; // Don't change. Interrupt pin

// RC-Switch data
struct BellData {
  unsigned int bell_bit_length;
  unsigned long bell_code;
  unsigned int pulse_length;
  unsigned int protocol;
  unsigned int magic;
};

// Storage of data
static BellData settings;

// Settings toggle
static bool got_settings = false;

// Timer variables
static long last_trigger = millis();
static long time_buffer = 0;

void setup() {

  //Serial.begin(9600);

  // Setup PINs for RF
  pinMode(TRANSMIT_PIN, OUTPUT);
  digitalWrite(TRANSMIT_PIN, LOW);
  pinMode(RECEIVE_PIN, INPUT);
  digitalWrite(RECEIVE_PIN, LOW);

  // Set pinmode and make sure transmitter/receiver don't get unnecessary power.
  pinMode(RCV_POWER_PIN, OUTPUT);
  digitalWrite(RCV_POWER_PIN, LOW);
  pinMode(TR_POWER_PIN, OUTPUT);
  digitalWrite(TR_POWER_PIN, LOW);

  // Setup program button behavior
  pinMode(PRG_PIN, INPUT);
  digitalWrite(PRG_PIN, LOW);
  programButton.assign(PRG_PIN);
  programButton.setMode(OneShotTimer);
  programButton.setTimer(1500);

  // Make sure we READ from the sensor
  pinMode(PIR_PIN, INPUT);
  digitalWrite(PIR_PIN, LOW);

  // Setup Status led
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  mySwitch.enableReceive(0);  // Receiver on inerrupt 0 => that is pin #2
  mySwitch.enableTransmit(TRANSMIT_PIN);
  mySwitch.setRepeatTransmit(10);
}


void loop() {
  if (!got_settings) {
    loadData();
    if (!checkSettings())
      statusBlink(1, 100);
  }

  checkButtons();

  if (got_settings)
    checkTrigger();
}

void saveData()
{
  digitalWrite(STATUS_LED, HIGH);
  settings.magic = 42;
  EEPROM.put(0, settings);
  digitalWrite(STATUS_LED, LOW);
  }

void loadData()
{
  EEPROM.get(0, settings);
  mySwitch.setProtocol(settings.protocol);
  mySwitch.setPulseLength(settings.pulse_length);
  }

void resetSettings() {
  settings.bell_bit_length = NULL;
  settings.bell_code = NULL;
  settings.pulse_length = NULL;
  settings.protocol = NULL;
  settings.magic = NULL;
}

bool checkSettings()
{
  if (settings.bell_bit_length == NULL)
    return false;

  if (settings.bell_code == NULL)
    return false;

  if (settings.pulse_length == NULL)
    return false;

  if (settings.protocol == NULL)
    return false;

  if (settings.magic != 42)
    return false;

  got_settings = true;
  return got_settings;
  }

bool copyKeys()
{
  bool result = false;
  long counter = millis();

  digitalWrite(RCV_POWER_PIN, HIGH);

  statusBlink(2, 200); // To indicate that we're ready for programming

  while ((millis() - counter) <= 5000) {
    if (mySwitch.available()) {
      resetSettings();

      // Set protocol
      settings.protocol = mySwitch.getReceivedProtocol();

      // Bell code
      settings.bell_code = mySwitch.getReceivedValue();

      // Bell bit length
      settings.bell_bit_length = mySwitch.getReceivedBitlength();

      // Pulse length.
      settings.pulse_length = mySwitch.getReceivedDelay();

      saveData();

      mySwitch.resetAvailable();

      result = true;
      break;
    }
  }

  digitalWrite(RCV_POWER_PIN, LOW);

  return result;
  }

void checkButtons(){
 switch (programButton.check()) {
   case Hold:

     if (!copyKeys())
       statusBlink(5, 50);

     else {
       // Test our newly stored settings and ring the bell
       loadData();
       delay(1000);
       ringDoorbell();
     }
     break;

   default:
     break;
  }
}

void checkTrigger()
{
  //static long last_trigger = millis();
  //static long time_buffer = 0;
  static bool reset_timer = false;
  static bool new_trigger = false;

  // Check if we've passed the trigger threshold and should go to sleep
  if ((millis() - last_trigger) >= TRIGGER_THRESHOLD)
  {
    //Serial.print("Good night!");
    sleep();
    detachInterrupt(interruptPin);
    statusBlink(5, 30);
    loadData();
    time_buffer = 0;
    reset_timer = true;
    
    // Reset snooze settings
    snoozing = false;
    number_of_calls = 0;
    }

  // Check if new trigger is outside of given time frame (TRIGGER_THRESHOLD) thus a new incindent
  if (digitalRead(PIR_PIN)) {
    if (!new_trigger)
      new_trigger = true;

    // Check if we have activity over given time frame assuming a cat wants in
    if (time_buffer >= TRIGGER_THRESHOLD){
      // Reset timer
      time_buffer = 0;

      // If forever tolerant never snooze
      if (ANNOYANCE_LEVEL == -1){
        ringDoorbell();

      // If not snoozing we ring the doorbell
      } else if (number_of_calls < ANNOYANCE_LEVEL && !snoozing){
        ringDoorbell();

        // Count consecutive calls
        number_of_calls += 1;

      // First time we reach snooze threshold
      } else if (number_of_calls == ANNOYANCE_LEVEL - 1){
        number_of_calls = 0;
        snooze_counter = millis();
        snoozing = true;
        //statusBlink(3, 500);

      // Enough snoozing. Get active again
      } else if (millis() - snooze_counter >= SNOOZE_TIME){
          snoozing = false;
      }

    // Increment time_buffer if reset_timer is false and we're sure we have a new trigger
    } else {
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

void ringDoorbell()
{
  // Give power to transmitter/receiver
  digitalWrite(TR_POWER_PIN, HIGH);

  // Status LED is mostly for debugging to indicate transmission of doorbell
  digitalWrite(STATUS_LED, HIGH);

  // Debug
  /*
  Serial.println("DING DONG!");
  Serial.print("bell code ");
  Serial.println(settings.bell_code);
  Serial.print("bit length ");
  Serial.println(settings.bell_bit_length);
  Serial.print("pulse length ");
  Serial.println(settings.pulse_length);
  Serial.print("protocol ");
  Serial.println(settings.protocol);
  */
  // Ring door bell with code caught by copyKeys()
  mySwitch.send(settings.bell_code, settings.bell_bit_length);
  digitalWrite(STATUS_LED, LOW);

  // Turn of power to transmitter/receiver
  digitalWrite(TR_POWER_PIN, LOW);

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
