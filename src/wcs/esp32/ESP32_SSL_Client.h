/*
 * ESP32 SSL Client v2.1.0
 *
 * Created July 27, 2023
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
 */

/* Provide SSL/TLS functions to ESP32 with Arduino IDE
 * by Evandro Copercini - 2017 - Apache 2.0 License
 */

#ifndef ESP32_SSL_Client_H
#define ESP32_SSL_Client_H

#include <Arduino.h>
#include "ESP_Mail_FS.h"
#if defined(ESP32) && (defined(ENABLE_SMTP) || defined(ENABLE_IMAP))

#include "./wcs/base/TCP_Client_Base.h"

#if !defined(ESP_MAIL_ESP32_USE_WIFICLIENT_TEST) && !defined(ESP_MAIL_ESP32_USE_WIFICLIENT_SOCKET_TEST)
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#endif

#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

#if !defined(SILENT_MODE)
static const char esp_ssl_client_str_1[] PROGMEM = "Skipping SSL Verification. INSECURE!";
static const char esp_ssl_client_str_2[] PROGMEM = "starting socket";
static const char esp_ssl_client_str_3[] PROGMEM = "opening socket";
static const char esp_ssl_client_str_4[] PROGMEM = "could not get ip from host";
static const char esp_ssl_client_str_5[] PROGMEM = "connecting to Server";
static const char esp_ssl_client_str_6[] PROGMEM = "server connected";
static const char esp_ssl_client_str_7[] PROGMEM = "connect to Server failed!";
static const char esp_ssl_client_str_8[] PROGMEM = "root certificate, PSK identity or keys are required for secured connection";
static const char esp_ssl_client_str_9[] PROGMEM = "seeding the random number generator";
static const char esp_ssl_client_str_10[] PROGMEM = "setting up the SSL/TLS structure";
static const char esp_ssl_client_str_11[] PROGMEM = "loading CA cert";
static const char esp_ssl_client_str_12[] PROGMEM = "setting up PSK";
static const char esp_ssl_client_str_13[] PROGMEM = "pre-shared key not valid hex or too long";
static const char esp_ssl_client_str_14[] PROGMEM = "set mbedtls config";
static const char esp_ssl_client_str_15[] PROGMEM = "loading CRT cert";
static const char esp_ssl_client_str_16[] PROGMEM = "loading private key";
static const char esp_ssl_client_str_17[] PROGMEM = "setting hostname for TLS session";
static const char esp_ssl_client_str_18[] PROGMEM = "perform the SSL/TLS handshake";
static const char esp_ssl_client_str_19[] PROGMEM = "verifying peer X.509 certificate";
static const char esp_ssl_client_str_20[] PROGMEM = "failed to verify peer certificate!";
static const char esp_ssl_client_str_21[] PROGMEM = "certificate verified";
static const char esp_ssl_client_str_22[] PROGMEM = "cleaning SSL connection";
static const char esp_ssl_client_str_23[] PROGMEM = "fingerprint too short";
static const char esp_ssl_client_str_24[] PROGMEM = "invalid hex sequence";
static const char esp_ssl_client_str_25[] PROGMEM = "could not fetch peer certificate";
static const char esp_ssl_client_str_26[] PROGMEM = "fingerprint doesn't match";
static const char esp_ssl_client_str_27[] PROGMEM = "SSL/TLS negotiation";
static const char esp_ssl_client_str_28[] PROGMEM = "Free internal heap before TLS %u";
static const char esp_ssl_client_str_29[] PROGMEM = "Starting socket";
static const char esp_ssl_client_str_30[] PROGMEM = "ERROR opening socket";
static const char esp_ssl_client_str_31[] PROGMEM = "connect on fd %d, errno: %d, \"%s\"";
static const char esp_ssl_client_str_32[] PROGMEM = "select on fd %d, errno: %d, \"%s\"";
static const char esp_ssl_client_str_33[] PROGMEM = "select returned due to timeout %d ms for fd %d";
static const char esp_ssl_client_str_34[] PROGMEM = "getsockopt on fd %d, errno: %d, \"%s\"";
static const char esp_ssl_client_str_35[] PROGMEM = "socket error on fd %d, errno: %d, \"%s\"";
#endif

