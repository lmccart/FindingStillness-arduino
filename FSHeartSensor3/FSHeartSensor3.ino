#include <Process.h>

#define mains 60 // 60: north america, japan; 50: most other places
#define refresh 1000000 / mains // power cycle wavelength in microseconds
int analogPin = 0;
float heartRateBpmMin = 50;
float heartRateBpmMax = 140;
float heartRateBpm = 0;

String hr_url = "http://stillness.local:3000/send_heartrate?hr=";
String contact_url = "http://stillness.local:3000/start";

unsigned long ping_interval = 3000;
unsigned long last_ping = 0;
unsigned long cur_time = 0;

void setup() {
  Bridge.begin();
  Serial.begin(9600);
  pinMode(analogPin, INPUT);
  resetTimer();
}

unsigned long lastTime = 0, nextTime = 0;
unsigned long previousHeartbeatMs = 0;

void resetTimer() {
  lastTime = micros();
  nextTime = lastTime + refresh;
  // this will have a brief phase glitch on the ~1 hour 11 minutes mark
}

void loop() {
  
  // Check send hr
  cur_time = millis();
  if (cur_time - last_ping > ping_interval) {
    sendHR();
    last_ping = cur_time;
  }
//  
//  
//  unsigned long count = 0, total = 0;
//  while (micros() < nextTime) {
//    count += analogRead(analogPin);
//    total++;
//  }
//  resetTimer(); // this must be immediately after the loop
//  if (count > 0 && total > 0) {
//    float cur = count / total;
//    unsigned long curHeartbeatMs = millis();
//    if (heartbeatFilter(cur, curHeartbeatMs)) {
//      unsigned long diffHeartbeatMs = curHeartbeatMs - previousHeartbeatMs;
//      heartRateBpm = (60. * 1000.) / diffHeartbeatMs;
//      heartRateBpm = constrain(heartRateBpm, heartRateBpmMin, heartRateBpmMax);
//      Serial.println(heartRateBpm);
//      previousHeartbeatMs = curHeartbeatMs;
//    }
//  }
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
  p.addParameter(hr_url + heartRateBpm);
  p.addParameter("-i");
  p.run();

  String s = "";
  while (p.available() > 0) {
    char c = p.read();
    s += c;
  }
  Serial.println(s);

}
