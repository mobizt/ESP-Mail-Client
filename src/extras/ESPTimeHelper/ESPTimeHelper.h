/*
 * ESP8266/ESP32 Internet Time Helper Arduino Library v 1.0.6
 *
 * December 21, 2021 
 * 
 * The MIT License (MIT)
 * Copyright (c) 2021 K. Suwatchai (Mobizt)
 * 
 * 
 * Permission is hereby granted, free of charge, to any person returning a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef ESPTimeHelper_H
#define ESPTimeHelper_H

#if !defined(__AVR__)
#include <vector>
#endif

#include <time.h>
#include <Arduino.h>
#include "./ESP_Mail_FS.h"

#if defined(ESP_Mail_USE_PSRAM)
#define MB_STRING_USE_PSRAM
#endif

#include "../MB_String.h"
#include "../MB_List.h"

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include "extras/SDK_Version_Common.h"
#elif defined(MB_MCU_ATMEL_ARM) || defined(MB_MCU_RP2040)
#include "../../wcs/samd/lib/WiFiNINA.h"
#endif

#define ESP_TIME_DEFAULT_TS 1618971013


class ESPTimeHelper
{
public:
  ESPTimeHelper();

  /** Set the system time from the NTP server
   * 
   * @param gmtOffset The GMT time offset in hour.
   * @param daylightOffset The Daylight time offset in hour.
   * @param servers Optional. The NTP servers, use comma to separate the server.
   * @return boolean The status indicates the success of operation.
   * 
   * @note This requires internet connection
  */
  bool setClock(float gmtOffset, float daylightOffset, const char *servers = "pool.ntp.org,time.nist.gov");

  /** Set system time with provided timestamp
   * 
   * @param ts timestamp in seconds from midnight Jan 1, 1970.
   * @return error number, 0 for success.
  */
  int setTimestamp(time_t ts);

  /** Get the Unix time
   * 
   * @return uint32_t The value of current Unix time.
  */
  uint32_t getUnixTime();

  /** Get the timestamp from the year, month, date, hour, minute, 
   * and second provided.
   * 
   * @param year The year.
   * @param mon The month from 1 to 12.
   * @param date The dates.
   * @param hour The hours.
   * @param mins The minutes.
   * @param sec The seconds.
   * @return time_t The value of timestamp.
  */
  time_t getTimestamp(int year, int mon, int date, int hour, int mins, int sec);

  /** Get the timestamp from the time string.
   * @param gmt Return the GMT time.
   * @return timestamp of time string.
  */
  time_t getTimestamp(const char *timeString, bool gmt = false);

  /** Get the current year.
   * 
   * @return int The value of current year.
  */
  int getYear();

  /** Get the current month.
   * 
   * @return int The value of current month.
  */
  int getMonth();

  /** Get the current date.
   * 
   * @return int The value of current date.
  */
  int getDay();

  /** Get the current day of week.
   * 
   * @return int The value of current day of week.
   * 
   * @note 1 for sunday and 7 for saturday.
  */
  int getDayOfWeek();

  /** Get the current day of week in String.
   * 
   * @return String The value of day of week.
  */
  String getDayOfWeekString();

  /** Get the current hour.
   * 
   * @return int The value of current hour (0 to 23).
  */
  int getHour();

  /** Get the current minute.
   * 
   * @return int The value of current minute.
  */
  int getMin();

  /** Get the current second.
   * 
   * @return int The value of current second.
  */
  int getSec();

  /** Get the total days of current year.
   * 
   * @return int The value of total days of current year.
  */
  int getNumberOfDayThisYear();

  /** Get the total days of from January 1, 1970 to specific date.
   * 
   * @param year The year from 1970.
   * @param mon The month from 1 to 12.
   * @param day The dates.
   * @return int The value of total days.
  */
  int getTotalDays(int year, int month, int day);

  /** Get the day of week from specific date.
   * 
   * @param year The year from 1970.
   * @param mon The month from 1 to 12.
   * @param day The dates.
   * @return int the value of day of week.
   * @note 1 for sunday and 7 for saturday
  */
  int dayofWeek(int year, int month, int day);

  /** Get the second of current hour.
   * 
   * @return int The value of current second.
  */
  int getCurrentSecond();

  /** Get the current timestamp.
   * 
   * @return uint64_t The value of current timestamp.
  */
  uint64_t getCurrentTimestamp();

  /** Get the date and time from second counted from January 1, 1970.
   * 
   * @param sec The seconds from January 1, 1970 00.00.
   * @return tm The tm structured data.
   * 
   * @note The returned structured data tm has the members e.g. 
   * tm_year (from 1900), tm_mon (from 0 to 11), tm_mday, tm_hour, 
   * tm_min and tm_sec.
  */
  struct tm getTimeFromSec(int seconds);

  /** Get the current date time string that valid for Email.
   * 
   * @return String The current date time string.
  */
  String getDateTimeString();


  /** get the clock ready state */
  bool clockReady();

  time_t now;
  uint64_t msec_time_diff = 0;
  struct tm timeinfo;
  float TZ = 0.0;
  float DST_MN = 0.0;

private:
  int totalDays(int y, int m, int d);
  void splitTk(MB_String &str, MB_VECTOR<MB_String> &tk, const char *delim);
  char *intStr(int value);
  void setSysTime();
  char *trimwhitespace(char *str);
  int compareVersion(uint8_t major1, uint8_t minor1, uint8_t patch1, uint8_t major2, uint8_t minor2, uint8_t patch2);
  void *newP(size_t len);
  size_t getReservedLen(size_t len);
  void delP(void *ptr);

  bool _clockReady = false;
  const char *dow[7] = {"sunday", "monday", "tuesday", "wednesday", "thurseday", "friday", "saturday"};
  const char *months[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  const char *sdow[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

  //in ESP8266 these NTP sever strings should be existed during configuring time.
  MB_String _sv1, _sv2, _sv3;
};

#endif //ESPTimeHelper_H

