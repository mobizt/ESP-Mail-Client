
/*
 *ESP32 WiFi Client Secure v1.0.3
 * 
 * January 24, 2022 
 * 
 * The MIT License (MIT)
 * Copyright (c) 2022 K. Suwatchai (Mobizt)
 * 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
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

/*
  WiFiClientSecure.h - Base class that provides Client SSL to ESP32
  Copyright (c) 2011 Adrian McEwen.  All right reserved.
  Additions Copyright (C) 2017 Evandro Luis Copercini.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef ESP32_WCS_H
#define ESP32_WCS_H

#ifdef ESP32

#include "Arduino.h"
#include "IPAddress.h"
#include "ESP32_SSL_Client.h"
#include <WiFiClient.h>

typedef void (*DebugMsgCallback)(const char *msg);

class ESP32_WCS : public WiFiClient
{
    friend class ESP32_TCP_Client;

protected:
    ESP32_SSL_Client::ssl_data *ssl;

    int _lastError = 0;
    int _peek = -1;
    int _timeout = 0;
    bool _use_insecure;
    const char *_CA_cert;
    const char *_cert;
    const char *_private_key;
    const char *_pskIdent; // identity for PSK cipher suites
    const char *_psKey;    // key in hex for PSK cipher suites

public:
    ESP32_WCS *next;
    ESP32_WCS();
    ESP32_WCS(int socket);
    ESP32_WCS(bool secured);
    ~ESP32_WCS();
    int connect(IPAddress ip, uint16_t port);
    int connect(IPAddress ip, uint16_t port, int32_t timeout);
    int connect(const char *host, uint16_t port);
    int connect(const char *host, uint16_t port, int32_t timeout);
    int connect(IPAddress ip, uint16_t port, const char *rootCABuff, const char *cli_cert, const char *cli_key);
    int connect(const char *host, uint16_t port, const char *rootCABuff, const char *cli_cert, const char *cli_key);
    int connect(IPAddress ip, uint16_t port, const char *pskIdent, const char *psKey);
    int connect(const char *host, uint16_t port, const char *pskIdent, const char *psKey);
    int peek();
    size_t write(uint8_t data);
    size_t write(const uint8_t *buf, size_t size);
    int available();
    int read();
    int read(uint8_t *buf, size_t size);
    void flush() {}
    void stop();
    uint8_t connected();
    int lastError(char *buf, const size_t size);
    void setInsecure();                                            // Don't validate the chain, just accept whatever is given.  VERY INSECURE!
    void setPreSharedKey(const char *pskIdent, const char *psKey); // psKey in Hex
    void setCACert(const char *rootCA);
    void setCertificate(const char *client_ca);
    void setPrivateKey(const char *private_key);
    bool loadCACert(Stream &stream, size_t size);
    bool loadCertificate(Stream &stream, size_t size);
    bool loadPrivateKey(Stream &stream, size_t size);
    bool verify(const char *fingerprint, const char *domain_name);
    void setHandshakeTimeout(unsigned long handshake_timeout);
    int setTimeout(uint32_t seconds) { return 0; }
    void setSecure(bool secure);
    void setVerify(bool verify);
    bool isSecure();
    bool isVerify();
    void setDebugCB(DebugMsgCallback *cb);
    bool connectSSL(bool verify);

    operator bool()
    {
        return connected();
    }
    ESP32_WCS &operator=(const ESP32_WCS &other);
    bool operator==(const bool value)
    {
        return bool() == value;
    }
    bool operator!=(const bool value)
    {
        return bool() != value;
    }
    bool operator==(const ESP32_WCS &);
    bool operator!=(const ESP32_WCS &rhs)
    {
        return !this->operator==(rhs);
    };

    int socket()
    {
        return ssl->socket = -1;
    }

private:
    ESP32_SSL_Client esp32_ssl_client;
    char *_streamLoad(Stream &stream, size_t size);
    bool _secured = true;
    bool _withCert = false;
    bool _withKey = false;
    MB_String _host;
    MB_String _rxBuf;
    int _port;

    int ns_available();
    size_t ns_write(const uint8_t *buf, size_t size);
    size_t ns_read(uint8_t *buf, size_t size);
    int ns_read();
    uint8_t ns_connected();

    //friend class WiFiServer;
    using Print::write;


};

#endif //ESP32

#endif //ESP32_WCS_H
