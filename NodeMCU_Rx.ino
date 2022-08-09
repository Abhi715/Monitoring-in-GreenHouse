// for I2C and LCD_i2C
#include <Wire.h>
#include "I2C_Anything.h"

//#include <LiquidCrystal_I2C.h>              
//LiquidCrystal_I2C lcd(0x27, 16, 2);                             // 0x27

// for time
#include <WiFiUdp.h>
#include <NTPClient.h>

// for sending data to server
#include<ESP8266HTTPClient.h>
#include<WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include<ESP8266WiFi.h>
#include <ArduinoJson.h>

float sensorVoltage, sensorValue, sensorUV, dht_temp, dht_hum, dht_tempf, ds_temp, ds_tempf, latitude, longitude;
short Moisture, PH;
String Time;
// SD_Store = 9 for not storing data on SD card and Reboot_System = 10 for not rebooting arduino or system
int SD_Store = 9, Reboot_System = 10;                           // For storing data in SD card and rebooting the Arduino UNO


#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
// WiFi connect timeout per AP. Increase when connecting takes longer.
const uint32_t connectTimeoutMs = 5000;


const long utcOffsetInSeconds = 19800;                          // Replace with your GMT offset (seconds)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


// server API
const char* serverLogin = "https://api.appslause.com:35795/api/v1/Auth/Login";
const char* DeviceSettings = "https://api.appslause.com:35795/api/v2/IoT/Setting?_projectId=1";
const char* serverHeart = "https://api.appslause.com:35795/api/v2/IoT/Heartbeat";
const char* serverData = "https://api.appslause.com:35795/api/v2/IoT/DataV1";
const char* DeviceActivity = "https://api.appslause.com:35795/api/v2/IoT/DeviceActivities?disabledAfterGet=false";

const int httpsPort = 443;
const char fingerpr[] PROGMEM = "BB 3B 5F 83 41 E9 F3 C3 5C 6F 3F EB 39 54 A4 42 B3 0E E3 7A";

String B_token;

unsigned long interval = 3600000;                               // timer(interval) for one hour for if 


// Start Setup -------------------------------------------------
void setup()
{
  
  Serial.begin(115200);                                         // start serial for output

  //Wire.begin(D4, D3);
  //lcd.init();                                                   // initializing the LCD
  //lcd.backlight();                                              // Enable or Turn On the backlight 
  //lcd.clear();                                                  // clear the lcd

  Wire.begin(D1, D2);                                           // join i2c bus (address optional for master)

  // scan Wi-Fi network and connect
  WiFi.mode(WIFI_STA);
  // Register multi WiFi networks
  wifiMulti.addAP("Redmi_1", "Abhiwifii");                      // wifi name and password
  wifiMulti.addAP("Mustang GT 5.0", "@bhiwifi");                // wifi name and password
  //wifiMulti.addAP("Name", "Password");                        // wifi name and password
  //wifiMulti.addAP("SSID", "Password");                        // wifi name and password
  //wifiMulti.addAP("SSID", "Password");                        // wifi name and password
 // Maintain WiFi connection
  if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED) {
    //Serial.print("WiFi connected :)");
    //Serial.println(WiFi.SSID());                              // to show the connected Wi-Fi name
    //Serial.print(" ");
    //Serial.println(WiFi.localIP());                           // to show the IP address of connected Wi-Fi
  } else {
    Serial.println("WiFi not connected!");
  }

timeClient.begin();

