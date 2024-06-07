#include<Arduino.h>
#include <ld2410.h>
#include "Timer.h"
#include<vector>

ld2410 radar;
bool engineeringMode = false;
String command;
const int PinInLd{18};
const int PinOutLd{19};
Timer t1(750);
bool ledStat{true};
bool presenceLd{false};
uint16_t zone{0};
uint16_t dt{500};
std::vector <uint16_t> frZone;

//*********************************************************
void setVector(){
  for(int n = 0; n < 10; ++n){
    frZone.push_back((n + 1) * 200);
  }
}
//*********************************************************
void setup(void)
{
  pinMode(PinInLd, INPUT);
  pinMode(PinOutLd, OUTPUT);
  Serial.begin(115200); //Feedback over Serial Monitor
  Serial.println(F("============================="));
  delay(500); //Give a while for Serial Monitor to wake up
  radar.debug(Serial); //Uncomment to show debug information from the library on the Serial Monitor. By default this does not show sensor reads as they are very frequent.
  Serial2.begin (256000, SERIAL_8N1, 16, 17); //UART for monitoring the radar
  delay(500);
  Serial.print(F("\nLD2410 radar sensor initialising: "));
  if(radar.begin(Serial2))
  {
    Serial.println(F("OK"));
  }
  else
  {
    Serial.println(F("not connected"));
  }
  Serial.println(F("Supported commands\nread: read current values from the sensor\nreadconfig: read the configuration from the sensor\nsetmaxvalues <motion gate> <stationary gate> <inactivitytimer>\nsetsensitivity <gate> <motionsensitivity> <stationarysensitivity>\nenableengineeringmode: enable engineering mode\ndisableengineeringmode: disable engineering mode\nrestart: restart the sensor\nreadversion: read firmware version\nfactoryreset: factory reset the sensor\n"));
  setVector();
  delay(5000);
}
//********************************************************
void loop()
{
  radar.read(); //Always read frames from the sensor
        if(radar.isConnected())
        {
          if(radar.presenceDetected()){
            if(radar.stationaryTargetDetected()){
              // Serial.print(F("Stationary target: "));
              uint16_t dist = radar.stationaryTargetDistance();
              // Serial.print(dist);
              // Serial.print(F("cm energy: "));
              // Serial.println(radar.stationaryTargetEnergy());
              uint16_t z = dist / 75;
              if(z != zone){
                dt = frZone[z];
                zone = z;
                if(z < 6)
                  presenceLd = true;
                else {
                  presenceLd = false;
                  digitalWrite(PinOutLd, LOW);
                }
                Serial.println(frZone[z]);
              }
            }
            // if(radar.movingTargetDetected()){
            //   Serial.print(F(". Moving target: "));
            //   Serial.print(radar.movingTargetDistance());
            //   // Serial.print(F("cm energy: "));
            //   // Serial.println(radar.movingTargetEnergy());
            // }
            // Serial.println();
          }// else {
            // Serial.println(F("nothing detected"));
          // }
        }

  if(t1.getTimer()){
    t1.setTimer(dt);
    ledStat = !ledStat;
    if(presenceLd)
      digitalWrite(PinOutLd, ledStat);
  }
}