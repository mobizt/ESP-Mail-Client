#ifndef ESP_MAIL_CLIENT_CPP
#define ESP_MAIL_CLIENT_CPP

/**
 * Mail Client Arduino Library for Espressif's ESP32 and ESP8266, Raspberry Pi RP2040 Pico, and SAMD21 with u-blox NINA-W102 WiFi/Bluetooth module
 *
 * Created March 5, 2023
 *
 * This library allows Espressif's ESP32, ESP8266, SAMD and RP2040 Pico devices to send and read Email through the SMTP and IMAP servers.
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

#include "ESP_Mail_Client.h"
#include "ESP_Mail_Client_Version.h"

#include "ESP_Mail_IMAP.h"
#include "ESP_Mail_SMTP.h"

#if defined(ENABLE_SMTP) || defined(ENABLE_IMAP)
void ESP_Mail_Client::resumeNetwork(ESP_MAIL_TCP_CLIENT *client)
{

#if defined(ENABLE_CUSTOM_CLIENT)
  client->networkReconnect();
#else

#if defined(ESP32) || defined(ESP8266)
  WiFi.reconnect();
#else
  if (wifi.credentials.size() > 0)
  {
#if __has_include(<WiFi.h>) || __has_include(<WiFiNINA.h>) || __has_include(<WiFi101.h>)
    if (!networkStatus)
    {
      WiFi.disconnect();
#if defined(HAS_WIFIMULTI)
      if (multi)
        delete multi;
      multi = nullptr;

      multi = new WiFiMulti();
      for (size_t i = 0; i < wifi.credentials.size(); i++)
        multi->addAP(wifi.credentials[i].ssid.c_str(), wifi.credentials[i].password.c_str());

      if (wifi.credentials.size() > 0)
        multi->run();
#else
      WiFi.begin(wifi.credentials[0].ssid.c_str(), wifi.credentials[0].password.c_str());
#endif
    }
#endif
  }

#endif

#endif
}
#endif

void ESP_Mail_Client::networkReconnect(bool reconnect)
{
#if defined(ESP32) || defined(ESP8266)
  WiFi.setAutoReconnect(reconnect);
#endif
  networkAutoReconnect = reconnect;
}

void ESP_Mail_Client::setUDPClient(UDP *client, float gmtOffset)
{
  Time.setUDPClient(client, gmtOffset);
}

void ESP_Mail_Client::addAP(const String &ssid, const String &password)
{
  wifi.addAP(ssid, password);
}

void ESP_Mail_Client::clearAP()
{
  wifi.clearAP();
}

#if defined(MBFS_SD_FS) && defined(MBFS_CARD_TYPE_SD)

bool ESP_Mail_Client::sdBegin(int8_t ss, int8_t sck, int8_t miso, int8_t mosi, uint32_t frequency)
{
  return mbfs->sdBegin(ss, sck, miso, mosi, frequency);
}

#if defined(ESP8266) || defined(MB_ARDUINO_PICO)
bool ESP_Mail_Client::sdBegin(SDFSConfig *sdFSConfig)
{
  return mbfs->sdFatBegin(sdFSConfig);
}
#endif

#if defined(ESP32)
bool ESP_Mail_Client::sdBegin(int8_t ss, SPIClass *spiConfig, uint32_t frequency)
{
  return mbfs->sdSPIBegin(ss, spiConfig, frequency);
}
#endif

#if defined(MBFS_ESP32_SDFAT_ENABLED) || defined(MBFS_SDFAT_ENABLED)
bool ESP_Mail_Client::sdBegin(SdSpiConfig *sdFatSPIConfig, int8_t ss, int8_t sck, int8_t miso, int8_t mosi)
{
  return mbfs->sdFatBegin(sdFatSPIConfig, ss, sck, miso, mosi);
}

bool ESP_Mail_Client::sdBegin(SdioConfig *sdFatSDIOConfig)
{
  return mbfs->sdFatBegin(sdFatSDIOConfig);
}
#endif

#endif

#if defined(ESP32) && defined(MBFS_SD_FS) && defined(MBFS_CARD_TYPE_SD_MMC)

bool ESP_Mail_Client::sdMMCBegin(const char *mountpoint, bool mode1bit, bool format_if_mount_failed)
{
  return mbfs->sdMMCBegin(mountpoint, mode1bit, format_if_mount_failed);
}

#endif

int ESP_Mail_Client::getFreeHeap()
{
#if defined(MB_ARDUINO_ESP)
  return ESP.getFreeHeap();
#elif defined(MB_ARDUINO_PICO)
  return rp2040.getFreeHeap();
#else
  return 0;
#endif
}

#if defined(ENABLE_SMTP) || defined(ENABLE_IMAP)

void ESP_Mail_Client::setTimezone(const char *TZ_Var, const char *TZ_file)
{

  if (!TZ_Var)
    return;

#if defined(ESP32) || defined(ESP8266) || defined(MB_ARDUINO_PICO)
  if (strlen(TZ_Var) > 0)
  {

#if defined(ESP32)

    mb_fs_mem_storage_type type;

#if defined(MBFS_FLASH_FS)
    type = mb_fs_mem_storage_type_flash;
#elif defined(MBFS_SD_FS)
    type = mb_fs_mem_storage_type_sd;
#endif

    if (type != mb_fs_mem_storage_type_undefined)
    {
      MB_String filename = TZ_file;

      if (mbfs->open(filename, type, mb_fs_open_mode_write) > -1)
      {
        mbfs->print(type, TZ_Var);
        mbfs->close(type);
      }
    }

#endif

    setenv("TZ", TZ_Var, 1);
    tzset();
  }
#endif
}

void ESP_Mail_Client::preparePortFunction(Session_Config *session_config, bool isSMTP, bool &secure, bool &secureMode, bool &ssl)
{

  if (session_config->ports_functions.list)
  {
    if (session_config->ports_functions.use_internal_list)
    {
      session_config->ports_functions.use_internal_list = false;
      delete[] session_config->ports_functions.list;
    }
  }

  if (!session_config->ports_functions.list)
  {
    session_config->ports_functions.use_internal_list = true;

    if (isSMTP)
    {
      session_config->ports_functions.list = new port_function[3];
      session_config->ports_functions.size = 3;

      session_config->ports_functions.list[0].port = 25;
      session_config->ports_functions.list[0].protocol = esp_mail_protocol_plain_text;

      session_config->ports_functions.list[1].port = 465;
      session_config->ports_functions.list[1].protocol = esp_mail_protocol_ssl;

      session_config->ports_functions.list[2].port = 587;
      session_config->ports_functions.list[2].protocol = esp_mail_protocol_tls;
    }
    else
    {
      session_config->ports_functions.list = new port_function[2];
      session_config->ports_functions.size = 2;

      session_config->ports_functions.list[0].port = 143;
      session_config->ports_functions.list[0].protocol = esp_mail_protocol_plain_text;

      session_config->ports_functions.list[1].port = 993;
      session_config->ports_functions.list[1].protocol = esp_mail_protocol_ssl;
    }
  }

  getPortFunction(session_config->server.port, session_config->ports_functions, secure, secureMode, ssl, session_config->secure.startTLS);
}

void ESP_Mail_Client::getPortFunction(uint16_t port, struct esp_mail_ports_functions &ports_functions, bool &secure, bool &secureMode, bool &ssl, bool &starttls)
{
  for (size_t i = 0; i < ports_functions.size; i++)
  {
    if (ports_functions.list[i].port == port)
    {
      if (ports_functions.list[i].protocol == esp_mail_protocol_plain_text)
      {
        secure = false;
        secureMode = false;
      }
      else
      {
        if (ports_functions.list[i].protocol == esp_mail_protocol_tls)
          starttls = true;

        secureMode = !starttls;

        if (ports_functions.list[i].protocol == esp_mail_protocol_ssl)
          ssl = true;
      }
      return;
    }
  }
}

void ESP_Mail_Client::getTimezone(const char *TZ_file, MB_String &out)
{

#if defined(ESP32)

  mb_fs_mem_storage_type type;

#if defined(MBFS_FLASH_FS)
  type = mb_fs_mem_storage_type_flash;
#elif defined(MBFS_SD_FS)
  type = mb_fs_mem_storage_type_sd;
#endif

  if (type != mb_fs_mem_storage_type_undefined)
  {
    MB_String filename = TZ_file;

    if (mbfs->open(filename, type, mb_fs_open_mode_read) > 0)
    {
      out.clear();
      while (mbfs->available(type))
      {
        out += (char)mbfs->read(type);
      }
      mbfs->close(type);
    }
  }
#endif
}

void ESP_Mail_Client::idle()
{
#if defined(ARDUINO_ESP8266_MAJOR) && defined(ARDUINO_ESP8266_MINOR) && defined(ARDUINO_ESP8266_REVISION) && ((ARDUINO_ESP8266_MAJOR == 3 && ARDUINO_ESP8266_MINOR >= 1) || ARDUINO_ESP8266_MAJOR > 3)
  esp_yield();
#else
  delay(0);
#endif
}

void ESP_Mail_Client::setTime(float gmt_offset, float day_light_offset, const char *ntp_server, const char *TZ_Var, const char *TZ_file, bool wait)
{

#if defined(ESP32) || defined(ESP8266) || defined(MB_ARDUINO_PICO) || defined(ARDUINO_ARCH_SAMD) || defined(__AVR_ATmega4809__) || defined(MB_ARDUINO_NANO_RP2040_CONNECT)

  _clockReady = Time.clockReady();

  if (!_clockReady)
  {
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
    if (!Time.initUDP())
      ESP_MAIL_PRINTF("> W: UDP client is required for NTP server time synching based on your network type \ne.g. WiFiUDP or EthernetUDP. Please call MailClient.setUDPClient(&udpClient, gmtOffset); to assign the UDP client.\n");
#endif
    Time.setClock(gmt_offset, day_light_offset, ntp_server);

    if (wait)
    {
      unsigned long waitMs = millis();
      while (!Time.clockReady() && millis() - waitMs < 10000)
      {
        idle();
      }
    }
  }

  // set and get TZ environment variable

  MB_String timezone;

  // only ESP32 only
  getTimezone(TZ_file, timezone);

  if (timezone.length() == 0)
    timezone = TZ_Var;

  // if timezone string assign
  setTimezone(timezone.c_str(), TZ_file);

#endif

  _clockReady = Time.clockReady();
}

bool ESP_Mail_Client::validEmail(const char *s)
{
  MB_String str(s);
  size_t at = str.find('@');
  size_t dot = str.find('.', at);
  return (at != MB_String::npos) && (dot != MB_String::npos);
}

#if defined(ENABLE_SMTP) && defined(ENABLE_IMAP)
bool ESP_Mail_Client::mAppendMessage(IMAPSession *imap, SMTP_Message *msg, bool lastAppend, MB_StringPtr flags, MB_StringPtr dateTime)
{
  this->imap = imap;
  calDataLen = true;
  dataLen = 0;
  imap_ts = 0;

  MB_String _flags = flags;
  _flags.trim();

  MB_String _dt = dateTime;
  _dt.trim();

  bool rfc822MSG = false;

  sendContent(nullptr, msg, false, rfc822MSG);

  MB_String cmd;

  if (!imap->_read_capability.multiappend)
  {
    lastAppend = true;
    imap->_prev_imap_cmd = esp_mail_imap_cmd_sasl_login;
  }

  if (imap->_prev_imap_cmd != esp_mail_imap_cmd_append)
  {
    cmd = esp_mail_str_27 /* "Xmail" */;
    cmd += esp_mail_str_131 /* " " */;
    cmd += esp_mail_str_360 /* "APPEND" */;
    cmd += esp_mail_str_131 /* " " */;
    cmd += imap->_currentFolder;
  }

  cmd += esp_mail_str_131 /* " " */;

  if (_flags.length() > 0)
  {
    cmd += esp_mail_str_198 /* "(" */;
    cmd += _flags;
    cmd += esp_mail_str_192 /* ")" */;
    cmd += esp_mail_str_131 /* " " */;
  }

  if (_dt.length() > 0)
  {
    cmd += esp_mail_str_136 /* "\"" */;
    cmd += _dt;
    cmd += esp_mail_str_136 /* "\"" */;
    cmd += esp_mail_str_131 /* " " */;
  }

  cmd += esp_mail_str_193 /* "{" */;
  cmd += dataLen;
  cmd += esp_mail_str_194 /* "}" */;

  if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
  {
    imap->_prev_imap_cmd = esp_mail_imap_cmd_sasl_login;
    return false;
  }

  imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_append;

  if (!handleIMAPResponse(imap, IMAP_STATUS_BAD_COMMAND, false))
  {
    imap->_prev_imap_cmd = esp_mail_imap_cmd_sasl_login;
    return false;
  }

  calDataLen = false;

  rfc822MSG = false;

  if (!sendContent(nullptr, msg, false, rfc822MSG))
  {
    imap->_prev_imap_cmd = esp_mail_imap_cmd_sasl_login;
    return false;
  }

  if (lastAppend)
  {
    if (imapSend(imap, esp_mail_str_34 /* "\r\n" */, false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    {
      imap->_prev_imap_cmd = esp_mail_imap_cmd_sasl_login;
      return false;
    }

    imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_append_last;

    if (!handleIMAPResponse(imap, IMAP_STATUS_BAD_COMMAND, false))
    {
      imap->_prev_imap_cmd = esp_mail_imap_cmd_sasl_login;
      return false;
    }

    imap->_prev_imap_cmd = esp_mail_imap_cmd_sasl_login;
  }

  if (!lastAppend)
    imap->_prev_imap_cmd = esp_mail_imap_cmd_append;
  else
    altSendCallback(nullptr, esp_mail_str_363 /* "Message append successfully" */, esp_mail_str_364 /* "> C: Message append successfully" */, true, false);

  return true;
}

#endif
char *ESP_Mail_Client::getRandomUID()
{
  char *tmp = (char *)newP(36);
  sprintf(tmp, (const char *)MBSTRING_FLASH_MCR("%d"), (int)random(2000, 4000));
  return tmp;
}

/* Safe string splitter to avoid strsep bugs */
void ESP_Mail_Client::splitToken(MB_String &str, MB_VECTOR<MB_String> &tk, const char *delim)
{
  uint32_t current, previous = 0;
  current = str.find(delim, previous);
  MB_String s;
  while (current != MB_String::npos)
  {
    s = str.substr(previous, current - previous);
    tk.push_back(s);
    previous = current + strlen(delim);
    current = str.find(delim, previous);
  }
  s = str.substr(previous, current - previous);
  tk.push_back(s);
  s.clear();
}

int ESP_Mail_Client::strpos(const char *haystack, const char *needle, int offset, bool caseSensitive)
{
  if (!haystack || !needle)
    return -1;

  int hlen = strlen(haystack);
  int nlen = strlen(needle);

  if (hlen == 0 || nlen == 0)
    return -1;

  int hidx = offset, nidx = 0;
  while ((*(haystack + hidx) != '\0') && (*(needle + nidx) != '\0') && hidx < hlen)
  {
    bool nm = caseSensitive ? *(needle + nidx) != *(haystack + hidx) : tolower(*(needle + nidx)) != tolower(*(haystack + hidx));

    if (nm)
    {
      hidx++;
      nidx = 0;
    }
    else
    {
      nidx++;
      hidx++;
      if (nidx == nlen)
        return hidx - nidx;
    }
  }

  return -1;
}

char *ESP_Mail_Client::subStr(const char *buf, PGM_P begin_PGM, PGM_P end_PGM, int beginPos, int endPos, bool caseSensitive)
{
  char *tmp = nullptr;
  if (begin_PGM)
  {
    int p1 = strposP(buf, begin_PGM, beginPos, caseSensitive);
    if (p1 != -1)
    {
      while (buf[p1 + strlen_P(begin_PGM)] == ' ' || buf[p1 + strlen_P(begin_PGM)] == '\r' || buf[p1 + strlen_P(begin_PGM)] == '\n')
      {
        p1++;
        if (strlen(buf) <= p1 + strlen_P(begin_PGM))
        {
          p1--;
          break;
        }
      }

      int p2 = -1;
      if (endPos == 0)
        p2 = strposP(buf, end_PGM, p1 + strlen_P(begin_PGM), caseSensitive);

      if (p2 == -1)
        p2 = strlen(buf);

      int len = p2 - p1 - strlen_P(begin_PGM);
      int ofs = end_PGM ? strlen_P(end_PGM) : 1;
      tmp = (char *)newP(len + ofs);
      memcpy(tmp, &buf[p1 + strlen_P(begin_PGM)], len);
    }
  }
  else
  {
    int p1 = strposP(buf, end_PGM, beginPos);
    if (p1 != -1)
    {
      tmp = (char *)newP(p1);
      memcpy(tmp, &buf[2], p1 - 1);
    }
  }

  return tmp;
}

bool ESP_Mail_Client::getHeader(const char *buf, PGM_P begin_PGM, MB_String &out, bool caseSensitive)
{
  if (strcmpP(buf, 0, begin_PGM, caseSensitive))
  {
    char *tmp = subStr(buf, begin_PGM, NULL, 0, -1, caseSensitive);
    if (tmp)
    {
      out = tmp;
      delP(&tmp);
      return true;
    }
  }

  return false;
}

void ESP_Mail_Client::getExtfromMIME(const char *mime, MB_String &ext)
{
  ext.clear();
  for (int i = 0; i < esp_mail_file_extension_maxType; i++)
  {
    if (strcmp_P(mime, mimeinfo[i].mimeType) == 0)
    {
      ext = mimeinfo[i].endsWith;
      break;
    }
  }

  if (ext.length() == 0)
    ext = esp_mail_str_40; /* ".dat" */
}

MB_String ESP_Mail_Client::mGetBase64(MB_StringPtr str)
{
  MB_String data = str;
  return encodeBase64Str((uint8_t *)(data.c_str()), data.length());
}

int ESP_Mail_Client::readLine(ESP_MAIL_TCP_CLIENT *client, char *buf, int bufLen, bool crlf, int &count)
{
  int ret = -1;
  char c = 0;
  char _c = 0;
  int idx = 0;

  while (client->connected() && client->available() && idx < bufLen)
  {

    idle();

    ret = client->read();
    if (ret > -1)
    {
      c = (char)ret;
      buf[idx++] = c;
      count++;
      if (_c == '\r' && c == '\n')
      {
        if (!crlf)
        {
          buf[idx - 2] = 0;
          idx -= 2;
        }
        return idx;
      }
      _c = c;

      if (idx >= bufLen - 1)
        return idx;
    }
  }
  return idx;
}

bool ESP_Mail_Client::isResponseCB(void *cb, bool isSMTP)
{
  bool isCb = false;
  if (isSMTP)
  {
#if defined(ENABLE_SMTP)
    isCb = (smtpResponseCallback)cb != NULL;
#endif
  }
  else
  {
#if defined(ENABLE_IMAP)
    isCb = (imapResponseCallback)cb != NULL;
#endif
  }
  return isCb;
}

void ESP_Mail_Client::printLibInfo(void *cb, void *sessionPtr, ESP_MAIL_TCP_CLIENT *client, bool debug, bool isSMTP)
{
  MB_String s;

  bool isCb = isResponseCB(cb, isSMTP);

  // Server connection attempt: no status code
  if (isSMTP)
  {
#if defined(ENABLE_SMTP)
    if (((SMTPSession *)sessionPtr)->_sendCallback != NULL && !isCb)
      smtpCB((SMTPSession *)sessionPtr, esp_mail_str_120 /* "Connecting to SMTP server..." */, false, false);
#endif
  }
  else
  {
#if defined(ENABLE_IMAP)
    if (((IMAPSession *)sessionPtr)->_readCallback != NULL && !isCb)
      imapCB((IMAPSession *)sessionPtr, esp_mail_str_50 /* "Connecting to IMAP server..." */, false, false);
#endif
  }

  if (debug && !isCb)
  {
    s = esp_mail_str_314; /* "> C: ESP Mail Client v" */
    s += ESP_MAIL_VERSION;
    s += client->fwVersion();
    esp_mail_debug_print(s.c_str(), true);

#if defined(BOARD_HAS_PSRAM) && defined(MB_STRING_USE_PSRAM)
    if (ESP.getPsramSize() == 0 && !isCb)
    {
      s = esp_mail_str_353; /* "! W: PSRAM was enabled but not detected." */
      esp_mail_debug_print(s.c_str(), true);
    }
#endif
  }
}

bool ESP_Mail_Client::beginConnection(Session_Config *session_config, void *cb, void *sessionPtr, ESP_MAIL_TCP_CLIENT *client, bool debug, bool isSMTP, bool secureMode)
{
  MB_String s;

  bool isCb = isResponseCB(cb, isSMTP);

  if (debug && !isCb)
  {
    if (isSMTP)
    {
#if defined(ENABLE_SMTP)
      esp_mail_debug_print(esp_mail_str_236 /* "> C: Connect to SMTP server" */, true);
#endif
    }
    else
    {
#if defined(ENABLE_IMAP)
      esp_mail_debug_print(esp_mail_str_225 /* "> C: Connect to IMAP server" */, true);
#endif
    }

    s = esp_mail_str_261;  /* "> C: " */
    s += esp_mail_str_211; /* "Host > " */
    s += session_config->server.host_name;
    esp_mail_debug_print(s.c_str(), true);
    s = esp_mail_str_261;  /* "> C: " */
    s += esp_mail_str_201; /* "Port > " */
    s += session_config->server.port;
    esp_mail_debug_print(s.c_str(), true);
  }

#if defined(ESP32) && defined(ESP32_TCP_CLIENT)
  if (debug && !isCb)
    client->setDebugCallback(esp_mail_debug_print);
#endif

  client->begin(session_config->server.host_name.c_str(), session_config->server.port);

  client->ethDNSWorkAround();

  if (!client->connect(secureMode, session_config->certificate.verify))
  {
    if (isSMTP)
    {
#if defined(ENABLE_SMTP)
      return handleSMTPError((SMTPSession *)sessionPtr, SMTP_STATUS_SERVER_CONNECT_FAILED);
#endif
    }
    else
    {
#if defined(ENABLE_IMAP)
      return handleIMAPError((IMAPSession *)sessionPtr, IMAP_STATUS_SERVER_CONNECT_FAILED, false);
#endif
    }
  }
  return true;
}

void ESP_Mail_Client::prepareTime(Session_Config *session_config, void *cb, void *caller, bool isSMTP, bool debug)
{

#if defined(MB_ARDUINO_ESP) || defined(MB_ARDUINO_PICO) || defined(ARDUINO_ARCH_SAMD) || defined(__AVR_ATmega4809__) || defined(MB_ARDUINO_NANO_RP2040_CONNECT)
  bool validTime = false;
#endif

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)
  if (isSMTP)
    validTime = true;
  else
    validTime = session_config->certificate.cert_file.length() > 0 || session_config->cert_ptr != 0;
#endif

#if defined(MB_ARDUINO_ESP) || defined(MB_ARDUINO_PICO) || defined(ARDUINO_ARCH_SAMD) || defined(__AVR_ATmega4809__) || defined(MB_ARDUINO_NANO_RP2040_CONNECT)

  bool isCb = isResponseCB(cb, isSMTP);

  if (!isCb && (session_config->time.ntp_server.length() > 0 || validTime))
  {
    MB_String s = esp_mail_str_355; /* "> C: Wait for NTP server time synching" */
    if (debug && !isCb)
      esp_mail_debug_print(s.c_str(), true);

    setTime(session_config->time.gmt_offset, session_config->time.day_light_offset, session_config->time.ntp_server.c_str(), session_config->time.timezone_env_string.c_str(), session_config->time.timezone_file.c_str(), true);

    if (!Time.clockReady())
    {
      if (isSMTP)
      {
#if defined(ENABLE_SMTP)

        errorStatusCB((SMTPSession *)caller, MAIL_CLIENT_ERROR_NTP_TIME_SYNC_TIMED_OUT);
#endif
      }
      else
      {
#if defined(ENABLE_IMAP)
        errorStatusCB((IMAPSession *)caller, MAIL_CLIENT_ERROR_NTP_TIME_SYNC_TIMED_OUT);
#endif
      }
    }
  }

#endif
}

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)