// for Login API
  WiFiClientSecure client;
  client.connect(serverLogin, httpsPort);                       // API address for heartbeat and port
  client.setFingerprint(fingerpr);                              // fingerprint

  HTTPClient https;                                             // for serverlogin API
  https.begin(client, serverLogin);                             // Specify request destination for Login
  https.addHeader("Content-Type", "application/json");          // Specify content-type header

  String logindata;
  StaticJsonDocument<256> doc;                                  // json array and data
  doc["applicationId"] = 1;
  doc["socialNetworkType"] = 0;
  doc["email"] = "greenhouse.iot.appslause@gmail.com";
  doc["password"] = "P@ssw0rd";

  serializeJsonPretty(doc, logindata);

  int httpsCode_loginID = https.POST(logindata);                // Send the POST request to Login API
  Serial.print("HTTPS code for Login API: ");                   
  Serial.println(httpsCode_loginID);                            // Print HTTP return response code, 200 means success

  //lcd.setCursor(0,0);
  //lcd.print("1:");
  //lcd.setCursor(2,0);
  //lcd.print(httpsCode_loginID);

  //Serial.println("data format-- ");
  //Serial.println(logindata);

  // for payload_loginID and bearer token
  String payload_loginID = https.getString();                   // Get the response payload_loginID
  Serial.print("payload_loginID: ");
  Serial.println(payload_loginID);                              // Print request response payload_loginID

  int in = payload_loginID.indexOf("message");                  // for bearer token from Login API 
  //Serial.println(in);
  int in1 = payload_loginID.indexOf("isSuccess");
  //Serial.println(in1);

  B_token = payload_loginID.substring((in + 10), (in1 - 3));    // without double quotes
  //String B_token = payload_loginID.substring((in+9), (in1-2));// with double quotes
  //Serial.print("Bearer token: ");
  Serial.println(B_token);                                      // Bearer Token got from Server

  Wire.beginTransmission(8);                                    // begin transmission to arduino UNO
  Wire.write(SD_Store);                                         // send request to not store data on SD card
  Wire.endTransmission();                                       // end transmission to arduino UNO

  if (httpsCode_loginID != 200)                                 // if Login API is not responding 
  {
    Serial.println("Sent request to save data on SD card");
    SD_Store = 5;                                               // change the value of SD_Store to 
    // send to arduino to save data in SD card.
      Wire.beginTransmission(8);                                // begin transmission to arduino UNO
      Wire.write(SD_Store);                                     // send request to store data on SD card
      Wire.endTransmission();                                   // end transmission to arduino UNO
    // retry for 3 times in every 10 seconds
    for (int i = 0; i <= 2; i++)                               
    { 
      if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED) { Serial.print("WiFi connected :)");} 
        else { Serial.println("WiFi not connected!");}
      
      httpsCode_loginID = https.POST(logindata);                // Send the POST request to Login API
      Serial.print("HTTPS code for Login API: ");
      Serial.println(httpsCode_loginID);                        // Print HTTP return response code, 200 means success

      // for payload_loginID and bearer token
      String payload_loginID = https.getString();               // Get the response payload_loginID
      Serial.print("payload_loginID: ");
      Serial.println(payload_loginID);                          // Print request response payload_loginID
      int in = payload_loginID.indexOf("message");
      int in1 = payload_loginID.indexOf("isSuccess");
      B_token = payload_loginID.substring((in + 10), (in1 - 3));// without double quotes
      Serial.println(B_token);        // Bearer Token got from Server
      if (httpsCode_loginID == 200)
      {
        Serial.println("Sent request not to save data on SD card");
        SD_Store = 9;                                           // send request to not store data on SD card
        Wire.beginTransmission(8);
        Wire.write(SD_Store);
        Wire.endTransmission();

        //lcd.setCursor(0,0);
        //lcd.print("1:");
        //lcd.setCursor(2,0);
        //lcd.print(httpsCode_loginID);
        break;                                                  // if login API can communicate then break
      }        
      delay(10000);                                             // wait for 10 seconds.
      //else
      //continue;
    }
  }

  // retry in every 3 times in every 10 seconds
  if (httpsCode_loginID != 200)
  { 
 // still can't get the token then retry in every 3 minutes
    while (httpsCode_loginID != 200)
    { 
     if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED) { Serial.print("WiFi connected :)");} 
      else { Serial.println("WiFi not connected!");}
      
      httpsCode_loginID = https.POST(logindata);                // Send the POST request to Login API
      Serial.print("HTTPS code for Login API: ");
      Serial.println(httpsCode_loginID);                        // Print HTTP return response code, 200 means success

      // for payload_loginID and bearer token
      String payload_loginID = https.getString();               // Get the response payload_loginID
      Serial.print("payload_loginID: ");
      Serial.println(payload_loginID);                          // Print request response payload_loginID
      int in = payload_loginID.indexOf("message");
      int in1 = payload_loginID.indexOf("isSuccess");
      B_token = payload_loginID.substring((in + 10), (in1 - 3));// without double quotes
      Serial.println(B_token);                                  // Bearer Token got from Server
      if (httpsCode_loginID == 200)
      {
        SD_Store = 9;                                           // change value because no need to store data on SD card
        Wire.beginTransmission(8);
        Wire.write(SD_Store);
        Wire.endTransmission();
        break;                                                  // if can communicate then break
      }
      delay(180000);                                            // wait for 3 minutes                                        
      if (millis() >= interval)                                 // if can't communicate to API for one hour then restart the NodeMCU
      {
        Serial.println("Restarting ");
        ESP.restart();                                          // restart NodeMCU after one hour
      }
    }
  }
     //SD_Store = 9;
     //Wire.beginTransmission(8);
     //Wire.write(SD_Store);
     //Wire.endTransmission();

