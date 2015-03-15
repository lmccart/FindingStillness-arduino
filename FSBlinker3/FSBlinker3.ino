#include <avr/wdt.h>
#include <Process.h>

#include "IRremote.h"
#include "crc.h"

const uint8_t DEFAULT_SENSITIVITY = 20; // 15-120
int DEFAULT_COUNTS = 1; // 1-5 (repeat 7-beat pattern)
int heart_rate = 60; // rate 50-150
unsigned long flash_time = 3000;
unsigned long flash_start = 0;
bool flashing = false;
bool waitingForFlash = false;
unsigned long ping_interval = 3000;
unsigned long last_ping = 0;

int mode = 0; // 0 - off, 1 - pinging, 2 - flashing

String url = "http://10.0.1.2:3000/get_update?arduino=1";

IRsend irsend;

void setup() {
  Bridge.begin();
  Serial.begin(9600);
  wdt_enable(WDTO_8S); // restart if we don't call wdt_reset() every 8 seconds
  Serial.println("powered up");
}

void sendHeartbeatParameters(uint8_t rate, uint8_t counts, uint8_t sensitivity) {

  // Convert the heart rate from BPM to the system value
  rate = uint8_t(((60.0 / rate / 500)) * 100000);

  resetCRC();
  updateCRC(rate);
  updateCRC(counts);
  updateCRC(sensitivity);

  long data = 0;
  data |= ((long)rate)        << 24;
  data |= ((long)counts)      << 16;
  data |= ((long)sensitivity) << 8;
  data |= (getCRC());

  irsend.sendNEC(data, 32); // NEC code
}

void sendBlank() {
  resetCRC();
//  updateCRC(rate);
//  updateCRC(counts);
//  updateCRC(sensitivity);

  long data = 0;
//  data |= ((long)rate)        << 24;
//  data |= ((long)counts)      << 16;
//  data |= ((long)sensitivity) << 8;
  data |= (getCRC());

  irsend.sendNEC(data, 32); // NEC code
}

Process p;
  
void checkHR() {
  p.begin("curl");
  p.addParameter(url);
  p.addParameter("-i");
  p.run();

  String s = "";
  while (p.available() > 0) {
    char c = p.read();
    s += c;
  }
  // Serial.println(s);
  int hr_start = s.indexOf("hr") + 4;
  if (hr_start != 3) {
    int hr_stop = s.indexOf(",\"remaining");
    int hr = s.substring(hr_start, hr_stop).toInt();
    heart_rate = min(max(50, hr), 140);
    Serial.println(heart_rate);

    int rem_start = s.indexOf("remaining") + 11;
    int rem_stop = s.indexOf("}");
    int rem = s.substring(rem_start, rem_stop).toInt();
    Serial.println(rem);

    if (rem == 0) {
      if (waitingForFlash) {
        flashing = true;
        flash_start = millis();
        waitingForFlash = false;
        Serial.println("flash on");
      }
    } else {
      waitingForFlash = true;
    }
  }

}

void loop() {
  wdt_reset();
  
  // Serial.println(heart_rate);
  if (flashing) {
    int m = millis();
    if (m - flash_start >= flash_time) {
      Serial.println(m);
      flashing = false;
      waitingForFlash = false;
      Serial.println("flash off");
      delay(10 * 1000); // get out of room
    }
    sendHeartbeatParameters(heart_rate, DEFAULT_COUNTS, DEFAULT_SENSITIVITY);
  } else {
    unsigned long m = millis();
    if (m - last_ping > ping_interval) {
      last_ping = m;
      checkHR();
      Serial.println("pinging");
    }
    //sendBlank();
    sendHeartbeatParameters(heart_rate, DEFAULT_COUNTS, DEFAULT_SENSITIVITY);
  }
  delay(150);
}
