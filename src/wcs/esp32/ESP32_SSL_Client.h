/*
 * ESP32 SSL Client v1.0.4
 *
 * June 13, 2022
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
 */

/* Provide SSL/TLS functions to ESP32 with Arduino IDE
 * by Evandro Copercini - 2017 - Apache 2.0 License
 */

#ifndef ESP32_SSL_Client_H
#define ESP32_SSL_Client_H


#ifdef ESP32
#include <Arduino.h>
#include "./extras/MB_FS.h"

#include "mbedtls/platform.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

static const char esp_ssl_client_str_1[] PROGMEM = "! E: ";
static const char esp_ssl_client_str_2[] PROGMEM = "> C: starting socket";
static const char esp_ssl_client_str_3[] PROGMEM = "! E: opening socket";
static const char esp_ssl_client_str_4[] PROGMEM = "! E: could not get ip from host";
static const char esp_ssl_client_str_5[] PROGMEM = "> C: connecting to Server";
static const char esp_ssl_client_str_6[] PROGMEM = "> C: server connected";
static const char esp_ssl_client_str_7[] PROGMEM = "! E: connect to Server failed!";
static const char esp_ssl_client_str_8[] PROGMEM = "< S: ";
static const char esp_ssl_client_str_9[] PROGMEM = "> C: seeding the random number generator";
static const char esp_ssl_client_str_10[] PROGMEM = "> C: setting up the SSL/TLS structure";
static const char esp_ssl_client_str_11[] PROGMEM = "> C: loading CA cert";
static const char esp_ssl_client_str_12[] PROGMEM = "> C: setting up PSK";
static const char esp_ssl_client_str_13[] PROGMEM = "! E: pre-shared key not valid hex or too long";
static const char esp_ssl_client_str_14[] PROGMEM = "> C: set mbedtls config";
static const char esp_ssl_client_str_15[] PROGMEM = "> C: loading CRT cert";
static const char esp_ssl_client_str_16[] PROGMEM = "> C: loading private key";
static const char esp_ssl_client_str_17[] PROGMEM = "> C: setting hostname for TLS session";
static const char esp_ssl_client_str_18[] PROGMEM = "> C: performing the SSL/TLS handshake";
static const char esp_ssl_client_str_19[] PROGMEM = "> C: verifying peer X.509 certificate";
static const char esp_ssl_client_str_20[] PROGMEM = "! E: failed to verify peer certificate!";
static const char esp_ssl_client_str_21[] PROGMEM = "> C: certificate verified";
static const char esp_ssl_client_str_22[] PROGMEM = "> C: cleaning SSL connection";
static const char esp_ssl_client_str_23[] PROGMEM = "! E: fingerprint too short";
static const char esp_ssl_client_str_24[] PROGMEM = "! E: invalid hex sequence";
static const char esp_ssl_client_str_25[] PROGMEM = "! E: could not fetch peer certificate";
static const char esp_ssl_client_str_26[] PROGMEM = "! E: fingerprint doesn't match";
static const char esp_ssl_client_str_27[] PROGMEM = "! E: root certificate, PSK identity or keys are required for secured connection";
static const char esp_ssl_client_str_28[] PROGMEM = "! W: Skipping SSL Verification. INSECURE!";

class ESP32_SSL_Client
{
public:
    ESP32_SSL_Client(){};

    typedef void (*DebugMsgCallback)(const char *msg);
    
    // The SSL context
    typedef struct ssl_data_t
    {
        int socket;
        mbedtls_ssl_context ssl_ctx;
        mbedtls_ssl_config ssl_conf;

        mbedtls_ctr_drbg_context drbg_ctx;
        mbedtls_entropy_context entropy_ctx;

        mbedtls_x509_crt ca_cert;
        mbedtls_x509_crt client_cert;
        mbedtls_pk_context client_key;

        DebugMsgCallback *_debugCallback = NULL;

        // milliseconds SSL handshake time out
        unsigned long handshake_timeout;
    } ssl_data;

    void ssl_init(ssl_data *ssl);

