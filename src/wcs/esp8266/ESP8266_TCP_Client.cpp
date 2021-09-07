/**
 * 
 * The Network Upgradable ESP8266 Secure TCP Client Class, ESP8266_TCP_Client.cpp v1.0.0
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

#ifndef ESP8266_TCP_Client_CPP
#define ESP8266_TCP_Client_CPP

#ifdef ESP8266

#include "ESP8266_TCP_Client.h"

ESP8266_TCP_Client::ESP8266_TCP_Client()
{
}

ESP8266_TCP_Client::~ESP8266_TCP_Client()
{
  if (_wcs)
  {
    _wcs->stop();
    _wcs.reset(nullptr);
    _wcs.release();
  }

#ifndef USING_AXTLS
  if (x509)
    delete x509;
#endif

  std::string().swap(_host);
  std::string().swap(_caCertFile);
  _cacert.reset(new char);
  _cacert = nullptr;
}

bool ESP8266_TCP_Client::begin(const char *host, uint16_t port)
{
  if (strcmp(_host.c_str(), host) != 0)
    mflnChecked = false;

  _host = host;
  _port = port;

  //probe for fragmentation support at the specified size
  if (!mflnChecked)
  {
    fragmentable = WiFiClientSecure::probeMaxFragmentLength(_host.c_str(), _port, chunkSize);
    if (fragmentable)
    {
      _bsslRxSize = chunkSize;
      _bsslTxSize = chunkSize;
      _wcs->setBufferSizes(_bsslRxSize, _bsslTxSize);
    }
    mflnChecked = true;
  }

  if (!fragmentable)
    _wcs->setBufferSizes(maxRXBufSize / rxBufDivider, maxTXBufSize / txBufDivider);

  return true;
}

bool ESP8266_TCP_Client::connected()
{
  if (_wcs)
    return _wcs->connected();
  return false;
}

int ESP8266_TCP_Client::send(const char *data)
{
  if (!connect(_wcs->isSecure(), _wcs->isVerify()))
  {
    return TCP_CLIENT_ERROR_CONNECTION_REFUSED;
  }

  if (_wcs->write((const uint8_t *)data, strlen(data)) != strlen(data))
  {
    return TCP_CLIENT_ERROR_SEND_DATA_FAILED;
  }

  return strlen(data);
}

ESP8266_WCS *ESP8266_TCP_Client::stream(void)
{
  if (connected())
    return _wcs.get();
  return nullptr;
}

bool ESP8266_TCP_Client::connect(bool secured, bool verify)
{
  _wcs->setSecure(secured);
  _wcs->setVerify(verify);

  if (connected())
  {
    while (_wcs->available() > 0)
      _wcs->read();
    return true;
  }

  if (!_wcs->connect(_host.c_str(), _port))
    return false;

  return connected();
}

void ESP8266_TCP_Client::setCACert(const char *caCert)
{

#ifndef USING_AXTLS
  _wcs->setBufferSizes(_bsslRxSize, _bsslTxSize);
#endif

  if (caCert)
  {
#ifndef USING_AXTLS
    if (x509)
      delete x509;
    x509 = new X509List(caCert);
    _wcs->setTrustAnchors(x509);
    _wcs->setTA(true);
#else
    _wcs->setCACert_P(caCert, strlen_P(caCert));
#endif
    _certType = 1;
  }
  else
  {
#ifndef USING_AXTLS
    _wcs->setInsecure();
#endif
    _certType = 0;
    _wcs->setTA(false);
  }

  _wcs->setNoDelay(true);
}

void ESP8266_TCP_Client::setCertFile(const char *caCertFile, esp_mail_file_storage_type storageType, uint8_t sdPin)
{

#ifndef USING_AXTLS
  _sdPin = sdPin;
  _wcs->setBufferSizes(_bsslRxSize, _bsslTxSize);

  if (_clockReady && strlen(caCertFile) > 0)
  {

    fs::File f;
    if (storageType == esp_mail_file_storage_type_flash)
    {
      ESP_MAIL_FLASH_FS.begin();
      if (ESP_MAIL_FLASH_FS.exists(caCertFile))
        f = ESP_MAIL_FLASH_FS.open(caCertFile, "r");
    }
    else if (storageType == esp_mail_file_storage_type_sd)
    {
      ESP_MAIL_SD_FS.begin(_sdPin);
      if (ESP_MAIL_SD_FS.exists(caCertFile))
        f = ESP_MAIL_SD_FS.open(caCertFile, FILE_READ);
    }

    if (f)
    {
      size_t len = f.size();
      uint8_t *der = new uint8_t[len];
      if (f.available())
        f.read(der, len);
      f.close();
      if (x509)
        delete x509;
      x509 = new X509List(der, len);
      _wcs->setTrustAnchors(x509);
      _wcs->setTA(true);
      delete[] der;
    }
    _certType = 2;
  }
#endif

  _wcs->setNoDelay(true);
}

#endif /* ESP8266 */

#endif /* ESP8266_TCP_Client_CPP */