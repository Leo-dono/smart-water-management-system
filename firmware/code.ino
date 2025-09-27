#include <EEPROM.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include <TimeLib.h> 
#include <OneWire.h> 
#include <DallasTemperature.h>
#include "GravityTDS.h"
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
//#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
//#endif
#define ONE_WIRE_BUS 30
#define CHECK_INTERVAL 1000 
#include <SoftwareSerial.h>
#include <ThreeWire.h>  
#include <RtcDS1302.h>
ThreeWire myWire(18,7,19);
RtcDS1302<ThreeWire> Rtc(myWire);
OneWire oneWire(ONE_WIRE_BUS);
GravityTDS gravityTds;
DallasTemperature sensors(&oneWire);
U8G2_ST7920_128X64_1_HW_SPI u8g2(U8G2_R0, /* CS=*/ 10, /* reset=*/ 8);
volatile bool resetInterruptFlag = false;
volatile bool shutdownInterruptFlag = false;
float ph_calibration_value = 25.34;
unsigned long int avgval; 
int buffer_arr[10],phtemp;
String leakageSection;
String quality;
const int volumeEEPROMAddr = 0; 
const int volumeTodayEEPROMAddr = 0;
const int volumeWeekEEPROMAddr = sizeof(float);
const int volumeMonthEEPROMAddr = sizeof(float) * 2;
const int volumeYearEEPROMAddr = sizeof(float) * 3;
float volumeToday = 0;
float volumeWeek = 0;
float volumeMonth = 0;
float volumeYear = 0;
const int sValveMain = 44; 
const int sValveB = 11; 
const int sValveC = 45; 
const int sValveRelief = 12; 
#define TdsSensorPin A7
const int psensA_pin = A0; 
const int psensB_pin = A1; 
const int psensC_pin = A2; 
const int phPin = A3;
const int turbidityPin=A5;
const int conductivityPin = A7;
int flowSensorPin = 2;
int resetPin = 6;
int softResetPin = 15;
const int hallSensorPin = A6; 
const int greenIndPin = 39; 
const int redIndPin = 5; 
const int buzzerPin = 38;
const int shutDownPin = 13;
volatile long pulse;
float pressureA=0.0;
float pressureB=0.0;
float pressureC=0.0;
float temp =0.0 ;
float average_pressure=0.0;
float conductivity=0.0; 
float tds = 0.0;
float turbidity = 0.0;
float ph=0;
const float pressureTreshhold = 5.2;
const float temperatureTreshhold= 28.0;
const float turbidityTreshhold = 15.0;
const float conductivityTreshhold = 1000.0;
const float phTreshhold = 5.0;
bool lcdPrinted = false;
volatile unsigned long pulseCount = 0; 
unsigned long oldMillis = 0;        
float calibrationFactor = 15; 
float flowRate = 0.0;              
float totalFlow = 0.0;
float volume = 0.0;
float totalPh = 0.0;
int numReadings = 100; 
int hallSensorState = digitalRead(hallSensorPin);
int flowSensorState = digitalRead(flowSensorPin);
int resetPinState = digitalRead(resetPin);
#include <RTClib.h>
RTC_DS3231 rtc;
 char volumestr[20];
#define countof(a) (sizeof(a) / sizeof(a[0]))

void sendDataToSlave(String leakageSection,String quality ) {
  String data = String(temp) + "," + String(conductivity) + "," + String(tds) + "," +
                String(pressureA) + "," + String(pressureB) + "," + String(pressureC) + "," +
                String(ph) + "," + String(flowRate) + "," + String(volume) + "," +String(volume) + "," +String(volume) + "," +
                String(turbidity) ;
  slaveSerial.println(data);
}

void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

void shutdownISR() {
  shutdownInterruptFlag = true; // Set the shutdown flag
}

