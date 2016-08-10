#include "RTCDue.h"

uint8_t daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};

// Based on https://github.com/PaulStoffregen/Time.cpp
// for 4 digits per year
#define switch_years(Y) ( !(Y % 4) && ( (Y % 100) || !(Y % 400) ) )

// Format conversion
#define BCD2BIN(val) (((val)&15) + ((val)>>4)*10)
#define BIN2BCD(val) ((((val)/10)<<4) + (val)%10)

#define seperator 0x64
#define SECONDS_PER_HOUR 3600

RTCDue::RTCDue (int source)
{
  if (source) {
    pmc_switch_sclk_to_32kxtal(0);
  
  while (!pmc_osc_is_ready_32kxtal());
  }
}

voidFuncPtr RTC_callBack = NULL;

void RTCDue::begin ()
{
  RTC_SetHourMode(RTC, 0);
  
  NVIC_DisableIRQ(RTC_IRQn);
  NVIC_ClearPendingIRQ(RTC_IRQn);
  NVIC_SetPriority(RTC_IRQn, 0);
  //NVIC_EnableIRQ(RTC_IRQn);
  RTC_EnableIt(RTC, RTC_IER_SECEN | RTC_IER_ALREN | RTC_IER_ACKEN);
}

void RTCDue::setTime (int hour, int minute, int second)
{
  RTC_SetTime (RTC, hour, minute, second);
}

/*
// Based on https://github.com/adafruit/RTClib/blob/master/RTClib.cpp
int conv2d (char* p)
{
  int v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  
  return 10 * v + *++p - '0';
}
*/

// Based on http://www.geeksforgeeks.org/write-your-own-atoi/
// Better version converts until none number shows up
int conv2d (const char* p)
{
  int v = 0;
  int x = 0;
  
  // Prevent error if Days has only 1 digit
  if (p[0] == ' ')
    ++x;
  
  // Iterate through all characters of input string and update result
  for (int i = x; p[i] != '\0'; ++i) {
    if ('0' <= p[i] && p[i] <= '9')
      v = v*10 + p[i] - '0';
    else
      break;
  }
  
  return v;
}

void RTCDue::setTime (const char* time)
{
  _hour   = conv2d(time);
  _minute = conv2d(time + 3);
  _second = conv2d(time + 6);
  
  RTC_SetTime (RTC, _hour, _minute, _second);
}

/**
 * \brief Set the RTC hour mode.
 *
 * \param p_rtc Pointer to an RTC instance.
 * \param ul_mode 1 for 12-hour mode, 0 for 24-hour mode.
 */
void RTCDue::setHourMode (int mode)
{
  if (mode == 12)
    RTC->RTC_MR |= RTC_MR_HRMOD;
  else if (mode == 24)
    RTC->RTC_MR &= (~RTC_MR_HRMOD);
}

// Based on https://github.com/PaulStoffregen/Time.cpp
void RTCDue::setClock (unsigned long timestamp)
{
  int monthLength;
  unsigned long time, days;
  
  // Sunday, 01-Jan-40 00:00:00 UTC 70 years after the beginning of the unix timestamp so
  // if the timestamp bigger than this the "offset" will automatic removed
  if (timestamp >= 2208988800UL)
    time -= 2208988800UL;
  
  time = timestamp;
  _second = time % 60;
  time /= 60; // now it is minutes
  _minute = time % 60;
  time /= 60; // now it is hours
  _hour = time % 24;
  time /= 24; // now it is days
  
  _year = 1970;
  days = 0;
  while(( days += (365 + switch_years(_year)) ) <= time)
    _year++;
  
  days -= (365 + switch_years(_year));
  
  time -= days; // now it is days in this year, starting at 0
  
  days = 0;
  for ( _month = 0; _month < 12; _month++ ) {
    if (_month == 1) // february
      monthLength = daysInMonth[_month] + switch_years(_year);
    else
      monthLength = daysInMonth[_month];
    
    if (time >= monthLength)
      time -= monthLength;
    else
      break;
  }
  
  _month++;                 // jan is month 1
  _day = (int)time + 1;     // day of month
  _day_of_week = calculateDayofWeek (_year, _month, _day);
  
  RTC_SetTime (RTC, _hour, _minute, _second);
  RTC_SetDate (RTC, (uint16_t)_year, (uint8_t)_month, (uint8_t)_day, (uint8_t)_day_of_week);
}