void ESP_Mail_Client::setCert(Session_Config *session_config, const char *ca)
{
  int ptr = reinterpret_cast<int>(ca);
  if (ptr != session_config->cert_ptr)
  {
    session_config->cert_updated = true;
    session_config->cert_ptr = ptr;
  }
}

void ESP_Mail_Client::setSecure(ESP_MAIL_TCP_CLIENT &client, Session_Config *session_config)
{

  client.setMBFS(mbfs);

  client.setSession(session_config);

  if (client.getCertType() == esp_mail_cert_type_undefined || session_config->cert_updated)
  {
    if (session_config->certificate.cert_file.length() > 0 || session_config->certificate.cert_data != NULL || session_config->cert_ptr > 0)
    {
      client.setClockReady(_clockReady);
    }

    if (session_config->certificate.cert_file.length() == 0)
    {
      if (session_config->cert_ptr > 0)
        client.setCACert(reinterpret_cast<const char *>(session_config->cert_ptr));
      else if (session_config->certificate.cert_data != NULL)
        client.setCACert(session_config->certificate.cert_data);
      else
        client.setCACert(NULL);
    }
    else
    {
      if (!client.setCertFile(session_config->certificate.cert_file.c_str(), mbfs_type session_config->certificate.cert_file_storage_type))
        client.setCACert(NULL);
    }
    session_config->cert_updated = false;
  }
}

