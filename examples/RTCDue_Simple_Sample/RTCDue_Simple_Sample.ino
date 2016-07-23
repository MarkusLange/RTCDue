/*
  Simple RTC Example for Arduino Due

  Demonstrates the use of the RTC library for the Arduino Due

  This example code is in the public domain

  created by Markus Lange
  12 Feb 2016
*/

#include <RTCDue.h>

/* Create an rtc object ans choose SlowClock source*/
//RTCDue rtc(RC);
RTCDue rtc(XTAL);

/* Change these values to set the current initial time */
const uint8_t seconds = 24;
const uint8_t minutes = 59;
const uint8_t hours = 11;

/* Change these values to set the current initial date */
const uint8_t day = 12;
const uint8_t month = 2;
const uint16_t year = 2016;

const char* daynames[]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void setup() {
  Serial.begin(9600);
  
  rtc.begin(); // initialize RTC
  // Set the time
  rtc.setHours(hours);
  rtc.setMinutes(minutes);
  rtc.setSeconds(seconds);
  
  // Set the date
  rtc.setDay(day);
  rtc.setMonth(month);
  rtc.setYear(year);
  
  // you can use also
//  rtc.setTime(hours, minutes, seconds);
//  rtc.setDate(day, month, year);
}

void loop() {
  // Print date...
  Serial.print(daynames[rtc.getDayofWeek()]);
  Serial.print(" ");
  Serial.print(rtc.getDay());
  Serial.print("/");
  Serial.print(rtc.getMonth());
  Serial.print("/");
  Serial.print(rtc.getYear());
  Serial.print("\t");
  
  // ...time...
  Serial.print(rtc.getHours());
  Serial.print(":");
  Serial.print(rtc.getMinutes());
  Serial.print(":");
  Serial.println(rtc.getSeconds());
  
  // ...and check for Summertime in the United States (US) or
  // the European Economic Community (EEC)
  if (rtc.isSummertime(US))
    Serial.println("Summertime");
  
  Serial.println();
  delay(1000);
}