uint32_t RTCDue::currentTime ()
{
  uint32_t dwTime;
  
  /* Get current RTC time */
  dwTime = RTC->RTC_TIMR ;
  while ( dwTime != RTC->RTC_TIMR ) {
    dwTime = RTC->RTC_TIMR ;
  }
  
  return (dwTime);
}

int RTCDue::isDateAlreadySet ()
{
  uint32_t dateregister;
  
  /* Get current RTC date */
  dateregister = currentDate ();
  
  if ( RESET_VALUE != dateregister )
    return 1;
  else
    return 0;
}

void RTCDue::getTime (int *hour, int *minute, int *second)
{
  RTC_GetTime(RTC, (uint8_t*)hour, (uint8_t*)minute, (uint8_t*)second);
}

/**
 * \brief Get the RTC hour mode.
 *
 * \param p_rtc Pointer to an RTC instance.
 *
 * \return 1 (12) for 12-hour mode, 0 (24) for 24-hour mode.
 */
int RTCDue::getHourMode ()
{
  if (RTC->RTC_MR & RTC_MR_HRMOD)
    return 12;
  else
    return 24;
}

int RTCDue::getHours ()
{
  _hour = currentTime();
  
  return BCD2BIN((_hour & RTC_TIMR_HOUR_Msk) >> RTC_TIMR_HOUR_Pos);
}

int RTCDue::getMinutes ()
{
  _minute = currentTime();
  
  return BCD2BIN((_minute & RTC_TIMR_MIN_Msk) >> RTC_TIMR_MIN_Pos);
}

int RTCDue::getSeconds ()
{
  _second = currentTime();
  
  return BCD2BIN((_second & RTC_TIMR_SEC_Msk) >> RTC_TIMR_SEC_Pos);
}

/**
 * \brief Calculate day_of_week from year, month, day.
 * Based on SAM3X rtc_example.c from Atmel Software Framework (Real-Time Clock (RTC) example for SAM) also available
 * https://github.com/eewiki/asf/blob/master/sam/drivers/rtc/example/rtc_example.c
 */
int RTCDue::calculateDayofWeek (uint16_t _year, int _month, int _day)
{
  int _week;
  
  if (_month == 1 || _month == 2) {
    _month += 12;
    --_year;
  }
  
  /**
   * Zeller's congruence for the Gregorian calendar.
   * With 0=Monday, ... 5=Saturday, 6=Sunday
   * http://www.mikrocontroller.net/topic/144905
   * 2 * _month + 3 * (_month + 1) / 5 != (13 * _month + 3) / 5
   * With 0=Monday, ... 5=Saturday, 6=Sunday
   */
  /*
  _week = (_day + 2 * _month + 3 * (_month + 1) / 5 + _year + _year / 4 - _year / 100 + _year / 400) % 7;
  */
  
  // http://esmz-designz.com/index.php?site=blog&entry=68
  // With 0=Sunday, 1=Monday... , 6=Saturday
  _week = (_day + 2 * _month + 3 * (_month + 1) / 5 + _year + _year / 4 - _year / 100 + _year / 400 + 1) % 7;
  
  // Range is from 1 to 7 in the SAM3X so increment _week with one
  ++_week;
  
  return _week;
}

void RTCDue::setDate (int day, int month, uint16_t year)
{
  _day_of_week = calculateDayofWeek(year, month, day);
  
  RTC_SetDate (RTC, (uint16_t)year, (uint8_t)month, (uint8_t)day, (uint8_t)_day_of_week);
}

