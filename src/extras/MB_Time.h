#ifndef MB_Time_H
#define MB_Time_H

#include "ESP_Mail_Client_Version.h"
#if !VALID_VERSION_CHECK(30202)
#error "Mixed versions compilation."
#endif

/*
 * Time helper class v1.0.6
 *
 * Created March 25, 2023
 *
 * Do not remove or modify this file as it required for AVR, ARM, SAMD devices and external client to work.
 *
 * The MIT License (MIT)
 * Copyright (c) 2022 K. Suwatchai (Mobizt)
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

#include <Arduino.h>
#include "ESP_Mail_FS.h"

#if !defined(__AVR__)
#include <vector>
#include <string>
#endif

#if defined(ESP32)
#include <ETH.h>
#include <WiFiUdp.h>
#endif

#if defined(ESP32) && !defined(ESP_ARDUINO_VERSION) /* ESP32 core < v2.0.x */
#include <sys/time.h>
#else
#include <time.h>
#endif

#include "./ESP_Mail_FS.h"

#if defined(ESP_MAIL_USE_PSRAM)
#define MB_STRING_USE_PSRAM
#endif

#include "MB_String.h"
#include "MB_List.h"
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#if defined(ENABLE_NTP_TIME)
#include <WiFiNTP.h>
#endif
#endif

#if defined(ENABLE_NTP_TIME)
#include <Udp.h>
#include "MB_NTP.h"
#endif

#if defined(MB_ARDUINO_ARCH_SAMD) || defined(MB_ARDUINO_NANO_RP2040_CONNECT)
#include "../wcs/samd/lib/WiFiNINA.h"
#endif

#define ESP_TIME_DEFAULT_TS 1577836800

#if defined(__AVR__) || defined(MB_ARDUINO_TEENSY)
#define MB_TIME_PGM_ATTR
#else
#define MB_TIME_PGM_ATTR PROGMEM
#endif

#if !defined(MB_ARDUINO_ESP)
static const char *mb_months[12] MB_TIME_PGM_ATTR = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
#endif

class MB_Time
{
public:
  MB_Time()
  {
  }

  ~MB_Time()
  {
#if defined(ENABLE_NTP_TIME)

    _sv1.clear();
    _sv2.clear();
    _sv3.clear();

    if (useInternalUDP)
    {
#if defined(ESP32)
      delete udp;
      udp = nullptr;
#endif
    }

#endif
  }

#if defined(ENABLE_NTP_TIME)

  void setUDPClient(UDP *client, float gmtOffset)
  {
    this->udp = client;
    this->gmtOffset = gmtOffset;
  }

  bool initUDP()
  {
    if (!udp)
      return false;

    return true;
  }

  /** Set the system time from the NTP server
   *
   * @param gmtOffset The GMT time offset in hour.
   * @param daylightOffset The Daylight time offset in hour.
   * @param servers Optional. The NTP servers, use comma to separate the server.
   * @return boolean The status indicates the success of operation.
   *
   * @note This requires internet connection
   */
  bool setClock(float gmtOffset, float daylightOffset, const char *servers = "pool.ntp.org,time.nist.gov")
  {

#if defined(ENABLE_CUSTOM_CLIENT)
    if (ts_offset == 0)
    {
      // Use local gmt offset set from setUDPClient function instead.
      TZ = this->gmtOffset;
      _sv1 = "pool.ntp.org";
      ntpSetTime();
    }
#else

#if defined(MB_ARDUINO_ESP) || defined(MB_ARDUINO_PICO) || defined(ARDUINO_ARCH_SAMD) || defined(__AVR_ATmega4809__) || defined(MB_ARDUINO_NANO_RP2040_CONNECT)

    if (TZ != gmtOffset || DST_MN != daylightOffset)
      configUpdated = true;

#if (defined(ARDUINO_ARCH_SAMD) && !defined(ARDUINO_SAMD_MKR1000)) || defined(MB_ARDUINO_NANO_RP2040_CONNECT)

#elif defined(MB_ARDUINO_ESP) || defined(MB_ARDUINO_PICO)
    sys_ts = time(nullptr);
    if ((millis() - lastSyncMillis > 5000 || lastSyncMillis == 0) && (sys_ts < ESP_TIME_DEFAULT_TS || configUpdated))
    {

      lastSyncMillis = millis();

      MB_VECTOR<MB_String> tk;
      MB_String sv = servers;

      _sv1.clear();
      _sv2.clear();
      _sv3.clear();

      splitToken(sv, tk, ',');

      if (tk.size() > 0)
        _sv1 = tk[0];
      if (tk.size() > 1)
        _sv2 = tk[1];
      if (tk.size() > 2)
        _sv3 = tk[2];

      ntpSetTime();
    }

#endif

#endif

#endif

    TZ = gmtOffset;
    DST_MN = daylightOffset;

    return clockReady(100);
  }

