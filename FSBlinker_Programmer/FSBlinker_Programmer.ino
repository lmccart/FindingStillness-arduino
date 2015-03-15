
#include <Process.h>

#include "IRremote.h"
#include "crc.h"

const uint8_t DEFAULT_SENSITIVITY = 15; // 15-120
const uint8_t DEFAULT_COUNTS = 5; // 1-5 (repeat 7-beat pattern)
int heart_rate = 50; // rate 40-150

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


void loop() {
  sendHeartbeatParameters(heart_rate, DEFAULT_COUNTS, DEFAULT_SENSITIVITY);
  delay(150);
}
