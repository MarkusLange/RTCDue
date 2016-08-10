#ifndef RTCDue_h
#define RTCDue_h

#include "Arduino.h"

// Include Atmel CMSIS driver
#include <include/rtc.h>

#define RESET_VALUE 0x01210720

#define RC   0
#define XTAL 1

#define US  0
#define EEC 1

typedef void(*voidFuncPtr)(void);

class RTCDue
{
  public:
    RTCDue (int source);
    void begin ();
    void setTime (int hour, int minute, int second);
    void setTime (const char* time);
    int getHourMode ();
    int getHours ();
    int getMinutes ();
    int getSeconds ();
    void setDate (int day, int month, uint16_t year);
    void setDate (const char* date);
    void setClock (const char* date, const char* time);
    void setClock (unsigned long timestamp);
    uint16_t getYear ();
    int getMonth ();
    int getDay ();
    int getDayofWeek ();
    int calculateDayofWeek (uint16_t year, int month, int day);
    void setHourMode (int _mode);
    int setHours (int hour);
    int setMinutes (int minute);
    int setSeconds (int second);
    int setDay (int day);
    int setMonth (int month);
    int setYear (uint16_t year);
    
    void setAlarmTime (int hour, int minute, int second);
    void setAlarmDate (int month, int day);
    void disableAlarmTime ();
    void disableAlarmDate ();
    void attachAlarm (voidFuncPtr callback);
    void detachAlarm ();
    
    uint32_t unixtime ();
    uint32_t unixtime (String abbreviation);
    void getTime (int *hour, int *minute, int *second);
    void getDate (int *day_of_week, int *day, int *month, uint16_t *year);
    int getValidEntry ();
    int isSummertime (int select);
	int isDateAlreadySet ();
    uint32_t changeTime (uint32_t _now);
    uint32_t changeDate (uint32_t _now);
    
  private:
    uint32_t _hour;
    uint32_t _minute;
    uint32_t _second;
    uint32_t _day;
    uint32_t _month;
    uint32_t _year;
    uint32_t _day_of_week;
    uint32_t currentTime ();
    uint32_t currentDate ();
    uint32_t _current_time;
    uint32_t _current_date;
    uint32_t _now;
    uint32_t _changed;
};
#endif