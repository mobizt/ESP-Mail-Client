/*
 * ESP32 TCP Client Library. 
 * 
 * v 1.0.1
 * 
 * The MIT License (MIT)
 * Copyright (c) 2021 K. Suwatchai (Mobizt)
 * 
 * TCPClient Arduino library for ESP32
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the TCPClient for Arduino.
 * Port to ESP32 by Evandro Luis Copercini (2017), 
 * changed fingerprints to CA verification. 	
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
*/

#ifndef ESP32_TCP_Client_CPP
#define ESP32_TCP_Client_CPP

#ifdef ESP32

#include "ESP32_TCP_Client.h"

ESP32_TCP_Client::ESP32_TCP_Client()
{
}

ESP32_TCP_Client::~ESP32_TCP_Client()
{
    if (_wcs)
    {
        _wcs->stop();
        _wcs.reset(nullptr);
        _wcs.release();
    }
    std::string().swap(_host);
    std::string().swap(_caCertFile);
}

bool ESP32_TCP_Client::begin(const char *host, uint16_t port)
{
    _host = host;
    _port = port;
    return true;
}

bool ESP32_TCP_Client::connected()
{
    if (_wcs)
        return _wcs->connected();
    return false;
}

int ESP32_TCP_Client::send(const char *data)
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

ESP32_WCS *ESP32_TCP_Client::stream(void)
{
    if (connected())
        return _wcs.get();
    return nullptr;
}

bool ESP32_TCP_Client::connect(void)
{
    return connect(false, false);
}

bool ESP32_TCP_Client::connect(bool secured, bool verify)
{
    _wcs->setSecure(secured);
    _wcs->setVerify(verify);

    if (connected())
    {
        while (_wcs->available() > 0)
            _wcs->read();
        return true;
    }

    if (_debugCallback)
        _wcs->setDebugCB(&_debugCallback);

    if (!_wcs->connect(_host.c_str(), _port))
        return false;
    return connected();
}

void ESP32_TCP_Client::setDebugCallback(DebugMsgCallback cb)
{
    _debugCallback = std::move(cb);
}

void ESP32_TCP_Client::setCACert(const char *caCert)
{
    _wcs->setCACert(caCert);
    if (caCert)
        _certType = 1;
    else
    {
        setInsecure();
        _certType = 0;
    }
    //_wcs->setNoDelay(true);
}

void ESP32_TCP_Client::setCertFile(const char *caCertFile, esp_mail_file_storage_type storageType)
{

    if (strlen(caCertFile) > 0)
    {
        File f;
        if (storageType == esp_mail_file_storage_type_flash)
        {
            ESP_MAIL_FLASH_FS.begin();
            if (ESP_MAIL_FLASH_FS.exists(caCertFile))
                f = ESP_MAIL_FLASH_FS.open(caCertFile, FILE_READ);
        }
        else if (storageType == esp_mail_file_storage_type_sd)
        {
            ESP_MAIL_SD_FS.begin();
            if (ESP_MAIL_SD_FS.exists(caCertFile))
                f = ESP_MAIL_SD_FS.open(caCertFile, FILE_READ);
        }

        if (f)
        {
            size_t len = f.size();
            _wcs->loadCACert(f, len);
            f.close();
        }
        _certType = 2;
    }
    //_wcs->setNoDelay(true);
}

void ESP32_TCP_Client::setInsecure()
{
    _wcs->setInsecure();
}

#endif //ESP32

#endif //ESP32_TCP_Client_CPP
