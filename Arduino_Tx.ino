//  --------------------------- GPS
#include <NeoSWSerial.h>
NeoSWSerial gpsPort(2, 3);                            // Rx and Tx

#include <NMEAGPS.h>
NMEAGPS gps;
gps_fix fix;                                          // all the parsed pieces from GPS
//unsigned int fixCount;                              // to count: how many GPS updates we have received

//  --------------------------- DHT11 Temperature and Humidity sensor
#include <DHT.h>
#include <DHT_U.h>

#define DHTTYPE DHT11                                 // to define the sensor type
#define DHTdata 6                                     //connected to pin number 2 and 'temp' is a variable
DHT dht(DHTdata, DHTTYPE); 

//  --------------------------- DS18B20 Temperature sensor
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 7        // Arduino pin connected to DS18B20 we can use 'const int ONE_WIRE_BUS 2', ONE_WIRE_BUS is variable
OneWire oneWire(ONE_WIRE_BUS);      // setup a oneWire instance
DallasTemperature sensors(&oneWire);        // pass oneWire to DallasTemperature library

//  ------------------------- SD card and RTC (Data logger shield)
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

#define CHIPSELECT 10        // this should be 10 for Arduino UNO (Do not change)
RTC_DS3231 rtc;

//  -------------------------- LCD I2C
#include <Wire.h>  
#include <LiquidCrystal_I2C.h>
#include "I2C_Anything.h"         

//LiquidCrystal_I2C lcd(0x27, 16, 2);     // Set the LCD I2C address

float sensorVoltage, sensorValue, sensorUV, dht_temp, dht_hum, dht_tempf, ds_temp, ds_tempf, latitude, longitude, Received_value;
short Moisture, PH;
#define Reset 4                          // connect pin 4 on arduino UNO to Reset pin on the board

// ************************** begin setup *************************************

void setup(){
  Serial.begin(115200);

  digitalWrite(Reset, HIGH);
  delay(200); 
  pinMode(Reset, OUTPUT);

  //lcd.init();             // Initialize LCD      
  //lcd.backlight();        // Turn ON LCD backlight
  //lcd.clear();            // Clear LCD
  
  gpsPort.begin(9600);
  gps.send_P( &gpsPort, F("PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0") ); // RMC & GGA
  gps.send_P( &gpsPort, F("PMTK220,1000") ); // 1Hz
  
  Wire.begin(8);                      // join i2c bus with address #8
  Wire.onReceive(receiveEvent);       // register receive event
  Wire.onRequest(requestEvent);       // register event
  
  dht.begin();              // sensor DHT11 start
  sensors.begin();          // Senspr DS18B20 start

  while (!Serial) 
  {
    ; // wait for serial port to connect.
  }
  Serial.print(F("Initializing SD card..."));

  // Check if the SD card is present and can be initialized:
  if (!SD.begin(CHIPSELECT)) 
  {
    Serial.println(F("Card failed, or not present"));
    //lcd.setCursor(0,0);
    //lcd.print(F("card failed"));
    while (1);
  }
  Serial.println(F("card initialized."));
  //lcd.setCursor(0,0);
  //lcd.print(F("card initialized"));
  

  //#ifndef ESP8266
     //while (!Serial);     // wait for serial port to connect. Needed for native USB
  //#endif

  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if(!rtc.begin()) 
  {
    Serial.println(F("Couldn't find RTC"));
    Serial.flush();
    abort();
  }
  if(rtc.lostPower()) 
  {
    Serial.println(F("RTC lost power, let's set the time!"));
      // When time needs to be set on a new device, or after a power loss, the
      // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      // This line sets the RTC with an explicit date & time, for example to set
      // June 8, 2021 at 9am you would call:
      // rtc.adjust(DateTime(2021, 6, 8, 9, 0, 0));
  }

  //lcd.clear();            // Clear LCD
} // end setup


// function that executes whenever data is received from NodeMCU for SD card
void receiveEvent() 
{
  while (0 < Wire.available()) 
  {
    //Reboot_System = Wire.read();      // receive byte for rebooting 
    Received_value = Wire.read();      // receive byte for storing data in SD card 
    //Serial.println(F("Received value: "));
    Serial.println(Received_value);           // print the character 
  }
}


// ***************************************************************

