#include <avr/wdt.h>
#include <Process.h>

#define mains 60 // 60: north america, japan; 50: most other places
#define refresh 1000000 / mains // power cycle wavelength in microseconds
int analogPin = 0;

float leftThreshold = 610, rightThreshold = 660;
int leftPin = 2, rightPin = 1;
float contactSlow = 0;
unsigned long lastContact = 0;
unsigned long contactDelay = 1000;

float heartRateBpmMin = 50;
float heartRateBpmMax = 140;
float heartRateBpm = 60;

String hr_url = "http://10.0.1.2:3000/send_heartrate?hr=";
String contact_url = "http://10.0.1.2:3000/start";

unsigned long ping_interval = 1000;
unsigned long last_ping = 0;
unsigned long cur_time = 0;

Process p;

void setup() {
  Bridge.begin();
  Serial.begin(115200);
  wdt_enable(WDTO_8S); // restart if we don't call wdt_reset() every 8 seconds
  pinMode(analogPin, INPUT);
  pinMode(leftPin, INPUT);
  pinMode(rightPin, INPUT);
  resetTimer();
}

unsigned long lastTime = 0, nextTime = 0;
void resetTimer() {
  lastTime = micros();
  nextTime = lastTime + refresh;
  // this will have a brief phase glitch on the ~1 hour 11 minutes mark
}

void loop() {
  wdt_reset();
  
  unsigned long curTime = millis();
  if(curTime - lastContact > contactDelay) {
    int left = analogRead(leftPin);
    int right = analogRead(rightPin);
    float contact = (left > leftThreshold) && (right > rightThreshold);
    contactSlow = .5*contactSlow + .5*(contact ? 1 : 0);
    if(contactSlow > .999) {
      sendContact();
      lastContact = curTime;
    }
  }
  
  // this is the sensing loop
  unsigned long count = 0, total = 0;
  while (micros() < nextTime) {
    count += analogRead(analogPin);
    total++;
  }
  resetTimer(); // this must be immediately after the loop
  
  if (total > 0 && count > 0) {
    float cur = count / total;
    unsigned long curHeartbeatMs = millis();
    heartbeatFilter(cur, curHeartbeatMs, heartRateBpm);
  }
  sendHR();
}

void sendContact() {
  Serial.println("send contact");
  Process p;
  p.begin("curl");
  p.addParameter(contact_url);
  p.addParameter("-i");
  p.run();
}

void sendHR() {
  cur_time = millis();
  if (cur_time - last_ping > ping_interval) {
    heartRateBpm = random(10, 100);
    last_ping = cur_time;
    Serial.println("send hr");
    Serial.println(heartRateBpm);
    p.begin("curl");
    p.addParameter(hr_url + heartRateBpm);
    p.runAsynchronously();
  }
}
