/*
  Simple Unixtime for Arduino Due

  Demonstrates the use of the RTC library for the Arduino Due

  This example code is in the public domain

  created by Markus Lange
  10 Feb 2016
*/

#include <RTCDue.h>

/* Create an rtc object and select Slowclock source */
//RTCDue rtc(RC);
RTCDue rtc(XTAL);

const char* daynames[]={"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void setup() {
  Serial.begin(9600);
  rtc.begin(); // initialize RTC

  // Set the time
  rtc.setTime(__TIME__);

  // Set the date
  rtc.setDate(__DATE__);

  // you can use also
  //rtc.setClock(__TIME__, __DATE__);
}

void loop() {
  // Print unixtime...
  Serial.print("Unixtime: ");
  Serial.println(rtc.unixtime());

  // Print time...
  Serial.println("And in plain for everyone");
  Serial.print("Time: ");
  digitprint(rtc.getHours(), 2);
  Serial.print(":");
  digitprint(rtc.getMinutes(), 2);
  Serial.print(":");
  digitprint(rtc.getSeconds(), 2);
  Serial.println("");

  // Print date...
  Serial.print("Date: ");
  Serial.print(daynames[rtc.getDayofWeek()]);
  Serial.print(" ");
  digitprint(rtc.getDay(), 2);
  Serial.print(".");
  digitprint(rtc.getMonth(), 2);
  Serial.print(".");
  Serial.println(rtc.getYear());
  Serial.println("");
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