#endif

const char *ESP_Mail_Client::errorReason(bool isSMTP, int statusCode, int respCode, const char *msg)
{
  MB_String ret;

  if (!isSMTP && strlen(msg) > 0)
    return msg;

  switch (statusCode)
  {
#if defined(ENABLE_SMTP)
  case SMTP_STATUS_SERVER_CONNECT_FAILED:
    ret += esp_mail_str_38; /* "unable to connect to server" */
    break;
  case SMTP_STATUS_SMTP_GREETING_GET_RESPONSE_FAILED:
    ret += esp_mail_str_39; /* "SMTP server greeting failed" */
    break;
  case SMTP_STATUS_SMTP_GREETING_SEND_ACK_FAILED:
    ret += esp_mail_str_39; /* "SMTP server greeting failed" */
    break;
  case SMTP_STATUS_AUTHEN_NOT_SUPPORT:
    ret += esp_mail_str_42; /* "the provided SASL authentication mechanism is not support" */
    break;
  case SMTP_STATUS_AUTHEN_FAILED:
    ret += esp_mail_str_43; /* "authentication failed" */
    break;
  case SMTP_STATUS_USER_LOGIN_FAILED:
    ret += esp_mail_str_43; /* "authentication failed" */
    break;
  case SMTP_STATUS_PASSWORD_LOGIN_FAILED:
    ret += esp_mail_str_47; /* "login password is not valid" */
    break;
  case SMTP_STATUS_SEND_HEADER_SENDER_FAILED:
    ret += esp_mail_str_48; /* "send header failed" */
    break;
  case SMTP_STATUS_SEND_HEADER_RECIPIENT_FAILED:
    ret += esp_mail_str_222; /* "set recipient failed" */
    break;
  case SMTP_STATUS_SEND_BODY_FAILED:
    ret += esp_mail_str_49; /* "send body failed" */
    break;
  case SMTP_STATUS_SERVER_OAUTH2_LOGIN_DISABLED:
    ret += esp_mail_str_293; /* "OAuth2.0 log in was disabled for this server" */
    break;
  case SMTP_STATUS_NO_VALID_RECIPIENTS_EXISTED:
    ret += esp_mail_str_206; /* "some of the recipient Email address is not valid" */
    break;
  case SMTP_STATUS_NO_VALID_SENDER_EXISTED:
    ret += esp_mail_str_205; /* "sender Email address is not valid" */
    break;
  case SMTP_STATUS_NO_SUPPORTED_AUTH:
    ret += esp_mail_str_42; /* "the provided SASL authentication mechanism is not support" */
    break;
#endif

#if defined(ENABLE_IMAP)
  case IMAP_STATUS_SERVER_CONNECT_FAILED:
    ret += esp_mail_str_38; /* "unable to connect to server" */
    break;
  case IMAP_STATUS_NO_MESSAGE:
    ret += esp_mail_str_306; /* "some of the requested messages no longer exist" */
    break;
  case IMAP_STATUS_ERROR_DOWNLAD_TIMEOUT:
    ret += esp_mail_str_93; /* "connection timeout" */
    break;
  case IMAP_STATUS_CLOSE_MAILBOX_FAILED:
    ret += esp_mail_str_188; /* "fail to close the mailbox" */
    break;
  case IMAP_STATUS_OPEN_MAILBOX_FAILED:
    ret += esp_mail_str_281; /* "fail to open the mailbox" */
    break;
  case IMAP_STATUS_LIST_MAILBOXS_FAILED:
    ret += esp_mail_str_62; /* "fail to list the mailboxes" */
    break;
  case IMAP_STATUS_NO_SUPPORTED_AUTH:
    ret += esp_mail_str_42; /* "the provided SASL authentication mechanism is not support" */
    break;
  case IMAP_STATUS_CHECK_CAPABILITIES_FAILED:
    ret += esp_mail_str_63; /* "fail to check the capabilities" */
    break;
  case IMAP_STATUS_NO_MAILBOX_FOLDER_OPENED:
    ret += esp_mail_str_153; /* "No mailbox opened" */
    break;
#endif

  case MAIL_CLIENT_ERROR_CONNECTION_CLOSED:
    ret += esp_mail_str_221; /* "connection closed" */
    break;
  case MAIL_CLIENT_ERROR_READ_TIMEOUT:
    ret += esp_mail_str_258; /* "session timed out" */
    break;

  case MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED:
    ret += esp_mail_str_305; /* "connection failed" */
    break;
  case MAIL_CLIENT_ERROR_SSL_TLS_STRUCTURE_SETUP:
    ret += esp_mail_str_132; /* "fail to set up the SSL/TLS structure" */
    break;

  case MAIL_CLIENT_ERROR_OUT_OF_MEMORY:
    ret += esp_mail_str_186; /* "out of memory" */
    break;
  case TCP_CLIENT_ERROR_SEND_DATA_FAILED:
    ret += esp_mail_str_344; /* "data sending failed" */
    break;
  case TCP_CLIENT_ERROR_CONNECTION_REFUSED:
    ret += esp_mail_str_345; /* "connection refused" */
    break;
  case TCP_CLIENT_ERROR_NOT_INITIALIZED:
    ret += esp_mail_str_346; /* "Client and/or necessary callback functions are not yet assigned" */
    break;
  case TCP_CLIENT_ERROR_NOT_CONNECTED:
    ret += esp_mail_str_357; /* "not connected" */
    break;
  case MAIL_CLIENT_ERROR_NTP_TIME_SYNC_TIMED_OUT:
    ret += esp_mail_str_356; /* "NTP server time synching timed out" */
    break;

  case MAIL_CLIENT_ERROR_CUSTOM_CLIENT_DISABLED:
    ret += esp_mail_str_352; /* "Custom Client is not yet enabled" */
    break;
  case MB_FS_ERROR_FILE_IO_ERROR:
    ret += esp_mail_str_282; /* "file I/O error" */
    break;

#if defined(MBFS_FLASH_FS) || defined(MBFS_SD_FS)

  case MB_FS_ERROR_FLASH_STORAGE_IS_NOT_READY:
    ret += esp_mail_str_348; /* "Flash Storage is not ready." */
    break;

  case MB_FS_ERROR_SD_STORAGE_IS_NOT_READY:
    ret += esp_mail_str_349; /* "SD Storage is not ready." */
    break;

  case MB_FS_ERROR_FILE_STILL_OPENED:
    ret += esp_mail_str_350; /* "File is still opened." */
    break;

  case MB_FS_ERROR_FILE_NOT_FOUND:
    ret += esp_mail_str_351; /* "File not found." */
    break;

#endif
  default:
    break;
  }

#if defined(ENABLE_SMTP)
  if (isSMTP && strlen(msg) > 0 && ret.length() == 0)
  {
    ret = esp_mail_str_312; /* "code: " */
    ret += respCode;
    ret += esp_mail_str_313; /* ", text: " */
    ret += msg;
    return ret.c_str();
  }
#endif
  return ret.c_str();
}