typedef void (*_ConnectionRequestCallback)(const char *, int);
typedef void (*DebugMsgCallback)(PGM_P msg, esp_mail_debug_tag_type type, bool newLine);

// The SSL context
typedef struct ssl_context_t
{
    int socket;

    // using the basic Client
    Client *client = nullptr;
#if defined(ESP_MAIL_ESP32_USE_WIFICLIENT_SOCKET_TEST)
    WiFiClient *wc = nullptr;
#endif

    mbedtls_ssl_context ssl_ctx;
    mbedtls_ssl_config ssl_conf;

    mbedtls_ctr_drbg_context drbg_ctx;
    mbedtls_entropy_context entropy_ctx;

    mbedtls_x509_crt ca_cert;
    mbedtls_x509_crt client_cert;
    mbedtls_pk_context client_key;

    DebugMsgCallback *_debugCallback = NULL;

    // milliseconds of socket time out and SSL handshake time out
    unsigned long socket_timeout;
    unsigned long handshake_timeout;
} ssl_ctx;

/**
 * Send the mbedTLS error info to the callback.
 *
 * @param ssl The pointer to ssl data (context) which its ssl->_debugCallback will be used.
 * @param errNo The mbedTLS error number that will be translated to string via mbedtls_strerror.
 * @param type The debug tag type.
 */
static void ssl_client_send_mbedtls_error_cb(ssl_ctx *ssl, int errNo, esp_mail_debug_tag_type type)
{
#if !defined(SILENT_MODE)
    char *error_buf = new char[100];
    mbedtls_strerror(errNo, error_buf, 100);
    DebugMsgCallback cb = *ssl->_debugCallback;
    cb(error_buf, type, true);
    delete[] error_buf;
#endif
}
/**
 * Send the predefined flash string error to the callback.
 *
 * @param ssl The pointer to ssl data (context) which its ssl->_debugCallback will be used.
 * @param info The PROGMEM error string.
 * @param type The debug tag type.
 */
static void ssl_client_debug_pgm_send_cb(ssl_ctx *ssl, PGM_P info, esp_mail_debug_tag_type type)
{
#if !defined(SILENT_MODE)
    DebugMsgCallback cb = *ssl->_debugCallback;
    cb(info, type, true);
#endif
}

class ESP32_SSL_Client
{
    friend class ESP32_WCS;

public:
    ESP32_SSL_Client(){};

    void ssl_init(ssl_ctx *ssl);

    /**
     * Upgrade the current connection by setting up the SSL and perform the SSL handshake.
     *
     * @param ssl The pointer to ssl data (context).
     * @param host The server host name.
     * @param rootCABuff The server's root CA or CA cert.
     * @param cli_cert The client cert.
     * @param cli_key The private key.
     * @param pskIdent The Pre Shared Key identity.
     * @param psKey The Pre Shared Key.
     * @param insecure The authentication by-pass option.
     * @return The socket for success or -1 for error.
     * @note The socket should be already open prior to calling this function or shared ssl context with start_tcp_connection.
     */
    int connect_ssl(ssl_ctx *ssl, const char *host, const char *rootCABuff, const char *cli_cert, const char *cli_key, const char *pskIdent, const char *psKey, bool insecure);

    /**
     * Stop the TCP connection and release resources.
     *
     * @param ssl The pointer to ssl data (context).
     * @param rootCABuff The server's root CA or CA cert.
     * @param cli_cert The client cert.
     * @param cli_key The private key.
     * @return The socket for success or -1 for error.
     */
    void stop_tcp_connection(ssl_ctx *ssl, const char *rootCABuff, const char *cli_cert, const char *cli_key);

    /**
     * Get the available data size to read.
     *
     * @param ssl The pointer to ssl data (context).
     * @return The avaiable data size or negative for error.
     */
    int data_to_read(ssl_ctx *ssl);

    /**
     * Send ssl encrypted data.
     *
     * @param ssl The pointer to ssl data (context).
     * @param data The unencrypted data to send.
     * @param len The length of data to send.
     * @return size of data that was successfully send or negative for error.
     */
    int send_ssl_data(ssl_ctx *ssl, const uint8_t *data, size_t len);