void handleShutdown() {
  digitalWrite(sValveMain, HIGH);
   DateTime now = rtc.now(); // Get the current time from the RTC
  char timeStr[20];
  sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
  u8g2.drawStr(0, 10, timeStr);

  u8g2.firstPage();
  do {
    u8g2.drawStr(0, 10, timeStr); 
    char volumestr[20];
    dtostrf(volume, 8, 2, volumestr);
    u8g2.drawStr(0, 20, "Total: ");
    u8g2.drawStr(50, 20, volumestr);
    u8g2.drawStr(120, 20, "L");
    u8g2.drawStr(0, 30, "SYSTEM SHUT!!!!");
    u8g2.drawStr(0, 40, "SHUT DOWN MODE");
    u8g2.drawStr(0, 50, "ACTIVATED");
    u8g2.drawStr(0, 60, "!!!!!!!!!!!");
  } while (u8g2.nextPage());
  while(true){
    int resetPinState = digitalRead(resetPin);
    if (resetInterruptFlag|| resetPinState == HIGH) {
    resetInterruptFlag = false;
    handleReset();
    break;
  }
    }
  shutdownInterruptFlag = false;
  
}

void resetISR() {
  resetInterruptFlag = true; // Set the flag
}

void handleReset() {
  digitalWrite(buzzerPin, LOW);
  Serial.println("broken loop");
  resetInterruptFlag = false;
  // Reset system by turning off valves
  digitalWrite(sValveMain, LOW);
  delay(200);
  digitalWrite(sValveRelief, LOW);
  delay(200);
  digitalWrite(sValveB, LOW);
  delay(200);
  digitalWrite(sValveC, LOW);
  delay(200);
  digitalWrite(greenIndPin, HIGH);
  delay(200);
  digitalWrite(redIndPin, LOW);

  DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);
  u8g2.firstPage();
  do {
    u8g2.drawStr(0, 10, timeStr); 
    char volumestr[20];
    dtostrf(volume, 8, 2, volumestr);
    u8g2.drawStr(0, 20, "Total: ");
    u8g2.drawStr(50, 20, volumestr);
    u8g2.drawStr(120, 20, "L");
    u8g2.drawStr(0, 30, "SYSTEM NORMALIZED!!!!");
    u8g2.drawStr(0, 40, "NORMAL MODE");
    u8g2.drawStr(0, 50, "NORMAL MODE");
    u8g2.drawStr(0, 60, "ACTIVATED");
  } while (u8g2.nextPage());

  sendDataToSlave( leakageSection, quality );

  // Delay before returning to the main loop
  delay(100);
}

void resetEEPROMValue() {
  EEPROM.write(volumeEEPROMAddr, 0); // Write zero to the EEPROM address
}

void increase() {
  pulse++;
}