  void ntpSetTime()
  {

#if defined(ESP32)

    // Ethernet connected
    if (strcmp(ETH.localIP().toString().c_str(), (const char *)MBSTRING_FLASH_MCR("0.0.0.0")) != 0)
    {
      if (!udp && !useInternalUDP)
      {
        udp = new WiFiUDP();
        useInternalUDP = true;
      }
    }
    // WiFi connected
    else if (WiFi.status() == WL_CONNECTED)
      configTime(TZ * 3600, DST_MN * 60, _sv1.c_str(), _sv2.c_str(), _sv3.c_str());

#elif defined(ESP8266)
    configTime(TZ * 3600, DST_MN * 60, _sv1.c_str(), _sv2.c_str(), _sv3.c_str());
#elif defined(MB_ARDUINO_PICO)
    NTP.begin(_sv1.c_str(), _sv2.c_str());
    NTP.waitSet();
#endif

    if (udp)
      ntp.begin(udp, _sv1.c_str() /* NTP host */, 123 /* NTP port */, TZ * 3600 /* timezone offset in seconds */);
  }

#endif

  int setTimestamp(time_t ts)
  {
#if defined(MB_ARDUINO_ESP)
    struct timeval tm; // sec, us
    tm.tv_sec = ts;
    tm.tv_usec = 0;
    return settimeofday((const struct timeval *)&tm, 0);
#else
    ts_offset = ts - millis() / 1000;
    return 1;
#endif
  }

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
  time_t getTimestamp(int year, int mon, int date, int hour, int mins, int sec)
  {
    struct tm timeinfo;
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = mon - 1;
    timeinfo.tm_mday = date;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = mins;
    timeinfo.tm_sec = sec;
    time_t ts = mktime(&timeinfo);
    return ts;
  }

  /** Get the timestamp from the RFC 2822 time string.
   * e.g. Mon, 02 May 2022 00:30:00 +0000 or 02 May 2022 00:30:00 +0000
   *
   * @param gmt Return the GMT time.
   * @return timestamp of time string.
   */
  time_t getTimestamp(const char *timeString, bool gmt = false)
  {
#if defined(MB_ARDUINO_ESP)
    struct tm _timeinfo;

    if (strstr(timeString, ",") != NULL)
      strptime(timeString, "%a, %d %b %Y %H:%M:%S %z", &_timeinfo);
    else
      strptime(timeString, "%d %b %Y %H:%M:%S %z", &_timeinfo);

    time_t ts = mktime(&_timeinfo);
    return ts;

#else

    time_t ts = 0;
    MB_VECTOR<MB_String> tk;
    MB_String s1 = timeString;

    splitToken(s1, tk, ' ');
    int day = 0, mon = 0, year = 0, hr = 0, mins = 0, sec = 0, tz_h = 0, tz_m = 0;

    int tkindex = tk.size() == 5 ? -1 : 0; // No week days?

    // some response may include (UTC) and (ICT)
    if (tk.size() >= 5)
    {
      day = atoi(tk[tkindex + 1].c_str());
      for (size_t i = 0; i < 12; i++)
      {
        if (strcmp_P(mb_months[i], tk[tkindex + 2].c_str()) == 0)
          mon = i;
      }

      // RFC 822 year to RFC 2822
      if (tk[tkindex + 3].length() == 2)
        tk[tkindex + 3].prepend("20");

      year = atoi(tk[tkindex + 3].c_str());

      MB_VECTOR<MB_String> tk2;
      splitToken(tk[tkindex + 4], tk2, ':');
      if (tk2.size() == 3)
      {
        hr = atoi(tk2[0].c_str());
        mins = atoi(tk2[1].c_str());
        sec = atoi(tk2[2].c_str());
      }

      ts = getTimestamp(year, mon + 1, day, hr, mins, sec);

      if (tk[tkindex + 5].length() == 5 && gmt)
      {
        char tmp[6];
        memset(tmp, 0, 6);
        strncpy(tmp, tk[tkindex + 5].c_str() + 1, 2);
        tz_h = atoi(tmp);

        memset(tmp, 0, 6);
        strncpy(tmp, tk[tkindex + 5].c_str() + 3, 2);
        tz_m = atoi(tmp);

        time_t tz = tz_h * 60 * 60 + tz_m * 60;
        if (tk[tkindex + 5][0] == '+')
          ts -= tz; // remove time zone offset
        else
          ts += tz;
      }
    }
    return ts;
#endif
  }

  /** Get the current timestamp.
   *
   * @return uint64_t The value of current timestamp.
   */
  uint64_t getCurrentTimestamp()
  {
    getTime();
    return sys_ts;
  }

