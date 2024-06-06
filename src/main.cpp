#include <ld2410.h>

ld2410 radar;
bool engineeringMode = false;
String command;

void setup(void)
{
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
}

void loop()
{
  radar.read(); //Always read frames from the sensor
        if(radar.isConnected())
        {
          if(radar.presenceDetected()){
            if(radar.stationaryTargetDetected()){
              Serial.print(F("Stationary target: "));
              Serial.println(radar.stationaryTargetDistance());
              // Serial.print(F("cm energy: "));
              // Serial.println(radar.stationaryTargetEnergy());
            }
            if(radar.movingTargetDetected()){
              Serial.print(F("Moving target: "));
              Serial.println(radar.movingTargetDistance());
              // Serial.print(F("cm energy: "));
              // Serial.println(radar.movingTargetEnergy());
            }
          } else {
            Serial.println(F("nothing detected"));
          }
        }
}