void flowInterrupt() {
  pulseCount++;
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

String getSenderNumber(String message) {
  if (message.length() < 21) { // Handle messages shorter than expected format
    return allowedNumber1;
  }

  // Remove the first 5 characters
  String modifiedMessage = message.substring(5);

  // Find the index of the next 15 characters after the first quote
  int quoteIndex = modifiedMessage.indexOf('"') + 1;
  if (quoteIndex == 0) { // No quotes found, return original message
    return message;
  }
  int newEndIndex = quoteIndex + 15;

  // Remove characters after the next 15 characters after the first quote
  return modifiedMessage.substring(0, newEndIndex);
}

float readPressure(int pin) {
  int sensorValue = analogRead(pin); // read the analog input value
  float voltage = sensorValue * (5.0 / 1023.0); // convert analog value to voltage (assuming 5V reference)
  float pressure = mapFloat(voltage, 0.48 , 4.46, 0.0, 4.0); // map voltage range to pressure range
  return pressure;
}
   
void detectLeakage() {
  digitalWrite(buzzerPin, HIGH);
    quality = "normal water quality";
    String leakageSection = "";
   digitalWrite(greenIndPin,LOW );
   digitalWrite(redIndPin, HIGH);
  // Leakage detected
   Serial.println("Leakage detected");
  
    u8g2.firstPage();
    do {
      DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);
      dtostrf(volume, 8, 2, volumestr);
            u8g2.drawStr(0, 20, "Total: ");
            u8g2.drawStr(50, 20, volumestr);
            u8g2.drawStr(120, 20, "L");
      u8g2.drawStr(0, 30, "LEAKAGE DETECTED!!!!!!!!");
      u8g2.drawStr(0, 40, "IN LEAKAGE DETECTION MODE");
      u8g2.drawStr(0, 50, "SEARCHING FOR LEAKAGE");
      u8g2.drawStr(0, 60, "SECTION !!!!!!!!");
    } while (u8g2.nextPage());
    
  
    
   int previousPressureA=pressureA;
  int previousPressureB=pressureB;
  int previousPressureC=pressureC;

    digitalWrite(sValveMain, HIGH);
     digitalWrite(sValveB, HIGH);
     digitalWrite(sValveC, HIGH);
   delay(10000);


  float newpressureA = readPressure(psensA_pin); // read pressure value for psensA
  float newpressureB = readPressure(psensB_pin); // read pressure value for psensB
  float newpressureC = readPressure(psensC_pin); // read pressure value for psensC
  
  while (true) {
  int resetPinState = digitalRead(resetPin);
    if (resetInterruptFlag|| resetPinState == HIGH) {
    resetInterruptFlag = false;
    handleReset();
    break;
  }

  if (!lcdPrinted) {
  Serial.println("inside leakage loop");

  if (newpressureA < previousPressureA) {
    leakageSection = "Section A";
    sendDataToSlave( leakageSection, quality );

    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_ncenB08_tr);
       DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);
      dtostrf(volume, 8, 2, volumestr);
            u8g2.drawStr(0, 20, "Total: ");
            u8g2.drawStr(50, 20, volumestr);
            u8g2.drawStr(120, 20, "L");
      u8g2.drawStr(0, 30, "LEAKAGE DETECTED!!!!!!!!");
      u8g2.drawStr(0, 40, "IN LEAKAGE DETECTION MODE");
      u8g2.drawStr(0, 50, "LEAKAGE FOUND IN");
      u8g2.drawStr(0, 60, "SECTION A!!!");
    } while (u8g2.nextPage());
  }
  else if (newpressureB < previousPressureB) {
    leakageSection = "Section B";
    sendDataToSlave( leakageSection, quality );
    
    Serial.println("Pressure sensor B is reducing!");
   
    u8g2.firstPage();
    do {
       DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 10, timeStr); // Draw time and date
      char str[20];
      sprintf(str, "totalizer= %lu Litres", volume);
      u8g2.drawStr(0, 20, str);
      u8g2.drawStr(0, 30, "LEAKAGE DETECTED!!!!!!!!");
      u8g2.drawStr(0, 40, "IN LEAKAGE DETECTION MODE");
      u8g2.drawStr(0, 50, "LEAKAGE FOUND IN");
      u8g2.drawStr(0, 60, "SECTION B!!!");
    } while (u8g2.nextPage());
  }
  else if (newpressureC < previousPressureC) {
    leakageSection = "Section C";
    sendDataToSlave( leakageSection, quality );

    Serial.println("Pressure sensor C is reducing!");
    
    u8g2.firstPage();
    do {
       DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 10, timeStr); // Draw time and date
      dtostrf(volume, 8, 2, volumestr);
            u8g2.drawStr(0, 20, "Total: ");
            u8g2.drawStr(50, 20, volumestr);
            u8g2.drawStr(120, 20, "L");
      u8g2.drawStr(0, 30, "LEAKAGE DETECTED!!!!!!!!");
      u8g2.drawStr(0, 40, "IN LEAKAGE DETECTION MODE");
      u8g2.drawStr(0, 50, "LEAKAGE FOUND IN");
      u8g2.drawStr(0, 60, "SECTION C!!!");
    } while (u8g2.nextPage());
  }
  else if (newpressureC < previousPressureC && newpressureA < previousPressureA) {
    leakageSection = "Section A AND C";
    sendDataToSlave( leakageSection, quality );

    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_ncenB08_tr);
       DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);
      dtostrf(volume, 8, 2, volumestr);
            u8g2.drawStr(0, 20, "Total: ");
            u8g2.drawStr(50, 20, volumestr);
            u8g2.drawStr(120, 20, "L");
      u8g2.drawStr(0, 30, "LEAKAGE DETECTED!!!!!!!!");
      u8g2.drawStr(0, 40, "IN LEAKAGE DETECTION MODE");
      u8g2.drawStr(0, 50, "LEAKAGE FOUND IN");
      u8g2.drawStr(0, 60, "SECTION A AND C!!!");
    } while (u8g2.nextPage());
  }
  else if (newpressureB < previousPressureB && newpressureA < previousPressureA) {
    leakageSection = "Section A AND B";
    sendDataToSlave( leakageSection, quality );
    
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_ncenB08_tr);
    DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);      char str[20];
      dtostrf(volume, 8, 2, volumestr);
            u8g2.drawStr(0, 20, "Total: ");
            u8g2.drawStr(50, 20, volumestr);
            u8g2.drawStr(120, 20, "L");
      u8g2.drawStr(0, 30, "LEAKAGE DETECTED!!!!!!!!");
      u8g2.drawStr(0, 40, "IN LEAKAGE DETECTION MODE");
      u8g2.drawStr(0, 50, "LEAKAGE FOUND IN");
      u8g2.drawStr(0, 60, "SECTION A AND B!!!");
    } while (u8g2.nextPage());
  }
  else if (newpressureC < previousPressureC && newpressureB < previousPressureB) {
    leakageSection = "Section B AND C";
    sendDataToSlave( leakageSection, quality );

    
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_ncenB08_tr);
      DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);      char str[20];
      dtostrf(volume, 8, 2, volumestr);
            u8g2.drawStr(0, 20, "Total: ");
            u8g2.drawStr(50, 20, volumestr);
            u8g2.drawStr(120, 20, "L");
      u8g2.drawStr(0, 30, "LEAKAGE DETECTED!!!!!!!!");
      u8g2.drawStr(0, 40, "IN LEAKAGE DETECTION MODE");
      u8g2.drawStr(0, 50, "LEAKAGE FOUND IN");
      u8g2.drawStr(0, 60, "SECTION B AND C!!!");
    } while (u8g2.nextPage());
  }
  else if (newpressureC < previousPressureC && newpressureB < previousPressureB && newpressureA < previousPressureA) {
    leakageSection = "Section A, B AND C";
    sendDataToSlave( leakageSection, quality );

    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", hour(), minute(), second(), day(), month(), year());
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 10, timeStr); // Draw time and date
      dtostrf(volume, 8, 2, volumestr);
            u8g2.drawStr(0, 20, "Total: ");
            u8g2.drawStr(50, 20, volumestr);
            u8g2.drawStr(120, 20, "L");
      u8g2.drawStr(0, 30, "LEAKAGE DETECTED!!!!!!!!");
      u8g2.drawStr(0, 40, "IN LEAKAGE DETECTION MODE");
      u8g2.drawStr(0, 50, "LEAKAGE FOUND IN");
      u8g2.drawStr(0, 60, "SECTION A, B AND C!!!");
    } while (u8g2.nextPage());
    }

  lcdPrinted = true; // Set flag to true after printing
    }

    sendDataToSlave( leakageSection, quality );
    }
    
    delay(100);
    lcdPrinted = false;
}
   
