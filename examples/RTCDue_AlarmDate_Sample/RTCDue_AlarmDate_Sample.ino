/*
  Simple Alarm Example for Arduino Due

  Demonstrates the use of the RTC library for the Arduino Due

  This example code is in the public domain

  created by Markus Lange
  12 Feb 2016
*/

#include <RTCDue.h>

const int ledPin =  13;      // the number of the LED pin

int ledState = LOW;          // ledState used to set the LED
long previousMillis = 0;     // will store last time LED was updated
long interval = 250;         // interval at which to blink (milliseconds)

// Select the Slowclock source
//RTCDue rtc(RC);
RTCDue rtc(XTAL);

const char* daynames[]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);

  rtc.begin();
  rtc.setTime(23, 59, 56);
  rtc.setDate(22, 11, 2016);
  rtc.setAlarmDate(23, 11);
  
  rtc.attachAlarm(announcement);
  //rtc.disableAlarmDate();
}

void loop() {
  blinker();
  Serial.print("At the third stroke, it will be ");
  digitprint(rtc.getHours(), 2);
  Serial.print(":");
  digitprint(rtc.getMinutes(), 2);
  Serial.print(":");
  digitprint(rtc.getSeconds(), 2);
  Serial.print(" ");
  Serial.print("Date: ");
  Serial.print(daynames[rtc.getDayofWeek()]);
  Serial.print(" ");
  digitprint(rtc.getDay(), 2);
  Serial.print(".");
  digitprint(rtc.getMonth(), 2);
  Serial.print(".");
  Serial.println(rtc.getYear());
  delay(100);
}

void announcement() {
  Serial.println("Get up it's a new day.");
  rtc.setTime(23, 59, 56);
  rtc.setDate(22, 11, 2016);
  //rtc.detachAlarm();
  //rtc.disableAlarmDate();
}

void blinker() {
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    
    // if the LED is off turn it on and vice-versa
    ledState = !ledState;
    
    // set the LED with the ledState of the variable
    digitalWrite(ledPin, ledState);
  }
}

void digitprint(int value, int lenght){
  for (int i = 0; i < (lenght - numdigits(value)); i++){
    Serial.print("0");
  }
  Serial.print(value);
}

int numdigits(int i){
  int digits;
  if (i < 10)
    digits = 1;
  else
    digits = (int)(log10((double)i)) + 1;
  return digits;
}