
#include <Process.h>

#include "IRremote.h"
#include "crc.h"

const uint8_t DEFAULT_SENSITIVITY = 120; // 15-120
const uint8_t DEFAULT_COUNTS = 2; // 1-5 (repeat 7-beat pattern)
int heart_rate = 100; // rate 40-150
int flash_time = 5000;
float flash_start = 0;
bool flashing = false;

int mode = 0; // 0 - off, 1 - pinging, 2 - flashing

String url = "http://findingstillness.herokuapp.com/get_update";

IRsend irsend;

void setup() {
  Bridge.begin();
  Serial.begin(9600);
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

void checkHR() {

  Process p;
  p.begin("curl");
  p.addParameter(url);
  p.addParameter("-i");
  p.run();

  String s = "";
  while (p.available() > 0) {
    char c = p.read();
    s += c;
  }
  //Serial.println(s);
  int hr_start = s.indexOf("hr") + 4;
  if (hr_start != 3) {
    int hr_stop = s.indexOf(",\"remaining");
    int hr = s.substring(hr_start, hr_stop).toInt();
    heart_rate = min(max(40, hr), 120);
    //Serial.println(heart_rate);

    int rem_start = s.indexOf("remaining") + 11;
    int rem_stop = s.indexOf("}");
    int rem = s.substring(rem_start, rem_stop).toInt();
    //Serial.println(rem);

    if (rem == 0) {
      flashing = true;
      flash_start = millis();
      Serial.println("flash on");
    }
  }

}

void loop() {
  if (flashing && millis() - flash_start >= flash_time) {
    flashing = false;
    Serial.println("flash off");
    delay(10000);
  }
  if (!flashing) {
    Serial.println("pinging");
    checkHR();
  }
  sendHeartbeatParameters(heart_rate, DEFAULT_COUNTS, DEFAULT_SENSITIVITY);
  delay(150);
}