void ESP_Mail_Client::closeTCPSession(void *sessionPtr, bool isSMTP)
{
  if (!sessionPtr)
    return;

  if (isSMTP)
  {
#if defined(ENABLE_SMTP)
    if (((SMTPSession *)sessionPtr)->_tcpConnected)
    {
      ((SMTPSession *)sessionPtr)->client.stop();
      _lastReconnectMillis = millis();
    }

    ((SMTPSession *)sessionPtr)->_tcpConnected = false;
    ((SMTPSession *)sessionPtr)->_auth_capability.clear();
    ((SMTPSession *)sessionPtr)->_send_capability.clear();
    ((SMTPSession *)sessionPtr)->_authenticated = false;

#endif
  }
  else
  {
#if defined(ENABLE_IMAP)
    if (((IMAPSession *)sessionPtr)->_tcpConnected)
    {
      ((IMAPSession *)sessionPtr)->client.stop();
      _lastReconnectMillis = millis();
    }

    ((IMAPSession *)sessionPtr)->_tcpConnected = false;
    ((IMAPSession *)sessionPtr)->_auth_capability.clear();
    ((IMAPSession *)sessionPtr)->_read_capability.clear();
    ((IMAPSession *)sessionPtr)->_authenticated = false;

#endif
  }
}

bool ESP_Mail_Client::connected(void *sessionPtr, bool isSMTP)
{
  if (isSMTP)
  {
#if defined(ENABLE_SMTP)
    return ((SMTPSession *)sessionPtr)->client.connected();
#endif
  }
  else
  {
#if defined(ENABLE_IMAP)
    return ((IMAPSession *)sessionPtr)->client.connected();
#endif
  }

  return false;
}

