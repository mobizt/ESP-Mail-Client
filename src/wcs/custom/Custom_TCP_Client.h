/**
 * The custom TCP Client Class v1.0.3.
 *
 * May 22, 2022
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
 *
 */

#ifndef CUSTOM_TCP_CLIENT_H
#define CUSTOM_TCP_CLIENT_H

// This file was included in wcs/clients.h

#include <Arduino.h>
#include "./wcs/base/TCP_Client_Base.h"

class Custom_TCP_Client : public TCP_Client_Base
{

public:
    Custom_TCP_Client(){};
    ~Custom_TCP_Client()
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
        if (network_status_cb)
            network_status_cb();
        return networkStatus;
    }

    void networkReconnect()
    {
        if (network_connection_cb)
            network_connection_cb();
    }

    void networkDisconnect()
    {
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
        // return WiFi.hostByName(name, ip);
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

        if (!networkReady())
            return TCP_CLIENT_ERROR_NOT_CONNECTED;

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

    void networkConnectionRequestCallback(NetworkConnectionRequestCallback networkConnectionCB)
    {
        this->network_connection_cb = networkConnectionCB;
    }

    void networkStatusRequestCallback(NetworkStatusRequestCallback networkStatusCB)
    {
        this->network_status_cb = networkStatusCB;
    }

    void setNetworkStatus(bool status)
    {
        networkStatus = status;
    }

private:
    Client *wcs = nullptr;
    ConnectionRequestCallback connection_cb = NULL;
    ConnectionUpgradeRequestCallback connection_upgrade_cb = NULL;
    NetworkConnectionRequestCallback network_connection_cb = NULL;
    NetworkStatusRequestCallback network_status_cb = NULL;
    volatile bool networkStatus = false;
};

#endif