#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Ultrasonic.h>
#include <EEPROM.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

//Blynk preparation
#define BLYNK_TEMPLATE_ID "TMPLm4KZ8qqx"
#define BLYNK_DEVICE_NAME "water level sensor"
#define BLYNK_AUTH_TOKEN "aVPFWO9LRIJAeR8XL_5qTNISE7ZCYn6l"


#define TRIG_PIN 5 // ESP32 pin GIOP23 connected to Ultrasonic Sensor's TRIG pin
#define ECHO_PIN 18 // ESP32 pin GIOP22 connected to Ultrasonic Sensor's ECHO pin

#define EEPROM_SIZE 3

LiquidCrystal_I2C lcd(0x3F, 16, 2);
const int ledPin = 32;
const int buzzer = 12;

float duration, distance_cm;


int h1 = 0; //distance from sensor to 100%                          | V2
int h2 = 0; //distance from 0% to current liquid level in cm 
int h3 = 0; //distance from sensor to 0%                            | V3
float liq_level = 0; //distance from 0% to current liquid level     | V1
int liq_level_percent = 0; //liquid level percentage in container   | V5
int notify_percent = 0; //percentage value limit for alarming       | V4

int calibr0 = 0, calibr100 = 0;
float calibrSum = 0;


BlynkTimer timer;

void myTimer(){
  Blynk.virtualWrite(V1, liq_level);
  Blynk.virtualWrite(V5, liq_level_percent);
  Blynk.virtualWrite(V3, h3);
  Blynk.virtualWrite(V2, h1);
}

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);
  Blynk.begin(BLYNK_AUTH_TOKEN, "Galaxy A53 5G B530", "ppix5120", "blynk.cloud", 80);
  timer.setInterval(1000L, myTimer);

  //Ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  ///External LED
  pinMode(ledPin, OUTPUT);
  pinMode(buzzer, OUTPUT);

  h1 = EEPROM.read(0);
  h3 = EEPROM.read(1);
  notify_percent = EEPROM.read(2);

  Blynk.virtualWrite(V4, notify_percent);
  Blynk.virtualWrite(V32, 0);
  Blynk.virtualWrite(V12, 0);

  //16x2 LCD Display
  lcd.init();
  lcd.clear();
  lcd.backlight();
}

//LED switch
int LEDstate = 0;
BLYNK_WRITE(V32){
  LEDstate = param.asInt();
}

//BUZZER switch
int BUZZERstate = 0;
BLYNK_WRITE(V12){
  BUZZERstate = param.asInt();
}

//get distance from sensor to 100% from app
BLYNK_WRITE(V2){
  h1 = param.asInt();
}

//get distance from sensor to 0% from app
BLYNK_WRITE(V3){
  h3 = param.asInt();
}

//get limit % value from app
BLYNK_WRITE(V4){
  notify_percent = param.asInt();
}


BLYNK_WRITE(V10){
  if(calibr0 == 0 && (param.asInt() == 1)){
    calibr100 = 1;
  }
}

BLYNK_WRITE(V11){
  if(calibr100 == 0 && (param.asInt() == 1)){
    calibr0 = 1;
  }
}

//set liquid level percentage in app
void loop() {
  //Blynk
  Blynk.run();
  timer.run();
  
  // generate 20-microsecond pulse to TRIG pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(TRIG_PIN, LOW);
  
  // measure duration of pulse from ECHO pin
  duration = pulseIn(ECHO_PIN, HIGH);

  if(calibr0 == 1){
    calibrSum = 0;
    distance_cm = (duration / 2) * 0.034;
    calibrSum = distance_cm;

    delay(200);
     // generate 20-microsecond pulse to TRIG pin
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(20);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH);
    delay(200);

    distance_cm = (duration / 2) * 0.034;
    calibrSum = calibrSum + distance_cm;

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(20);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH);
    delay(200);

    distance_cm = (duration / 2) * 0.034;
    calibrSum = calibrSum + distance_cm;

    Blynk.virtualWrite(V11, 0);
    calibr0 = 0;
    h3 = calibrSum / 3;
    delay(100);
  }

  else if (calibr100 == 1){
    calibrSum = 0;
    distance_cm = (duration / 2) * 0.034;
    calibrSum = distance_cm;

    delay(200);
     // generate 20-microsecond pulse to TRIG pin
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(20);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH);
    
    delay(200);
    distance_cm = (duration / 2) * 0.034;
    calibrSum = calibrSum + distance_cm;

    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(20);
    digitalWrite(TRIG_PIN, LOW);
    duration = pulseIn(ECHO_PIN, HIGH);

    delay(200);
    distance_cm = (duration / 2) * 0.034;
    calibrSum = calibrSum + distance_cm;

    Blynk.virtualWrite(V10, 0);
    calibr100 = 0;
    h1 = calibrSum / 3;
    delay(100);
    //Blynk.virtualWrite(V2, h1);
  }

  // calculate the distance
  //distance_cm = 0.017 * duration_us;
  distance_cm = (duration / 2) * 0.034;
  h2 = distance_cm;
  
  //h2 = (h3 - h1) - (h3 - h2); //liquid level in cm
  if(h3 - h1 > 0){
    liq_level = h3 - h2; //liquid level in cm
    liq_level_percent = liq_level / (h3 - h1) * 100; //liquid level in %
  }

  if(liq_level_percent >= notify_percent){
    if(LEDstate == 1){
      digitalWrite(ledPin, HIGH);
    }
    else{
      digitalWrite(ledPin, LOW);
    }
    if(BUZZERstate == 1){
      digitalWrite(buzzer, HIGH);
    }
    else{
      digitalWrite(buzzer, LOW);
    }
  }
  else{
    digitalWrite(ledPin, LOW);
    digitalWrite(buzzer, LOW);
  }

  // print the value to LCD Display
  lcd.setCursor(0, 0);
  lcd.print("Level: ");
  lcd.print(liq_level);
  lcd.print(" cm");

  lcd.setCursor(0, 1);
  lcd.print("            ");
  if(liq_level_percent > 0 && liq_level_percent <= 100){
    if(liq_level_percent >=0 && liq_level_percent < 10){
      lcd.print("  ");
      lcd.print(liq_level_percent);
      lcd.print("%");
    }
    else if (liq_level_percent == 100){
      lcd.print(liq_level_percent);
      lcd.print("%");
    }
    else{
      lcd.print(" ");
      lcd.print(liq_level_percent);
      lcd.print("%");
    }
  }
  else{
    lcd.print("  0%");
  }
  delay(50);

  
  EEPROM.write(0, h1);
  EEPROM.write(1, h3);
  EEPROM.write(2, notify_percent);
  EEPROM.commit();
}

