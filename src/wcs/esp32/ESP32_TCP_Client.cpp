/*
 * ESP32 TCP Client Library v2.0.12
 *
 * Created June 17, 2023
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 K. Suwatchai (Mobizt)
 *
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

#include <Arduino.h>
#include "ESP_Mail_FS.h"
#if defined(ESP32) && (defined(ENABLE_SMTP) || defined(ENABLE_IMAP))

#include "ESP32_TCP_Client.h"

ESP32_TCP_Client::ESP32_TCP_Client()
{
}

ESP32_TCP_Client::~ESP32_TCP_Client()
{
    if (wcs)
    {
        if (cert_buf)
            wcs->mbfs->delP(&cert_buf);

        wcs->stop();
        wcs.reset(nullptr);
        wcs.release();
    }
}

void ESP32_TCP_Client::setClient(Client *client)
{
    if (wcs)
        wcs->setClient(client);
}

void ESP32_TCP_Client::setCACert(const char *caCert)
{
    wcs->setCACert(caCert);
    if (caCert)
        wcs->baseSetCertType(esp_mail_cert_type_data);
    else
    {
        setInsecure();
        wcs->baseSetCertType(esp_mail_cert_type_none);
    }
}

bool ESP32_TCP_Client::setCertFile(const char *certFile, mb_fs_mem_storage_type storageType)
{
    if (!wcs->mbfs)
        return false;

    if (strlen(certFile) > 0)
    {
        MB_String filename = certFile;
        if (filename.length() > 0)
        {
            if (filename[0] != '/')
                filename.prepend('/');
        }

        int len = wcs->mbfs->open(filename, storageType, mb_fs_open_mode_read);
        if (len > -1)
        {

            if (storageType == mb_fs_mem_storage_type_flash)
            {

#if defined(MBFS_FLASH_FS)
                fs::File file = wcs->mbfs->getFlashFile();
                wcs->loadCACert(file, len);
#endif
                wcs->mbfs->close(storageType);
                wcs->baseSetCertType(esp_mail_cert_type_file);
            }
            else if (storageType == mb_fs_mem_storage_type_sd)
            {

#if defined(MBFS_ESP32_SDFAT_ENABLED)

                if (cert_buf)
                    wcs->mbfs->delP(&cert_buf);

                cert_buf = (char *)wcs->mbfs->newP(len);
                if (wcs->mbfs->available(storageType))
                    wcs->mbfs->read(storageType, (uint8_t *)cert_buf, len);

                wcs->mbfs->close(storageType);
                wcs->setCACert((const char *)cert_buf);
                wcs->baseSetCertType(esp_mail_cert_type_file);

#elif defined(MBFS_SD_FS)
                fs::File file = wcs->mbfs->getSDFile();
                wcs->loadCACert(file, len);
                wcs->mbfs->close(storageType);
                wcs->baseSetCertType(esp_mail_cert_type_file);

#endif
            }
        }
    }

    return getCertType() == esp_mail_cert_type_file;
}

void ESP32_TCP_Client::setDebugCallback(DebugMsgCallback cb)
{
    debugCallback = std::move(cb);
}

void ESP32_TCP_Client::setTimeout(uint32_t timeoutSec)
{
    if (wcs)
        wcs->setTimeout(timeoutSec);

    wcs->baseSetTimeout(timeoutSec);
}

void ESP32_TCP_Client::setInsecure()
{
    wcs->setInsecure();
}

bool ESP32_TCP_Client::ethLinkUp()
{
    if (strcmp(ETH.localIP().toString().c_str(), "0.0.0.0") != 0)
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
#if defined(ENABLE_CUSTOM_CLIENT)

    if (network_status_cb)
        network_status_cb();

    return networkStatus;
#else
    return WiFi.status() == WL_CONNECTED || ethLinkUp();
#endif
}

void ESP32_TCP_Client::networkReconnect()
{
#if defined(ENABLE_CUSTOM_CLIENT)
    if (network_connection_cb)
        network_connection_cb();
#else
    esp_wifi_connect();
#endif
}

void ESP32_TCP_Client::networkDisconnect()
{
#if defined(ENABLE_CUSTOM_CLIENT)
    if (network_disconnection_cb)
        network_disconnection_cb();
#else
    WiFi.disconnect();
#endif
}

String ESP32_TCP_Client::fwVersion()
{
    return String();
}

esp_mail_client_type ESP32_TCP_Client::type()
{
#if defined(ENABLE_CUSTOM_CLIENT)
    return esp_mail_client_type_custom;
#else
    return esp_mail_client_type_internal;
#endif
}

bool ESP32_TCP_Client::isInitialized()
{
#if defined(ENABLE_CUSTOM_CLIENT)

    bool rdy = wcs != nullptr;

    bool upgradeRequired = false;

#if !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)
    if (wcs->getProtocol(_port) == (int)esp_mail_protocol_tls && !connection_upgrade_cb)
        upgradeRequired = true;
#endif

    if (!network_connection_cb || !network_status_cb || upgradeRequired)
    {
        rdy = false;
#if !defined(SILENT_MODE)
        if (wcs->debugLevel > 0)
        {
            if (!network_connection_cb)
                esp_mail_debug_print_tag(esp_mail_error_client_str_6 /* "network connection callback is required" */, esp_mail_debug_tag_type_error, true);

            if (!network_status_cb)
                esp_mail_debug_print_tag(esp_mail_error_client_str_7 /* "network connection status callback is required" */, esp_mail_debug_tag_type_error, true);

            if (upgradeRequired)
                esp_mail_debug_print_tag(esp_mail_error_client_str_5 /* "client connection upgrade callback (for TLS handshake) is required" */, esp_mail_debug_tag_type_error, true);
        }
