/**
 *
 * The Network Upgradable BearSSL Client Class, MB_BearSSL.h v2.0.1
 *
 * Created March 1, 2023
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

#if defined(ESP8266) && (defined(ENABLE_SMTP) || defined(ENABLE_IMAP))

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


#endif // MB_BEARSSL_H_