// Based on https://github.com/adafruit/RTClib/blob/master/RTClib.cpp
void RTCDue::setDate (const char* date)
{
  _day = conv2d(date + 4);
  
  //Month
  switch (date[0]) {
    case 'J': _month = date[1] == 'a' ? 1 : _month = date[2] == 'n' ? 6 : 7; break;
    case 'F': _month =  2; break;
    case 'A': _month = date[2] == 'r' ? 4 : 8; break;
    case 'M': _month = date[2] == 'r' ? 3 : 5; break;
    case 'S': _month =  9; break;
    case 'O': _month = 10; break;
    case 'N': _month = 11; break;
    case 'D': _month = 12; break;
  }
  
  _year = conv2d(date + 7);
  _day_of_week = calculateDayofWeek(_year, _month, _day);
  
  RTC_SetDate (RTC, (uint16_t)_year, (uint8_t)_month, (uint8_t)_day, (uint8_t)_day_of_week);
}

uint32_t RTCDue::currentDate ()
{
  uint32_t dwTime;
  
  /* Get current RTC date */
  dwTime = RTC->RTC_CALR ;
  while ( dwTime != RTC->RTC_CALR ) {
    dwTime = RTC->RTC_CALR ;
  }
  
  return (dwTime);
}

void RTCDue::getDate (int *day_of_week, int *day, int *month, uint16_t *year)
{
  RTC_GetDate(RTC, (uint16_t*)year, (uint8_t*)month, (uint8_t*)day, (uint8_t*)day_of_week);
  
  --*day_of_week;
}

uint16_t RTCDue::getYear ()
{
  _year = currentDate();
  
  uint32_t cent = BCD2BIN((_year & RTC_CALR_CENT_Msk) >> RTC_CALR_CENT_Pos);
  uint32_t year = BCD2BIN((_year & RTC_CALR_YEAR_Msk) >> RTC_CALR_YEAR_Pos);
  
  return (cent * seperator + year);
}

int RTCDue::getMonth ()
{
  _month = currentDate();
  
  return BCD2BIN((_month & RTC_CALR_MONTH_Msk) >> RTC_CALR_MONTH_Pos);
}

int RTCDue::getDay ()
{
  _day = currentDate();
  
  return BCD2BIN((_day & RTC_CALR_DATE_Msk) >> RTC_CALR_DATE_Pos);
}

int RTCDue::getDayofWeek ()
{
  _day_of_week = currentDate();
  
  return BCD2BIN((_day_of_week & RTC_CALR_DAY_Msk) >> RTC_CALR_DAY_Pos) - 1;
}

int RTCDue::getValidEntry ()
{
  return (RTC->RTC_VER);
}

int RTCDue::setHours (int hour)
{
  _changed = BIN2BCD(hour) << RTC_TIMR_HOUR_Pos;
  
  _current_time = (currentTime() & ~RTC_TIMR_HOUR_Msk) ^ _changed;
  
  return (int)changeTime(_current_time);
}

int RTCDue::setMinutes (int minute)
{
  _changed = BIN2BCD(minute) << RTC_TIMR_MIN_Pos;
  
  _current_time = (currentTime() & ~RTC_TIMR_MIN_Msk) ^ _changed;
  
  return (int)changeTime(_current_time);
}

int RTCDue::setSeconds (int second)
{
  _changed = BIN2BCD(second) << RTC_TIMR_SEC_Pos;
  
  _current_time = (currentTime() & ~RTC_TIMR_SEC_Msk) ^ _changed;
  
  return (int)changeTime(_current_time);
}

uint32_t RTCDue::changeTime (uint32_t now)
{
  while ((RTC->RTC_SR & RTC_SR_SEC) != RTC_SR_SEC);
    RTC->RTC_CR |= RTC_CR_UPDTIM ;
  
  while ((RTC->RTC_SR & RTC_SR_ACKUPD) != RTC_SR_ACKUPD);
  
  RTC->RTC_SCCR = RTC_SCCR_ACKCLR;
  RTC->RTC_TIMR = now;
  RTC->RTC_CR &= (uint32_t)(~RTC_CR_UPDTIM);
  RTC->RTC_SCCR |= RTC_SCCR_SECCLR;
  
  return (int)(RTC->RTC_VER & RTC_VER_NVTIM);
}

