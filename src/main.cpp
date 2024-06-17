#include<Arduino.h>
#include <ld2410.h>
#include "Timer.h"
#include <WiFi.h> // library to connect to Wi-Fi network
#include "NTPClient.h"
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <Wire.h>
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
float luxOld{0};  //предыдущее значение
//-----------------------------------------------------
ld2410 radar;
int16_t dist{0};
int16_t distNew{0};

bool led1_On{true};
bool led2_On{true};
const int PinOutLd{18};
const int PinOutPres{19};
const int PinOutDis{5};
Timer t1(750);
Timer tLed1(3000);
Timer tLed2(3000);
bool ledStat{true};
uint16_t dt{250};
enum class Motion : uint8_t {
  None,
  Far,
  Near,
} motion;
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
  Serial.println(F("=========================== ptr =="));
  Wire.begin();
  Wire.setClock(400000); // use 400 kHz I2C

  delay(500); //Give a while for Serial Monitor to wake up
  // radar.debug(Serial); //Uncomment to show debug information from the library on the Serial Monitor. By default this does not show sensor reads as they are very frequent.
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
    if (lightMeter.begin()) {//BH1750::CONTINUOUS_HIGH_RES_MODE)) {
      Serial.println(F("BH1750 Advanced begin"));
    }
    else {
      Serial.println(F("Error initialising BH1750"));
      while(1);
    }
}
//--------------------------------------------------------
Motion radarFunc(){
	radar.read(); //Always read frames from the sensor
	if(radar.isConnected()){
		if(radar.presenceDetected()){
			if(radar.stationaryTargetDetected()){
				distNew = radar.stationaryTargetDistance();
				uint16_t energy = radar.stationaryTargetEnergy();
				client.publish(msgMotion, String(dist).c_str());
				if((energy > 30) && (distNew < 300)){
					dist = distNew;
					if(dist < 200){
						led1_On = true;
						tLed1.setTimer();
					}else {
						if(!led1_On){
							led2_On = true;
							tLed2.setTimer();
						}
					}
				} 
			}
			uint16_t distMove = radar.movingTargetDistance();
			uint16_t energyMove = radar.movingTargetEnergy();
			if((distMove > 0) && (distMove < 350) && energyMove > 30){
				tLed2.setTimer();
				led2_On = true;
			}
		}
	} else {
		Serial.println(F("not connected"));
		// client.publish(msgMotion,  String(z).c_str());
	}

  if(tLed1.getTimer() && led1_On)  {
    led1_On = false;
  }
  
  if(tLed2.getTimer()  && led2_On) {
    led2_On = false;
  }
  
  if(led1_On){
		motion = Motion::Near;
  } else if(led2_On){
		motion = Motion::Far;
  } else {
		motion = Motion::None;
  }
	return motion;
}
//----------------------------------------------------------
void controlLed(Motion motion) {
  switch (motion) {
    case Motion::None:
      digitalWrite(PinOutLd, LOW);
      digitalWrite(PinOutPres, LOW);
      break;
    case Motion::Near:
      digitalWrite(PinOutLd, HIGH);
      digitalWrite(PinOutPres, LOW);
      break;
    case Motion::Far:
      digitalWrite(PinOutLd, LOW);
      digitalWrite(PinOutPres, HIGH);
      break;
  }
}
//********************************************************
void loop()
{
  reconnect_mqtt();
  // radar.read(); //Always read frames from the sensor
  // if(radar.isConnected()){
  //   if(radar.presenceDetected()){
  //     if(radar.stationaryTargetDetected()){
  //       uint16_t dist = radar.stationaryTargetDistance();

  //       z = dist / 75;
  //       if((radar.stationaryTargetEnergy() > 50) && (z != zone)){
  //         Serial.print(F("Stationary target: "));
  //         Serial.print(dist);
  //         Serial.print(F(" cm, energy: "));
  //         Serial.println(radar.stationaryTargetEnergy());
  //         zone = z;
  //           client.publish(msgMotion, String(z).c_str());
  //         if(z < 4){
  //           // digitalWrite(PinOutDis, LOW);
  //           presenceLd = true;
  //           if(z <= 3){
  //             led1_On = true;
  //             tLed1.setTimer();
  //             digitalWrite(PinOutLd, HIGH);
  //           }else {
  //             digitalWrite(PinOutLd, LOW);
  //           }
  //           if(z > 2){
  //             led2_On = true;
  //             tLed2.setTimer();
  //             digitalWrite(PinOutPres, HIGH);
  //           }else {
  //             digitalWrite(PinOutPres, LOW);
  //           }
  //         } else {
  //           presenceLd = false;
  //           // digitalWrite(PinOutDis, HIGH);
  //           digitalWrite(PinOutPres, LOW);
  //           digitalWrite(PinOutLd, LOW);
  //         }
  //       } 
  //     }
  //     if(radar.movingTargetDetected()){
  //       uint16_t distMove = radar.movingTargetDistance();
  //       digitalWrite(PinOutDis, HIGH);
  //       tLed2.setTimer();
  //       led2_On = true;
  //     //   Serial.print(F(". Moving target: "));
  //     //   Serial.print(radar.movingTargetDistance());
  //     //   // Serial.print(F("cm energy: "));
  //     //   // Serial.println(radar.movingTargetEnergy());
  //     } else digitalWrite(PinOutDis, LOW);
  //     // Serial.println();
  //   } else {
  //     if(z != zone){
  //       client.publish(msgMotion,  String(z).c_str());
  //       zone = -1;
  //     }
  //     z = -1;
  //   }
  //   if(tLed1.getTimer() && led1_On)
  //   //&&&&&&&&&&&&&&&&&&&&&&&&&&&????????????????????????????????
  //   ;
  // }
  controlLed(radarFunc());

  //....... измерение освещённости
  if (t1.getTimer() && lightMeter.measurementReady()) {
    lux = lightMeter.readLightLevel();
    t1.setTimer(dt);
    if(abs(lux - luxOld) > 5){
      // if(light_1.setLux(lux))
      client.publish(msgLight, String(lux).c_str());
      // Serial.print("lux = ");
      // Serial.println(lux);
      luxOld = lux;
    }
  }
  // if(t1.getTimer()){
  //   t1.setTimer(dt);
  //   ledStat = !ledStat;
  //   if(presenceLd)
  //     digitalWrite(PinOutLd, ledStat);
  // }
}