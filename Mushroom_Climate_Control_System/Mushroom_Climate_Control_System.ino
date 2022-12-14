//Libraries 
//#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <TFT_eSPI.h> //TFT LCD library
#include <SensirionI2CScd4x.h> //SCD41 library

//Definitions
#define DHTPIN 0 //Define signal pin of DHT sensor 
#define CO2_THRESHOLD 1200
#define ADDRESS_BH1750FVI 0x23    //ADDR="L" for this module
#define ONE_TIME_H_RESOLUTION_MODE 0x20

//Initializations

TFT_eSPI tft = TFT_eSPI(); //Initializing TFT LCD
TFT_eSprite spr = TFT_eSprite(&tft); //Initializing buffer
SensirionI2CScd4x scd4x;

 byte highByte = 0;
 byte lowByte = 0;
 unsigned int sensorOut = 0;
 unsigned int illuminance = 0;

unsigned int prev_time = 0;
bool alarm = false;

void printUint16Hex(uint16_t value) 
{
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) 
{
  Serial.print("Serial: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}
int Liquid_level=0;


void setup() {
  Serial.begin(115200); //Start serial communication

  pinMode(WIO_5S_PRESS, INPUT_PULLUP);
  //pinMode(WIO_LIGHT, INPUT); //Set light sensor pin as INPUT
  pinMode(WIO_BUZZER, OUTPUT); //Set buzzer pin as OUTPUT
  pinMode(D8, INPUT); // Water level senor
  pinMode(D0, OUTPUT); // Relay
  digitalWrite(D0, LOW);
  
  //dht.begin(); //Start DHT sensor 
  tft.begin(); //Start TFT LCD
  tft.setRotation(3); //Set TFT LCD rotation
  spr.createSprite(TFT_HEIGHT,TFT_WIDTH); //Create buffer

  Wire.begin();
  uint16_t error;
  char errorMessage[256];

  scd4x.begin(Wire);

  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    tft.print(errorMessage);
  }

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;

  error = scd4x.getSerialNumber(serial0, serial1, serial2);
  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    tft.print(errorMessage);
  } else {
    printSerialNumber(serial0, serial1, serial2);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    tft.print(errorMessage);
  }
}

void loop() {
  
  uint16_t error;
  char errorMessage[256];
   
   delay(5000);
    
 // Read Measurement
  uint16_t co2;
  float temperature;
  float humidity;
  //int t = dht.readTemperature(); //Assign variable to store temperature 
  //int h = dht.readHumidity(); //Assign variable to store humidity 
  int light =                 //analogRead(WIO_LIGHT); //Assign variable to store light sensor values
  
  error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else if (co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
    } else {

     Wire.beginTransmission(ADDRESS_BH1750FVI); //"notify" the matching device
     Wire.write(ONE_TIME_H_RESOLUTION_MODE);     //set operation mode
     Wire.endTransmission();

     delay(180);

     Wire.requestFrom(ADDRESS_BH1750FVI, 2); //ask Arduino to read back 2 bytes from the sensor
     highByte = Wire.read();  // get the high byte
     lowByte = Wire.read(); // get the low byte

     sensorOut = (highByte<<8)|lowByte;
     illuminance = sensorOut/1.2;
     Serial.print(illuminance);    Serial.println(" lux");
     delay(100);

     Liquid_level=digitalRead(D8);
     Serial.print("Liquid_level= ");
     Serial.print(Liquid_level,DEC);
     delay(500);
     
  //Setting the title header 
  spr.fillSprite(TFT_WHITE); //Fill background with white color
  spr.fillRect(0,0,320,50,TFT_DARKGREEN); //Rectangle fill with dark green 
  spr.setTextColor(TFT_WHITE); //Setting text color
  spr.setTextSize(3); //Setting text size 
  spr.drawString("MCC SYSTEM",50,15); //Drawing a text string 

  spr.drawFastVLine(150,50,190,TFT_DARKGREEN); //Drawing verticle line
  spr.drawFastHLine(0,140,320,TFT_DARKGREEN); //Drawing horizontal line

  //Setting temperature
  spr.setTextColor(TFT_BLACK);
  spr.setTextSize(2);
  spr.drawString("Temperature",10,65);
  spr.setTextSize(3);
  spr.drawNumber(temperature,50,95); //Display temperature values 
  spr.drawString("C",90,95);

  //Setting humidity
  spr.setTextSize(2);
  spr.drawString("Humidity",25,160);
  spr.setTextSize(3);
  spr.drawNumber(humidity,30,190); //Display humidity values 
  spr.drawString("%RH",70,190);

  //Setting CO2
  //sensorValue = analogRead(sensorPin); //Store sensor values 
  //sensorValue = map(sensorValue,1023,400,0,100); //Map sensor values 
  spr.setTextSize(2);
  spr.drawString("CO2",225,65);
  spr.setTextSize(3);
  spr.drawNumber(co2,195,95); //Display sensor values as percentage  
  spr.drawString("PPM",260,95);
  
  //Setting light 
  spr.setTextSize(2);
  spr.drawString("Light",200,160);
  spr.setTextSize(3);
  //light = map(illuminance,0, 1023,0,100); //Map sensor values 
  spr.drawNumber(illuminance,185,190); //Display sensor values as percentage  
  spr.drawString("lux",250,190);

  //Condition for low soil moisture
  if(Liquid_level==0){
    spr.fillSprite(TFT_RED);
    spr.drawString("Time to fill water!",35,100);
    analogWrite(WIO_BUZZER, 150); //beep the buzzer
    delay(1000);
    analogWrite(WIO_BUZZER, 0); //Silence the buzzer
    delay(1000);
    digitalWrite(D0, HIGH); // Relay Switch on
  }else {
    digitalWrite(D0, LOW); // Relay Switch off
  }

  spr.pushSprite(0,0); //Push to LCD
  delay(50);
    }

}