void poorWaterQuality(){
  digitalWrite(buzzerPin, HIGH);
   digitalWrite(greenIndPin,LOW );
    digitalWrite(redIndPin, HIGH);
    digitalWrite(sValveMain, HIGH);
    //quality = "";
    leakageSection = "No Leakagae Detected";
    Serial.println("POOR WATER QUALITY detected");
  while(true){
    int resetPinState = digitalRead(resetPin);
    if (resetInterruptFlag|| resetPinState == HIGH) {
    resetInterruptFlag = false;
    handleReset();
    break;
  }

   if (!lcdPrinted) {
    Serial.println("inside poor quality loop");
    char tempStr[10], conductivityStr[10], tdsStr[10], phStr[10],turbidityStr[10];
    dtostrf(temp, 6, 1, tempStr);
    dtostrf(conductivity, 6, 1, conductivityStr);
    dtostrf(turbidity, 6, 1, turbidityStr);
    dtostrf(ph, 6, 1, phStr);

    // Check for high temperature
    if (temp > temperatureTreshhold) {
       

        u8g2.firstPage();
        do {
             DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);
            char str[30];
            char volumestr[20];
            dtostrf(volume, 8, 2, volumestr);
            u8g2.drawStr(0, 20, "Total: ");
            u8g2.drawStr(50, 20, volumestr);
            u8g2.drawStr(120, 20, "L");
            u8g2.drawStr(0, 30, "Temperature: ");
            u8g2.drawStr(70, 30, tempStr);
            u8g2.drawStr(110, 30, "°C");
            u8g2.drawStr(0, 40, "WARNING!!!!!!!!");
            u8g2.drawStr(0, 50, "TEMPERATURE ");
            u8g2.drawStr(0, 60, "ABNORMAL!!!!!!");
        } while (u8g2.nextPage());

         quality = "temperature of " + String(tempStr) + " is too high";
        sendDataToSlave( leakageSection, quality );
    }

    // Check for low pH
    if (ph < phTreshhold) {        
                u8g2.firstPage();
        do {
             DateTime now = rtc.now(); // Get the current time from the RTC
        char timeStr[20];
        sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
        u8g2.drawStr(0, 10, timeStr);
            char volumestr[20];
            dtostrf(volume, 8, 2, volumestr);
            u8g2.drawStr(0, 20, "Total: ");
            u8g2.drawStr(50, 20, volumestr);
            u8g2.drawStr(120, 20, "L");
            u8g2.drawStr(0, 30, "PH: ");
            u8g2.drawStr(70, 30, phStr);
            u8g2.drawStr(100, 30, " ");;
            u8g2.drawStr(0, 40, "WARNING!!!!!!!!");
            u8g2.drawStr(0, 50, "PH ABNORMAL");
            u8g2.drawStr(0, 60, "!!!!!!!!!");
        } while (u8g2.nextPage());

        quality = "pH of " + String(ph) + " is too low";
        sendDataToSlave( leakageSection, quality );
    }

    // Check for high conductivity
    if (conductivity > conductivityTreshhold) {        
        
        u8g2.firstPage();
        do {
             DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);
            dtostrf(volume, 8, 2, volumestr);
            u8g2.drawStr(0, 20, "Total: ");
            u8g2.drawStr(50, 20, volumestr);
            u8g2.drawStr(120, 20, "L");
            u8g2.drawStr(0, 30, "Conductivity: ");
            u8g2.drawStr(0, 40, conductivityStr);
            u8g2.drawStr(60, 40, "µS/cm");
            u8g2.drawStr(0, 50, "WARNING!!!!!!!!");
            u8g2.drawStr(0, 60, "CONDUCTIVITY ");
            u8g2.drawStr(0, 70, "ABNORMAL!!!!");
        } while (u8g2.nextPage());

         quality = "conductivity of " + String(conductivity) + " is too high";
        sendDataToSlave( leakageSection, quality );
    }

    // Check for high turbidity
    if (turbidity > turbidityTreshhold) {
       
        u8g2.firstPage();
        do {
             DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);
            dtostrf(volume, 8, 2, volumestr);
            u8g2.drawStr(0, 20, "Total: ");
            u8g2.drawStr(50, 20, volumestr);
            u8g2.drawStr(120, 20, "L");
            u8g2.drawStr(0, 30, "turbidity: ");
            u8g2.drawStr(70, 30, turbidityStr);
            u8g2.drawStr(110, 30, "°NTU");
            u8g2.drawStr(0, 40, "WARNING!!!!!!!!");
            u8g2.drawStr(0, 50, "TURBIDITY ");
            u8g2.drawStr(0, 60, "ABNORMAL!!!");
        } while (u8g2.nextPage());

        quality = "turbidity of " + String(turbidity) + " is too high";
        sendDataToSlave( leakageSection, quality );
    }

    lcdPrinted = true; // Set flag to true after printing
}
  delay(100);

  }      
  lcdPrinted = false; // Set flag to true after printing
}