#endif
    }

    return rdy;
#else
    return true;
#endif
}

int ESP32_TCP_Client::hostByName(const char *name, IPAddress &ip)
{
#if !defined(ENABLE_CUSTOM_CLIENT)
    return WiFi.hostByName(name, ip);
#else
    return 1;
#endif
}

bool ESP32_TCP_Client::begin(const char *host, uint16_t port)
{
    _host = host;
    _port = port;
    wcs->begin(host, port);
    return true;
};

bool ESP32_TCP_Client::connect(bool secured, bool verify)
{

    wcs->setSecure(secured);
    wcs->setVerify(verify);

    if (connected())
    {
        flush();
        return true;
    }

    if (debugCallback)
        wcs->setDebugCB(&debugCallback);

#if defined(ENABLE_CUSTOM_CLIENT)

    // no client assigned?
    if (!wcs->_ssl->client)
    {
#if !defined(SILENT_MODE)
        if (wcs->debugLevel > 0)
            esp_mail_debug_print_tag(esp_mail_error_client_str_1 /* "client and/or necessary callback functions are not yet assigned" */, esp_mail_debug_tag_type_error, true);
#endif
        return false;
    }

    // no client type assigned?
    if (wcs->ext_client_type == esp_mail_external_client_type_none)
    {
#if !defined(SILENT_MODE)
        if (wcs->debugLevel > 0)
            esp_mail_debug_print_tag(esp_mail_error_client_str_4 /* "the client type must be provided, see example" */, esp_mail_debug_tag_type_error, true);
#endif
        return false;
    }

    // plain text via ssl client?
    if (!secured && wcs->ext_client_type == esp_mail_external_client_type_ssl)
    {
#if !defined(SILENT_MODE)
        if (wcs->debugLevel > 0)
            esp_mail_debug_print_tag(esp_mail_error_client_str_3 /* "simple Client is required" */, esp_mail_debug_tag_type_error, true);
#endif
        return false;
    }

#endif

// internal client or external client with internal ssl engine
#if !defined(ENABLE_CUSTOM_CLIENT) || defined(ESP_MAIL_USE_SDK_SSL_ENGINE)

    // internal or external client with innternal ssl client
    if (!wcs->connect(_host.c_str(), _port))
        return false;

#endif

#if !defined(ENABLE_CUSTOM_CLIENT)
    if (wcs->isKeepAliveSet())
    {
        if (wcs->tcpKeepIdleSeconds == 0 || wcs->tcpKeepIntervalSeconds == 0 || wcs->tcpKeepCount == 0)
        {
            wcs->tcpKeepIdleSeconds = 0;
            wcs->tcpKeepIntervalSeconds = 0;
            wcs->tcpKeepCount = 0;
        }

        bool success = wcs->setOption(TCP_KEEPIDLE, &wcs->tcpKeepIdleSeconds) > -1 &&
                       wcs->setOption(TCP_KEEPINTVL, &wcs->tcpKeepIntervalSeconds) > -1 &&
                       wcs->setOption(TCP_KEEPCNT, &wcs->tcpKeepCount) > -1;
        if (!success)
            wcs->isKeepAlive = false;
    }
#endif

    bool res = connected();

    if (!res)
        stop();

    return res;
}

bool ESP32_TCP_Client::connectSSL(bool verify)
{
    if (!wcs)
        return false;

    bool res = false;

#if defined(ENABLE_CUSTOM_CLIENT) && !defined(ESP_MAIL_USE_SDK_SSL_ENGINE)

    wcs->tls_required = true;

    if (connection_upgrade_cb)
        connection_upgrade_cb();

    res = this->connected();

    if (!res)
        stop();

    return res;
#endif

    res = wcs->connectSSL(verify);

    if (!res)
        wcs->stop();

    return res;
}

void ESP32_TCP_Client::stop()
{
    _host.clear();
    wcs->stop();
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

    if (!networkReady())
        return TCP_CLIENT_ERROR_NOT_CONNECTED;

    if (!connect(wcs->isSecure(), wcs->isVerify()))
        return TCP_CLIENT_ERROR_CONNECTION_REFUSED;

    int toSend = _chunkSize;
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
    char *buf = (char *)wcs->mbfs->newP(64);
    sprintf(buf, (const char *)FPSTR("%d"), data);
    int ret = send(buf);
    wcs->mbfs->delP(&buf);
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
    char *buf = (char *)wcs->mbfs->newP(64);
    sprintf(buf, (const char *)FPSTR("%d\r\n"), data);
    int ret = send(buf);
    wcs->mbfs->delP(&buf);
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

    return wcs->read(buf, len);
}

int ESP32_TCP_Client::readBytes(char *buf, int len)
{
    return readBytes((uint8_t *)buf, len);
}

void ESP32_TCP_Client::flush()
{
    if (!wcs)
        wcs->flush();
}

#endif // ESP32

#endif // ESP32_TCP_Client_CPP
