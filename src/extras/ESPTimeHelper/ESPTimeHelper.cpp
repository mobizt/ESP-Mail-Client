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

#ifndef ESPTimeHelper_CPP
#define ESPTimeHelper_CPP

#include "ESPTimeHelper.h"
#include <string>

ESPTimeHelper::ESPTimeHelper()
{
}

uint32_t ESPTimeHelper::getUnixTime()
{
    uint32_t utime = (msec_time_diff + millis()) / 1000;
    return utime;
}

int ESPTimeHelper::setTimestamp(time_t ts)
{
#if defined(ESP32) || defined(ESP8266)
    struct timeval tm = {ts, 0}; //sec, us
    return settimeofday((const timeval *)&tm, 0);
#else
    return 0;
#endif
}

time_t ESPTimeHelper::getTimestamp(int year, int mon, int date, int hour, int mins, int sec)
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

bool ESPTimeHelper::setClock(float gmtOffset, float daylightOffset, const char *servers)
{
    bool newConfig = TZ != gmtOffset || DST_MN != daylightOffset;
    TZ = gmtOffset;
    DST_MN = daylightOffset;
    now = time(nullptr);
#if defined(ARDUINO_ARCH_SAMD)
    unsigned long ts = WiFi.getTime();
    if (ts > 0)
    {
        now = ts;
        if (newConfig)
            now += TZ * 3600;
    }
#else
    if (now < ESP_TIME_DEFAULT_TS || newConfig)
    {

        std::vector<MB_String> tk = std::vector<MB_String>();
        MB_String sv = servers;

        splitTk(sv, tk, (const char*)FLASH_STR_MCR(","));

        switch (tk.size())
        {
        case 1:

            _sv1 = tk[0];
            configTime((TZ)*3600, (DST_MN)*60, _sv1.c_str());

            break;
        case 2:

            _sv1 = tk[0];
            _sv2 = tk[1];
            configTime((TZ)*3600, (DST_MN)*60, _sv1.c_str(), _sv2.c_str());

            break;
        case 3:

            _sv1 = tk[0];
            _sv2 = tk[1];
            _sv3 = tk[2];
            configTime((TZ)*3600, (DST_MN)*60, _sv1.c_str(), _sv2.c_str(), _sv3.c_str());

            break;
        default:

            configTime((TZ)*3600, (DST_MN)*60, "pool.ntp.org", "time.nist.gov");

            break;
        }

        now = time(nullptr);
        uint64_t tmp = now;
        tmp = tmp * 1000;
        msec_time_diff = tmp - millis();

#if defined(ESP32)
        getLocalTime(&timeinfo);
#elif defined(ESP8266)
        gmtime_r(&now, &timeinfo);
#endif
    }

#endif

    _clockReady = now > ESP_TIME_DEFAULT_TS;

    if (_clockReady)
    {
        _sv1.clear();
        _sv2.clear();
        _sv3.clear();
    }

    return _clockReady;
}

char *ESPTimeHelper::trimwhitespace(char *str)
{
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0) // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}

bool ESPTimeHelper::clockReady()
{
    time_t now = time(nullptr);
    _clockReady = now > ESP_TIME_DEFAULT_TS;

    if (_clockReady)
    {
        _sv1.clear();
        _sv2.clear();
        _sv3.clear();
    }
    return _clockReady;
}

int ESPTimeHelper::getYear()
{
    setSysTime();
    return timeinfo.tm_year + 1900;
}
int ESPTimeHelper::getMonth()
{
    setSysTime();
    return timeinfo.tm_mon + 1;
}
int ESPTimeHelper::getDay()
{
    setSysTime();
    return timeinfo.tm_mday;
}

int ESPTimeHelper::getDayOfWeek()
{
    setSysTime();
    return timeinfo.tm_wday;
}
String ESPTimeHelper::getDayOfWeekString()
{
    setSysTime();
    return dow[timeinfo.tm_wday];
}

int ESPTimeHelper::getHour()
{
    setSysTime();
    return timeinfo.tm_hour;
}

int ESPTimeHelper::getMin()
{
    setSysTime();
    return timeinfo.tm_min;
}
int ESPTimeHelper::getSec()
{
    setSysTime();
    return timeinfo.tm_sec;
}
int ESPTimeHelper::getNumberOfDayThisYear()
{
    setSysTime();
    return timeinfo.tm_yday + 1;
}

