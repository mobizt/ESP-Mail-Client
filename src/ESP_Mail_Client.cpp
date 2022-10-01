#ifndef ESP_MAIL_CLIENT_CPP
#define ESP_MAIL_CLIENT_CPP

/**
 * Mail Client Arduino Library for Espressif's ESP32 and ESP8266 and SAMD21 with u-blox NINA-W102 WiFi/Bluetooth module
 *
 * Created October 1, 2022
 *
 * This library allows Espressif's ESP32, ESP8266 and SAMD devices to send and read Email through the SMTP and IMAP servers.
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

#include "ESP_Mail_Client.h"
#include "ESP_Mail_Client_Version.h"

#include "ESP_Mail_IMAP.h"
#include "ESP_Mail_SMTP.h"

void ESP_Mail_Client::networkReconnect(bool reconnect)
{
#if defined(ESP32) || defined(ESP8266)
  WiFi.setAutoReconnect(reconnect);
#endif
  networkAutoReconnect = reconnect;
}

#if defined(MBFS_SD_FS) && defined(MBFS_CARD_TYPE_SD)

bool ESP_Mail_Client::sdBegin(int8_t ss, int8_t sck, int8_t miso, int8_t mosi, uint32_t frequency)
{
  return mbfs->sdBegin(ss, sck, miso, mosi, frequency);
}

#if defined(ESP8266)
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
#if defined(MB_MCU_ESP)
  return ESP.getFreeHeap();
#elif defined(MB_MCU_ATMEL_ARM) || defined(MB_MCU_RP2040) || defined(MB_MCU_TEENSY_ARM)
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char *>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
#else
  return 0;
#endif
}

#if defined(ENABLE_SMTP) || defined(ENABLE_IMAP)

void ESP_Mail_Client::setTimezone(const char *TZ_Var, const char *TZ_file)
{

  if (!TZ_Var)
    return;

#if defined(ESP32) || defined(ESP8266)
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

void ESP_Mail_Client::setTime(float gmt_offset, float day_light_offset, const char *ntp_server, const char *TZ_Var, const char *TZ_file, bool wait)
{

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_SAMD) || defined(__AVR_ATmega4809__) || defined(ARDUINO_NANO_RP2040_CONNECT)

  _clockReady = Time.clockReady();

  if (!_clockReady)
  {

    Time.setClock(gmt_offset, day_light_offset, ntp_server);

    if (wait)
    {
      unsigned long waitMs = millis();
      while (!Time.clockReady() && millis() - waitMs < 10000)
      {
        delay(0);
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
    imap->_prev_imap_cmd = esp_mail_imap_cmd_login;
  }

  if (imap->_prev_imap_cmd != esp_mail_imap_cmd_append)
  {
    cmd = esp_mail_str_27;
    cmd += esp_mail_str_131;
    cmd += esp_mail_str_360;
    cmd += esp_mail_str_131;
    cmd += imap->_currentFolder;
  }
  cmd += esp_mail_str_131;

  if (_flags.length() > 0)
  {
    cmd += esp_mail_str_198;
    cmd += _flags;
    cmd += esp_mail_str_192;
    cmd += esp_mail_str_131;
  }

  if (_dt.length() > 0)
  {
    cmd += esp_mail_str_136;
    cmd += _dt;
    cmd += esp_mail_str_136;
    cmd += esp_mail_str_131;
  }

  cmd += esp_mail_str_193;
  cmd += dataLen;
  cmd += esp_mail_str_194;

  if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
  {
    imap->_prev_imap_cmd = esp_mail_imap_cmd_login;
    return false;
  }

  imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_append;

  if (!handleIMAPResponse(imap, IMAP_STATUS_BAD_COMMAND, false))
  {
    imap->_prev_imap_cmd = esp_mail_imap_cmd_login;
    return false;
  }

  calDataLen = false;

  rfc822MSG = false;

  if (!sendContent(nullptr, msg, false, rfc822MSG))
  {
    imap->_prev_imap_cmd = esp_mail_imap_cmd_login;
    return false;
  }

  if (lastAppend)
  {
    if (imapSend(imap, esp_mail_str_34, false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    {
      imap->_prev_imap_cmd = esp_mail_imap_cmd_login;
      return false;
    }

    imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_append_last;

    if (!handleIMAPResponse(imap, IMAP_STATUS_BAD_COMMAND, false))
    {
      imap->_prev_imap_cmd = esp_mail_imap_cmd_login;
      return false;
    }

    imap->_prev_imap_cmd = esp_mail_imap_cmd_login;
  }

  if (!lastAppend)
    imap->_prev_imap_cmd = esp_mail_imap_cmd_append;
  else
    altSendCallback(nullptr, esp_mail_str_363, esp_mail_str_364, true, false);

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

void ESP_Mail_Client::strcat_c(char *str, char c)
{
  for (; *str; str++)
    ;
  *str++ = c;
  *str++ = 0;
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

char *ESP_Mail_Client::subStr(const char *buf, PGM_P beginH, PGM_P endH, int beginPos, int endPos, bool caseSensitive)
{
  char *tmp = nullptr;
  int p1 = strposP(buf, beginH, beginPos, caseSensitive);
  if (p1 != -1)
  {

    while (buf[p1 + strlen_P(beginH)] == ' ' || buf[p1 + strlen_P(beginH)] == '\r' || buf[p1 + strlen_P(beginH)] == '\n')
    {
      p1++;
      if (strlen(buf) <= p1 + strlen_P(beginH))
      {
        p1--;
        break;
      }
    }

    int p2 = -1;
    if (endPos == 0)
      p2 = strposP(buf, endH, p1 + strlen_P(beginH), caseSensitive);

    if (p2 == -1)
      p2 = strlen(buf);

    int len = p2 - p1 - strlen_P(beginH);
    tmp = (char *)newP(len + 1);
    memcpy(tmp, &buf[p1 + strlen_P(beginH)], len);
    return tmp;
  }

  return nullptr;
}

bool ESP_Mail_Client::getHeader(const char *buf, PGM_P beginH, MB_String &out, bool caseSensitive)
{
  if (strcmpP(buf, 0, beginH, caseSensitive))
  {
    char *tmp = subStr(buf, beginH, NULL, 0, -1, caseSensitive);
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
    ext = esp_mail_str_40;
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

    mbfs->feed();

    ret = client->read();
    if (ret > -1)
    {
      c = (char)ret;
      strcat_c(buf, c);
      idx++;
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

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)
void ESP_Mail_Client::setCACert(ESP_MAIL_TCP_CLIENT &client, ESP_Mail_Session *session, std::shared_ptr<const char> caCert)
{

  client.setMBFS(mbfs);

  client.setSession(session);

  if (client.getCertType() == esp_mail_cert_type_undefined)
  {

    if (strlen(session->certificate.cert_file) > 0 || caCert != nullptr)
    {
      client.setClockReady(_clockReady);
    }

    if (strlen(session->certificate.cert_file) == 0)
    {
      if (caCert != nullptr)
        client.setCACert(caCert.get());
      else
        client.setCACert(nullptr);
    }
    else
    {
      client.setCertFile(session->certificate.cert_file, mbfs_type session->certificate.cert_file_storage_type);
    }
  }
}

#endif

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

bool ESP_Mail_Client::strcmpP(const char *buf, int ofs, PGM_P beginH, bool caseSensitive)
{
  if (ofs < 0)
  {
    int p = strposP(buf, beginH, 0, caseSensitive);
    if (p == -1)
      return false;
    ofs = p;
  }

  char *tmp2 = (char *)newP(strlen_P(beginH) + 1);
  memcpy(tmp2, &buf[ofs], strlen_P(beginH));
  tmp2[strlen_P(beginH)] = 0;
  MB_String s = beginH;
  bool ret = (strcasecmp(s.c_str(), tmp2) == 0);
  delP(&tmp2);
  return ret;
}

int ESP_Mail_Client::strposP(const char *buf, PGM_P beginH, int ofs, bool caseSensitive)
{
  MB_String s = beginH;
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
      ret = strposP((char *)decoded, esp_mail_str_294, 0) > -1;
      delP(&decoded);
    }
    chunkIdx++;
  }
  return ret;
}

MB_String ESP_Mail_Client::getXOAUTH2String(const MB_String &email, const MB_String &accessToken)
{
  MB_String raw = esp_mail_str_285;
  raw += email;
  raw += esp_mail_str_286;
  raw += accessToken;
  raw += esp_mail_str_287;
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
