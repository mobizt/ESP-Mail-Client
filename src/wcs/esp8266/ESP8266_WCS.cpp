/**
 * 
 * The Network Upgradable ESP8266 Secure WiFi Client Class, ESP8266_WCS.cpp v1.0.3
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

#ifndef ESP8266_WCS_CPP
#define ESP8266_WCS_CPP

#ifdef ESP8266

#define LWIP_INTERNAL

#include <list>
#include <errno.h>
#include <algorithm>

extern "C"
{
#include <osapi.h>
#include <ets_sys.h>
}
#include <debug.h>
#include <ESP8266WiFi.h>
#include <PolledTimeout.h>
#include <WiFiClient.h>
#include "ESP8266_WCS.h"
#include <StackThunk.h>
#include <lwip/opt.h>
#include <lwip/ip.h>
#include <lwip/tcp.h>
#include <lwip/inet.h>
#include <lwip/netif.h>
#include <include/ClientContext.h>
#include <c_types.h>
#include <coredecls.h>


ESP8266_WCS::ESP8266_WCS()
{
}

ESP8266_WCS::~ESP8266_WCS()
{
}

int ESP8266_WCS::connect(const char *name, uint16_t port)
{

  IPAddress remote_addr;

  if (!WiFi.hostByName(name, remote_addr))
  {
    DEBUG_BSSL("connect: Name loopup failure\n");
    return 0;
  }

  if (!WC_CLASS::connect(remote_addr, port))
  {
    DEBUG_BSSL("connect: Unable to connect TCP socket\n");
    return 0;
  }

  _host_name = name;

  if (!_secured)
    return 1;

  return WCS_CLASS::_connectSSL(name);
}

uint8_t ESP8266_WCS::connected()
{
  if (!_secured)
    return ns_connected();

  return WCS_CLASS::connected();
}

int ESP8266_WCS::available()
{
  if (!_secured)
    return ns_available();

  return WCS_CLASS::available();
}

int ESP8266_WCS::read()
{
  if (!_secured)
    return ns_read();
  return WCS_CLASS::read();
}

int ESP8266_WCS::read(uint8_t *buf, size_t size)
{
  if (!_secured)
    return ns_read(buf, size);
  return WCS_CLASS::read(buf, size);
}

size_t ESP8266_WCS::write(const uint8_t *buf, size_t size)
{
  if (!_secured)
    return ns_write(buf, size);
  return WCS_CLASS::write(buf, size);
}

size_t ESP8266_WCS::write_P(PGM_P buf, size_t size)
{
  if (!_secured)
    return ns_write_P(buf, size);
  return WCS_CLASS::write_P(buf, size);
}

int ESP8266_WCS::peek()
{
  if (!_secured)
    return ns_peek();
  return WCS_CLASS::peek();
}

size_t ESP8266_WCS::peekBytes(uint8_t *buffer, size_t length)
{
  if (!_secured)
    return ns_peekBytes(buffer, length);

  return WCS_CLASS::peekBytes(buffer, length);
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
    WCS_CLASS::setInsecure();
}

bool ESP8266_WCS::isSecure()
{
  return _secured;
}

bool ESP8266_WCS::isVerify()
{
  return !_base_use_insecure;
}

bool ESP8266_WCS::connectSSL(bool verify)
{
  setVerify(verify);

  bool ret = WCS_CLASS::_connectSSL(_host_name.c_str());
  if (ret)
    _secured = true;
  return ret;
}

size_t ESP8266_WCS::ns_write(uint8_t b)
{
  return WCS_CLASS::write(&b, 1);
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
  return WC_CLASS::write(stream);
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

size_t ESP8266_WCS::ns_write_P(PGM_P buf, size_t size)
{

  if (!_client || !size)
    return 0;

  _client->setTimeout(_timeout);

#if defined(ESP8266_CORE_SDK_V3_X_X)
  char dest[size];
  memcpy_P((void *)dest, (PGM_VOID_P)buf, size);
  return _client->write((const char *)dest, size);
#else
  return _client->write_P(buf, size);
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

size_t ESP8266_WCS::ns_peekBytes(uint8_t *buffer, size_t length)
{
  size_t count = 0;

  if (!_client)
    return 0;

  _startMillis = millis();

  while ((ns_available() < (int)length) && ((millis() - _startMillis) < _timeout))
  {
    yield();
  }

  count = (ns_available() < (int)length) ? ns_available() : length;

  return _client->peekBytes((char *)buffer, count);
}

uint8_t ESP8266_WCS::ns_connected()
{
  if (!_client || _client->state() == CLOSED)
    return 0;

  return _client->state() == ESTABLISHED || ns_available();
}



#endif /* ESP8266 */

#endif /* ESP8266_WCS_CPP */