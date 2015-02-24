
//#include <Process.h>

int hr = 80;
String url = "http://findingstillness.herokuapp.com/send_heartrate?hr=";

int pulse_pin = 2;
int contact_pin = 3;

void setup() {
  //Bridge.begin();
  Serial.begin(9600);
  pinMode(pulse_pin, INPUT);
  pinMode(contact_pin, INPUT);
  
  // Wait until a Serial Monitor is connected.
  // while (!Serial);
}

void loop() {
  
//  hr += random(-5, 1);
//  
//  Process p;
//  p.begin("curl");
//  p.addParameter(url+String(hr));
//  p.addParameter("-i");
//  p.run();
//
//  while (p.available()>0) {
//    char c = p.read();
//    Serial.print(c);
//  }

  int pulse = digitalRead(pulse_pin);
  int contact = digitalRead(contact_pin);
  Serial.print(pulse);
  Serial.print(" ");
  Serial.println(contact);
  delay(100);

}

