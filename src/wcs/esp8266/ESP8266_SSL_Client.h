
/**
 *
 * The Network Upgradable BearSSL Client Class, ESP8266_SSL_Client.h v2.0.6
 *
 * Created August 3, 2023
 *
 * This works based on Earle F. Philhower ServerSecure class
 *
 * The MIT License (MIT)
 * Copyright (c) 2023 K. Suwatchai (Mobizt)
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

/*
  WiFiClientBearSSL- SSL client/server for esp8266 using BearSSL libraries
  - Mostly compatible with Arduino WiFi shield library and standard
    WiFiClient/ServerSecure (except for certificate handling).

  Copyright (c) 2018 Earle F. Philhower, III

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef ESP8266_SSL_Client_H
#define ESP8266_SSL_Client_H

#include <Arduino.h>
#include "ESP_Mail_FS.h"
#if (defined(ESP8266) || defined(MB_ARDUINO_PICO)) && (defined(ENABLE_SMTP) || defined(ENABLE_IMAP))

#include "ESP_Mail_FS.h"

#if defined(ESP8266)
#if __has_include(<core_esp8266_version.h>)
#include <core_esp8266_version.h>
#endif
#endif

#if defined(ESP_MAIL_USE_SDK_SSL_ENGINE) && !defined(USING_AXTLS)

#if !defined(SILENT_MODE)
static const char esp_ssl_client_str_1[] PROGMEM = "SSL/TLS negotiation";
#endif

#include <vector>
#include <bearssl/bearssl.h>
#include "MB_BearSSL.h"
#include "CertStoreBearSSL.h"

#include <memory>
#include <Client.h>
#include <Stream.h>

#if defined(ESP8266)
#include "extras/SDK_Version_Common.h"
#include "PolledTimeout.h"
#if defined(ESP8266_CORE_SDK_V3_X_X)
#include <umm_malloc/umm_heap_select.h>
#endif
#endif

#include "./wcs/base/TCP_Client_Base.h"

typedef void (*_ConnectionRequestCallback)(const char *, int);

namespace BearSSL
{

    class ESP8266_SSL_Client : public TCP_Client_Base
    {

    public:
        ESP8266_SSL_Client();
        ESP8266_SSL_Client(const ESP8266_SSL_Client &rhs) = delete;

        virtual ~ESP8266_SSL_Client();

        void setClient(Client *client);
        int connect(IPAddress ip, uint16_t port);
        int connect(const String &host, uint16_t port);
        int connect(const char *host, uint16_t port);
        bool connected();
        size_t write(const uint8_t *buf, size_t size);
        size_t write(uint8_t b);
        size_t write_P(PGM_P buf, size_t size);
        size_t write(Stream &stream);
        int read(uint8_t *buf, size_t size);
        int read(char *buf, size_t size) { return read((uint8_t *)buf, size); }
        int readBytes(uint8_t *buf, size_t size) { return read(buf, size); }
        int available();
        int read();
        int peek();
        size_t peekBytes(uint8_t *buffer, size_t length);
        void setInsecure();
        int connectSSL(IPAddress ip, uint16_t port);
        int connectSSL(const char *host, uint16_t port);
        void stop();
        void setTimeout(unsigned int timeoutMs);
        void setHandshakeTimeout(unsigned int timeoutMs);
        void flush();
        void setBufferSizes(int recv, int xmit);
        operator bool() { return connected() > 0; }
        int availableForWrite();
        void setSession(_BearSSL_Session *session);
        void setKnownKey(const PublicKey *pk, unsigned usages = BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN);
        bool setFingerprint(const uint8_t fingerprint[20]);
        bool setFingerprint(const char *fpStr);
        void allowSelfSignedCerts();
        void setTrustAnchors(const X509List *ta);
        void setX509Time(time_t now);
        void setClientRSACert(const X509List *cert, const PrivateKey *sk);
        void setClientECCert(const X509List *cert, const PrivateKey *sk,
                             unsigned allowed_usages, unsigned cert_issuer_key_type);
        int getMFLNStatus();
        int getLastSSLError(char *dest = nullptr, size_t len = 0);
        void setCertStore(CertStoreBase *certStore);
        bool setCiphers(const uint16_t *cipherAry, int cipherCount);
        bool setCiphers(const std::vector<uint16_t> &list);
        bool setCiphersLessSecure(); // Only use the limited set of RSA ciphers without EC
        bool setSSLVersion(uint32_t min = BR_TLS10, uint32_t max = BR_TLS12);
        size_t peekAvailable();
        const char *peekBuffer();
        void peekConsume(size_t consume);
        void setCACert(const char *rootCA);
        void setCertificate(const char *client_ca);
        void setPrivateKey(const char *private_key);
        bool loadCACert(Stream &stream, size_t size);
        bool loadCertificate(Stream &stream, size_t size);
        bool loadPrivateKey(Stream &stream, size_t size);
        int connect(IPAddress ip, uint16_t port, const char *rootCABuff, const char *cli_cert, const char *cli_key);
        int connect(const char *host, uint16_t port, const char *rootCABuff, const char *cli_cert, const char *cli_key);
        ESP8266_SSL_Client &operator=(const ESP8266_SSL_Client &other);
        bool operator==(const bool value) { return bool() == value; }
        bool operator!=(const bool value) { return bool() != value; }
        bool operator==(const ESP8266_SSL_Client &);
        bool operator!=(const ESP8266_SSL_Client &rhs) { return !this->operator==(rhs); };
        unsigned int getTimeout() const;
        void setSecure(const char *rootCABuff, const char *cli_cert, const char *cli_key);
        bool hasPeekBufferAPI() const{return true;} // return number of byte accessible by peekBuffer()
        

        // return a pointer to available data buffer (size = peekAvailable())
        // semantic forbids any kind of read() before calling peekConsume()

        int connect(IPAddress ip, uint16_t port, int32_t timeout)
        {
            auto save = _timeout;
            _timeout = timeout * 1000; // timeout is in secs, _timeout in milliseconds
            auto ret = connect(ip, port);
            _timeout = save;
            return ret;
        }
        int connect(const char *host, uint16_t port, int32_t timeout)
        {
            auto save = _timeout;
            _timeout = timeout * 1000; // timeout is in secs, _timeout in milliseconds
            auto ret = connect(host, port);
            _timeout = save;
            return ret;
        }

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

        Client *getClient()
        {
            return _basic_client;
        };

    protected:
        enum esp_mail_client_error_types
        {
            esp_ssl_ok,
            esp_ssl_connection_fail,
            esp_ssl_write_error,
            esp_ssl_read_error,
            esp_ssl_out_of_memory,
            esp_ssl_internal_error
        };

        void setWriteError(int err)
        {
            wErr = err;
        }
        int getWriteError()
        {
            return wErr;
        }
        unsigned long getTimeout()
        {
            return _timeout;
        }

        void setDebugLevel(int debug) { debugLevel = debug; }

        int connectSSL(const char *host);

    private:
#if defined(ENABLE_CUSTOM_CLIENT)
        _ConnectionRequestCallback connection_cb = NULL;
#endif
        int mIsClientInitialized();
        int mConnectBasicClient(const char *host, IPAddress ip, uint16_t port);
        void mClear();
        void mClearAuthenticationSettings();
        unsigned mUpdateEngine();
        void mBSSLX509InsecureInit(struct br_x509_insecure_context *ctx, int _use_fingerprint, const uint8_t _fingerprint[20], int _allow_self_signed);
        // Only one of the following two should ever be != nullptr!
        std::shared_ptr<br_ssl_client_context> _sc;
        br_ssl_engine_context *_eng; // &_sc->eng, to allow for client or server contexts
        std::shared_ptr<br_x509_minimal_context> _x509_minimal;
        std::shared_ptr<struct br_x509_insecure_context> _x509_insecure;
        std::shared_ptr<br_x509_knownkey_context> _x509_knownkey;
        std::shared_ptr<unsigned char> _iobuf_in;
        std::shared_ptr<unsigned char> _iobuf_out;
        time_t _now;
        const X509List *_ta;
        CertStoreBase *_certStore;
        int _iobuf_in_size;
        int _iobuf_out_size;
        bool _handshake_done;
        bool _oom_err;

        // Optional storage space pointer for session parameters
        // Will be used on connect and updated on close
        _BearSSL_Session *_session;

        bool _use_insecure;
        bool _use_fingerprint;
        uint8_t _fingerprint[20];
        bool _use_self_signed;
        const PublicKey *_knownkey;
        unsigned _knownkey_usages;

        // Custom cipher list pointer or nullptr if default
        std::shared_ptr<uint16_t> _cipher_list;
        uint8_t _cipher_cnt;

        // TLS ciphers allowed
        uint32_t _tls_min;
        uint32_t _tls_max;

        unsigned char *_recvapp_buf;
        size_t _recvapp_len;

        std::shared_ptr<unsigned char> mIOBufMemAloc(size_t sz);
        void mFreeSSL();
        int mRunUntil(const unsigned target, unsigned long timeout = 0);
        bool mSoftConnected();

        // Optional client certificate
        const X509List *_chain;
        const PrivateKey *_sk;
        unsigned _allowed_usages;
        unsigned _cert_issuer_key_type;

        // X.509 validators differ from server to client
        bool mInstallClientX509Validator(); // Set up X509 validator for a client conn.

        uint8_t *mStreamLoad(Stream &stream, size_t size);
        void idle();

        // ESP32 compatibility
        X509List *_esp32_ta = nullptr;
        X509List *_esp32_chain = nullptr;
        PrivateKey *_esp32_sk = nullptr;

        String host;
        bool _is_connected;

        //  store the index of where we are writing in the buffer
        //  so we can send our records all at once to prevent
        //  weird timing issues
        size_t _write_idx;

        // store the last BearSSL state so we can print changes to the console
        unsigned _bssl_last_state;

        bool _secure = false;

        Client *_basic_client = nullptr;
        unsigned long _timeout = 15000;
        unsigned long _handshake_timeout = 60000;
        int wErr = 0;
    };

};
#endif // Internal SSL engine for basic clients

#endif // ESP8266

#endif // ESP8266_SSL_Client_H