int RTCDue::setDay (int day)
{
  _day_of_week = calculateDayofWeek( getYear(), getMonth(), day );
  _day_of_week = BIN2BCD( _day_of_week ) << RTC_CALR_DAY_Pos;
  
  _changed = BIN2BCD( day ) << RTC_CALR_DATE_Pos;
  
  _current_date = ( currentDate() & ~RTC_CALR_DAY_Msk & ~RTC_CALR_DATE_Msk ) ^ ( _changed | _day_of_week );
  
  return (int)changeDate(_current_date);
}

int RTCDue::setMonth (int month)
{
  _day_of_week = calculateDayofWeek( getYear(), _month, getDay() );
  _day_of_week = BIN2BCD( _day_of_week ) << RTC_CALR_DAY_Pos;
  
  _changed = BIN2BCD( month ) << RTC_CALR_MONTH_Pos;
  
  _current_date = ( currentDate() & ~RTC_CALR_DAY_Msk & ~RTC_CALR_MONTH_Msk ) ^ ( _changed | _day_of_week );
  
  return (int)changeDate(_current_date);
}

int RTCDue::setYear (uint16_t year)
{
  _day_of_week = calculateDayofWeek( year, getMonth(), getDay() );
  _day_of_week = BIN2BCD( _day_of_week ) << RTC_CALR_DAY_Pos;
  
  _changed = BIN2BCD( year / seperator ) << RTC_CALR_CENT_Pos | BIN2BCD( year % seperator ) << RTC_CALR_YEAR_Pos;
  
  _current_date = ( currentDate() & ~RTC_CALR_DAY_Msk & ~RTC_CALR_YEAR_Msk ) ^ ( _changed | _day_of_week );
  
  return (int)changeDate(_current_date);
}

uint32_t RTCDue::changeDate (uint32_t now)
{
  while ((RTC->RTC_SR & RTC_SR_SEC) != RTC_SR_SEC);
    RTC->RTC_CR |= RTC_CR_UPDCAL;
  
  while ((RTC->RTC_SR & RTC_SR_ACKUPD) != RTC_SR_ACKUPD);
  
  RTC->RTC_SCCR = RTC_SCCR_ACKCLR;
  RTC->RTC_CALR = now;
  RTC->RTC_CR &= (uint32_t)(~RTC_CR_UPDCAL);
  RTC->RTC_SCCR |= RTC_SCCR_SECCLR;
  
  return (int)(RTC->RTC_VER & RTC_VER_NVCAL);
}

void RTCDue::setClock (const char* date, const char* time)
{
  setDate(date);
  setTime(time);
}

void RTCDue::attachAlarm (voidFuncPtr callback)
{
  RTC_callBack = callback;
}

void RTCDue::detachAlarm()
{
  RTC_callBack = NULL;
}

/**
 * \brief Interrupt handler for the RTC.
 */
void RTC_Handler (void)
{
  uint32_t status = RTC->RTC_SR;
  
  /* Second increment interrupt */
  if ((status & RTC_SR_SEC) == RTC_SR_SEC) {
    /* Disable RTC interrupt */
    RTC_DisableIt(RTC, RTC_IDR_SECDIS);
    
    RTC_ClearSCCR(RTC, RTC_SCCR_SECCLR);
    
    RTC_EnableIt(RTC, RTC_IER_SECEN);
  } else {
    /* Time or date alarm */
    if ((status & RTC_SR_ALARM) == RTC_SR_ALARM) {
      /* Disable RTC interrupt */
      RTC_DisableIt(RTC, RTC_IDR_ALRDIS);
      
      if (RTC_callBack != NULL) {
        RTC_callBack();
      }
      
      RTC_ClearSCCR(RTC, RTC_SCCR_ALRCLR);
      RTC_EnableIt(RTC, RTC_IER_ALREN);
    }
  }
}

void RTCDue::setAlarmTime (int hour, int minute, int second)
{
  uint8_t _hour   = hour;
  uint8_t _minute = minute;
  uint8_t _second = second;
  
  RTC_EnableIt(RTC, RTC_IER_ALREN);
  RTC_SetTimeAlarm(RTC, &_hour, &_minute, &_second);
  NVIC_EnableIRQ(RTC_IRQn);
}