int ESPTimeHelper::totalDays(int y, int m, int d)
{
    static char daytab[2][13] =
        {
            {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
            {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
    int daystotal = d;
    for (int year = 1; year <= y; year++)
    {
        int max_month = (year < y ? 12 : m - 1);
        int leap = (year % 4 == 0);
        if (year % 100 == 0 && year % 400 != 0)
            leap = 0;
        for (int month = 1; month <= max_month; month++)
        {
            daystotal += daytab[leap][month];
        }
    }
    return daystotal;
}
int ESPTimeHelper::getTotalDays(int year, int month, int day)
{
    return totalDays(year, month, day) - totalDays(1970, 1, 1);
}

int ESPTimeHelper::dayofWeek(int year, int month, int day) /* 1 <= m <= 12,  y > 1752 (in the U.K.) */
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    year -= month < 3;
    return (year + year / 4 - year / 100 + year / 400 + t[month - 1] + day) % 7;
}

int ESPTimeHelper::getCurrentSecond()
{
    return (timeinfo.tm_hour * 3600) + (timeinfo.tm_min * 60) + timeinfo.tm_sec;
}
uint64_t ESPTimeHelper::getCurrentTimestamp()
{
    return now;
}
struct tm ESPTimeHelper::getTimeFromSec(int seconds)
{
    struct tm timeinfo;
    int _yrs = seconds / (365 * 24 * 3600);
    seconds = seconds - _yrs * (365 * 24 * 3600);
    timeinfo.tm_year = _yrs - 1900;
    int _months = seconds / (30 * 24 * 3600);
    seconds = seconds - _months * (30 * 24 * 3600);
    timeinfo.tm_mon = _months - 1;
    int _days = seconds / (24 * 3600);
    seconds = seconds - _days * (24 * 3600);
    timeinfo.tm_mday = _days;
    int _hr = seconds / 3600;
    seconds = seconds - _hr * 3600;
    timeinfo.tm_hour = _hr;
    int _min = seconds / 60;
    seconds = seconds - _min * 60;
    timeinfo.tm_min = _min;
    timeinfo.tm_sec = seconds;
    return timeinfo;
}

char *ESPTimeHelper::intStr(int value)
{
    char *buf = (char *)newP(36);
    itoa(value, buf, 10);
    return buf;
}

String ESPTimeHelper::getDateTimeString()
{
    setSysTime();
    MB_String s;

    s = sdow[timeinfo.tm_wday];

    s += FLASH_STR_MCR(", ");
    char *tmp = intStr(timeinfo.tm_mday);
    s += tmp;
    delP(&tmp);

    s += FLASH_STR_MCR(" ");
    s += months[timeinfo.tm_mon];

    s += FLASH_STR_MCR(" ");
    tmp = intStr(timeinfo.tm_year + 1900);
    s += tmp;
    delP(&tmp);

    s += FLASH_STR_MCR(" ");
    if (timeinfo.tm_hour < 10)
        s += FLASH_STR_MCR("0");
    tmp = intStr(timeinfo.tm_hour);
    s += tmp;
    delP(&tmp);

    s += FLASH_STR_MCR(":");
    if (timeinfo.tm_min < 10)
        s += FLASH_STR_MCR("0");
    tmp = intStr(timeinfo.tm_min);
    s += tmp;
    delP(&tmp);

    s += FLASH_STR_MCR(":");
    if (timeinfo.tm_sec < 10)
        s += FLASH_STR_MCR("0");
    tmp = intStr(timeinfo.tm_sec);
    s += tmp;
    delP(&tmp);

    int p = 1;
    if (TZ < 0)
        p = -1;

    float dif = (p * (TZ * 10 - (int)TZ * 10)) * 60.0 / 10;

    if (TZ < 0)
        s += FLASH_STR_MCR(" -");
    else
        s += FLASH_STR_MCR(" +");

    if ((int)TZ < 10)
        s += FLASH_STR_MCR("0");
    tmp = intStr((int)TZ);
    s += tmp;
    delP(&tmp);

    if (dif < 10)
        s += FLASH_STR_MCR("0");
    tmp = intStr((int)dif);
    s += tmp;
    delP(&tmp);

    return s.c_str();
}

time_t ESPTimeHelper::getTimestamp(const char *timeString, bool gmt)
{
    time_t ts = 0;
    std::vector<MB_String> tk = std::vector<MB_String>();
    MB_String s1 = timeString;

    splitTk(s1, tk, (const char *)FLASH_STR_MCR(" "));
    int day = 0, mon = 0, year = 0, hr = 0, mins = 0, sec = 0, tz_h = 0, tz_m = 0;

    if (tk.size() == 6)
    {
        day = atoi(tk[1].c_str());
        for (size_t i = 0; i < 12; i++)
        {
            if (strcmp(months[i], tk[2].c_str()) == 0)
                mon = i;
        }

        year = atoi(tk[3].c_str());
      
        std::vector<MB_String> tk2 = std::vector<MB_String>();
        splitTk(tk[4], tk2, (const char *)FLASH_STR_MCR(":"));
        if (tk2.size() == 3)
        {
            hr = atoi(tk2[0].c_str());
            mins = atoi(tk2[1].c_str());
            sec = atoi(tk2[2].c_str());
        }

        ts = getTimestamp(year, mon + 1, day, hr, mins, sec);

        if (tk[5].length() == 5 && gmt)
        {
            char tmp[6];
            memset(tmp, 0, 6);
            strncpy(tmp, tk[5].c_str() + 1, 2);
            tz_h = atoi(tmp);

            memset(tmp, 0, 6);
            strncpy(tmp, tk[5].c_str() + 3, 2);
            tz_m = atoi(tmp);

            time_t tz = tz_h * 60 * 60 + tz_m * 60;
            if (tk[5][0] == '+')
                ts -= tz; //remove time zone offset
            else
                ts += tz;
        }
    }
    return ts;
}

void ESPTimeHelper::setSysTime()
{
#if defined(ESP32)
    getLocalTime(&timeinfo);
#elif defined(ESP8266)
    now = time(nullptr);
    localtime_r(&now, &timeinfo);
#elif defined(ARDUINO_ARCH_SAMD)
    unsigned long ts = WiFi.getTime();
    if (ts > 0)
        now = ts + TZ * 3600;
    localtime_r(&now, &timeinfo);
#endif
}

int ESPTimeHelper::compareVersion(uint8_t major1, uint8_t minor1, uint8_t patch1, uint8_t major2, uint8_t minor2, uint8_t patch2)
{
    if (major1 > major2)
        return 1;
    else if (major1 < major2)
        return -1;

    if (minor1 > minor2)
        return 1;
    else if (minor1 < minor2)
        return -1;

    if (patch1 > patch2)
        return 1;
    else if (patch1 < patch2)
        return -1;

    return 0;
}

void ESPTimeHelper::splitTk(MB_String &str, std::vector<MB_String> &tk, const char *delim)
{
    std::size_t current, previous = 0;
    current = str.find(delim, previous);
    MB_String s;
    while (current != MB_String::npos)
    {

        s = str.substr(previous, current - previous);

#if defined(MB_STRING_MAJOR) && defined(MB_STRING_MINOR) && defined(MB_STRING_PATCH)
        if (compareVersion(MB_STRING_MAJOR, 1, MB_STRING_MINOR, 0,  MB_STRING_PATCH, 1) >=0)
            s.trim();
#endif

        tk.push_back(s);
        previous = current + strlen(delim);
        current = str.find(delim, previous);
    }
    s = str.substr(previous, current - previous);
    tk.push_back(s);
    MB_String().swap(s);
}

void *ESPTimeHelper::newP(size_t len)
{
    void *p;
#if defined(BOARD_HAS_PSRAM) && defined(ESP_Mail_USE_PSRAM)

    if ((p = (void *)ps_malloc(getReservedLen(len))) == 0)
        return NULL;

#else

#if defined(ESP8266_USE_EXTERNAL_HEAP)
    ESP.setExternalHeap();
#endif

    bool nn = ((p = (void *)malloc(getReservedLen(len))) > 0);

#if defined(ESP8266_USE_EXTERNAL_HEAP)
    ESP.resetHeap();
#endif

    if (!nn)
        return NULL;

#endif
    memset(p, 0, len);
    return p;
}

size_t ESPTimeHelper::getReservedLen(size_t len)
{
    int blen = len + 1;

    int newlen = (blen / 4) * 4;

    if (newlen < blen)
        newlen += 4;

    return (size_t)newlen;
}

void ESPTimeHelper::delP(void *ptr)
{
    void **p = (void **)ptr;
    if (*p)
    {
        free(*p);
        *p = 0;
    }
}

#endif //ESPTimeHelper_CPP