    /**
     * Receive ssl decrypted data.
     *
     * @param ssl The pointer to ssl data (context).
     * @param data The data buffer to store decrypted data.
     * @param length The length of decrypted data read.
     * @return size of decrypted data that was successfully read or negative for error.
     */
    int get_ssl_receive(ssl_ctx *ssl, uint8_t *data, int length);

    /**
     * Verify certificate's SHA256 fingerprint.
     *
     * @param ssl The pointer to ssl data (context).
     * @param fp The certificate's SHA256 fingerprint data to compare with server certificate's SHA256 fingerprint.
     * @param domain_name The optional domain name to check in server certificate.
     * @return verification result.
     */
    bool verify_ssl_fingerprint(ssl_ctx *ssl, const char *fp, const char *domain_name);

    /**
     * Verify ssl domain name.
     *
     * @param ssl The pointer to ssl data (context).
     * @param domain_name The domain name.
     * @return verification result.
     */
    bool verify_ssl_dn(ssl_ctx *ssl, const char *domain_name);

    /**
     * Convert Hex char to decimal number
     *
     * @param pb The Hex char.
     * @param res The pointer to result data byte.
     * @return The parsing result.
     */
    bool parseHexNibble(char pb, uint8_t *res);

    /**
     * Compare a name from certificate and domain name
     *
     * @param name The name.
     * @param domainName The domain name.
     * @return The compare result. Return true if they match
     */
    bool matchName(const std::string &name, const std::string &domainName);

#if defined(ENABLE_CUSTOM_CLIENT)
    /**
     * Set the connection request callback.
     * @param connectCB The callback function that accepts the host name (const char*) and port (int) as parameters.
     */
    void connectionRequestCallback(_ConnectionRequestCallback connectCB)
    {
        this->connection_cb = connectCB;
    }

#endif

protected:
#if defined(ENABLE_CUSTOM_CLIENT)
    _ConnectionRequestCallback connection_cb = NULL;
#endif
};

#if !defined(ESP_MAIL_ESP32_USE_WIFICLIENT_TEST) && !defined(ESP_MAIL_ESP32_USE_WIFICLIENT_SOCKET_TEST)
// lwIP based TCP Client v1.0.0
// This class is the WiFiClient replacement that used in ESP32_SSL_Client with more reliable and faster.
// Using WiFiClient socket via fd() also slower than use lwIP directly.
class tcpClient
{
private:
    ssl_ctx *_ssl = nullptr;
    MB_String _rxBuf;

public:
    tcpClient() { _rxBuf.reserve(2048); };
    ~tcpClient() { tcpClose(); };
    void setCtx(ssl_ctx *ssl) { this->_ssl = ssl; };
    int tcpConnect(const IPAddress &ip, uint32_t port, int timeout)
    {
        if (!_ssl)
            return -1;

        int enable = 1;
        int bufLen = 100;
        char buf[bufLen];

#if !defined(SILENT_MODE)
        if (_ssl->_debugCallback)
        {
            snprintf(buf, bufLen, pgm2Str(esp_ssl_client_str_28 /* "Free internal heap before TLS %u" */), ESP.getFreeHeap());
            ssl_client_debug_pgm_send_cb(_ssl, buf, esp_mail_debug_tag_type_client);
        }
        log_v("Free internal heap before TLS %u", ESP.getFreeHeap());
#endif

#if !defined(SILENT_MODE)
        if (_ssl->_debugCallback)
            ssl_client_debug_pgm_send_cb(_ssl, esp_ssl_client_str_29 /* "Starting socket" */, esp_mail_debug_tag_type_client);
        log_v("Starting socket");
#endif

        _ssl->socket = -1;

        _ssl->socket = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_ssl->socket < 0)
        {
#if !defined(SILENT_MODE)
            if (_ssl->_debugCallback)
                ssl_client_debug_pgm_send_cb(_ssl, esp_ssl_client_str_30 /* "ERROR opening socket" */, esp_mail_debug_tag_type_error);
            log_e("ERROR opening socket");
#endif
            return _ssl->socket;
        }

        fcntl(_ssl->socket, F_SETFL, fcntl(_ssl->socket, F_GETFL, 0) | O_NONBLOCK);
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = ip;
        serv_addr.sin_port = htons(port);

