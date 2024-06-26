#include <SoftwareSerial.h>
#include <TinyGPS.h>

TinyGPS gps;
SoftwareSerial ss(11, 10);

void setup() {
  Serial.begin(115200);
  ss.begin(9600);
  Serial.print("Simple TinyGPS library v. ");
  Serial.print(TinyGPS::library_version());
  Serial.println();
}

void loop() {
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  for(unsigned long start = millis(); millis() - start < 1000;)
  {
    while(ss.available())
    {
      char c = ss.read();
      if(gps.encode(c))
        newData = true;
    }
  }

  if(newData)
  {
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    Serial.print("LAT = ");
    Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    Serial.print(" LON = ");
    Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    Serial.print(" SAT = ");
    Serial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
    Serial.print(" PREC = ");
    Serial.print(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());    
  }

  gps.stats(&chars, &sentences, &failed);
  Serial.print(" CHARS = ");
  Serial.print(chars);
  Serial.print(" SENTENCES = ");
  Serial.print(sentences);  
  Serial.print(" CSUM ERR = ");
  Serial.print(failed);

  if(chars == 0)
    Serial.println("** No characters received from GPS : check wiring **");
}