void ESP_Mail_Client::delP(void *ptr)
{
  mbfs->delP(ptr);
}

size_t ESP_Mail_Client::getReservedLen(size_t len)
{
  return mbfs->getReservedLen(len);
}

void *ESP_Mail_Client::newP(size_t len)
{
  return mbfs->newP(len);
}

bool ESP_Mail_Client::strcmpP(const char *buf, int ofs, PGM_P begin_PGM, bool caseSensitive)
{
  if (ofs < 0)
  {
    int p = strposP(buf, begin_PGM, 0, caseSensitive);
    if (p == -1)
      return false;
    ofs = p;
  }

  char *tmp2 = (char *)newP(strlen_P(begin_PGM) + 1);
  memcpy(tmp2, &buf[ofs], strlen_P(begin_PGM));
  tmp2[strlen_P(begin_PGM)] = 0;
  MB_String s = begin_PGM;
  bool ret = (strcasecmp(s.c_str(), tmp2) == 0);
  delP(&tmp2);
  return ret;
}

int ESP_Mail_Client::strposP(const char *buf, PGM_P begin_PGM, int ofs, bool caseSensitive)
{
  MB_String s = begin_PGM;
  return strpos(buf, s.c_str(), ofs, caseSensitive);
}

char *ESP_Mail_Client::strP(PGM_P pgm)
{
  size_t len = strlen_P(pgm) + 1;
  char *buf = (char *)newP(len);
  strcpy_P(buf, pgm);
  buf[len - 1] = 0;
  return buf;
}

