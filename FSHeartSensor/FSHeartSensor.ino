
//#include <Process.h>

int hr = 80;
String url = "http://findingstillness.herokuapp.com/send_heartrate?hr=";

int contact_pin = 2;
int pulse_pin = 3;

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

  int contact = digitalRead(contact_pin);
  int pulse = digitalRead(pulse_pin);
  Serial.print(contact);
  Serial.print(" ");
  Serial.println(pulse);
  delay(100);

}