  /** Get the current rfc822 date time string that valid for Email.
   * @return String The current date time string.
   */
  String getDateTimeString()
  {
    getTime();

    char tbuf[40];
    strftime(tbuf, 40, "%a, %d %b %Y %H:%M:%S %z", &timeinfo);
    MB_String tStr = tbuf, tzStr;

    int p = TZ < 0 ? -1 : 1;
    float dif = (p * (TZ * 10 - (int)TZ * 10)) * 60.0 / 10;
    tzStr = (TZ < 0) ? '-' : '+';

    if ((int)TZ < 10)
      tzStr += 0;

    tzStr += (int)TZ;

    if (dif < 10)
      tzStr += 0;

    tzStr += (int)dif;

    // replace for valid timezone
    tStr.replaceAll("+0000", tzStr);
    return tStr.c_str();
  }

  String getDateTimeString(time_t ts, const char *format)
  {
    char tbuf[50];
    strftime(tbuf, 50, format, localtime(&ts));
    return tbuf;
  }

  /** get the clock ready state
   *  Do not remove or modify this file as it required for AVR, ARM,
   * SAMD devices and external client to work.
   */
  bool clockReady(uint32_t wait_ms = 0)
  {
    uint32_t ts = 0;

#if defined(ENABLE_NTP_TIME)
    ts = udp ? ntp.getTime(wait_ms /* wait 10000 ms */) : 0;
    if (ts > 0)
      ts_offset = ts - millis() / 1000;
#endif

    getTime(ts);

    _clockReady = sys_ts > ESP_TIME_DEFAULT_TS;

    // Update system timestamp and its offset when time/timezone changed.
    if (_clockReady && configUpdated)
    {
      sys_ts += TZ * 3600;
      ts_offset = sys_ts - millis() / 1000;
      configUpdated = false;
    }

#if defined(MB_ARDUINO_ESP)
    // If system timestamp was set, update the device time
    if (sys_ts > ESP_TIME_DEFAULT_TS && time(nullptr) < sys_ts)
      setTimestamp(sys_ts);
#endif

#if defined(ENABLE_NTP_TIME)
    if (_clockReady)
    {
      _sv1.clear();
      _sv2.clear();
      _sv3.clear();
    }
#endif

    return _clockReady;
  }
  // The library's internal timestamp which can be assigned via
  // many methods e.g., device local time, NTP and manually set.
  time_t sys_ts = 0;
  struct tm timeinfo;
  float TZ = 0.0;
  float DST_MN = 0.0;

private:
  void splitToken(MB_String &str, MB_VECTOR<MB_String> &tk, char delim)
  {
    size_t current, previous = 0;
    current = str.find(delim, previous);
    MB_String s;
    while (current != MB_String::npos)
    {

      s = str.substr(previous, current - previous);
      s.trim();

      if (s.length() > 0)
        tk.push_back(s);
      previous = current + 1;
      current = str.find(delim, previous);
    }

    s = str.substr(previous, current - previous);

    s.trim();

    if (s.length() > 0)
      tk.push_back(s);

    s.clear();
  }

  void getTime(uint32_t ctime = 0)
  {

#if (defined(ARDUINO_ARCH_SAMD) && !defined(ARDUINO_SAMD_MKR1000)) || defined(MB_ARDUINO_NANO_RP2040_CONNECT)

    unsigned long ts = WiFi.getTime();
    if (ts > 0)
      sys_ts = ts;

#else

#if defined(ENABLE_CUSTOM_CLIENT) || defined(MB_ARDUINO_PICO) || (defined(ARDUINO_ARCH_SAMD) && defined(__AVR_ATmega4809__))
    // set sys time using the offset since the time was manually set via setTimestamp function and current seconds count
    sys_ts = ts_offset + millis() / 1000;
#if defined(MB_ARDUINO_PICO)
    // set sys time using device timestamp
    if (sys_ts < time(nullptr))
      sys_ts = time(nullptr);
    localtime_r(&sys_ts, &timeinfo);
#endif
#else

#if defined(MB_ARDUINO_ESP)
    sys_ts = ctime == 0 ? time(nullptr) : ctime;
#endif

#if defined(ESP32) || defined(ESP8266)
    if (ctime == 0 && time(nullptr) > ESP_TIME_DEFAULT_TS)
    {
#if defined(ESP32)
      getLocalTime(&timeinfo);
#elif defined(ESP8266)
      localtime_r(&sys_ts, &timeinfo);
#endif
    }
#endif

#endif

#endif
  }

#if defined(ENABLE_NTP_TIME)

  UDP *udp = nullptr;
  bool useInternalUDP = false;
  MB_NTP ntp;

  // in ESP8266 these NTP sever strings should be existed during configuring time.
  MB_String _sv1, _sv2, _sv3;
#endif

  uint32_t ts_offset = 0;
  bool configUpdated = false;
  float gmtOffset = 0;
  bool _clockReady = false;
  unsigned long lastSyncMillis = 0;
};

#endif // MB_Time_H
