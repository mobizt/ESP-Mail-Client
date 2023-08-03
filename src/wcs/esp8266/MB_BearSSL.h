/**
 *
 * The Network Upgradable BearSSL Client Class, MB_BearSSL.h v2.0.1
 *
 * Created March 21, 2023
 *
 * Do not modify.
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

#ifndef MB_BEARSSL_H_
#define MB_BEARSSL_H_

#pragma once
#include <Arduino.h>
#include "ESP_Mail_FS.h"

#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wvla"

#if (defined(ESP8266) || defined(MB_ARDUINO_PICO)) && (defined(ENABLE_SMTP) || defined(ENABLE_IMAP))

#if defined(DEBUG_ESP_SSL) && defined(DEBUG_ESP_PORT)
#define DEBUG_BSSL(fmt, ...) DEBUG_ESP_PORT.printf_P((PGM_P)PSTR("BSSL:" fmt), ##__VA_ARGS__)
#else
#define DEBUG_BSSL(...)
#endif

#if defined(ESP8266)

#include <bearssl/bearssl.h>
#include <vector>
#include <StackThunk.h>
#include <sys/time.h>
#include <IPAddress.h>
#include <Client.h>
#include <FS.h>
#include <time.h>
#include <ctype.h>
#include <vector>
#include <algorithm>

#elif defined(MB_ARDUINO_PICO)

#include <Arduino.h>
#include <bearssl/bearssl.h>
#include <Updater.h>
#include <StackThunk.h>

#endif

namespace BearSSL
{

    static uint8_t htoi(unsigned char c)
    {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        else if (c >= 'A' && c <= 'F')
        {
            return 10 + c - 'A';
        }
        else if (c >= 'a' && c <= 'f')
        {
            return 10 + c - 'a';
        }
        else
        {
            return 255;
        }
    }

    extern "C"
    {

        // BearSSL doesn't define a true insecure decoder, so we make one ourselves
        // from the simple parser.  It generates the issuer and subject hashes and
        // the SHA1 fingerprint, only one (or none!) of which will be used to
        // "verify" the certificate.

        // Private x509 decoder state
        struct br_x509_insecure_context
        {
            const br_x509_class *vtable;
            bool done_cert;
            const uint8_t *match_fingerprint;
            br_sha1_context sha1_cert;
            bool allow_self_signed;
            br_sha256_context sha256_subject;
            br_sha256_context sha256_issuer;
            br_x509_decoder_context ctx;
        };

        // Callback for the x509_minimal subject DN
        static void insecure_subject_dn_append(void *ctx, const void *buf, size_t len)
        {
            br_x509_insecure_context *xc = (br_x509_insecure_context *)ctx;
            br_sha256_update(&xc->sha256_subject, buf, len);
        }

        // Callback for the x509_minimal issuer DN
        static void insecure_issuer_dn_append(void *ctx, const void *buf, size_t len)
        {
            br_x509_insecure_context *xc = (br_x509_insecure_context *)ctx;
            br_sha256_update(&xc->sha256_issuer, buf, len);
        }

        // Callback on the first byte of any certificate
        static void insecure_start_chain(const br_x509_class **ctx, const char *server_name)
        {
            br_x509_insecure_context *xc = (br_x509_insecure_context *)ctx;
            br_x509_decoder_init(&xc->ctx, insecure_subject_dn_append, xc, insecure_issuer_dn_append, xc);
            xc->done_cert = false;
            br_sha1_init(&xc->sha1_cert);
            br_sha256_init(&xc->sha256_subject);
            br_sha256_init(&xc->sha256_issuer);
            (void)server_name;
        }

        // Callback for each certificate present in the chain (but only operates
        // on the first one by design).
        static void insecure_start_cert(const br_x509_class **ctx, uint32_t length)
        {
            (void)ctx;
            (void)length;
        }

        // Callback for each byte stream in the chain.  Only process first cert.
        static void insecure_append(const br_x509_class **ctx, const unsigned char *buf, size_t len)
        {
            br_x509_insecure_context *xc = (br_x509_insecure_context *)ctx;
            // Don't process anything but the first certificate in the chain
            if (!xc->done_cert)
            {
                br_sha1_update(&xc->sha1_cert, buf, len);
                br_x509_decoder_push(&xc->ctx, (const void *)buf, len);
#if defined(DEBUG_ESP_SSL) && defined(DEBUG_ESP_PORT)
                DEBUG_BSSL("CERT: ");
                for (size_t i = 0; i < len; i++)
                {
                    DEBUG_ESP_PORT.printf_P(PSTR("%02x "), buf[i] & 0xff);
                }
                DEBUG_ESP_PORT.printf_P(PSTR("\n"));
#endif
            }
        }

        // Callback on individual cert end.
        static void insecure_end_cert(const br_x509_class **ctx)
        {
            br_x509_insecure_context *xc = (br_x509_insecure_context *)ctx;
            xc->done_cert = true;
        }

        // Callback when complete chain has been parsed.
        // Return 0 on validation success, !0 on validation error
        static unsigned insecure_end_chain(const br_x509_class **ctx)
        {
            const br_x509_insecure_context *xc = (const br_x509_insecure_context *)ctx;
            if (!xc->done_cert)
            {
                DEBUG_BSSL("insecure_end_chain: No cert seen\n");
                return 1; // error
            }

            // Handle SHA1 fingerprint matching
            char res[20];
            br_sha1_out(&xc->sha1_cert, res);
            if (xc->match_fingerprint && memcmp(res, xc->match_fingerprint, sizeof(res)))
            {
#ifdef DEBUG_ESP_SSL
                DEBUG_BSSL("insecure_end_chain: Received cert FP doesn't match\n");
                char buff[3 * sizeof(res) + 1]; // 3 chars per byte XX_, and null
                buff[0] = 0;
                for (size_t i = 0; i < sizeof(res); i++)
                {
                    char hex[4]; // XX_\0
                    snprintf(hex, sizeof(hex), "%02x ", xc->match_fingerprint[i] & 0xff);
                    // strlcat(buff, hex, sizeof(buff));
                }
                DEBUG_BSSL("insecure_end_chain: expected %s\n", buff);
                buff[0] = 0;
                for (size_t i = 0; i < sizeof(res); i++)
                {
                    char hex[4]; // XX_\0
                    snprintf(hex, sizeof(hex), "%02x ", res[i] & 0xff);
                    // strlcat(buff, hex, sizeof(buff));
                }
                DEBUG_BSSL("insecure_end_chain: received %s\n", buff);
#endif
                return BR_ERR_X509_NOT_TRUSTED;
            }

            // Handle self-signer certificate acceptance
            char res_issuer[32];
            char res_subject[32];
            br_sha256_out(&xc->sha256_issuer, res_issuer);
            br_sha256_out(&xc->sha256_subject, res_subject);
            if (xc->allow_self_signed && memcmp(res_subject, res_issuer, sizeof(res_issuer)))
            {
                DEBUG_BSSL("insecure_end_chain: Didn't get self-signed cert\n");
                return BR_ERR_X509_NOT_TRUSTED;
            }

            // Default (no validation at all) or no errors in prior checks = success.
            return 0;
        }

        // Return the public key from the validator (set by x509_minimal)
        static const br_x509_pkey *insecure_get_pkey(const br_x509_class *const *ctx, unsigned *usages)
        {
            const br_x509_insecure_context *xc = (const br_x509_insecure_context *)ctx;
            if (usages != nullptr)
            {
                *usages = BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN; // I said we were insecure!
            }
            return &xc->ctx.pkey;
        }

        // Some constants uses to init the server/client contexts
        // Note that suites_P needs to be copied to RAM before use w/BearSSL!
        // List copied verbatim from BearSSL/ssl_client_full.c
        /*
            The "full" profile supports all implemented cipher suites.

            Rationale for suite order, from most important to least
            important rule:

            -- Don't use 3DES if AES or ChaCha20 is available.
            -- Try to have Forward Secrecy (ECDHE suite) if possible.
            -- When not using Forward Secrecy, ECDH key exchange is
              better than RSA key exchange (slightly more expensive on the
              client, but much cheaper on the server, and it implies smaller
              messages).
            -- ChaCha20+Poly1305 is better than AES/GCM (faster, smaller code).
            -- GCM is better than CCM and CBC. CCM is better than CBC.
            -- CCM is preferable over CCM_8 (with CCM_8, forgeries may succeed
              with probability 2^(-64)).
            -- AES-128 is preferred over AES-256 (AES-128 is already
              strong enough, and AES-256 is 40% more expensive).
        */
        static const uint16_t suites_P[] PROGMEM = {
#ifndef BEARSSL_SSL_BASIC
            BR_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
            BR_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CCM,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CCM,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA,
            BR_TLS_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_RSA_WITH_AES_128_CCM,
            BR_TLS_RSA_WITH_AES_256_CCM,
            BR_TLS_RSA_WITH_AES_128_CCM_8,
            BR_TLS_RSA_WITH_AES_256_CCM_8,
#endif
            BR_TLS_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_256_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_RSA_WITH_AES_256_CBC_SHA,
#ifndef BEARSSL_SSL_BASIC
            BR_TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_RSA_WITH_3DES_EDE_CBC_SHA
#endif
        };
