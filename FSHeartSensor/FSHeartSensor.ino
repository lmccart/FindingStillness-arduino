
#include <Process.h>
#include "RunningMedian.h"

String hr_url = "http://192.168.2.10:3000/send_heartrate?hr=";
String contact_url = "http://192.168.2.10:3000/start";

unsigned long ping_interval = 3000;
unsigned long last_ping = 0;
unsigned long cur_time = 0;

int contact_pin = A1;
int pulse_pin = A0;

int pulse_read, contact_read;

long previousMillis = 0; //variable for timing music
long contStartTime = 0;
int barCount = 1;
int leadno = 0;
int play_count = 1; // Incremented every play for statistics only

// Setup for the heart beat timings
RunningMedian beat_samples = RunningMedian(100);
RunningMedian contact_samples = RunningMedian(5000);
int average = 0;
int median = 0;

unsigned long pulse_time = 0;
unsigned long last_pulse_time = 0;

unsigned long last_contact_time = 0;
unsigned long contact_time = 0;
unsigned long contact_interval = 150;

int time_diff = 0;
int prev_pulse = 0, prev_contact = 0;
int contact;
float bpm = 60, target_bpm = 60;
float easing = 0.01;

void setup() {

  Bridge.begin();
  Serial.begin(9600);//setup serial to send debug data over Bluetooth

  pinMode(contact_pin, INPUT);
  pinMode(pulse_pin, INPUT);

  Serial.println("Powered up...");
}

void loop() {

  // Check send hr
  cur_time = millis();
  if (cur_time - last_ping > ping_interval) {
    //sendHR();
    last_ping = cur_time;
  }
  
  // Check for heartbeat
  bpm = (target_bpm - bpm) * easing + bpm;
  pulse_read = digitalRead(pulse_pin);
//  Serial.print(pulse_read);
//  Serial.print(" ");
//  Serial.println(contact_read);
//  
  if (pulse_read != prev_pulse) handle_pulse();

  // Check for contact
  contact_time = millis();
  if (contact_time - last_contact_time > contact_interval) {
    last_contact_time = contact_time;
    contact_read = digitalRead(contact_pin);
    contact_samples.add(contact_read);
    contact = contact_samples.getAverage() > 0.5 ? 1 : 0;
    if (contact != prev_contact) handle_contact();
  }


}

void handle_contact() {
  Serial.print("contact ");
  Serial.println(contact);
  if (contact == 1) {
    //sendContact();
  } else {
    contact_samples.clear();
  }
  prev_contact = contact;
}

void handle_pulse() {

  Serial.print("^");

  prev_pulse = pulse_read;

  if (pulse_read == 1) {
    last_pulse_time = pulse_time;
    pulse_time = millis();
    time_diff = pulse_time - last_pulse_time;
    if (time_diff < 2000 && time_diff > 350) {
      beat_samples.add(time_diff);
    }

    // Get the median and average for comparison
    median = 60000 / beat_samples.getMedian();
    average = 60000 / beat_samples.getAverage();

    //  // Check if median and average are close, this indicates reasonably consistant heartbeat periods
    //  if ((average > 50) && (average < 130) && (average < 4 + median) && (average + 4 > median)) {
    //    good_bpm = 1;
    //    bpm = median;
    //  } else {

    Serial.print("contact ");
    Serial.print(contact);
    Serial.print(" pulse ");
    Serial.print(prev_pulse);

    Serial.print(" avg ");
    Serial.print(average);
    Serial.print(" med ");
    Serial.print(median);
    Serial.print(" bpm ");
    Serial.println(bpm);

    target_bpm = average;
  }
}


void sendContact() {
  Serial.println("send contact");
  Process p;
  p.begin("curl");
  p.addParameter(contact_url);
  p.addParameter("-i");
  p.run();
  
//  String s = "";
//  while (p.available() > 0) {
//    char c = p.read();
//    s += c;
//  }
//  Serial.println(s);
}

void sendHR() {
  Serial.println("send hr");
  Process p;
  p.begin("curl");
  p.addParameter(hr_url + bpm);
  p.addParameter("-i");
  p.run();

//  String s = "";
//  while (p.available() > 0) {
//    char c = p.read();
//    s += c;
//  }
//  Serial.println(s);

}
