
#include <Process.h>

#include "IRremote2.h"
#include "crc.h"

const uint8_t DEFAULT_SENSITIVITY = 100; // 15-120
const uint8_t DEFAULT_COUNTS = 3; // 1-5 (repeat 7-beat pattern)
// rate 40-150
String url = "http://findingstillness.herokuapp.com/get_update";
int heart_rate;

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
  while (p.available()>0) {
    char c = p.read();
    s += c;
  }
  Serial.println(s);
  int hr_start = s.indexOf("hr") + 4;
  int hr_stop = s.indexOf(",\"remaining");
  int hr = s.substring(hr_start, hr_stop).toInt();
  heart_rate = min(max(40, hr), 120);
  Serial.println(heart_rate);
  
  int rem_start = s.indexOf("remaining") + 11;
  int rem_stop = s.indexOf("}");
  int rem = s.substring(rem_start, rem_stop).toInt();
  Serial.println(s.substring(rem_start, rem_stop));
  Serial.println(rem);

}

void loop() {
  checkHR();
  sendHeartbeatParameters(heart_rate, DEFAULT_COUNTS, DEFAULT_SENSITIVITY);
  delay(150);
}
