// This Arduino sketch has been created for the Bristol UK 2013 BIG Green Week:
// http://biggreenweek.com/
//
// The software is based on the San Francisco Urban Prototying Pulse of the City project
// created by George Zisiadis and Matt Ligon as documented here:
// http://www.instructables.com/id/Pulse-of-the-City/
//
// The Bristol Pulse of the City heart project has been built by Alan Senior and is
// documented here:
// http://www.instructables.com/id/Pulse-of-the-City-Bristol/
//
// The software is a bit greedy on RAM due to the large note arrays so we need an
// Arduino Mega 1280 or Mega 2560 to run it. We could use EEPROM to save RAM...
//
// This is version 8.0 of the software and uses bytes instead of ints for notes
// it still needs a Mega 1280 or 2560 to run...
//
// Date: May 14th 2013
//

// Call up the required libraries...
#include "RunningMedian.h"
#include <Wire.h>

int hr_pin = 2;
int hr_pull_pin = 3;

long previousMillis = 0; //variable for timing music
long contStartTime = 0;
int barCount = 1;
int leadno = 0;
int play_count = 1; // Incremented every play for statistics only

// Setup for the heart beat timings
RunningMedian beat_samples = RunningMedian(4);
int average = 0;
int median = 0;
volatile boolean pulse = 0; // Declared volatile as they are used in the interrupt service routine
volatile unsigned long pulse_time = 0;
boolean prevpulse = 0;
boolean good_bpm = 0;
int bpm = 60;
int beat_period = 1000;
int pitch = 0;
#define LED 13

int interval = 120;
int lockedBPM = 0;
int patternNo = 0;
int instr;//variable for instrument


// The main loop has different operating states as defined here:
byte heart_mode = 0;
byte last_heart_mode = 99;
#define WAITING 0
#define SENSING 1
#define PLAY 3
#define FADING  4

// Is the detected heartbeat signal good or bad?
#define BEAT_GOOD 0
#define BEAT_BAD  1

// Define stuff for the Olimex MOD-RGB board interface that drives the LEDs
#define ADDRESS 0xA0 // I2C address of MOD-RGB board
#define LED_PWM_ON 0x01
#define LED_PWM_OFF 0x02
#define LED_RGB 0x03
#define LED_AUDIO_ON 0x14
#define LED_AUDIO_OFF 0x15

// Setup variables for the LED states
byte lightState = 0;
byte red = 0;
byte green = 0;
byte blue = 0;
byte glow = 40;

// Various time values used to sequence activities
unsigned long currentMillisFade = 0;
unsigned long currentMillis = 0;
unsigned long loopMillis = 0;
unsigned long ledMillis = 0;
unsigned long periodMillis = 0;
unsigned long detectMillis = 0;


void setup() {

  Serial.begin(57600);//setup serial to send debug data over Bluetooth
  Wire.begin();	// Setup I2C interface for Olimex MOD-RGB LED drive board

  // Pin 22 is used to detect human contact
  pinMode(hr_pin, INPUT_PULLUP); // add a pullup so that there is a contact for testing
  pinMode(hr_pull_pin, OUTPUT); // temporary control for pin 22
  digitalWrite(hr_pull_pin, LOW); // set it low for no contact


  // Randomise the seed for random numbers...
  randomSeed(analogRead(0));

  // Send out a boot message and finally setup the heartbeat interrupt handler
  Serial.println("Powered up...");
  attachInterrupt(0, interrupt, RISING);//set interrupt 0,digital port 2
}

void loop() {

  // If a heartbeat interrupt has been detected...
  if (pulse != prevpulse) handle_pulse();

  // Report each operating mode transition for debug purposes
  if (heart_mode != last_heart_mode) {
    last_heart_mode = heart_mode;
    if (heart_mode == WAITING) Serial.println("Waiting...");
    if (heart_mode == SENSING) Serial.println("Sensing...");
    if (heart_mode == PLAY) Serial.println("Playing...");
    if (heart_mode == FADING) Serial.println("Fading...");
    //if (heart_mode ==xxxxxx) Serial.println("xxxxxx...");
  }

  // Debug info for loop speed check
  //Serial.println(millis()-loopMillis);
  loopMillis = millis();
  if (loopMillis > detectMillis) digitalWrite(hr_pull_pin, LOW); // Heartbeat lost


  // Here we detect what operating mode we are in and do the appropriate thing
  switch (heart_mode) {
    case WAITING:
      good_bpm = 0; // No heart beat
      glow = 0; // LEDs off to save power

      // Do we have contact, if so switch to sensing mode
      if (digitalRead(hr_pin) == 1) {
        heart_mode = SENSING;
        Serial.println("sensing");
        //heart_mode = COMPOSE; // Skip to compose for debug only
        // Choose a random heart rate in case we don't get a reliable ECG signal
        // this is just so the person does not walk away dissapointed!
        bpm = random(65, 80);
        beat_period = 60000 / bpm;
        // Now set the LEDs to a dim red glow to show we have contact
        glow = 40;
      }
      // Store the contact time so we can set a timer to control how long
      // we wait for a good heartbeat
      contStartTime = loopMillis; //get time at start of contact
      break;
    case SENSING:
      // Wait here for a while for a steady heart beat or a time-out of about 10s
      if ((millis() > (contStartTime + 12 * beat_period)) || (good_bpm == 1)) {
      }
      // If contact is lost go back to waiting
      if (digitalRead(hr_pin) == 0) heart_mode = WAITING;
      break;
    case PLAY:
      // Go to Fading mode immediately if contact is lost
      if (digitalRead(hr_pin) == 0) heart_mode = FADING;
      break;
  } // End of switch
}

// Interrupt handler for the heartbeat
void interrupt()
{
  // Toggle the pulse flag to show we have a beat signal
  pulse = !pulse;
  // Maintain a running median of the beat period
  beat_samples.add(millis() - pulse_time);
  // Record the pulse time ready for the next period calculation
  pulse_time = millis();
}

void handle_pulse()
{
  prevpulse = pulse;
  // Change the state of the onboard LED as a pulse indicator when the RGB LEDs are off
  digitalWrite(LED, pulse);
  Serial.print("^"); // Bluetooth output to show a pulse has been detected

  // Signal contact, don't stop for a period after contact is lost to allow
  // to allow for temporary loss of ECG signal while people jump about!
  digitalWrite(hr_pull_pin, HIGH);
  detectMillis = pulse_time + 4000;

  // During sensing mode make a heartbeat sound in time with the pulse
  if ( heart_mode == SENSING) {
    // Signal a beat
    Serial.println("beat");
  }

  // Get the median and average for comparison
  median = 60000 / beat_samples.getMedian();
  average = 60000 / beat_samples.getAverage();
  good_bpm = 0;

  // Check if median and average are close, this indicates reasonably consistant heartbeat periods
  if ((average > 50) && (average < 130) && (average < 4 + median) && (average + 4 > median)) {
    good_bpm = 1;
    bpm = median;
    // Print the rate for debug purposes
    Serial.print(bpm);
    Serial.println("bpm");
  }
}

