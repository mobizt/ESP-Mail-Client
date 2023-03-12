/**
 *
 * The Network Upgradable ESP8266 Secure WiFi Client Class, ESP8266_WCS.cpp v2.0.2
 *
 * Created March 12, 2023
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 K. Suwatchai (Mobizt)
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

#ifndef ESP8266_WCS_CPP
#define ESP8266_WCS_CPP

#include <Arduino.h>
#include "ESP_Mail_FS.h"
#if (defined(ESP8266) || defined(MB_ARDUINO_PICO)) && (defined(ENABLE_SMTP) || defined(ENABLE_IMAP))

#define LWIP_INTERNAL

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "ESP8266_WCS.h"
#include <lwip/tcp.h>
#include <include/ClientContext.h>

ESP8266_WCS::ESP8266_WCS()
{
}

ESP8266_WCS::~ESP8266_WCS()
{
  stop();
#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
  if (_basic_client && _use_internal_basic_client)
  {
    delete _basic_client;
    _basic_client = nullptr;
    ESP_Mail_WC_CLASS::setClient(nullptr);
    _use_internal_basic_client = false;
  }
#endif
}

void ESP8266_WCS::setClient(Client *client)
{
  _basic_client = client;
#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
  ESP_Mail_WCS_CLASS::setClient(client);
#endif
}

int ESP8266_WCS::connect(const char *name, uint16_t port)
{
  prepareBasicClient();

  _host = name;

#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE)

  if (!ESP_Mail_WCS_CLASS::connect(name, port))
  {
    ESP_Mail_WCS_CLASS::stop();
    return 0;
  }
  // if external SSL Client successfully connected to ssl port
  if (_secured && ext_client_type == esp_mail_external_client_type_ssl)
    return true;

#else

  if (!ESP_Mail_WC_CLASS::connect(name, port))
  {
    ESP_Mail_WC_CLASS::stop();
    return 0;
  }

#endif

  if (!_secured)
    return 1;

  bool res = ESP_Mail_WCS_CLASS::_connectSSL(_host.c_str());

  if (!res)
    ESP_Mail_WCS_CLASS::stop();

  return res;
}

bool ESP8266_WCS::connectSSL(bool verify)
{
  setVerify(verify);

  bool res = ESP_Mail_WCS_CLASS::_connectSSL(_host.c_str());

  if (res)
    _secured = true;
  else
    ESP_Mail_WCS_CLASS::stop();

  return res;
}

uint8_t ESP8266_WCS::_connected()
{

#if !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
  if (!_secured)
    return ns_connected();
#endif

  return ESP_Mail_WCS_CLASS::connected();
}

void ESP8266_WCS::setTimeout(unsigned long timeout)
{
  ESP_Mail_WCS_CLASS::setTimeout(timeout);
}

void ESP8266_WCS::stop()
{
  _host.clear();
  ESP_Mail_WCS_CLASS::stop();
}

int ESP8266_WCS::available()
{

#if !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
  if (!_secured)
    return ns_available();
#endif

  return ESP_Mail_WCS_CLASS::available();
}

int ESP8266_WCS::read()
{

#if !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
  if (!_secured)
    return ns_read();
#endif

  return ESP_Mail_WCS_CLASS::read();
}

int ESP8266_WCS::read(uint8_t *buf, size_t size)
{

#if !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
  if (!_secured)
    return ns_read(buf, size);
#endif

  return ESP_Mail_WCS_CLASS::read(buf, size);
}

size_t ESP8266_WCS::write(const uint8_t *buf, size_t size)
{

#if !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
  if (!_secured)
    return ns_write(buf, size);
#endif

  return ESP_Mail_WCS_CLASS::write(buf, size);
}

void ESP8266_WCS::prepareBasicClient()
{
#if !defined(ENABLE_CUSTOM_CLIENT) && defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
  if (!_basic_client && !_use_internal_basic_client)
  {
    _basic_client = new WiFiClient();
    ESP_Mail_WCS_CLASS::setClient(_basic_client);
    _use_internal_basic_client = true;
  }
#endif
}

int ESP8266_WCS::peek()
{
#if !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
  if (!_secured)
    return ns_peek();
#endif
  return ESP_Mail_WCS_CLASS::peek();
}

void ESP8266_WCS::setTA(bool hasTA)
{
  _has_ta = hasTA;
}

void ESP8266_WCS::setSecure(bool secure)
{
  _secured = secure;
}

void ESP8266_WCS::setVerify(bool verify)
{
  if (_has_ta)
    _base_use_insecure = !verify;

  if (_base_use_insecure)
    ESP_Mail_WCS_CLASS::setInsecure();
}

bool ESP8266_WCS::isSecure()
{
  return _secured;
}

bool ESP8266_WCS::isVerify()
{
  return !_base_use_insecure;
}

#if !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
size_t ESP8266_WCS::ns_write(uint8_t b)
{
  return ESP_Mail_WCS_CLASS::write(&b, 1);
}

size_t ESP8266_WCS::ns_write(const uint8_t *buf, size_t size)
{

  if (!_client || !size)
    return 0;

  _client->setTimeout(_timeout);
#if defined(ESP8266_CORE_SDK_V3_X_X)
  return _client->write((const char *)buf, size);
#else
  return _client->write(buf, size);
#endif
}

size_t ESP8266_WCS::ns_write(Stream &stream, size_t unused)
{
  (void)unused;
  return ESP_Mail_WC_CLASS::write(stream);
}

size_t ESP8266_WCS::ns_write(Stream &stream)
{
  if (!_client || !stream.available())
    return 0;
  _client->setTimeout(_timeout);
#if defined(ESP8266_CORE_SDK_V3_X_X)
  size_t dl = stream.available();
  uint8_t buf[dl];
  stream.readBytes(buf, dl);
  return _client->write((const char *)buf, dl);
#else
  return _client->write(stream);
#endif
}

int ESP8266_WCS::ns_available()
{
  if (!_client)
    return false;

  int result = _client->getSize();

  if (!result)
    optimistic_yield(100);

  return result;
}

int ESP8266_WCS::ns_read()
{
  if (!ns_available())
    return -1;

  return _client->read();
}

int ESP8266_WCS::ns_read(uint8_t *buf, size_t size)
{
  return (int)_client->read(reinterpret_cast<char *>(buf), size);
}

int ESP8266_WCS::ns_peek()
{
  if (!ns_available())
    return -1;

  return _client->peek();
}

uint8_t ESP8266_WCS::ns_connected()
{
  if (!_client || _client->state() == CLOSED)
    return 0;

  return _client->state() == ESTABLISHED || ns_available();
}

#endif

#endif /* ESP8266 */

#endif /* ESP8266_WCS_CPP */