        if (timeout <= 0)
            timeout = 30000; // Milli seconds.

        _ssl->socket_timeout = timeout;

        fd_set fdset;
        struct timeval tv;
        FD_ZERO(&fdset);
        FD_SET(_ssl->socket, &fdset);
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;

        int res = lwip_connect(_ssl->socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (res < 0 && errno != EINPROGRESS)
        {
#if !defined(SILENT_MODE)
            if (_ssl->_debugCallback)
            {
                snprintf(buf, bufLen, pgm2Str(esp_ssl_client_str_31 /* "connect on fd %d, errno: %d, \"%s\"" */), _ssl->socket, errno, strerror(errno));
                ssl_client_debug_pgm_send_cb(_ssl, buf, esp_mail_debug_tag_type_error);
            }
            log_e("connect on fd %d, errno: %d, \"%s\"", _ssl->socket, errno, strerror(errno));
#endif
            lwip_close(_ssl->socket);
            _ssl->socket = -1;
            return -1;
        }

        res = select(_ssl->socket + 1, nullptr, &fdset, nullptr, timeout < 0 ? nullptr : &tv);
        if (res < 0)
        {
#if !defined(SILENT_MODE)
            if (_ssl->_debugCallback)
            {
                snprintf(buf, bufLen, pgm2Str(esp_ssl_client_str_32 /* "select on fd %d, errno: %d, \"%s\"" */), _ssl->socket, errno, strerror(errno));
                ssl_client_debug_pgm_send_cb(_ssl, buf, esp_mail_debug_tag_type_error);
            }
            log_e("select on fd %d, errno: %d, \"%s\"", _ssl->socket, errno, strerror(errno));
#endif
            lwip_close(_ssl->socket);
            _ssl->socket = -1;
            return -1;
        }
        else if (res == 0)
        {
#if !defined(SILENT_MODE)
            if (_ssl->_debugCallback)
            {
                snprintf(buf, bufLen, pgm2Str(esp_ssl_client_str_33 /* "select returned due to timeout %d ms for fd %d" */), timeout, _ssl->socket);
                ssl_client_debug_pgm_send_cb(_ssl, buf, esp_mail_debug_tag_type_error);
            }
            log_i("select returned due to timeout %d ms for fd %d", timeout, _ssl->socket);
#endif
            lwip_close(_ssl->socket);
            _ssl->socket = -1;
            return -1;
        }
        else
        {
            int sockerr;
            socklen_t len = (socklen_t)sizeof(int);
            res = getsockopt(_ssl->socket, SOL_SOCKET, SO_ERROR, &sockerr, &len);

            if (res < 0)
            {
#if !defined(SILENT_MODE)
                if (_ssl->_debugCallback)
                {
                    snprintf(buf, bufLen, pgm2Str(esp_ssl_client_str_34 /* "getsockopt on fd %d, errno: %d, \"%s\"" */), _ssl->socket, errno, strerror(errno));
                    ssl_client_debug_pgm_send_cb(_ssl, buf, esp_mail_debug_tag_type_error);
                }
                log_e("getsockopt on fd %d, errno: %d, \"%s\"", _ssl->socket, errno, strerror(errno));
#endif
                lwip_close(_ssl->socket);
                _ssl->socket = -1;
                return -1;
            }

            if (sockerr != 0)
            {
#if !defined(SILENT_MODE)
                if (_ssl->_debugCallback)
                {
                    snprintf(buf, bufLen, pgm2Str(esp_ssl_client_str_35 /* "socket error on fd %d, errno: %d, \"%s\"" */), _ssl->socket, sockerr, strerror(sockerr));
                    ssl_client_debug_pgm_send_cb(_ssl, buf, esp_mail_debug_tag_type_error);
                }
                log_e("socket error on fd %d, errno: %d, \"%s\"", _ssl->socket, sockerr, strerror(sockerr));
#endif
                lwip_close(_ssl->socket);
                _ssl->socket = -1;
                return -1;
            }
        }

#if !defined(SILENT_MODE)
#define ROE(x, msg)                                         \
    {                                                       \
        if (((x) < 0))                                      \
        {                                                   \
            log_e("LWIP Socket config of " msg " failed."); \
            return -1;                                      \
        }                                                   \
    }
        ROE(lwip_setsockopt(_ssl->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)), "SO_RCVTIMEO");
        ROE(lwip_setsockopt(_ssl->socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)), "SO_SNDTIMEO");

        ROE(lwip_setsockopt(_ssl->socket, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)), "TCP_NODELAY");
        ROE(lwip_setsockopt(_ssl->socket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)), "SO_KEEPALIVE");
