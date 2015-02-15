
#include <Process.h>

int hr = 80;
String url = "http://findingstillness.herokuapp.com/send_heartrate?hr=";

void setup() {
  Bridge.begin();
  Serial.begin(9600);

  // Wait until a Serial Monitor is connected.
  // while (!Serial);
}

void loop() {
  
  hr += random(-5, 1);
  
  Process p;
  p.begin("curl");
  p.addParameter(url+String(hr));
  p.addParameter("-i");
  p.run();

  while (p.available()>0) {
    char c = p.read();
    Serial.print(c);
  }

  delay(1000);

}

