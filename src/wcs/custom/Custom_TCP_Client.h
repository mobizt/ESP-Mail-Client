/**
 * The custom TCP Client Class v2.0.3
 *
 * Created January 21, 2023
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 K. Suwatchai (Mobizt)
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
        _host.clear();
        if (wcs)
            wcs->stop();
    }

    bool begin(const char *host, uint16_t port)
    {
        _host = host;
        _port = port;
        return true;
    }

    /**
     * Set TCP connection time out in seconds.
     * @param timeoutSec The time out in seconds.
     */
    void setTimeout(uint32_t timeoutSec)
    {
        if (wcs)
            wcs->setTimeout(timeoutSec);

        baseSetTimeout(timeoutSec);
    }

    /**
     * Ethernet DNS workaround.
     */
    void ethDNSWorkAround()
    {
    }

    /**
     * Get the network status.
     * @return true for connected or false for not connected.
     */
    bool networkReady()
    {
        if (network_status_cb)
            network_status_cb();
        return networkStatus;
    }

    /**
     * Reconnect the network.
     */
    void networkReconnect()
    {
        if (network_connection_cb)
            network_connection_cb();
        else
        {
            if (debugLevel > 0)
                esp_mail_debug_print(esp_mail_str_369, true);
        }
    }

    /**
     * Disconnect the network.
     */
    void networkDisconnect()
    {
        if (network_disconnection_cb)
            network_disconnection_cb();
    }

    /**
     * Get firmware version string.
     * @return The firmware version string.
     */
    String fwVersion()
    {
        return String();
    }

    /**
     * Get the Client type.
     * @return The esp_mail_client_type enum value.
     */
    esp_mail_client_type type()
    {
        return esp_mail_client_type_custom;
    }

    /**
     * Get the Client initialization status.
     * @return The initialization status.
     */
    bool isInitialized()
    {
        bool rdy = wcs != nullptr;

        if (!network_connection_cb)
        {
            rdy = false;
            if (debugLevel > 0)
                esp_mail_debug_print(esp_mail_str_369, true);
        }

        if (getProtocol(_port) == (int)esp_mail_protocol_tls && !connection_upgrade_cb)
        {
            rdy = false;
            if (debugLevel > 0)
                esp_mail_debug_print(esp_mail_str_368, true);
        }

        return rdy;
    }

    /**
     * Set Root CA certificate to verify.
     * @param name The host name.
     * @param ip The ip address result.
     * @return 1 for success or 0 for failed.
     */
    int hostByName(const char *name, IPAddress &ip)
    {
        // return WiFi.hostByName(name, ip);
        return 1;
    }

    /**
     * Start TCP connection using stored host name and port.
     * @param secure The secure mode option.
     * @param verify The Root CA certificate verification option.
     * @return true for success or false for error.
     */
    bool connect(bool secured, bool verify)
    {
        if (connected())
        {
            flush();
            return true;
        }

        // no client assigned?
        if (!wcs)
        {
            if (debugLevel > 0)
            {
                MB_String s = esp_mail_str_185;
                s += esp_mail_str_346;
                esp_mail_debug_print(s.c_str(), true);
            }
            return false;
        }

        // no client type assigned?
        if (ext_client_type == esp_mail_external_client_type_none)
        {
            if (debugLevel > 0)
                esp_mail_debug_print(esp_mail_str_372, true);
            return false;
        }

        // nonsecure with ssl client?
        if (!secured && ext_client_type == esp_mail_external_client_type_ssl)
        {
            if (debugLevel > 0)
                esp_mail_debug_print(esp_mail_str_366, true);
            return false;
        }

        if (this->connection_cb)
            this->connection_cb(_host.c_str(), _port);
        else
            wcs->connect(_host.c_str(), _port);

        bool res = connected();

        if (!res)
            stop();

        if (res && secured && ext_client_type == esp_mail_external_client_type_basic)
        {
            res = connectSSL(verify);
        }

        return res;
    }

    /**
     * Upgrade the current connection by setting up the SSL and perform the SSL handshake.
     *
     * @param verify The Root CA certificate verification option
     * @return operating result.
     */
    bool connectSSL(bool verify)
    {
        if (!wcs)
            return false;

        tls_required = true;

        if (connection_upgrade_cb)
            connection_upgrade_cb();
        else
        {
            if (debugLevel > 0)
                esp_mail_debug_print(esp_mail_str_368, true);
            return false;
        }

        bool res = connected();

        if (!res)
            stop();

        return res;
    }

    /**
     * Stop TCP connection.
     */
    void stop()
    {
        _host.clear();
        wcs->stop();
    }

    /**
     * Get the TCP connection status.
     * @return true for connected or false for not connected.
     */
    bool connected()
    {
        if (wcs)
            return wcs->connected();
        return false;
    }

    /**
     * The TCP data write function.
     * @param data The data to write.
     * @param len The length of data to write.
     * @return The size of data that was successfully written or 0 for error.
     */
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

    /**
     * The TCP data send function.
     * @param data The data to send.
     * @return The size of data that was successfully send or 0 for error.
     */
    int send(const char *data)
    {
        return write((uint8_t *)data, strlen(data));
    }

    /**
     * The TCP data print function.
     * @param data The data to print.
     * @return The size of data that was successfully print or 0 for error.
     */
    int print(int data)
    {
        char *buf = (char *)mbfs->newP(64);
        sprintf(buf, (const char *)MBSTRING_FLASH_MCR("%d"), data);
        int ret = send(buf);
        mbfs->delP(&buf);
        return ret;
    }

    /**
     * The TCP data print function.
     * @param data The data to print.
     * @return The size of data that was successfully print or 0 for error.
     */
    int print(const char *data)
    {
        return send(data);
    }

    /**
     * The TCP data print with new line function.
     * @param data The data to print.
     * @return The size of data that was successfully print or 0 for error.
     */
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

    /**
     * The TCP data print with new line function.
     * @param data The data to print.
     * @return The size of data that was successfully print or 0 for error.
     */
    int println(int data)
    {
        char *buf = (char *)mbfs->newP(64);
        sprintf(buf, (const char *)MBSTRING_FLASH_MCR("%d\r\n"), data);
        int ret = send(buf);
        mbfs->delP(&buf);
        return ret;
    }

    /**
     * Get available data size to read.
     * @return The avaiable data size.
     */
    int available()
    {
        if (!wcs)
            return TCP_CLIENT_ERROR_NOT_INITIALIZED;

        return wcs->available();
    }

    /**
     * The TCP data read function.
     * @return The read value or -1 for error.
     */
    int read()
    {
        if (!wcs)
            return TCP_CLIENT_ERROR_NOT_INITIALIZED;

        return wcs->read();
    }

    /**
     * The TCP data read function.
     * @param buf The data buffer.
     * @param len The length of data that read.
     * @return The size of data that was successfully read or negative value for error.
     */
    int readBytes(uint8_t *buf, int len)
    {
        if (!wcs)
            return TCP_CLIENT_ERROR_NOT_INITIALIZED;

        return wcs->readBytes(buf, len);
    }

    /**
     * The TCP data read function.
     * @param buf The data buffer.
     * @param len The length of data that read.
     * @return The size of data that was successfully read or negative value for error.
     */
    int readBytes(char *buf, int len)
    {
        return readBytes((uint8_t *)buf, len);
    }

    /**
     * Wait for all receive buffer data read.
     */
    void flush()
    {
        while (available() > 0)
            read();
    }

    /**
     * Set the Client for TCP connection.
     * @param client The pointer to Client.
     */
    void setClient(Client *client)
    {
        wcs = client;
    }

    /**
     * Set the connection request callback.
     * @param connectCB The callback function that accepts the host name (const char*) and port (int) as parameters.
     */
    void connectionRequestCallback(ConnectionRequestCallback connectCB)
    {
        this->connection_cb = connectCB;
    }

    /**
     * Set the connection upgrade request callback.
     * @param upgradeCB The callback function to do the SSL setup and handshake.
     */
    void connectionUpgradeRequestCallback(ConnectionUpgradeRequestCallback upgradeCB)
    {
        this->connection_upgrade_cb = upgradeCB;
    }

    /**
     * Set the network connection request callback.
     * @param networkConnectionCB The callback function that handles the network connection.
     */
    void networkConnectionRequestCallback(NetworkConnectionRequestCallback networkConnectionCB)
    {
        this->network_connection_cb = networkConnectionCB;
    }

    /**
     * Set the network disconnection request callback.
     * @param networkConnectionCB The callback function that handles the network disconnection.
     */
    void networkDisconnectionRequestCallback(NetworkDisconnectionRequestCallback networkDisconnectionCB)
    {
        this->network_disconnection_cb = networkDisconnectionCB;
    }

    /**
     * Set the network status request callback.
     * @param networkStatusCB The callback function that calls the setNetworkStatus function to set the network status.
     */
    void networkStatusRequestCallback(NetworkStatusRequestCallback networkStatusCB)
    {
        this->network_status_cb = networkStatusCB;
    }

    /**
     * Set the network status which should call in side the networkStatusRequestCallback function.
     * @param status The status of network.
     */
    void setNetworkStatus(bool status)
    {
        networkStatus = status;
    }

    /**
     * Set the clock ready status.
     * @param rdy The ready status.
     */
    void setClockReady(bool rdy)
    {
        this->clockReady = rdy;
    }

    /**
     * Reset the nak error status.
     */
    void reset_tlsErr()
    {
        this->tls_required = false;
        this->tls_error = false;
    }

    /**
     * Set external Client type.
     * @param type The esp_mail_external_client_type enum type esp_mail_external_client_type_basic and esp_mail_external_client_type_ssl.
     */
    void setExtClientType(esp_mail_external_client_type type)
    {
        this->ext_client_type = type;
    }

    /**
     * Set the tls error status.
     * @param tls The tls error status.
     */
    void set_tlsErrr(bool tls)
    {
        this->tls_error = tls;
    }

    /**
     * Get the tls status.
     * @return bool status of tls error.
     */
    bool tlsErr()
    {
        return this->tls_error;
    }

    /**
     * Set the TLS required status.
     * @param req The tls required status.
     */
    void set_tlsRequired(bool req)
    {
        this->tls_required = req;
    }

    /**
     * Get the TLS upgrade required status.
     * @return bool status of TLS handshake required status.
     */
    bool tlsRequired()
    {
        return this->tls_required;
    }

private:
    MB_String _host;
    uint16_t _port;
    Client *wcs = nullptr;
    ConnectionRequestCallback connection_cb = NULL;
    ConnectionUpgradeRequestCallback connection_upgrade_cb = NULL;
    NetworkConnectionRequestCallback network_connection_cb = NULL;
    NetworkDisconnectionRequestCallback network_disconnection_cb = NULL;
    NetworkStatusRequestCallback network_status_cb = NULL;
    volatile bool networkStatus = false;
};

#endif