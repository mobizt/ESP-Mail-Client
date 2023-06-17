/*
 * ESP32 WiFi Client Secure v2.0.4
 *
 * Created June 17, 2023
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 K. Suwatchai (Mobizt)
 *
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

#include <Arduino.h>
#include "ESP_Mail_FS.h"
#if defined(ESP32) && (defined(ENABLE_SMTP) || defined(ENABLE_IMAP))

#include "ESP32_WCS.h"
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <errno.h>

#undef connect
#undef write
#undef read

ESP32_WCS::ESP32_WCS()
{
    _ssl = new ssl_ctx;
    ssl_init(_ssl);

    setClient(nullptr);

    _ssl->handshake_timeout = 120000;
    _use_insecure = false;
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
#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
    if (_client && _use_internal_basic_client)
    {
        delete _client;
        _client = nullptr;
        _ssl->client = nullptr;
        _use_internal_basic_client = false;
    }
#endif
    _use_external_sslclient = false;
    delete _ssl;
}

void ESP32_WCS::setClient(Client *client)
{
#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
    _ssl->client = client;
#endif
}

ESP32_WCS &ESP32_WCS::operator=(const ESP32_WCS &other)
{
    stop();
    _ssl->client = other._ssl->client;
    _use_insecure = other._use_insecure;
    _secured = other._secured;
    return *this;
}

bool ESP32_WCS::begin(const char *host, uint16_t port)
{
    _host = host;
    _port = port;
    return true;
}

void ESP32_WCS::stop()
{
    _host.clear();

    if (!_use_external_sslclient)
        stop_tcp_connection(_ssl, _CA_cert, _cert, _private_key);

    if (_ssl->client)
        _ssl->client->stop();
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

int ESP32_WCS::_connect(const char *host, uint16_t port)
{
    prepareBasicClient();

    if (!_ssl->client)
        return -1;

    _use_external_sslclient = (_ssl->client && _secured && ext_client_type == esp_mail_external_client_type_ssl) ? true : false;

    if (_timeout > 0)
    {
        _ssl->handshake_timeout = _timeout;
    }

    _ssl->client->setTimeout(_timeout);

#if defined(ENABLE_CUSTOM_CLIENT)
    if (connection_cb)
        connection_cb(host, port);
    else
        _ssl->client->connect(host, port);
#else
    _ssl->client->connect(host, port);
#endif

    if (!_ssl->client->connected())
    {
        stop();
        return -1;
    }

    _host = host;
    _port = port;

    return 1;
}

int ESP32_WCS::connect(const char *host, uint16_t port, const char *CA_cert, const char *cert, const char *private_key)
{

    _withCert = true;

    if (!_connect(host, port))
        return 0;

#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)

    // if external SSL Client successfully connected to ssl port
    if (_secured && _use_external_sslclient && ext_client_type == esp_mail_external_client_type_ssl)
        return true;

#endif

    if (!_secured)
        return 1;

    // in case secure connection required and internal client was used or
    // secure connection required and basic external client type was assigned

    int ret = connect_ssl(_ssl, host, CA_cert, cert, private_key, NULL, NULL, _use_insecure);
    _lastError = ret;
    if (ret < 0)
    {
#if !defined(SILENT_MODE)
        log_e("ESP32_WCS Error: upgrade connection, %d", ret);
#endif
        stop();
        return 0;
    }

    return 1;
}

int ESP32_WCS::connect(IPAddress ip, uint16_t port, const char *pskIdent, const char *psKey)
{
    return connect(ip.toString().c_str(), port, pskIdent, psKey);
}

int ESP32_WCS::connect(const char *host, uint16_t port, const char *pskIdent, const char *psKey)
{

    _withKey = true;

#if !defined(SILENT_MODE)
    log_v("ESP32_WCS connect with PSK");
#endif

    if (!_connect(host, port))
        return 0;

#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)

    // if external SSL Client successfully connected to ssl port
    if (_secured && ext_client_type == esp_mail_external_client_type_ssl)
        return true;

#endif

    if (!_secured)
        return 1;

    // in case secure connection required and internal client was used or
    // secure connection required and basic external client type was assigned

    int ret = connect_ssl(_ssl, host, NULL, NULL, NULL, pskIdent, psKey, _use_insecure);
    _lastError = ret;
    if (ret < 0)
    {

#if !defined(SILENT_MODE)
        log_e("ESP32_WCS Error: upgrade connection, %d", ret);
#endif
        stop();
        return 0;
    }

    return 1;
}

bool ESP32_WCS::connectSSL(bool verify)
{

    if (!_ssl->client || !_ssl->client->connected())
        return false;

    setVerify(verify);

    int ret = 0;
    if (_withCert)
        ret = connect_ssl(_ssl, _host.c_str(), _CA_cert, _cert, _private_key, NULL, NULL, _use_insecure);
    else if (_withKey)
        ret = connect_ssl(_ssl, _host.c_str(), NULL, NULL, NULL, _pskIdent, _psKey, _use_insecure);

    _lastError = ret;
    if (ret < 0)
    {

#if !defined(SILENT_MODE)
        log_e("ESP32_WCS Error: upgrade connection, %d", ret);
#endif
        stop();
        return 0;
    }

    _secured = true;

    return 1;
}

void ESP32_WCS::prepareBasicClient()
{
#if !defined(ENABLE_CUSTOM_CLIENT)
    if (!_ssl->client && !_use_internal_basic_client)
    {
        _client = new WiFiClient();
        _ssl->client = _client;
        _use_internal_basic_client = true;
        _use_external_sslclient = false;
    }
#endif
}

int ESP32_WCS::peek()
{
    if (!_ssl->client->connected())
        return 0;

    return _ssl->client->peek();
}

size_t ESP32_WCS::write(uint8_t data)
{
    return write(&data, 1);
}

int ESP32_WCS::read()
{
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
    if (!_ssl->client->connected())
        return 0;

    int res = (!_secured || _use_external_sslclient) ? _ssl->client->write(buf, size) : send_ssl_data(_ssl, buf, size);
    if (res < 0)
    {
        stop();
        res = 0;
    }
    return res;
}

int ESP32_WCS::read(uint8_t *buf, size_t size)
{
    if (!_ssl->client->connected())
        return 0;

    int res = (!_secured || _use_external_sslclient) ? _ssl->client->read(buf, size) : get_ssl_receive(_ssl, buf, size);

    if (res < 0)
        stop();

    return res;
}

int ESP32_WCS::available()
{
    if (!_ssl->client->connected())
        return 0;

    int res = (!_secured || _use_external_sslclient) ? _ssl->client->available() : data_to_read(_ssl);

    if (res < 0)
        stop();

    return res;
}

bool ESP32_WCS::connected()
{

    if (!_ssl->client)
        return 0;

    return _ssl->client->connected();
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
    if (!_ssl)
        return false;

    return verify_ssl_fingerprint(_ssl, fp, domain_name);
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
    if (_ssl)
        _ssl->handshake_timeout = handshake_timeout * 1000;
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
    if (_ssl)
        _ssl->_debugCallback = cb;
}

void ESP32_WCS::flush()
{
    while (available() > 0)
        read();
}

#endif // ESP32

#endif // ESP32_WCS_CPP