#else
        lwip_setsockopt(_ssl->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        lwip_setsockopt(_ssl->socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        lwip_setsockopt(_ssl->socket, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
        lwip_setsockopt(_ssl->socket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
#endif

        return 1;
    }

    void tcpClose()
    {
        if (!_ssl)
            return;
        lwip_close(_ssl->socket);
        _ssl->socket = -1;
    }

    int readBuff(uint8_t *buf, int bufLen)
    {
        if (!_ssl)
            return 0;

        fd_set readset;
        fd_set writeset;
        fd_set errset;
        struct timeval tv;
        FD_ZERO(&readset);
        FD_SET(_ssl->socket, &readset);
        FD_ZERO(&writeset);
        FD_SET(_ssl->socket, &writeset);
        FD_ZERO(&errset);
        FD_SET(_ssl->socket, &errset);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int ret = lwip_select(_ssl->socket, &readset, &writeset, &errset, &tv);
        if (ret < 0)
            return ret;
        // return ssize_t or signed size_t for error
        return read(_ssl->socket, buf, bufLen);
    }

    int tcpAavailable()
    {
        if (tcpConnected())
        {
            if (_rxBuf.length() == 0)
            {
                int bufLen = 2048;
                uint8_t buf[bufLen];
                int ret = readBuff(buf, bufLen);
                if (ret > 0)
                    _rxBuf.append((const char *)buf, ret);
            }
            return _rxBuf.length();
        }
        return 0;
    }

    size_t tcpWrite(const uint8_t *buf, size_t size)
    {
        if (!tcpConnected() || !size)
            return 0;
        // return ssize_t or signed size_t for error
        return lwip_write(_ssl->socket, buf, size);
    }

    size_t tcpRead(uint8_t *buf, size_t size)
    {
        if (_rxBuf.length() == 0)
            return readBuff(buf, size);
        else
        {
            size_t sz = size;
            if (sz > _rxBuf.length())
                sz = _rxBuf.length();
            memcpy(buf, _rxBuf.c_str(), sz);
            _rxBuf.erase(0, sz);
            return sz;
        }
    }

    int tcpConnected()
    {
        if (!_ssl)
            return 0;
        return _ssl->socket >= 0;
    }
};

// Basic TCP Client v1.0.0
class basicClient : public Client
{
public:
    basicClient(ssl_ctx *ssl) { client.setCtx(ssl); };
    ~basicClient() { client.tcpClose(); };
    operator bool() { return connected(); }
    bool operator==(const bool value) { return bool() == value; }
    void setTimeout(int timeout) { _timeout = timeout; }
    int connect(IPAddress address, uint16_t port) { return client.tcpConnect(address, port, _timeout); }
    int connect(const char *host, uint16_t port)
    {
        IPAddress address((uint32_t)0);
        if (!WiFiGenericClass::hostByName(host, address))
            return -1;

        return client.tcpConnect(address, port, _timeout);
    }
    uint8_t connected() { return client.tcpConnected(); }
    int peek() { return 0; } // not implemented
    int available() { return client.tcpAavailable(); }
    void stop() { client.tcpClose(); }
    size_t write(uint8_t v)
    {
        uint8_t buf[1];
        buf[0] = v;
        return client.tcpWrite(buf, 1);
    }
    void flush()
    {
        while (available() > 0)
            read();
    };
    int read()
    {
        uint8_t buf[1];
        buf[0] = 0;
        client.tcpRead(buf, 1);
        return buf[0];
    }
    size_t write(const uint8_t *buf, size_t size) { return client.tcpWrite(buf, size); }
    int read(uint8_t *buf, size_t size) { return client.tcpRead(buf, size); }

private:
    tcpClient client;
    int _timeout = 30000;
};

#endif

#endif // ESP32

#endif // ESP32_SSL_Client_H