void highPressure() {
    digitalWrite(greenIndPin, LOW);
    digitalWrite(redIndPin, HIGH);
    digitalWrite(buzzerPin, HIGH);
    Serial.println("high pressure detected");
    quality = "normal water quality";
    String leakageSection = "";
    digitalWrite(sValveRelief, HIGH);
    sendDataToSlave( leakageSection, quality );

    while (true) {
        int pressureA = analogRead(psensA_pin); // Read pressure from pin A
        int pressureB = analogRead(psensB_pin); // Read pressure from pin B
        int pressureC = analogRead(psensC_pin); // Read pressure from pin C
        int average_pressure = (pressureA + pressureB + pressureC) / 3; // Calculate average pressure

        
        do {
             DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);
            char str[20];
            sprintf(str, "Avg Pressure: %d", average_pressure);
            u8g2.drawStr(0, 20, str); // Display avgPressure
            u8g2.drawStr(0, 30, "Pressure Too High!"); // Display the message
            u8g2.drawStr(0, 40, "Pressure Warning"); // Display warning
        } while (u8g2.nextPage());

        
        
        if (resetInterruptFlag||average_pressure < pressureTreshhold|| resetPinState == HIGH) {
            resetInterruptFlag = false;
            handleReset();
            break;
          }
 

        delay(1000); 
    }

    Serial.println("Pressure normalized, exiting highPressure function.");
}

