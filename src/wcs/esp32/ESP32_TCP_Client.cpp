/*
 * ESP32 TCP Client Library v1.0.6.
 * 
 * February 12, 2022
 * 
 * The MIT License (MIT)
 * Copyright (c) 2022 K. Suwatchai (Mobizt)
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
    if (wcs)
    {
        wcs->stop();
        wcs.reset(nullptr);
        wcs.release();
    }
    if (cert_buf)
        mbfs->delP(&cert_buf);
}

void ESP32_TCP_Client::setCACert(const char *caCert)
{
    wcs->setCACert(caCert);
    if (caCert)
        baseSetCertType(esp_mail_cert_type_data);
    else
    {
        setInsecure();
        baseSetCertType(esp_mail_cert_type_none);
    }
    //wcs->setNoDelay(true);
}

void ESP32_TCP_Client::setCertFile(const char *certFile, mb_fs_mem_storage_type storageType)
{
    if (!mbfs)
        return;

    if (strlen(certFile) > 0)
    {
        MB_String filename = certFile;
        if (filename.length() > 0)
        {
            if (filename[0] != '/')
                filename.prepend('/');
        }

        int len = mbfs->open(filename, storageType, mb_fs_open_mode_read);
        if (len > -1)
        {

            if (storageType == mb_fs_mem_storage_type_flash)
            {
                fs::File file = mbfs->getFlashFile();
                wcs->loadCACert(file, len);
                mbfs->close(storageType);
                baseSetCertType(esp_mail_cert_type_file);
            }
            else if (storageType == mb_fs_mem_storage_type_sd)
            {

#if defined(MBFS_ESP32_SDFAT_ENABLED)

                if (cert_buf)
                    mbfs->delP(&cert_buf);

                cert_buf = (char *)mbfs->newP(len);
                if (mbfs->available(storageType))
                    mbfs->read(storageType, (uint8_t *)cert_buf, len);

                mbfs->close(storageType);
                wcs->setCACert((const char *)cert_buf);
                baseSetCertType(esp_mail_cert_type_file);

#elif defined(MBFS_SD_FS)
                fs::File file = mbfs->getSDFile();
                wcs->loadCACert(file, len);
                mbfs->close(storageType);
                baseSetCertType(esp_mail_cert_type_file);

#endif
            }
        }
    }

    //wcs->setNoDelay(true);
}

void ESP32_TCP_Client::setDebugCallback(DebugMsgCallback cb)
{
    debugCallback = std::move(cb);
}

void ESP32_TCP_Client::setTimeout(uint32_t timeoutSec)
{
    if (wcs)
        wcs->setTimeout(timeoutSec);

    baseSetTimeout(timeoutSec);
}

void ESP32_TCP_Client::setInsecure()
{
    wcs->setInsecure();
}

bool ESP32_TCP_Client::ethLinkUp()
{
    if (strcmp(ETH.localIP().toString().c_str(), (const char *)MBSTRING_FLASH_MCR("0.0.0.0")) != 0)
    {
        ETH.linkUp();
        return true;
    }
    return false;
}

void ESP32_TCP_Client::ethDNSWorkAround()
{
}

bool ESP32_TCP_Client::networkReady()
{
    return WiFi.status() == WL_CONNECTED || ethLinkUp();
}

void ESP32_TCP_Client::networkReconnect()
{
    esp_wifi_connect();
}

void ESP32_TCP_Client::networkDisconnect()
{
    WiFi.disconnect();
}

String ESP32_TCP_Client::fwVersion()
{
    return String();
}

esp_mail_client_type ESP32_TCP_Client::type()
{
    return esp_mail_client_type_internal;
}

bool ESP32_TCP_Client::isInitialized() { return true; }

int ESP32_TCP_Client::hostByName(const char *name, IPAddress &ip)
{
    return WiFi.hostByName(name, ip);
}

bool ESP32_TCP_Client::connect(bool secured, bool verify)
{
    wcs->setSecure(secured);
    wcs->setVerify(verify);

    if (connected())
    {
        while (wcs->available() > 0)
            wcs->read();
        return true;
    }

    if (debugCallback)
        wcs->setDebugCB(&debugCallback);

    if (!wcs->connect(host.c_str(), this->port))
        return false;
    return connected();
}

bool ESP32_TCP_Client::connectSSL(bool verify)
{
    if (!wcs)
        return false;
    return wcs->connectSSL(verify);
}

void ESP32_TCP_Client::stop()
{
    if (connected())
        return wcs->stop();
}

bool ESP32_TCP_Client::connected()
{
    if (wcs)
        return wcs->connected();
    return false;
}

int ESP32_TCP_Client::write(uint8_t *data, int len)
{
    if (!data)
        return TCP_CLIENT_ERROR_SEND_DATA_FAILED;

    if (len == 0)
        return TCP_CLIENT_ERROR_SEND_DATA_FAILED;

    if (!connect(wcs->isSecure(), wcs->isVerify()))
        return TCP_CLIENT_ERROR_CONNECTION_REFUSED;

    int toSend = chunkSize;
    int sent = 0;
    while (sent < len)
    {
        if (sent + toSend > len)
            toSend = len - sent;

        if (wcs->write(data + sent, toSend) != toSend)
            return TCP_CLIENT_ERROR_SEND_DATA_FAILED;

        sent += toSend;
    }

    return len;
}

int ESP32_TCP_Client::send(const char *data)
{
    return write((uint8_t *)data, strlen(data));
}

int ESP32_TCP_Client::print(int data)
{
    char *buf = (char *)mbfs->newP(64);
    sprintf(buf, (const char *)FPSTR("%d"), data);
    int ret = send(buf);
    mbfs->delP(&buf);
    return ret;
}

int ESP32_TCP_Client::print(const char *data)
{
    return send(data);
}

int ESP32_TCP_Client::println(const char *data)
{
    int len = send(data);
    if (len < 0)
        return len;
    int sz = send((const char *)FPSTR("\r\n"));
    if (sz < 0)
        return sz;
    return len + sz;
}

int ESP32_TCP_Client::println(int data)
{
    char *buf = (char *)mbfs->newP(64);
    sprintf(buf, (const char *)FPSTR("%d\r\n"), data);
    int ret = send(buf);
    mbfs->delP(&buf);
    return ret;
}

int ESP32_TCP_Client::available()
{
    if (!wcs)
        return TCP_CLIENT_ERROR_NOT_INITIALIZED;

    return wcs->available();
}

int ESP32_TCP_Client::read()
{
    if (!wcs)
        return TCP_CLIENT_ERROR_NOT_INITIALIZED;

    return wcs->read();
}

int ESP32_TCP_Client::readBytes(uint8_t *buf, int len)
{
    if (!wcs)
        return TCP_CLIENT_ERROR_NOT_INITIALIZED;

    return wcs->readBytes(buf, len);
}

int ESP32_TCP_Client::readBytes(char *buf, int len)
{
    return readBytes((uint8_t *)buf, len);
}

#endif //ESP32

#endif //ESP32_TCP_Client_CPP