void ESP_Mail_Client::strReplaceP(MB_String &buf, PGM_P name, PGM_P value)
{
  char *n = strP(name);
  char *v = strP(value);

  buf.replaceAll(n, v);

  delP(&n);
  delP(&v);
}

bool ESP_Mail_Client::authFailed(char *buf, int bufLen, int &chunkIdx, int ofs)
{
  bool ret = false;
  if (chunkIdx == 0)
  {
    size_t olen;
    unsigned char *decoded = decodeBase64((const unsigned char *)(buf + ofs), bufLen - ofs, &olen);
    if (decoded)
    {
      ret = strposP((char *)decoded, esp_mail_str_294 /* "{\"status\":" */, 0) > -1;
      delP(&decoded);
    }
    chunkIdx++;
  }
  return ret;
}

MB_String ESP_Mail_Client::getXOAUTH2String(const MB_String &email, const MB_String &accessToken)
{
  MB_String raw = esp_mail_str_285; /* "user=" */
  raw += email;
  raw += esp_mail_str_286; /* "\1auth=Bearer " */
  raw += accessToken;
  raw += esp_mail_str_287; /* "\1\1" */
  return encodeBase64Str((const unsigned char *)raw.c_str(), raw.length());
}

unsigned char *ESP_Mail_Client::decodeBase64(const unsigned char *src, size_t len, size_t *out_len)
{
  unsigned char *out, *pos, block[4], tmp;
  size_t i, count, olen;
  int pad = 0;
  size_t extra_pad;

  unsigned char *dtable = (unsigned char *)newP(256);

  memset(dtable, 0x80, 256);

  for (i = 0; i < sizeof(b64_index_table) - 1; i++)
    dtable[b64_index_table[i]] = (unsigned char)i;
  dtable['='] = 0;

  count = 0;
  for (i = 0; i < len; i++)
  {
    if (dtable[src[i]] != 0x80)
      count++;
  }

  if (count == 0)
    goto exit;

  extra_pad = (4 - count % 4) % 4;

  olen = (count + extra_pad) / 4 * 3;

  pos = out = (unsigned char *)newP(olen);

  if (out == NULL)
    goto exit;

  count = 0;

  for (i = 0; i < len + extra_pad; i++)
  {
    unsigned char val;

    if (i >= len)
      val = '=';
    else
      val = src[i];
    tmp = dtable[val];
    if (tmp == 0x80)
      continue;

    if (val == '=')
      pad++;
    block[count] = tmp;
    count++;
    if (count == 4)
    {
      *pos++ = (block[0] << 2) | (block[1] >> 4);
      *pos++ = (block[1] << 4) | (block[2] >> 2);
      *pos++ = (block[2] << 6) | block[3];
      count = 0;
      if (pad)
      {
        if (pad == 1)
          pos--;
        else if (pad == 2)
          pos -= 2;
        else
        {
          free(out);
          goto exit;
        }
        break;
      }
    }
  }

  *out_len = pos - out;
  delP(&dtable);
  return out;
exit:
  delP(&dtable);
  return nullptr;
}