void setup(void) {
 Serial.begin(9600); // Initialize serial communication
  slaveSerial.begin(9600);
  Rtc.Begin();
  Serial.println("Initializing...");
  EEPROM.get(volumeEEPROMAddr, volume);
  Serial.println("Totalizer value loaded from EEPROM:");
  Serial.print(volume);
  Serial.println(" L");
  EEPROM.get(volumeTodayEEPROMAddr, volumeToday);
  EEPROM.get(volumeWeekEEPROMAddr, volumeWeek);
  EEPROM.get(volumeMonthEEPROMAddr, volumeMonth);
  EEPROM.get(volumeYearEEPROMAddr, volumeYear);
  pinMode(resetPin, INPUT);
  pinMode(flowSensorPin, INPUT_PULLUP); // Set flow sensor pin as input with pull-up
  attachInterrupt(digitalPinToInterrupt(flowSensorPin), flowInterrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(resetPin), resetISR, RISING);
  attachInterrupt(digitalPinToInterrupt(shutDownPin), shutdownISR, RISING);
  pinMode(hallSensorPin, INPUT);


    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  u8g2.begin();

      // LCD Welcome Display with Rectangle Filling Graphic and Percentage
      u8g2.firstPage();
      do {
        u8g2.setFont(u8g2_font_ncenB08_tr); // Smaller font for the welcome message
      
        DateTime now = rtc.now(); // Get the current time from the RTC
          char timeStr[20];
          sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
          u8g2.drawStr(0, 10, timeStr);
      
        // Stylish welcome message
        u8g2.drawStr(0, 30, "Welcome to Smart");
        u8g2.drawStr(0, 40, " Water Meter System");
        u8g2.drawStr(0, 50, "Totalizer:");
        char volumestr[20];
          dtostrf(volume, 10, 2, volumestr);
          u8g2.drawStr(0, 50, "Total: ");
          u8g2.drawStr(35, 50, volumestr);
          u8g2.drawStr(120, 50, "L");
      } while (u8g2.nextPage());
      
      delay(2000); // Delay before showing the initializing message
      
      // Display initializing message with rectangle filling graphic
      for (int i = 0; i <= 128; i += 8) { // Adjust the increment value for smoother or faster filling
        u8g2.firstPage();
        do {
          char volumestr[20];
          dtostrf(volume, 10, 2, volumestr);
          u8g2.drawStr(0, 10, "Total: ");
          u8g2.drawStr(35, 10, volumestr);
          u8g2.drawStr(120, 10, "L");
          u8g2.setFont(u8g2_font_ncenB08_tr);
          u8g2.drawStr(0, 30, "Initializing...");
      
          // Draw the loading bar frame
          u8g2.drawFrame(0, 50, 128, 10); // Position and size of the loading bar frame
      
          // Draw the filling rectangle
          u8g2.drawBox(0, 50, i, 10); // Fill the rectangle as the loop progresses
      
          // Calculate and display percentage
          int percentage = (i * 100) / 128;
          char percentStr[6];
          sprintf(percentStr, "%d%%", percentage);
          u8g2.drawStr(55, 40, percentStr); // Adjust position as needed
      
        } while (u8g2.nextPage());

  delay(100); // Delay for the loading effect, adjust for speed of the filling
  }

  sensors.begin();
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);
  gravityTds.setAdcRange(1024);
  gravityTds.begin();

  pinMode(hallSensorPin, INPUT_PULLUP);
  pinMode(resetPin, INPUT_PULLUP);
  pinMode(softResetPin, OUTPUT);
  pinMode(shutDownPin, INPUT);
  pinMode(conductivityPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(sValveMain, OUTPUT);
  pinMode(sValveB, OUTPUT);
  pinMode(sValveC, OUTPUT);
  pinMode(sValveRelief, OUTPUT);
  digitalWrite(sValveMain, LOW);
  digitalWrite(sValveB, LOW);
  digitalWrite(sValveC, LOW);
  digitalWrite(sValveRelief, LOW);
  digitalWrite(softResetPin, LOW);

  pinMode(greenIndPin, OUTPUT); // Set green LED pin as output
  pinMode(redIndPin, OUTPUT); // Set red LED pin as output
    
  Serial.println("Initializing...");
  delay(1000);
  }

