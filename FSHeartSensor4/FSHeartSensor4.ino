#include <Process.h>

#define mains 60 // 60: north america, japan; 50: most other places
#define refresh 1000000 / mains // power cycle wavelength in microseconds
int analogPin = 0;
int pressurePin0 = 1;
int pressurePin1 = 2;

float heartRateBpmMin = 50;
float heartRateBpmMax = 140;
float heartRateBpm = 60;

struct ContactFilter {
  float lowPass = 0;
  float contactThreshold = 100;
  unsigned long contactDelay = 1000;
  unsigned long lastContact = 0;
  bool run(float x) {
    unsigned long curTime = millis();
    lowPass = lerp(lowPass, x, .1);
    float highPass = x - lowPass;
    bool contactStatus = highPass > contactThreshold;
    if(contactStatus) {
      if(curTime - lastContact > contactDelay) {
        lastContact = curTime;
        return true;
      }
    }
    return false;
  }
} contactFilter;

String hr_url = "http://10.0.1.2:3000/send_heartrate?hr=";
String contact_url = "http://10.0.1.2:3000/start";

unsigned long ping_interval = 1000;
unsigned long last_ping = 0;
unsigned long cur_time = 0;

void setup() {
  Bridge.begin();
  Serial.begin(115200);
  pinMode(analogPin, INPUT);
  pinMode(pressurePin0, INPUT);
  pinMode(pressurePin1, INPUT);
  resetTimer();
}

unsigned long lastTime = 0, nextTime = 0;
void resetTimer() {
  lastTime = micros();
  nextTime = lastTime + refresh;
  // this will have a brief phase glitch on the ~1 hour 11 minutes mark
}

void loop() {  
  int contact = analogRead(pressurePin0) + analogRead(pressurePin1);
  if(contactFilter.run(contact)) {
    sendContact();
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
  p.runAsynchronously();
}

void sendHR() {
  cur_time = millis();  
  if (cur_time - last_ping > ping_interval) {
    last_ping = cur_time;
    Serial.println("send hr");
    Serial.println(heartRateBpm);
    Process p;
    p.begin("curl");
    p.addParameter(hr_url + heartRateBpm);
    p.addParameter("-i");
    p.runAsynchronously();
  }
}