MB_String ESP_Mail_Client::encodeBase64Str(const unsigned char *src, size_t len)
{
  return encodeBase64Str((uint8_t *)src, len);
}

MB_String ESP_Mail_Client::encodeBase64Str(uint8_t *src, size_t len)
{
  MB_String outStr;
  unsigned char *out, *pos;
  const unsigned char *end, *in;
  size_t olen = 4 * ((len + 2) / 3);
  if (olen < len)
    return outStr;

  outStr.resize(olen);
  out = (unsigned char *)&outStr[0];

  end = src + len;
  in = src;
  pos = out;

  while (end - in >= 3)
  {
    *pos++ = b64_index_table[in[0] >> 2];
    *pos++ = b64_index_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
    *pos++ = b64_index_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
    *pos++ = b64_index_table[in[2] & 0x3f];
    in += 3;
  }

  if (end - in)
  {
    *pos++ = b64_index_table[in[0] >> 2];
    if (end - in == 1)
    {
      *pos++ = b64_index_table[(in[0] & 0x03) << 4];
      *pos++ = '=';
    }
    else
    {
      *pos++ = b64_index_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
      *pos++ = b64_index_table[(in[1] & 0x0f) << 2];
    }
    *pos++ = '=';
  }

  return outStr;
}

#endif

ESP_Mail_Client MailClient = ESP_Mail_Client();

#endif /* ESP_MAIL_CLIENT_CPP */
