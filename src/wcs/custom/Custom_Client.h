/**
 * The custom TCP Client Class v1.0.0.
 * 
 * February 1, 2022
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

#ifndef CUSTOM_TCP_CLIENT_H
#define CUSTOM_TCP_CLIENT_H

//This file was included in wcs/clients.h

#include <Arduino.h>
#include "./wcs/base/TCP_Client_Base.h"

class Custom_Client : public TCP_Client_Base
{

public:
    Custom_Client(){};
    ~Custom_Client()
    {
        if (wcs)
            wcs->stop();
    };

    void setTimeout(uint32_t timeoutSec)
    {
        if (wcs)
            wcs->setTimeout(timeoutSec);

        baseSetTimeout(timeoutSec);
    }

    void ethDNSWorkAround()
    {
    }

    bool networkReady()
    {
        return true;
    }

    void networkReconnect()
    {
    }

    void networkDisconnect()
    {
    }

    unsigned long getTime()
    {
#if defined(MB_MCU_ESP) || defined(MB_MCU_ATMEL_ARM) || defined(MB_MCU_RP2040)
        now = time(nullptr);
#endif
        return (unsigned long)now;
    }

    String fwVersion()
    {
        return String();
    }

    esp_mail_client_type type()
    {
        return esp_mail_client_type_custom;
    }

    bool isInitialized()
    {
        bool rdy = false;

        rdy = wcs != nullptr && connection_cb != NULL;

        if ((port == esp_mail_smtp_port_587) && connection_upgrade_cb == NULL)
            rdy = false;

        return rdy;
    }

    int hostByName(const char *name, IPAddress &ip)
    {
        //return WiFi.hostByName(name, ip);
        return 1;
    }

    bool connect(bool secured, bool verify)
    {
        if (connected())
        {
            while (wcs->available() > 0)
                wcs->read();
            return true;
        }

        if (this->connection_cb)
            this->connection_cb(host.c_str(), port);

        return connected();
    }

    bool connectSSL(bool verify)
    {
        if (!wcs)
            return false;

        if (connection_upgrade_cb)
            connection_upgrade_cb();

        return connected();
    }

    void stop()
    {
        if (connected())
            return wcs->stop();
    }

    bool connected()
    {
        if (wcs)
            return wcs->connected();
        return false;
    }

    int write(uint8_t *data, int len)
    {
        if (!data)
            return TCP_CLIENT_ERROR_SEND_DATA_FAILED;

        if (len == 0)
            return TCP_CLIENT_ERROR_SEND_DATA_FAILED;

        if (!connect(0, 0))
            return TCP_CLIENT_ERROR_CONNECTION_REFUSED;

        int toSend = chunkSize;
        int sent = 0;
        while (sent < len)
        {
            if (sent + toSend > len)
                toSend = len - sent;

            if ((int)wcs->write(data + sent, toSend) != toSend)
                return TCP_CLIENT_ERROR_SEND_DATA_FAILED;

            sent += toSend;
        }

        return len;
    }

    int send(const char *data)
    {
        return write((uint8_t *)data, strlen(data));
    }

    int print(int data)
    {
        char *buf = (char *)mbfs->newP(64);
        sprintf(buf, (const char *)MBSTRING_FLASH_MCR("%d"), data);
        int ret = send(buf);
        mbfs->delP(&buf);
        return ret;
    }

    int print(const char *data)
    {
        return send(data);
    }

    int println(const char *data)
    {
        int len = send(data);
        if (len < 0)
            return len;
        int sz = send((const char *)MBSTRING_FLASH_MCR("\r\n"));
        if (sz < 0)
            return sz;
        return len + sz;
    }

    int println(int data)
    {
        char *buf = (char *)mbfs->newP(64);
        sprintf(buf, (const char *)MBSTRING_FLASH_MCR("%d\r\n"), data);
        int ret = send(buf);
        mbfs->delP(&buf);
        return ret;
    }

    int available()
    {
        if (!wcs)
            return TCP_CLIENT_ERROR_NOT_INITIALIZED;

        return wcs->available();
    }

    int read()
    {
        if (!wcs)
            return TCP_CLIENT_ERROR_NOT_INITIALIZED;

        return wcs->read();
    }

    int readBytes(uint8_t *buf, int len)
    {
        if (!wcs)
            return TCP_CLIENT_ERROR_NOT_INITIALIZED;

        return wcs->readBytes(buf, len);
    }

    int readBytes(char *buf, int len)
    {
        return readBytes((uint8_t *)buf, len);
    }

    void setClient(Client *client)
    {
        wcs = client;
    }

    void connectionRequestCallback(ConnectionRequestCallback connectCB)
    {
        this->connection_cb = connectCB;
    }

    void connectionUpgradeRequestCallback(ConnectionUpgradeRequestCallback upgradeCB)
    {
        this->connection_upgrade_cb = upgradeCB;
    }

private:
    Client *wcs = nullptr;
    ConnectionRequestCallback connection_cb = NULL;
    ConnectionUpgradeRequestCallback connection_upgrade_cb = NULL;
};

#endif