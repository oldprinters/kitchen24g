#include<Arduino.h>
#include <ld2410.h>
#include "Timer.h"
#include<vector>
#include <WiFi.h> // library to connect to Wi-Fi network
#include "NTPClient.h"
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <BH1750.h> 

/*
Датчик присутствия, MQTT, датчик освещённости, OneLed на 2 лампы
датчик температуры и влажности SHT3X

*/

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 10800, 3600000);
//---------------------------------------------
const char* ssid = "ivanych";
const char* password = "stroykomitet";
WiFiClient espClient;
PubSubClient client(espClient);
//-----------------------------------------------------
const char* mqtt_server = "192.168.1.34";
const char* msgMotion = "kitchen/motion";
const char* msgPresence = "kitchen/presence";
const char* msgLight = "kitchen/light";
const char* msgHSMotion = "hall_small/motion";
//---------------------------------------------
BH1750 lightMeter(0x23);  //0x5c 23
float lux{8000};  //яркость света в помещении
//-----------------------------------------------------

ld2410 radar;
bool engineeringMode = false;
String command;
// const int PinInLd{18};
const int PinOutLd{18};
const int PinOutPres{19};
const int PinOutDis{21};
Timer t1(750);
bool ledStat{true};
bool presenceLd{false};
int16_t zone{0};
int16_t z{0};
uint16_t dt{500};

//--------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
  String str = {};
  String strTopic = topic;
  String sOut{};

  for (int i = 0; i < length; i++) {
    str += (char)payload[i];
  }

  if(strTopic == msgHSMotion){
    // light_1.extLightOn();
  } 
  // else if(strTopic == msgLightOff){
  // } else if(strTopic == msgLightOn){
  // } else if(strTopic == lightNightOn){
  // } else if(strTopic == lightNightOff){
  // }
}
//-----------------------------------
void reconnect_mqtt() {
  // Loop until we're reconnected
  if(!client.connected()) {
    while (!client.connected()) {
      Serial.println("Attempting MQTT connection...");
      String clientId = "Kitchen2410";
      // Attempt to connect
      Serial.println(clientId);
      if (client.connect(clientId.c_str())) {
        Serial.println("connected");
        // client.subscribe(msgHSMotion, 0);
        // client.subscribe(msgLightOff, 0);
        // client.subscribe(msgLightOn, 0);
      } else {
        Serial.print("failed, rc= ");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
  } else {
    client.loop();  //mqtt
  }
}
//*********************************************************
void setup(void)
{
  pinMode(PinOutPres, OUTPUT);
  pinMode(PinOutDis, OUTPUT);
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

    WiFi.begin(ssid, password); // initialise Wi-Fi and wait
    while (WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(500);
    }
    timeClient.begin();
    timeClient.update();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    reconnect_mqtt();
    //---------------------------
    // if (lightMeter.begin()) {//BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    //   Serial.println(F("BH1750 Advanced begin"));
    // }
    // else {
    //   Serial.println(F("Error initialising BH1750"));
    // }

}
//********************************************************
void loop()
{
  reconnect_mqtt();
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

              z = dist / 75;
              if(radar.stationaryTargetEnergy())
              if(z != zone){
                zone = z;
                  client.publish(msgMotion, String(z).c_str());
                if(z < 4){
                  // digitalWrite(PinOutDis, LOW);
                  presenceLd = true;
                  if(z < 3){
                    digitalWrite(PinOutLd, HIGH);
                  }else {
                    digitalWrite(PinOutLd, LOW);
                  }
                  if(z > 2){
                    digitalWrite(PinOutPres, HIGH);
                  }else {
                    digitalWrite(PinOutPres, LOW);
                  }
                } else {
                  presenceLd = false;
                  // digitalWrite(PinOutDis, HIGH);
                  digitalWrite(PinOutPres, LOW);
                  digitalWrite(PinOutLd, LOW);
                }
              } 
            }
            if(radar.movingTargetDetected()){
              uint16_t distMove = radar.movingTargetDistance();
              digitalWrite(PinOutDis, HIGH);
            //   Serial.print(F(". Moving target: "));
            //   Serial.print(radar.movingTargetDistance());
            //   // Serial.print(F("cm energy: "));
            //   // Serial.println(radar.movingTargetEnergy());
            } else digitalWrite(PinOutDis, LOW);
            // Serial.println();
          } else {
            if(z != zone){
              client.publish(msgMotion,  String(z).c_str());
              zone = -1;
            }
            z = -1;
          }
        }

  // if(t1.getTimer()){
  //   t1.setTimer(dt);
  //   ledStat = !ledStat;
  //   if(presenceLd)
  //     digitalWrite(PinOutLd, ledStat);
  // }
}