void loop(void) {
  Serial.println("IN MAIN LOOP ");
  digitalWrite(greenIndPin, HIGH);
  digitalWrite(redIndPin, LOW);
 
  unsigned long currentMillis = millis();
    if (currentMillis > oldMillis ) {
        float elapsedTime = (currentMillis - oldMillis) / 1000.0; // Time in seconds
        if (elapsedTime > 0) {
            flowRate = (pulseCount / calibrationFactor) / elapsedTime * 60 ;
            volume += (flowRate * elapsedTime / 1000.0);
        }
         int hallSensorState = digitalRead(hallSensorPin);
         Serial.print("hallSensorState : ");
        Serial.println(hallSensorState);
        
        if (hallSensorState == 0 && pulseCount > 0) {
//        if (hallSensorState == HIGH && flowRate > 0) {
            detectLeakage();
        }
        pulseCount = 0;
        oldMillis = currentMillis;
    }
               
        // Print the flow rate and volume
        Serial.print("Flow Rate: ");
        Serial.print(flowRate, 2);
        Serial.print(" L/min");
        Serial.print("  Total Flow: ");
        Serial.print(volume,2);
        Serial.println(" L");

        

        EEPROM.put(volumeEEPROMAddr, volume);

  // TEMPERATURE SENSOR
  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);
  gravityTds.setTemperature(sensors.getTempCByIndex(0));  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate
  tds = gravityTds.getTdsValue();  // then get the value
  conductivity = tds / 0.7;

  Serial.println(conductivity);
  Serial.println(tds);
  Serial.println(temp);

  // PRESSURE SENSORS
  pressureA = readPressure(psensA_pin); // read pressure value for psensA
  pressureB = readPressure(psensB_pin); // read pressure value for psensB
  pressureC = readPressure(psensC_pin); // read pressure value for psensC

  Serial.print("Pressure psensA: ");
  Serial.print(pressureA); // print the pressure value for psensA with 2 decimal places
  Serial.println(" MPa");

  Serial.print("Pressure psensB: ");
  Serial.print(pressureB, 2); // print the pressure value for psensB with 2 decimal places
  Serial.println(" MPa");

  Serial.print("Pressure psensC: ");
  Serial.print(pressureC, 2); // print the pressure value for psensC with 2 decimal places
  Serial.println(" MPa");

  //PH SENSOR
       for(int i=0;i<10;i++) 
         { 
         buffer_arr[i]=analogRead(phPin);
         delay(30);
         }
         for(int i=0;i<9;i++)
         {
         for(int j=i+1;j<10;j++)
         {
         if(buffer_arr[i]>buffer_arr[j])
         {
         phtemp=buffer_arr[i];
         buffer_arr[i]=buffer_arr[j];
         buffer_arr[j]=phtemp;
         }
         }
         }
         avgval=0;
         for(int i=2;i<8;i++)
         avgval+=buffer_arr[i];
         float volt=(float)avgval*5.0/1024/6;
         float ph = -5.70 * volt + ph_calibration_value;
         Serial.print("pH Val:");
         Serial.println(ph);
  
