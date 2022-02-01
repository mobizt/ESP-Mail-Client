/*
 * ESP32 WiFi Client Secure v1.0.3
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
  WiFiClientSecure.cpp - Client Secure class for ESP32
  Copyright (c) 2016 Hristo Gochkov  All right reserved.
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

#ifndef ESP32_WCS_CPP
#define ESP32_WCS_CPP

#ifdef ESP32

#include "ESP32_WCS.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <errno.h>

#undef connect
#undef write
#undef read

ESP32_WCS::ESP32_WCS()
{
    _connected = false;

    ssl = new ESP32_SSL_Client::ssl_data;
    esp32_ssl_client.ssl_init(ssl);
    ssl->socket = -1;
    ssl->handshake_timeout = 120000;
    _use_insecure = false;
    _CA_cert = NULL;
    _cert = NULL;
    _private_key = NULL;
    _pskIdent = NULL;
    _psKey = NULL;
    next = NULL;
}

ESP32_WCS::ESP32_WCS(int sock)
{
    _connected = false;
    _timeout = 0;

    ssl = new ESP32_SSL_Client::ssl_data;
    esp32_ssl_client.ssl_init(ssl);
    ssl->socket = sock;
    ssl->handshake_timeout = 120000;

    if (sock >= 0)
    {
        _connected = true;
    }

    _CA_cert = NULL;
    _cert = NULL;
    _private_key = NULL;
    _pskIdent = NULL;
    _psKey = NULL;
    next = NULL;
}

ESP32_WCS::ESP32_WCS(bool secured)
{
    _connected = false;

    ssl = new ESP32_SSL_Client::ssl_data;
    esp32_ssl_client.ssl_init(ssl);
    ssl->socket = -1;
    ssl->handshake_timeout = 120000;
    _use_insecure = !secured;
    _secured = secured;
    _CA_cert = NULL;
    _cert = NULL;
    _private_key = NULL;
    _pskIdent = NULL;
    _psKey = NULL;
    next = NULL;
}

ESP32_WCS::~ESP32_WCS()
{
    stop();
    delete ssl;
}

ESP32_WCS &ESP32_WCS::operator=(const ESP32_WCS &other)
{
    stop();
    ssl->socket = other.ssl->socket;
    _connected = other._connected;
    return *this;
}

void ESP32_WCS::stop()
{
    if (ssl->socket >= 0)
    {
        close(ssl->socket);
        ssl->socket = -1;
        _connected = false;
        _peek = -1;
    }
    esp32_ssl_client.stop_ssl_socket(ssl, _CA_cert, _cert, _private_key);
}

int ESP32_WCS::connect(IPAddress ip, uint16_t port)
{
    if (_pskIdent && _psKey)
        return connect(ip, port, _pskIdent, _psKey);
    return connect(ip, port, _CA_cert, _cert, _private_key);
}

int ESP32_WCS::connect(IPAddress ip, uint16_t port, int32_t timeout)
{
    _timeout = timeout;
    return connect(ip, port);
}

int ESP32_WCS::connect(const char *host, uint16_t port)
{
    if (_pskIdent && _psKey)
        return connect(host, port, _pskIdent, _psKey);
    return connect(host, port, _CA_cert, _cert, _private_key);
}

int ESP32_WCS::connect(const char *host, uint16_t port, int32_t timeout)
{
    _timeout = timeout;
    return connect(host, port);
}

int ESP32_WCS::connect(IPAddress ip, uint16_t port, const char *CA_cert, const char *cert, const char *private_key)
{
    return connect(ip.toString().c_str(), port, CA_cert, cert, private_key);
}

int ESP32_WCS::connect(const char *host, uint16_t port, const char *CA_cert, const char *cert, const char *private_key)
{
    _host = host;
    _port = port;
    _withCert = true;

    if (_timeout > 0)
    {
        ssl->handshake_timeout = _timeout;
    }

    int ret = esp32_ssl_client.start_socket(ssl, host, port, _timeout, CA_cert, cert, private_key, NULL, NULL, _use_insecure);

    _lastError = ret;
    if (ret < 0)
    {
        log_e("startesp32_ssl_client: %d", ret);
        stop();
        return 0;
    }

    if (_secured)
    {
        ret = esp32_ssl_client.start_ssl_client(ssl, host, port, _timeout, CA_cert, cert, private_key, NULL, NULL, _use_insecure);
        _lastError = ret;
        if (ret < 0)
        {
            log_e("startesp32_ssl_client: %d", ret);
            stop();
            return 0;
        }
    }

    _connected = true;
    return 1;
}

int ESP32_WCS::connect(IPAddress ip, uint16_t port, const char *pskIdent, const char *psKey)
{
    return connect(ip.toString().c_str(), port, pskIdent, psKey);
}

int ESP32_WCS::connect(const char *host, uint16_t port, const char *pskIdent, const char *psKey)
{
    _host = host;
    _port = port;
    _withKey = true;

    log_v("startesp32_ssl_client with PSK");
    if (_timeout > 0)
    {
        ssl->handshake_timeout = _timeout;
    }

    int ret = esp32_ssl_client.start_socket(ssl, host, port, _timeout, NULL, NULL, NULL, pskIdent, psKey, _use_insecure);
    _lastError = ret;
    if (ret < 0)
    {
        log_e("startesp32_ssl_client: %d", ret);
        stop();
        return 0;
    }

    if (_secured)
    {
        ret = esp32_ssl_client.start_ssl_client(ssl, host, port, _timeout, NULL, NULL, NULL, pskIdent, psKey, _use_insecure);
        _lastError = ret;
        if (ret < 0)
        {
            log_e("startesp32_ssl_client: %d", ret);
            stop();
            return 0;
        }
    }
    _connected = true;
    return 1;
}

int ESP32_WCS::peek()
{
    if (_peek >= 0)
    {
        return _peek;
    }
    _peek = timedRead();
    return _peek;
}

size_t ESP32_WCS::write(uint8_t data)
{
    return write(&data, 1);
}

int ESP32_WCS::read()
{
    if (!_secured)
        return ns_read();

    uint8_t data = -1;
    int res = read(&data, 1);
    if (res < 0)
    {
        return res;
    }
    return data;
}

size_t ESP32_WCS::write(const uint8_t *buf, size_t size)
{
    if (!_connected)
        return 0;

    if (!_secured)
        return ns_write(buf, size);

    int res = esp32_ssl_client.send_ssl_data(ssl, buf, size);
    if (res < 0)
    {
        stop();
        res = 0;
    }
    return res;
}

int ESP32_WCS::read(uint8_t *buf, size_t size)
{
    if (!_secured)
        return ns_read(buf, size);

    int peeked = 0;
    int avail = available();
    if ((!buf && size) || avail <= 0)
    {
        return -1;
    }
    if (!size)
    {
        return 0;
    }
    if (_peek >= 0)
    {
        buf[0] = _peek;
        _peek = -1;
        size--;
        avail--;
        if (!size || !avail)
        {
            return 1;
        }
        buf++;
        peeked = 1;
    }

    int res = esp32_ssl_client.get_ssl_receive(ssl, buf, size);
    if (res < 0)
    {
        stop();
        return peeked ? peeked : res;
    }
    return res + peeked;
}

int ESP32_WCS::available()
{
    if (!_secured)
        return ns_available();

    int peeked = (_peek >= 0);
    if (!_connected)
    {
        return peeked;
    }
    int res = esp32_ssl_client.data_to_read(ssl);
    if (res < 0)
    {
        stop();
        return peeked ? peeked : res;
    }
    return res + peeked;
}

uint8_t ESP32_WCS::connected()
{
    if (!_secured)
        return ns_connected();

    uint8_t dummy = 0;
    read(&dummy, 0);

    return _connected;
}

void ESP32_WCS::setInsecure()
{
    _CA_cert = NULL;
    _cert = NULL;
    _private_key = NULL;
    _pskIdent = NULL;
    _psKey = NULL;
    _use_insecure = true;
}

void ESP32_WCS::setCACert(const char *rootCA)
{
    _CA_cert = rootCA;
}

void ESP32_WCS::setCertificate(const char *client_ca)
{
    _cert = client_ca;
}

void ESP32_WCS::setPrivateKey(const char *private_key)
{
    _private_key = private_key;
}

void ESP32_WCS::setPreSharedKey(const char *pskIdent, const char *psKey)
{
    _pskIdent = pskIdent;
    _psKey = psKey;
}

bool ESP32_WCS::verify(const char *fp, const char *domain_name)
{
    if (!ssl)
        return false;

    return esp32_ssl_client.verify_ssl_fingerprint(ssl, fp, domain_name);
}

char *ESP32_WCS::_streamLoad(Stream &stream, size_t size)
{
    char *dest = (char *)malloc(size + 1);
    if (!dest)
    {
        return nullptr;
    }
    if (size != stream.readBytes(dest, size))
    {
        free(dest);
        dest = nullptr;
        return nullptr;
    }
    dest[size] = '\0';
    return dest;
}

bool ESP32_WCS::loadCACert(Stream &stream, size_t size)
{
    char *dest = _streamLoad(stream, size);
    bool ret = false;
    if (dest)
    {
        setCACert(dest);
        ret = true;
    }
    return ret;
}

bool ESP32_WCS::loadCertificate(Stream &stream, size_t size)
{
    char *dest = _streamLoad(stream, size);
    bool ret = false;
    if (dest)
    {
        setCertificate(dest);
        ret = true;
    }
    return ret;
}

bool ESP32_WCS::loadPrivateKey(Stream &stream, size_t size)
{
    char *dest = _streamLoad(stream, size);
    bool ret = false;
    if (dest)
    {
        setPrivateKey(dest);
        ret = true;
    }
    return ret;
}

int ESP32_WCS::lastError(char *buf, const size_t size)
{
    if (!_lastError)
    {
        return 0;
    }
    mbedtls_strerror(_lastError, buf, size);
    return _lastError;
}

void ESP32_WCS::setHandshakeTimeout(unsigned long handshake_timeout)
{
    ssl->handshake_timeout = handshake_timeout * 1000;
}

void ESP32_WCS::setSecure(bool secure)
{
    _secured = secure;
}

void ESP32_WCS::setVerify(bool verify)
{
    if (_CA_cert)
        _use_insecure = !verify;
}

bool ESP32_WCS::isSecure()
{
   return _secured;
}

bool ESP32_WCS::isVerify()
{
    return !_use_insecure;
}

void ESP32_WCS::setDebugCB(DebugMsgCallback *cb)
{
    ssl->_debugCallback = cb;
}

int ESP32_WCS::ns_available()
{
    if (ssl->socket < 0)
        return false;

    if (_rxBuf.length() == 0)
    {
        int bufLen = 1024;
        uint8_t *tmp = new uint8_t[bufLen];
        memset(tmp, 0, bufLen);
        int ret = esp32_ssl_client.ns_lwip_read(ssl, tmp, bufLen);
        if (ret > 0)
            _rxBuf += (char *)tmp;
        delete[] tmp;
    }

    int result = _rxBuf.length();

    if (!result)
    {
        optimistic_yield(100);
    }
    return result;
}

size_t ESP32_WCS::ns_write(const uint8_t *buf, size_t size)
{
    if (ssl->socket < 0 || !size)
        return 0;
    return esp32_ssl_client.ns_lwip_write(ssl, buf, size);
}

size_t ESP32_WCS::ns_read(uint8_t *buf, size_t size)
{
    if (_rxBuf.length() == 0)
        return esp32_ssl_client.ns_lwip_read(ssl, buf, size);
    else
    {
        size_t sz = size;
        if (sz > _rxBuf.length())
            sz = _rxBuf.length();
        memcpy(buf, _rxBuf.c_str(), sz);
        _rxBuf.erase(0, sz);
        return sz;
    }
}

int ESP32_WCS::ns_read()
{
    int c = -1;
    if (_rxBuf.length() == 0)
    {
        uint8_t *buf = new uint8_t[2];
        memset(buf, 0, 2);
        int ret = esp32_ssl_client.ns_lwip_read(ssl, buf, 1);
        if (ret > 0)
            c = buf[0];
        delete[] buf;
    }
    else
    {
        c = _rxBuf.c_str()[0];
        _rxBuf.erase(0, 1);
    }

    return c;
}

uint8_t ESP32_WCS::ns_connected()
{
    return ssl->socket >= 0;
}

bool ESP32_WCS::connectSSL(bool verify)
{
    setVerify(verify);

    int ret = 0;
    if (_withCert)
        ret = esp32_ssl_client.start_ssl_client(ssl, _host.c_str(), _port, _timeout, _CA_cert, _cert, _private_key, NULL, NULL, _use_insecure);
    else if (_withKey)
        ret = esp32_ssl_client.start_ssl_client(ssl, _host.c_str(), _port, _timeout, NULL, NULL, NULL, _pskIdent, _psKey, _use_insecure);

    _lastError = ret;
    if (ret < 0)
    {
        log_e("startesp32_ssl_client: %d", ret);
        stop();
        return 0;
    }

    _secured = true;

    return 1;
}

#endif //ESP32

#endif //ESP32_WCS_CPP