#ifndef BEARSSL_SSL_BASIC
        // Server w/EC has one set, not possible with basic SSL config
        static const uint16_t suites_server_ec_P[] PROGMEM = {
            BR_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CCM,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CCM,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA,
            BR_TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA};
#endif

        static const uint16_t suites_server_rsa_P[] PROGMEM = {
#ifndef BEARSSL_SSL_BASIC
            BR_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
            BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384,
            BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,
            BR_TLS_RSA_WITH_AES_128_GCM_SHA256,
            BR_TLS_RSA_WITH_AES_256_GCM_SHA384,
            BR_TLS_RSA_WITH_AES_128_CCM,
            BR_TLS_RSA_WITH_AES_256_CCM,
            BR_TLS_RSA_WITH_AES_128_CCM_8,
            BR_TLS_RSA_WITH_AES_256_CCM_8,
#endif
            BR_TLS_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_256_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_128_CBC_SHA,
            BR_TLS_RSA_WITH_AES_256_CBC_SHA,
#ifndef BEARSSL_SSL_BASIC
            BR_TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA,
            BR_TLS_RSA_WITH_3DES_EDE_CBC_SHA
#endif
        };

        // For apps which want to use less secure but faster ciphers, only
        static const uint16_t faster_suites_P[] PROGMEM = {
            BR_TLS_RSA_WITH_AES_256_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_128_CBC_SHA256,
            BR_TLS_RSA_WITH_AES_256_CBC_SHA,
            BR_TLS_RSA_WITH_AES_128_CBC_SHA};

        // Install hashes into the SSL engine
        static void br_ssl_client_install_hashes(br_ssl_engine_context *eng)
        {
            br_ssl_engine_set_hash(eng, br_md5_ID, &br_md5_vtable);
            br_ssl_engine_set_hash(eng, br_sha1_ID, &br_sha1_vtable);
            br_ssl_engine_set_hash(eng, br_sha224_ID, &br_sha224_vtable);
            br_ssl_engine_set_hash(eng, br_sha256_ID, &br_sha256_vtable);
            br_ssl_engine_set_hash(eng, br_sha384_ID, &br_sha384_vtable);
            br_ssl_engine_set_hash(eng, br_sha512_ID, &br_sha512_vtable);
        }

        static void br_x509_minimal_install_hashes(br_x509_minimal_context *x509)
        {
            br_x509_minimal_set_hash(x509, br_md5_ID, &br_md5_vtable);
            br_x509_minimal_set_hash(x509, br_sha1_ID, &br_sha1_vtable);
            br_x509_minimal_set_hash(x509, br_sha224_ID, &br_sha224_vtable);
            br_x509_minimal_set_hash(x509, br_sha256_ID, &br_sha256_vtable);
            br_x509_minimal_set_hash(x509, br_sha384_ID, &br_sha384_vtable);
            br_x509_minimal_set_hash(x509, br_sha512_ID, &br_sha512_vtable);
        }

        // Default initializion for our SSL clients
        static void br_ssl_client_base_init(br_ssl_client_context *cc, const uint16_t *cipher_list, int cipher_cnt)
        {
            uint16_t suites[cipher_cnt];
            memcpy_P(suites, cipher_list, cipher_cnt * sizeof(cipher_list[0]));
            br_ssl_client_zero(cc);
            br_ssl_engine_add_flags(&cc->eng, BR_OPT_NO_RENEGOTIATION); // forbid SSL renegotiation, as we free the Private Key after handshake
            br_ssl_engine_set_versions(&cc->eng, BR_TLS10, BR_TLS12);
            br_ssl_engine_set_suites(&cc->eng, suites, (sizeof suites) / (sizeof suites[0]));
            br_ssl_client_set_default_rsapub(cc);
            br_ssl_engine_set_default_rsavrfy(&cc->eng);
#ifndef BEARSSL_SSL_BASIC
            br_ssl_engine_set_default_ecdsa(&cc->eng);
#endif
            br_ssl_client_install_hashes(&cc->eng);
            br_ssl_engine_set_prf10(&cc->eng, &br_tls10_prf);
            br_ssl_engine_set_prf_sha256(&cc->eng, &br_tls12_sha256_prf);
            br_ssl_engine_set_prf_sha384(&cc->eng, &br_tls12_sha384_prf);
            br_ssl_engine_set_default_aes_cbc(&cc->eng);
#ifndef BEARSSL_SSL_BASIC
            br_ssl_engine_set_default_aes_gcm(&cc->eng);
            br_ssl_engine_set_default_aes_ccm(&cc->eng);
            br_ssl_engine_set_default_des_cbc(&cc->eng);
            br_ssl_engine_set_default_chapol(&cc->eng);
#endif
        }
    }

    // Opaque object which wraps the BearSSL SSL session to make repeated connections
    // significantly faster.  Completely optional.
    class MB_BSSL_SSL_Client;

    // Cache for a TLS session with a server
    // Use with BearSSL::WiFiClientSecure::setSession
    // to accelerate the TLS handshake
    class _BearSSL_Session
    {
        friend class ESP8266_SSL_Client;

    public:
        _BearSSL_Session()
        {
            memset(&_session, 0, sizeof(_session));
        }

    private:
        br_ssl_session_parameters *getSession()
        {
            return &_session;
        }
        // The actual BearSSL session information
        br_ssl_session_parameters _session;
    };

    // Represents a single server session.
    // Use with BearSSL::ServerSessions.
    typedef uint8_t MB_ServerSession[100];

    // Cache for the TLS sessions of multiple clients.
    // Use with BearSSL::WiFiServerSecure::setCache
    class _BearSSL_ServerSessions
    {
        friend class ESP8266_SSL_Client;

    public:
        // Uses the given buffer to cache the given number of sessions and initializes it.
        _BearSSL_ServerSessions(MB_ServerSession *sessions, uint32_t size) : _BearSSL_ServerSessions(sessions, size, false) {}

        // Dynamically allocates a cache for the given number of sessions and initializes it.
        // If the allocation of the buffer wasn't successful, the value
        // returned by size() will be 0.
        _BearSSL_ServerSessions(uint32_t size) : _BearSSL_ServerSessions(size > 0 ? new MB_ServerSession[size] : nullptr, size, true) {}

        ~_BearSSL_ServerSessions()
        {
            if (_isDynamic && _store != nullptr)
            {
                delete _store;
            }
        }

        // Returns the number of sessions the cache can hold.
        uint32_t size()
        {
            return _size;
        }

    private:
        _BearSSL_ServerSessions(MB_ServerSession *sessions, uint32_t size, bool isDynamic) : _size(sessions != nullptr ? size : 0),
                                                                                             _store(sessions), _isDynamic(isDynamic)
        {
            if (_size > 0)
            {
                br_ssl_session_cache_lru_init(&_cache, (uint8_t *)_store, size * sizeof(MB_ServerSession));
            }
        }

        // Returns the cache's vtable or null if the cache has no capacity.
        const br_ssl_session_cache_class **getCache()
        {
            return _size > 0 ? &_cache.vtable : nullptr;
        }

        // Size of the store in sessions.
        uint32_t _size;
        // Store where the information for the sessions are stored.
        MB_ServerSession *_store;
        // Whether the store is dynamically allocated.
        // If this is true, the store needs to be freed in the destructor.
        bool _isDynamic;

        // Cache of the server using the _store.
        br_ssl_session_cache_lru _cache;
    };

};

#endif

#endif // MB_BEARSSL_H_