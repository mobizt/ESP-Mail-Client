/*
 * TCP Client Base class, version 1.0.2
 *
 * February 28, 2022
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

#ifndef TCP_CLIENT_BASE_H
#define TCP_CLIENT_BASE_H

#include <Arduino.h>
#include "ESP_Mail_Const.h"
#include <IPAddress.h>
#include <Client.h>

#define TCP_CLIENT_DEFAULT_TCP_TIMEOUT_SEC 30

typedef enum
{
    esp_mail_cert_type_undefined = -1,
    esp_mail_cert_type_none = 0,
    esp_mail_cert_type_data,
    esp_mail_cert_type_file

} esp_mail_cert_type;

typedef enum
{
    esp_mail_client_type_undefined,
    esp_mail_client_type_internal,
    esp_mail_client_type_custom

} esp_mail_client_type;

class TCP_Client_Base
{
    friend class ESP_Mail_Client;
    friend class ESP32_WCS;

public:
    TCP_Client_Base()
    {
        certType = esp_mail_cert_type_undefined;
    };
    virtual ~TCP_Client_Base(){};

    virtual void ethDNSWorkAround(){};

    virtual bool networkReady() { return false; }

    virtual void networkReconnect(){};

    virtual void disconnect(){};

    virtual time_t getTime()
    {
        time_t tm = now;
      
#if defined(MB_MCU_ESP) || defined(MB_MCU_ATMEL_ARM) || defined(MB_MCU_RP2040)
        if (tm < ESP_MAIL_CLIENT_VALID_TS)
            tm = time(nullptr);
#else
        tm += millis() / 1000;
#endif

        return tm;
    }

    virtual bool setSystemTime(time_t ts)
    {

#if defined(ESP8266) || defined(ESP32)

            if (setTimestamp(ts) == 0)
            {
                this->now = time(nullptr);
                return true;
            }

#else
            if (ts > ESP_MAIL_CLIENT_VALID_TS)
                this->now = ts - (millis() / 1000);

#endif

        return false;
    }

    virtual String fwVersion()
    {
        return String();
    }

    virtual esp_mail_client_type type() { return esp_mail_client_type_undefined; }

    virtual bool isInitialized() { return false; }

    virtual int hostByName(const char *name, IPAddress &ip) { return 0; }

    virtual bool begin(const char *host, uint16_t port)
    {
        this->host = host;
        this->port = port;
        return true;
    };

    virtual bool connect(bool secured, bool verify) { return false; }

    virtual bool connectSSL(bool verify) { return false; }

    virtual void stop(){};

    virtual bool connected() { return false; }

    virtual int write(uint8_t *data, int len) { return 0; }

    virtual int send(const char *data) { return 0; }

    virtual int print(const char *data) { return 0; }

    virtual int print(int data) { return 0; }

    virtual int println(const char *data) { return 0; }

    virtual int println(int data) { return 0; }

    virtual int available() { return 0; }

    virtual int read() { return 0; }

    virtual int readBytes(uint8_t *buf, int len) { return 0; }

    virtual int readBytes(char *buf, int len) { return 0; }

    void baseSetCertType(esp_mail_cert_type type) { certType = type; }

    void baseSetTimeout(uint32_t timeoutSec) { tmo = timeoutSec * 1000; }

    int setTimestamp(time_t ts)
    {
#if defined(ESP32) || defined(ESP8266)
        struct timeval tm = {ts, 0}; // sec, us
        return settimeofday((const timeval *)&tm, 0);
#endif
        return -1;
    }

private:
    void setMBFS(MB_FS *mbfs) { this->mbfs = mbfs; }
#if defined(ENABLE_IMAP) || defined(ENABLE_SMTP)
    void setSession(ESP_Mail_Session *session)
    {
        this->session = session;
    }
#endif
    int tcpTimeout()
    {
        return tmo;
    }
    esp_mail_cert_type getCertType() { return certType; }
    esp_mail_cert_type certType = esp_mail_cert_type_undefined;

protected:
    MB_String host;
    uint16_t port = 0;
    MB_FS *mbfs = nullptr;
    int chunkSize = 4096;
    int tmo = 40000; // 40 sec
    bool clockReady = false;
    time_t now = 0;
#if defined(ENABLE_IMAP) || defined(ENABLE_SMTP)
    ESP_Mail_Session *session = nullptr;
#endif
};

#endif