//TURBIDITY SENSOR


  // Incoming Messages and Quality Check
  String quality = "Normal Water Quality";
  String leakageSection = "No Leakage";
  sendDataToSlave( leakageSection, quality );

average_pressure=(pressureA+pressureB+pressureC)/3;
if (average_pressure>pressureTreshhold){
  highPressure();
}


//POOR WATER QUALITY DETECTION
if (ph<phTreshhold || turbidity > turbidityTreshhold || conductivity>conductivityTreshhold || temp>temperatureTreshhold){
  poorWaterQuality();
}

int shutDownPinState = digitalRead(shutDownPin);
    if (shutdownInterruptFlag || shutDownPinState == HIGH) {
    shutdownInterruptFlag  = false;
    handleShutdown();
  }
  

  // First Page Display
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr); // Adjust font size here

    DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);

    char volumestr[20];
    dtostrf(volume, 10, 2, volumestr);
    u8g2.drawStr(0, 20, "Total: ");
    u8g2.drawStr(35, 20, volumestr);
    u8g2.drawStr(120, 20, "L");

    char tempStr[10], conductivityStr[10], flowrateStr[10], phStr[10],tdsStr[10];
    dtostrf(temp, 6, 1, tempStr);
    dtostrf(temp, 6, 1, tdsStr);
    dtostrf(conductivity, 6, 1, conductivityStr);
    dtostrf(flowRate, 6, 2, flowrateStr);
    dtostrf(ph, 1, 1, phStr);

    u8g2.drawStr(0, 30, "Flow: ");
    u8g2.drawStr(50, 30, flowrateStr);
    u8g2.drawStr(90, 30, "L/min");

    u8g2.drawStr(0, 40, "Temperature: ");
    u8g2.drawStr(70, 40, tempStr);
    u8g2.drawStr(110, 40, "°C");

    u8g2.drawStr(0, 50, "Cond.ty: ");
    u8g2.drawStr(60, 50, conductivityStr);
    u8g2.drawStr(100, 50, "µS/cm");

    u8g2.drawStr(0, 60, "PH: ");
    u8g2.drawStr(25, 60, phStr);
    u8g2.drawStr(45, 60, "TDS:");
    u8g2.drawStr(70, 60, tdsStr);
    u8g2.drawStr(100, 60, "mg/L");

  } while (u8g2.nextPage());

  delay(5000); 

  u8g2.firstPage();
  do {
    char pressureA_str[10], pressureB_str[10], pressureC_str[10];
dtostrf(pressureA, 5, 1, pressureA_str); // Convert pressureA to a string with 6 total characters and 2 decimal places
dtostrf(pressureB, 5, 1, pressureB_str); // Convert pressureB to a string with 6 total characters and 2 decimal places
dtostrf(pressureC, 5, 1, pressureC_str); // Convert pressureC to a string with 6 total characters and 2 decimal places

char turbidity_str[10];
dtostrf(turbidity, 5, 1, turbidity_str); 

    DateTime now = rtc.now(); // Get the current time from the RTC
    char timeStr[20];
    sprintf(timeStr, "%02d:%02d:%02d %02d/%02d/%04d", now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    u8g2.drawStr(0, 10, timeStr);
    char str[30];
    char volumestr[20];
    dtostrf(volume, 8, 2, volumestr);
    u8g2.drawStr(0, 20, "Total: ");
    u8g2.drawStr(50, 20, volumestr);
    u8g2.drawStr(120, 20, "L");
      sprintf(str, "Turbidity: %s NTU", turbidity_str);
      u8g2.drawStr(0, 30, str);
      u8g2.drawStr(0, 40, "Pressure A: "); // Display label
      u8g2.drawStr(60, 40, pressureA_str); // Display pressure value
      u8g2.drawStr(100, 40, "MPa"); // Display unit
      u8g2.drawStr(0, 50, "Pressure B: "); // Display label
      u8g2.drawStr(60, 50, pressureB_str); 
      u8g2.drawStr(100, 50, "MPa");
      u8g2.drawStr(0, 60, "Pressure C: "); 
      u8g2.drawStr(60, 60, pressureC_str); 
      u8g2.drawStr(100, 60, "MPa"); 
      } while (u8g2.nextPage());
      delay(5000);
}