// For DeviceSetting API
  https.begin(client, DeviceSettings);                          // Specify request destination for HeartBeat
  https.addHeader("Content-Type", "application/json");          // Specify content-type header
  https.addHeader("Authorization", "Bearer " + B_token);

//for GET request Device_Settings API
  int httpCode_DeviceSetting = https.GET();                     // send GET request to Device Setting API
  Serial.print("Https code for DeviceSettings API: ");
  Serial.println(httpCode_DeviceSetting);                       // Print HTTP return code

  //lcd.setCursor(6,0);
  //lcd.print("2:");
  //lcd.setCursor(8,0);
  //lcd.print(httpCode_DeviceSetting);

  String payloadD_Settings = https.getString();                 // Get the response payloadD_Settings
  Serial.print("payloadD_Settings: ");
  Serial.println(payloadD_Settings);                            // Print request response payloadD_Settings
     
} // end setup

// ################################################################

// start main loop
void loop()
{
  // for Wi-Fi
  // Maintain WiFi connection
  if (wifiMulti.run(connectTimeoutMs) == WL_CONNECTED) {
    Serial.println("Connected to Network :)");
    //Serial.println(WiFi.SSID());
    //Serial.print(" ");
    //Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi not connected!");
  }

  //
    timeClient.update();
    String formattedTime = timeClient.getFormattedTime();
    //Serial.println(formattedTime);
    //Serial.print("Time: ");
    //Serial.print(timeClient.getHours());
    //Serial.print(":");
    //Serial.print(timeClient.getMinutes());
    //Serial.print(":");
    //Serial.println(timeClient.getSeconds());

  Wire.requestFrom(8, 10000);
  // start receiving data from Arduino using I2C
  //  GPS
  I2C_readAnything(latitude);
  I2C_readAnything(longitude);

  //  UV sensor
  I2C_readAnything(sensorValue);
  sensorVoltage = sensorValue * (5.0 / 1023.0);
  sensorUV = sensorVoltage / 0.1;

  //  DHT11 Temperature and Humidity
  I2C_readAnything(dht_temp);
  I2C_readAnything(dht_hum);
  dht_tempf = dht_temp * 9 / 5 + 32;                            // convert Celsius to Fahrenheit

  //  DS18B20 Temperature sensor
  I2C_readAnything(ds_temp);
  //I2C_readAnything(ds_tempf);
  ds_tempf = ds_temp * 9 / 5 + 32;

  //  Moisture and Ph sensor
  I2C_readAnything(Moisture);
  I2C_readAnything(PH);
  // data received from Arduino

  // Print received data on Serial monitor
  /*
    // GPS
    Serial.print(F("Latitude: "));
      Serial.print( latitude, 6 );
      Serial.print(F(", Longitude: "));
      Serial.println( longitude, 6 );

    //  UV sensor
    Serial.print(F("UV Sensor- "));
    Serial.print(F(" Analog Value: "));
    Serial.print(sensorValue);
    Serial.print(F(", Voltage: "));
    Serial.print(sensorVoltage);
    Serial.print(F(" V"));
    Serial.print(F(", UV Index = "));
    Serial.println(sensorUV);

    //  DS18B20
    Serial.print(F("DS18B20 Sensor- "));
    Serial.print(ds_temp);
    Serial.print(F(" C, "));
    Serial.print(ds_tempf);
    Serial.println(F(" F"));
    //  DHT11
    Serial.print(F("DHT11 Sensor-"));
    Serial.print(F("  Temperature: "));
    Serial.print(dht_temp);
    Serial.print(F(", Humidity: "));
    Serial.print(dht_hum);
    Serial.print(F(", temp f: "));
    Serial.println(dht_tempf);

    //  moisture and PH
    Serial.print(F("Moisture and pH- "));
    Serial.print(F("   Moisture: "));
    Serial.print(Moisture/10,1);
    //Serial.print(moist/10,1);
    Serial.print(".");
    Serial.print(Moisture%10,1);
    Serial.print(F(", pH: "));
    Serial.print(PH/10,1);
    Serial.print(F("."));
    Serial.println(PH%10,1);
    Serial.print("\n");
  */
  String moisture1 = String(Moisture / 10);
  String moisture2 = String(Moisture % 10);
  String MOISTURE = moisture1 + "." + moisture2;
  String ph1 = String(PH / 10);
  String ph2 = String(PH % 10);
  String Ph = ph1 + "." + ph2;

// for server
  WiFiClientSecure client1;
  client1.connect(serverHeart, httpsPort);                      // API address for heartbeat and port
  client1.setFingerprint(fingerpr);






// for HeartBeat API ---------------=========================
  HTTPClient https1;
  https1.begin(client1, serverHeart);                           // Specify request destination for HeartBeat
  https1.addHeader("Content-Type", "application/json");         // Specify content-type header
  https1.addHeader("Authorization", "Bearer " + B_token);

//for GET request Heartbeat API
  int httpCode_Heartbeat = https1.GET();                        // send GET request to Heartbeat API 
  Serial.print("Https code for Heartbeat API: ");
  Serial.println(httpCode_Heartbeat);                           // Print HTTP return code

  //lcd.setCursor(12,0);
  //lcd.print("3:");
  //lcd.setCursor(14,0);
  //lcd.print(httpCode_Heartbeat);

  String payload_heartbeat = https1.getString();                // Get the response payload_loginID
  Serial.print("payload_heartbeat: ");
  //Serial.println(payload_heartbeat);                          // Print request response payload_loginID

  int T = payload_heartbeat.indexOf("T");                       // to extract the time from the payload of Heartbeat API
  //Serial.println(T);
  Time = payload_heartbeat.substring((T+1), (T+9));
  payload_heartbeat.replace(Time, formattedTime);
  //Serial.println("New payload_loginID: ");
  Serial.println(payload_heartbeat);                            // Got the time from the payload of Heartbeat API
  //Serial.println(Time);

if(httpCode_Heartbeat != 200)
{
  for(int i=0;i<=5;i++)                                         // retry for 6 times in every 30 seconds
  {
    httpCode_Heartbeat = https1.GET();                          // send GET request to Heartbeat API
    Serial.print("Https code for Heartbeat API: ");
    Serial.println(httpCode_Heartbeat);                         // Print HTTP return code, 200 means success

    String payload_heartbeat = https1.getString();              // Get the response from Heartbeat API
    Serial.print("payload_heartbeat: ");
    //Serial.println(payload_heartbeat);                        // Print request response payload_loginID

    int T = payload_heartbeat.indexOf("T");
    //Serial.println(T);
    Time = payload_heartbeat.substring((T+1), (T+9));
    payload_heartbeat.replace(Time, formattedTime);
    //Serial.println("New payload_loginID: ");
    Serial.println(payload_heartbeat);

    if(httpCode_Heartbeat == 200)
    {
      Reboot_System = 10;                                       // for not rebooting the Arduino UNO
      Wire.beginTransmission(8);
      Wire.write(Reboot_System);
      Wire.endTransmission();

      //lcd.setCursor(12,0);
      //lcd.print("3:");
      //lcd.setCursor(14,0);
      //lcd.print(httpCode_Heartbeat);
      break;
    }
    delay(30000);                                               // wait for 30 seconds
  }
    if(httpCode_Heartbeat != 200)
    {
      Reboot_System = 13;                                       // for rebooting the Arduino UNO(whole system)
      Wire.beginTransmission(8);
      Wire.write(Reboot_System);
      Wire.endTransmission();
    }   
}
/*
  if(httpCode_Heartbeat == 200)
    {
      Reboot_System = 10;
      Wire.beginTransmission(8);
      Wire.write(Reboot_System);
      Wire.endTransmission();
    }
*/






//for POST request or Postdata API---------------------====================
  https1.begin(client1, serverData);
  https1.addHeader("Content-Type", "application/json");         // Specify content-type header
  https1.addHeader("Authorization", "Bearer " + B_token); 

  String SensorPostData;
  // json array and data
  StaticJsonDocument<256> Data;
  //JsonObject root = Data.to<JsonObject>();
  JsonObject sample = Data.createNestedObject("info");

  sample["1"] = latitude;
  sample["2"] = longitude;
  sample["3"] = sensorValue;
  sample["4"] = sensorVoltage;
  sample["5"] = sensorUV;
  sample["6"] = ds_temp;
  sample["7"] = ds_tempf;
  sample["8"] = dht_temp;
  sample["9"] = dht_tempf;
  sample["10"] = dht_hum;
  sample["11"] = MOISTURE;
  sample["12"] = Ph;

  serializeJsonPretty(Data, SensorPostData);

  int httpsCode_PostData = https1.POST(SensorPostData);         // Send the POST request to Postdata API
  Serial.print("HTTPS code for POST data API: ");
  Serial.println(httpsCode_PostData);                           // Print HTTP return code, 200 means success
  //Serial.println("data format-- ");
  //Serial.println(SensorPostData);

  //lcd.setCursor(0,1);
  //lcd.print("4:");
  //lcd.setCursor(2,1);
  //lcd.print(httpsCode_PostData);

if(httpsCode_PostData != 200)                                   // if https code is not 200 then retry for 6 times
{
  for(int i=0; i<=5; i++)
  {
    int httpsCode_PostData = https1.POST(SensorPostData);         // Send the POST request to Postdata API
    Serial.print("HTTPS code for POST data API: ");
    Serial.println(httpsCode_PostData);                           // Print HTTP return code, 200 means success

    if(httpsCode_PostData == 200)
    {
      //lcd.setCursor(0,1);
      //lcd.print("4:");
      //lcd.setCursor(2,1);
      //lcd.print(httpsCode_PostData);
      break;
    }
    delay(30000);                                               // wait for 30 seconds
  }
  if(httpsCode_PostData != 200)                                 // if still can't communicate to DeviceActivity then store data on SD card
  {
    SD_Store = 5;                                               // send request to Arduino UNO to store the data on SD card
    Wire.beginTransmission(8);
    Wire.write(SD_Store);
    Wire.endTransmission();
  }
  else
  {
    SD_Store = 9;                                               // send request to Arduino UNO not to store the data on SD card
    Wire.beginTransmission(8);
    Wire.write(SD_Store);
    Wire.endTransmission();
  }
}






// for GET request DeviceActivity API
  https1.begin(client1, DeviceActivity);
  https1.addHeader("Content-Type", "application/json");         // Specify content-type header
  https1.addHeader("Authorization", "Bearer " + B_token);

//for GET request Device Activity API
  int httpCode_DeviceActivity = https1.GET();
  Serial.print("Https code for Device Activity API: ");
  Serial.println(httpCode_DeviceActivity);                      // Print HTTP return code, 200 means success

  String payload_deviceActivity = https1.getString();           // Get the response payload_deviceActivity
  Serial.print("payload_deviceActivity: ");
  Serial.println(payload_deviceActivity);                       // Print request response 

  //lcd.setCursor(6,1);
  //lcd.print("5:");
  //lcd.setCursor(8,1);
  //lcd.print(httpCode_DeviceActivity);

  if (httpCode_DeviceActivity != 200)
  { 
    for(int i=0; i<=5; i++)
    { 
     //for GET request Device Activity API
      int httpCode_DeviceActivity = https1.GET();
      Serial.print("Https code for Device Activity API: ");
      Serial.println(httpCode_DeviceActivity);                  // Print HTTP return code, 200 means success
      String payload_deviceActivity = https1.getString();       // Get the response payload_deviceActivity
      Serial.print("payload_deviceActivity: ");
      Serial.println(payload_deviceActivity);                   // Print request response 
      
      delay(30000);                                             // wait for 30 seconds.
    }
  }
  
https1.end();                                                   // Close HTTPS connection
Serial.println("\n");                                           // new line
delay(20000);                                                   // wait for 20 seconds
}