    /**
     * Start the TCP connection.
     *
     * @param ssl The pointer to ssl data (context).
     * @param host The server host name to connect.
     * @param port The server port to connect.
     * @param timeout The connection time out in miiliseconds.
     * @return The socket for success or -1 for error.
     */
    int start_tcp_connection(ssl_data *ssl, const char *host, uint32_t port, int timeout);

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
    int connect_ssl(ssl_data *ssl, const char *host, const char *rootCABuff, const char *cli_cert, const char *cli_key, const char *pskIdent, const char *psKey, bool insecure);

    /**
     * Stop the TCP connection and release resources.
     *
     * @param ssl The pointer to ssl data (context).
     * @param rootCABuff The server's root CA or CA cert.
     * @param cli_cert The client cert.
     * @param cli_key The private key.
     * @return The socket for success or -1 for error.
     */
    void stop_tcp_connection(ssl_data *ssl, const char *rootCABuff, const char *cli_cert, const char *cli_key);

    /**
     * Get the available data size to read.
     *
     * @param ssl The pointer to ssl data (context).
     * @return The avaiable data size or negative for error.
     */
    int data_to_read(ssl_data *ssl);

    /**
     * Send ssl encrypted data.
     *
     * @param ssl The pointer to ssl data (context).
     * @param data The unencrypted data to send.
     * @param len The length of data to send.
     * @return size of data that was successfully send or negative for error.
     */
    int send_ssl_data(ssl_data *ssl, const uint8_t *data, size_t len);

    /**
     * Receive ssl decrypted data.
     *
     * @param ssl The pointer to ssl data (context).
     * @param data The data buffer to store decrypted data.
     * @param length The length of decrypted data read.
     * @return size of decrypted data that was successfully read or negative for error.
     */
    int get_ssl_receive(ssl_data *ssl, uint8_t *data, int length);

    /**
     * Verify certificate's SHA256 fingerprint.
     *
     * @param ssl The pointer to ssl data (context).
     * @param fp The certificate's SHA256 fingerprint data to compare with server certificate's SHA256 fingerprint.
     * @param domain_name The optional domain name to check in server certificate.
     * @return verification result.
     */
    bool verify_ssl_fingerprint(ssl_data *ssl, const char *fp, const char *domain_name);

    /**
     * Verify ssl domain name.
     *
     * @param ssl The pointer to ssl data (context).
     * @param domain_name The domain name.
     * @return verification result.
     */
    bool verify_ssl_dn(ssl_data *ssl, const char *domain_name);

    /**
     * The non-secure mode lwIP write function.
     *
     * @param ssl The pointer to ssl data (context) which only its ssl->socket will be used.
     * @param buf The data to write.
     * @param bufLen The length of data to write.
     * @return size of data that was successfully written or negative value of error enum.
     */
    int ns_lwip_write(ssl_data *ssl, const uint8_t *buf, int bufLen);

    /**
     * The non-secure mode lwIP read function.
     *
     * @param ssl The pointer to ssl data (context) which only its ssl->socket will be used.
     * @param buf The data to read.
     * @param bufLen The length of data to read.
     * @return size of data that was successfully read or negative value of error enum.
     */
    int ns_lwip_read(ssl_data *ssl, uint8_t *buf, int bufLen);

    /**
     * Send the mbedTLS error info to the callback.
     *
     * @param ssl The pointer to ssl data (context) which its ssl->_debugCallback will be used.
     * @param errNo The mbedTLS error number that will be translated to string via mbedtls_strerror.
     */
    void ssl_client_send_mbedtls_error_cb(ssl_data *ssl, int errNo);

    /**
     * Send the predefined flash string error to the callback.
     *
     * @param ssl The pointer to ssl data (context) which its ssl->_debugCallback will be used.
     * @param info The PROGMEM error string.
     */
    void ssl_client_debug_pgm_send_cb(ssl_data *ssl, PGM_P info);

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
};

#endif // ESP32

#endif // ESP32_SSL_Client_H