void RTCDue::setAlarmDate (int day, int month)
{
  uint8_t _day   = day;
  uint8_t _month = month;
  
  RTC_EnableIt(RTC, RTC_IER_ALREN);
  RTC_SetDateAlarm(RTC, &_month, &_day);
  NVIC_EnableIRQ(RTC_IRQn);
}

void RTCDue::disableAlarmTime ()
{
  RTC->RTC_TIMALR = 0;
}

void RTCDue::disableAlarmDate ()
{
  /* Need a valid value without enabling */
  RTC->RTC_CALALR = RTC_CALALR_MONTH(0x01) | RTC_CALALR_DATE(0x01);
}

uint32_t RTCDue::unixtime (String abbreviation)
{
  float adjustment = abbreviation.toFloat() * SECONDS_PER_HOUR;
  
  return unixtime() - (int)adjustment;
}

uint32_t RTCDue::unixtime ()
{
  uint32_t _ticks;
  uint16_t _days;
  
  _hour   = getHours ();
  _minute = getMinutes ();
  _second = getSeconds ();
  
  _day    = getDay ();
  _month  = getMonth ();
  _year   = getYear (); //4 digits
  
  // Based on https://github.com/punkiller/workspace/blob/master/string2UnixTimeStamp.cpp
  // days of the years between start of unixtime and now
  _days = 365 * (_year - 1970);
  
  // add days from switch years in between except year from date
  for( int i = 1970; i < _year; i++ ){
    if( switch_years (i) ) {
      _days++;
    }
  }
  
  // Based on https://github.com/adafruit/RTClib/blob/master/RTClib.cpp
  // add switch day from actuall year if necessary
  for ( int i = 1; i < _month; ++i )
    _days += daysInMonth[i - 1];
  
  if ( _month > 2 && switch_years (_year) )
    ++_days;
  
  _days += _day - 1;
  
  _ticks = ((_days * 24 + _hour) * 60 + _minute) * 60 + _second;
  
  return _ticks;
}

// Based on http://www.webexhibits.org/daylightsaving/i.html
// Equations by Wei-Hwa Huang (US), and Robert H. van Gent (EC)
int RTCDue::isSummertime (int select)
{
  int _sundaysommertime, _sundaywintertime, _begin_Month, _end_Month, _summertime;
  
  _summertime = 0;
  
  _hour  = getHours ();
  
  _day   = getDay ();
  _month = getMonth ();
  _year  = getYear (); //4 digits
  
  switch (select) {
    default:
    case EEC:
      // European Economic Community (EEC):
      // Begin DST: Sunday March
      _sundaysommertime = 31 - ( 4 + _year * 5 / 4 ) % 7;
      // End DST: Sunday October
      _sundaywintertime = 31 - ( 1 + _year * 5 / 4 ) % 7;
      _begin_Month =  3;
      _end_Month   = 10;
    break;
    case US:
      // For the United States (US):
      if ( _year < 2007 ) {
        // Valid for years 1900 to 2006, though DST wasn't adopted until the 1950s-1960s.
        // Begin DST: Sunday April
        _sundaysommertime = ( 2 + 6 * _year - _year / 4 ) % 7 + 1;
        // End DST: Sunday October
        _sundaywintertime = 31 - ( _year * 5 / 4 + 1 ) % 7;
        _begin_Month =  4;
        _end_Month   = 10;
      } else {
        // Valid for 2007 and after:
        // Begin DST: Sunday March
        _sundaysommertime = 14 - ( 1 + _year * 5 / 4 ) % 7;
        // End DST: Sunday November
        _sundaywintertime = 7 - ( 1 + _year * 5 / 4 ) % 7;
        _begin_Month =  3;
        _end_Month   = 11;
      }
    break;
  }
  
  if ( _month >= _begin_Month && _month <= _end_Month ) {
    _summertime = 1;
    if ( _month == _begin_Month && _day < _sundaysommertime ) {
      _summertime = 0;
    }
    if ( _month == _end_Month && _day > _sundaywintertime ) {
      _summertime = 0;
    }
  }
  
  return _summertime;
}