void loop(){

// ******************************************* GPS
  
  if (gps.available( gpsPort )) {
    
    fix = gps.read();
    
    Serial.print(F("\nFix: ") );
    if (fix.valid.status)
    Serial.println(fix.status);
    
  if (fix.valid.location) {
    
  latitude = fix.latitude();
  longitude = fix.longitude();
    /*
    Serial.print(F("Latitude: "));
    Serial.print( latitude, 6 );
    Serial.print(F(", Longitude: "));
    Serial.println( longitude, 6 );
    */
  } 

//  --------------------------------------------- RTC
    DateTime now = rtc.now();
/*
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(F(", Time: "));
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.println(now.second(), DEC);
    //Serial.println();
*/
/*
    lcd.setCursor(0,0);
    lcd.print(now.hour(), DEC);
    lcd.setCursor(2,0);
    lcd.print(":");
    lcd.setCursor(3,0);
    lcd.print(now.minute(), DEC);
    lcd.setCursor(5,0);
    lcd.print(":");
    lcd.setCursor(6,0);
    lcd.print(now.second(), DEC);   // RTC
*/

//  ----------------------------------- DHT11 --------------------------------------
  
  dht_temp = dht.readTemperature();       // to read temperature from sensor
  dht_hum = dht.readHumidity();           // to read humidity from sensor
  dht_tempf = dht_temp * 9 / 5 + 32;          // convert Celsius to Fahrenheit
  
  /*
  Serial.print(F("DHT11 Sensor-"));
  Serial.print(F("  Temperature: "));
  Serial.print(dht_temp);
  Serial.print(F(", Humidity: "));
  Serial.print(dht_hum);
  Serial.print(F(", temp f: "));
  Serial.println(dht_tempf);
  */

//  ------------------------------------ Analog UV sensor ----------------------      

  sensorValue = analogRead(A2);           //read the value from sensor and store to variable.
  sensorVoltage = sensorValue * (5.0/1023.0);
  sensorUV = sensorVoltage/0.1;
  /*
  Serial.print(F("UV Sensor- "));
  Serial.print(F(" Analog Value: "));
  Serial.print(sensorValue);
  Serial.print(F(", Voltage: "));
  Serial.print(sensorVoltage);
  Serial.print(F(" V"));
  Serial.print(F(", UV Index = "));
  Serial.println(sensorUV);
*/
//  --------------------------------------------- DS18B20 --------------------------
  sensors.requestTemperatures();                  // send the command to get temperatures
  ds_temp = sensors.getTempCByIndex(0);           // read temperature in Celsius
  ds_tempf = sensors.toFahrenheit(ds_temp);     // temperature in fahrenheit
  //if(isnan(ds_temp))
  //{
    //Serial.println(F("Failed to read the temperature"));
  //}
  /*
  else
  {
    Serial.print(F("DS18B20 Sensor- "));
    Serial.print(ds_temp);
    Serial.print(F(" C, "));
    Serial.print(ds_tempf);
    Serial.println(F(" F"));
  }     
*/

//  ------------------------------------ pH and Moisture -----------------------------------------
//  --------------------------------------- Moisture ---------------------------------------------------

   unsigned char i;
   
   Moisture = 0; 
   for(i=0;i<10;i++){Moisture = Moisture + analogRead(A0);delay(1);}
   Moisture = Moisture / 10; 
   
   if(Moisture >= 530){Moisture = ((Moisture - 550)/15) + 90;}else
   if(Moisture >= 430){Moisture = ((Moisture - 450)/10) + 80;}else
   if(Moisture >= 130){Moisture = ((Moisture - 130)/6) + 30;}else
   if(Moisture >=   0){Moisture = ((Moisture)/5);}   
   if(Moisture > 100){Moisture = 100;}
   /*
   Serial.print(F("Moisture and pH-"));
   Serial.print(F("   Moisture: "));
   Serial.print(Moisture/10,1);
   Serial.print(".");
   Serial.print(Moisture%10,1);
*/
//   -------------------------------------- pH -------------------------------------------

    PH = 0;
    for(i=0;i<10;i++) { PH = PH + analogRead(A1);delay(1); } 
    PH = PH/10;    
    if(PH >= 450){PH = 40-((PH - 450)/50);}else
    if(PH >= 280){PH = 50-((PH - 280)/17);}else
    if(PH >= 150){PH = 60-((PH - 150)/13);}else
    if(PH >= 20){PH = 70-((PH -  20)/13);}else
    if(PH >= 0){PH = 80-((PH -  0 )/2); }
    /*
    Serial.print(F(", pH: "));
    Serial.print(PH/10,1);
    Serial.print(F("."));
    Serial.println(PH%10,1);
    Serial.print("\n");
*/


if(Received_value == 5)
{
  //Serial.println(F("inside the SD card"));
// ---------------------------------------- SD card --------------------------------------
File GreenhouseFile = SD.open("Grnhouse.txt", FILE_WRITE);      // Filenames are limited to 8 characters.
  if(GreenhouseFile)
  {
//  ********************************************** RTC
    GreenhouseFile.print(now.year(), DEC);
    GreenhouseFile.print('/');
    GreenhouseFile.print(now.month(), DEC);
    GreenhouseFile.print('/');
    GreenhouseFile.print(now.day(), DEC);
    //GreenhouseFile.print(F(", Day- "));
    //GreenhouseFile.print(daysOfTheWeek[now.dayOfTheWeek()]);
    GreenhouseFile.print(F(", Time- "));
    GreenhouseFile.print(now.hour(), DEC);
    GreenhouseFile.print(':');
    GreenhouseFile.print(now.minute(), DEC);
    GreenhouseFile.print(':');
    GreenhouseFile.println(now.second(), DEC);
    //GreenhouseFile.print("\n");
    
//  ********************************************** GPS
    GreenhouseFile.print(F("Latitude= "));
    GreenhouseFile.print(fix.latitude(), 6 );
    GreenhouseFile.print(F(" Longitude= "));
    GreenhouseFile.println(fix.longitude(), 6 );
    //GreenhouseFile.print('\n');

//  ********************************* DS18B20 
    GreenhouseFile.print(F("DS18B20 Temperature- "));
    GreenhouseFile.print(ds_temp);
    GreenhouseFile.print(F(" C, "));
    GreenhouseFile.print(ds_tempf);
    GreenhouseFile.println(F(" F"));
    //GreenhouseFile.print('\n');

//  ********************************* DHT11
    GreenhouseFile.print(F("DHT11 Sensor-"));
    GreenhouseFile.print(F(" Temperature: "));
    GreenhouseFile.print(dht_temp);
    GreenhouseFile.print(F(", Humidity: "));
    GreenhouseFile.print(dht_hum);
    //GreenhouseFile.print(F(", Temperature in F: "));
    //GreenhouseFile.println(dht_tempf);
    //GreenhouseFile.print('\n');
    
//  ********************************* UV 
    GreenhouseFile.print(F("UV sensor- "));
    GreenhouseFile.print(F(" Analog Value: "));  
    GreenhouseFile.print(sensorValue);
    //GreenhouseFile.print(F(", Sensor voltage: "));
    //GreenhouseFile.print(sensorVoltage);
    GreenhouseFile.print(F(" V"));
    GreenhouseFile.print(F(", UV Index = "));
    GreenhouseFile.println(sensorUV);
    //GreenhouseFile.print('\n');

//  ********************************* Moisture and pH
    GreenhouseFile.print(F("Moisture and pH-"));
    GreenhouseFile.print(F(" Moisture: "));
    //GreenhouseFile.print(Moisture,3);
    GreenhouseFile.print(Moisture/10,1);
    GreenhouseFile.print(".");
    GreenhouseFile.print(Moisture%10,1);
    GreenhouseFile.print(F(", pH: "));
    GreenhouseFile.print(PH/10,1);
    GreenhouseFile.print(F("."));
    GreenhouseFile.println(PH%10,1);
    GreenhouseFile.print('\n');

    //lcd.setCursor(0,0);
    //lcd.print(F("Storing in file"));
    Serial.println(F("Storing in file"));
    
    GreenhouseFile.close();  
  }
  else
  {
    Serial.print(F("error opening Grnhouse.txt \n"));
    //lcd.setCursor(0,1);
    //lcd.print(F("error Grnhouse.txt"));
  }

    Serial.print("\n");
    delay(14000);
    }
    //lcd.clear();
    delay(5000);
    
if(Received_value == 13)
{
  //lcd.clear();
  //lcd.setCursor(0,1);
  //lcd.print(F("rebooting the arduino UNO"));
  Serial.println(F("Rebooting the arduino UNO"));
  delay(1000);
  digitalWrite(Reset, LOW);
  //Serial.println("Arduino will never reach there.");
}
    
  }
}

void requestEvent()
{
//  GPS
   I2C_writeAnything(latitude);
   I2C_writeAnything(longitude);
   
//  UV sensor
   I2C_writeAnything(sensorValue);   

//  DHT11 Temperature and Humidity
   I2C_writeAnything(dht_temp);   
   I2C_writeAnything(dht_hum);

//  DS18B20 Temperature sensor
   I2C_writeAnything(ds_temp);   
   
   //I2C_writeAnything(ds_tempf);

//  Moisture and Ph sensor
   I2C_writeAnything(Moisture);
   I2C_writeAnything(PH);

   Serial.println(F("Sent data successfully"));
   delay(500);
}
