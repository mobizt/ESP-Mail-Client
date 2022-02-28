/**
 * Mail Client Arduino Library for Espressif's ESP32 and ESP8266 and SAMD21 with u-blox NINA-W102 WiFi/Bluetooth module
 *
 *   Version:   2.1.0
 *   Released:  February 28, 2022
 *
 *   Updates:
 * - Change files structure.
 * - Fixed Arduino IDE compile error.
 *
 *
 * This library allows Espressif's ESP32, ESP8266 and SAMD devices to send and read Email through the SMTP and IMAP servers.
 *
 * The MIT License (MIT)
 * Copyright (c) 2021 K. Suwatchai (Mobizt)
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

#ifndef ESP_Mail_Client_CPP
#define ESP_Mail_Client_CPP

#include "ESP_Mail_Client.h"

#if defined(MBFS_SD_FS) && defined(MBFS_CARD_TYPE_SD)

bool ESP_Mail_Client::sdBegin(int8_t ss, int8_t sck, int8_t miso, int8_t mosi)
{
  return mbfs->sdBegin(ss, sck, miso, mosi);
}

#if defined(ESP8266)
bool ESP_Mail_Client::sdBegin(SDFSConfig *sdFSConfig)
{
  return mbfs->sdFatBegin(sdFSConfig);
}
#endif

#if defined(ESP32)

bool ESP_Mail_Client::sdBegin(int8_t ss, SPIClass *spiConfig)
{
  return mbfs->sdSPIBegin(ss, spiConfig);
}
#endif

#if defined(MBFS_ESP32_SDFAT_ENABLED) || defined(MBFS_SDFAT_ENABLED)
bool ESP_Mail_Client::sdBegin(SdSpiConfig *sdFatSPIConfig, int8_t ss, int8_t sck, int8_t miso, int8_t mosi)
{
  return mbfs->sdFatBegin(sdFatSPIConfig, ss, sck, miso, mosi);
}
#endif

#endif

#if defined(ESP8266) && defined(MBFS_SD_FS) && defined(MBFS_CARD_TYPE_SD_MMC)

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

void ESP_Mail_Client::setTime(float gmt_offset, float day_light_offset, const char *ntp_server, bool wait)
{
#if defined(ESP_MAIL_DEFAULT_DEBUG_PORT)
  if (wait)
    ESP_MAIL_DEFAULT_DEBUG_PORT.println(F("Acquiring time from NTP server...\n"));
#endif
  Time.setClock(gmt_offset, day_light_offset, ntp_server);
  if (wait)
  {
    unsigned long waitMs = millis();
    while (!Time.clockReady() && millis() - waitMs < 5000)
    {
      delay(0);
    }
  }
  _clockReady = Time.clockReady();
}

bool ESP_Mail_Client::validEmail(const char *s)
{
  MB_String str(s);
  size_t at = str.find('@');
  size_t dot = str.find('.', at);
  return (at != MB_String::npos) && (dot != MB_String::npos);
}

void ESP_Mail_Client::debugInfoP(PGM_P info)
{
  MB_String s1 = info;
  esp_mail_debug(s1.c_str());
}

char *ESP_Mail_Client::getRandomUID()
{
  char *tmp = (char *)newP(36);
  sprintf(tmp, (const char *)MBSTRING_FLASH_MCR("%d"), (int)random(2000, 4000));
  return tmp;
}

/* Safe string splitter to avoid strsep bugs*/
void ESP_Mail_Client::splitTk(MB_String &str, MB_VECTOR<MB_String> &tk, const char *delim)
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

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)
void ESP_Mail_Client::setSecure(ESP_MAIL_TCP_CLIENT &client, ESP_Mail_Session *session, std::shared_ptr<const char> caCert)
{

  client.setMBFS(mbfs);

  client.setSession(session);

  MailClient.setTime(session->time.gmt_offset, session->time.day_light_offset, session->time.ntp_server.c_str(), false);

  if (client.getCertType() == esp_mail_cert_type_undefined)
  {

    if (!MailClient._clockReady && (strlen(session->certificate.cert_file) > 0 || caCert != nullptr))
    {
      client.clockReady = MailClient._clockReady;
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

char *ESP_Mail_Client::strReplace(char *orig, char *rep, char *with)
{
  char *result = nullptr;
  char *ins = nullptr;
  char *tmp = nullptr;
  int len_rep;
  int len_with;
  int len_front;
  int count;

  len_with = strlen(with);
  len_rep = strlen(rep);

  ins = orig;
  for (count = 0; (tmp = strstr(ins, rep)); ++count)
    ins = tmp + len_rep;

  tmp = result = (char *)newP(strlen(orig) + (len_with - len_rep) * count + 1);
  while (count--)
  {
    ins = strstr(orig, rep);
    len_front = ins - orig;
    tmp = strncpy(tmp, orig, len_front) + len_front;
    tmp = strcpy(tmp, with) + len_with;
    orig += len_front + len_rep;
  }
  strcpy(tmp, orig);
  return result;
}

char *ESP_Mail_Client::strReplaceP(char *buf, PGM_P name, PGM_P value)
{
  char *n = strP(name);
  char *v = strP(value);
  char *out = strReplace(buf, n, v);
  delP(&n);
  delP(&v);
  return out;
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

#if defined(BOARD_HAS_PSRAM) && defined(MB_STRING_USE_PSRAM)

  pos = out = (unsigned char *)ps_malloc(olen);

#else

#if defined(ESP8266_USE_EXTERNAL_HEAP)
  ESP.setExternalHeap();
#endif

  pos = out = (unsigned char *)malloc(olen);

#if defined(ESP8266_USE_EXTERNAL_HEAP)
  ESP.resetHeap();
#endif

#endif

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

#if defined(ENABLE_IMAP)

int ESP_Mail_Client::readLine(IMAPSession *imap, char *buf, int bufLen, bool crlf, int &count)
{
  int ret = -1;
  char c = 0;
  char _c = 0;
  int idx = 0;
  if (!imap->client.connected())
    return idx;
  while (imap->client.available() && idx < bufLen)
  {
    ret = imap->client.read();
    if (ret > -1)
    {
      count++;
      if (idx >= bufLen - 1)
        return idx;

      c = (char)ret;
      strcat_c(buf, c);
      idx++;
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
    }
    if (!imap->client.connected())
      return idx;
  }
  return idx;
}

int ESP_Mail_Client::decodeChar(const char *s)
{
  return 16 * hexval(*(s + 1)) + hexval(*(s + 2));
}

void ESP_Mail_Client::decodeQP(const char *buf, char *out)
{
  char *tmp = strP(esp_mail_str_295);
  while (*buf)
  {
    if (*buf != '=')
      strcat_c(out, *buf++);
    else if (*(buf + 1) == '\r' && *(buf + 2) == '\n')
      buf += 3;
    else if (*(buf + 1) == '\n')
      buf += 2;
    else if (!strchr(tmp, *(buf + 1)))
      strcat_c(out, *buf++);
    else if (!strchr(tmp, *(buf + 2)))
      strcat_c(out, *buf++);
    else
    {
      strcat_c(out, decodeChar(buf));
      buf += 3;
    }
  }
  delP(&tmp);
}

char *ESP_Mail_Client::decode7Bit(char *buf)
{
  char *out = strReplaceP(buf, imap_7bit_key1, imap_7bit_val1);
  char *tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key2, imap_7bit_val2);

  delP(&tmp);
  tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key3, imap_7bit_val3);

  delP(&tmp);
  tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key4, imap_7bit_val4);

  delP(&tmp);
  tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key5, imap_7bit_val5);

  delP(&tmp);
  tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key6, imap_7bit_val6);

  delP(&tmp);
  tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key7, imap_7bit_val7);

  delP(&tmp);
  tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key8, imap_7bit_val8);

  delP(&tmp);
  tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key9, imap_7bit_val9);

  delP(&tmp);
  tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key10, imap_7bit_val10);

  delP(&tmp);
  tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key11, imap_7bit_val11);

  delP(&tmp);
  tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key12, imap_7bit_val12);

  delP(&tmp);
  tmp = (char *)newP(strlen(out) + 10);
  strcpy(tmp, out);
  delP(&out);
  out = strReplaceP(tmp, imap_7bit_key13, imap_7bit_val13);
  delP(&tmp);
  return out;
}

void ESP_Mail_Client::decodeHeader(MB_String &headerField)
{

  size_t p1 = 0, p2 = 0;
  MB_String headerEnc;

  while (headerField[p1] == ' ' && p1 < headerField.length() - 1)
    p1++;

  if (headerField[p1] == '=' && headerField[p1 + 1] == '?')
  {
    p2 = headerField.find("?", p1 + 2);
    if (p2 != MB_String::npos)
      headerEnc = headerField.substr(p1 + 2, p2 - p1 - 2);
  }

  int bufSize = 512;
  char *buf = (char *)newP(bufSize);

  RFC2047Decoder.rfc2047Decode(buf, headerField.c_str(), bufSize);

  if (getEncodingFromCharset(headerEnc.c_str()) == esp_mail_char_decoding_scheme_iso8859_1)
  {
    int len = strlen(buf);
    int olen = (len + 1) * 2;
    unsigned char *out = (unsigned char *)newP(olen);
    decodeLatin1_UTF8(out, &olen, (unsigned char *)buf, &len);
    delP(&buf);
    buf = (char *)out;
  }
  else if (getEncodingFromCharset(headerEnc.c_str()) == esp_mail_char_decoding_scheme_tis620)
  {
    size_t len2 = strlen(buf);
    char *tmp = (char *)newP((len2 + 1) * 3);
    decodeTIS620_UTF8(tmp, buf, len2);
    delP(&buf);
    buf = tmp;
  }

  headerField = buf;
  delP(&buf);
}

esp_mail_char_decoding_scheme ESP_Mail_Client::getEncodingFromCharset(const char *enc)
{
  esp_mail_char_decoding_scheme scheme = esp_mail_char_decoding_scheme_default;

  if (strposP(enc, esp_mail_str_237, 0) > -1 || strposP(enc, esp_mail_str_231, 0) > -1 || strposP(enc, esp_mail_str_226, 0) > -1)
    scheme = esp_mail_char_decoding_scheme_tis620;
  else if (strposP(enc, esp_mail_str_227, 0) > -1)
    scheme = esp_mail_char_decoding_scheme_iso8859_1;

  return scheme;
}

void ESP_Mail_Client::decodeTIS620_UTF8(char *out, const char *in, size_t len)
{
  // output is the 3-byte value UTF-8
  int j = 0;
  for (size_t i = 0; i < len; i++)
  {
    if (in[i] < 0x80)
      out[j++] = in[i];
    else if ((in[i] >= 0xa0 && in[i] < 0xdb) || (in[i] > 0xde && in[i] < 0xfc))
    {
      int unicode = 0x0e00 + in[i] - 0xa0;
      out[j++] = 0xe0 | ((unicode >> 12) & 0xf);
      out[j++] = 0x80 | ((unicode >> 6) & 0x3f);
      out[j++] = 0x80 | (unicode & 0x3f);
    }
  }
}

int ESP_Mail_Client::decodeLatin1_UTF8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen)
{
  unsigned char *outstart = out;
  const unsigned char *base = in;
  const unsigned char *processed = in;
  unsigned char *outend = out + *outlen;
  const unsigned char *inend;
  unsigned int c;
  int bits;

  inend = in + (*inlen);
  while ((in < inend) && (out - outstart + 5 < *outlen))
  {
    c = *in++;

    /* assertion: c is a single UTF-4 value */
    if (out >= outend)
      break;
    if (c < 0x80)
    {
      *out++ = c;
      bits = -6;
    }
    else
    {
      *out++ = ((c >> 6) & 0x1F) | 0xC0;
      bits = 0;
    }

    for (; bits >= 0; bits -= 6)
    {
      if (out >= outend)
        break;
      *out++ = ((c >> bits) & 0x3F) | 0x80;
    }
    processed = (const unsigned char *)in;
  }
  *outlen = out - outstart;
  *inlen = processed - base;
  return (0);
}

bool ESP_Mail_Client::sendIMAPCommand(IMAPSession *imap, int msgIndex, int cmdCase)
{

  MB_String cmd;
  if (imap->_uidSearch || imap->_config->fetch.uid.length() > 0)
    cmd += esp_mail_str_142;
  else
    cmd += esp_mail_str_143;

  cmd += imap->_msgUID[msgIndex];

  cmd += esp_mail_str_147;
  if (!imap->_config->fetch.set_seen)
  {
    cmd += esp_mail_str_152;
    cmd += esp_mail_str_214;
  }
  cmd += esp_mail_str_218;

  switch (cmdCase)
  {
  case 1:

    cmd += esp_mail_str_269;
    break;

  case 2:

    if (cPart(imap)->partNumFetchStr.length() > 0)
      cmd += cPart(imap)->partNumFetchStr;
    else
      cmd += esp_mail_str_215;
    cmd += esp_mail_str_219;
    break;

  case 3:

    cmd += cPart(imap)->partNumFetchStr;
    cmd += esp_mail_str_219;
    break;

  default:
    break;
  }

  if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;
  return true;
}

bool ESP_Mail_Client::readMail(IMAPSession *imap, bool closeSession)
{

  imap->checkUID();
  imap->checkPath();

  if (!imap->_tcpConnected)
    imap->_mailboxOpened = false;

  MB_String buf;
  MB_String command;
  MB_String _uid;
  command += esp_mail_str_27;

  size_t readCount = 0;
  imap->_multipart_levels.clear();

  if (!reconnect(imap))
    return false;

  int cmem = MailClient.getFreeHeap();

  if (cmem < ESP_MAIL_MIN_MEM)
  {
    if (imap->_debug)
    {
      esp_mail_debug("");
      errorStatusCB(imap, MAIL_CLIENT_ERROR_OUT_OF_MEMORY);
    }
    goto out;
  }

  // new session
  if (!imap->_tcpConnected)
  {
    // authenticate new
    if (!imapAuth(imap))
    {
      closeTCPSession(imap);
      return false;
    }
  }
  else
  {
    // reuse session
    for (size_t i = 0; i < imap->_headers.size(); i++)
      imap->_headers[i].part_headers.clear();
    imap->_headers.clear();

    if (imap->_config->fetch.uid.length() > 0)
      imap->_headerOnly = false;
    else
      imap->_headerOnly = true;
  }

  imap->_rfc822_part_count = 0;
  imap->_mbif._availableItems = 0;
  imap->_msgUID.clear();
  imap->_uidSearch = false;
  imap->_mbif._searchCount = 0;

  if (imap->_currentFolder.length() == 0)
    return handleIMAPError(imap, IMAP_STATUS_NO_MAILBOX_FOLDER_OPENED, false);

  if (!imap->_mailboxOpened || (imap->_config->fetch.set_seen && !imap->_headerOnly && imap->_readOnlyMode))
  {
    if (!imap->openFolder(imap->_currentFolder.c_str(), imap->_readOnlyMode && !imap->_config->fetch.set_seen))
      return handleIMAPError(imap, IMAP_STATUS_OPEN_MAILBOX_FAILED, false);
  }

  if (imap->_headerOnly)
  {
    if (imap->_config->search.criteria.length() > 0)
    {

      if (imap->_readCallback)
      {
        imapCB(imap, "", false);
        imapCBP(imap, esp_mail_str_66, false);
      }

      if (imap->_debug)
        debugInfoP(esp_mail_str_232);

      if (strposP(imap->_config->search.criteria.c_str(), esp_mail_str_137, 0) != -1)
      {
        imap->_uidSearch = true;
        command += esp_mail_str_138;
      }

      command += esp_mail_str_139;

      for (size_t i = 0; i < imap->_config->search.criteria.length(); i++)
      {
        if (imap->_config->search.criteria[i] != ' ' && imap->_config->search.criteria[i] != '\r' && imap->_config->search.criteria[i] != '\n' && imap->_config->search.criteria[i] != '$')
          buf.append(1, imap->_config->search.criteria[i]);

        if (imap->_config->search.criteria[i] == ' ')
        {

          MB_String s1 = esp_mail_str_140;
          MB_String s2 = esp_mail_str_224;
          MB_String s3 = esp_mail_str_141;

          if ((imap->_uidSearch && strcmp(buf.c_str(), s1.c_str()) == 0) || (imap->_unseen && buf.find(s2.c_str()) != MB_String::npos))
            buf.clear();

          if (strcmp(buf.c_str(), s3.c_str()) != 0 && buf.length() > 0)
          {
            command += esp_mail_str_131;
            command += buf;
          }

          buf.clear();
        }
      }

      MB_String s1 = esp_mail_str_223;

      if (imap->_unseen && strpos(imap->_config->search.criteria.c_str(), s1.c_str(), 0) == -1)
        command += esp_mail_str_223;

      if (buf.length() > 0)
      {
        command += esp_mail_str_131;
        command += buf;
      }

      if (imapSend(imap, command.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

      command.clear();

      imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_search;

      if (!handleIMAPResponse(imap, IMAP_STATUS_BAD_COMMAND, closeSession))
        return false;

      if (imap->_readCallback)
      {
        MB_String s = esp_mail_str_34;
        s += esp_mail_str_68;
        s += imap->_config->limit.search;
        imapCB(imap, s.c_str(), false);

        if (imap->_msgUID.size() > 0)
        {

          s = esp_mail_str_69;
          s += imap->_mbif._searchCount;
          s += esp_mail_str_70;
          imapCB(imap, s.c_str(), false);

          s = esp_mail_str_71;
          s += imap->_msgUID.size();
          s += esp_mail_str_70;
          imapCB(imap, s.c_str(), false);
        }
        else
          imapCBP(imap, esp_mail_str_72, false);
      }
    }
    else
    {
      if (imap->_readCallback)
        imapCBP(imap, esp_mail_str_73, false);

      imap->_mbif._availableItems++;
      uint32_t uid = imap->getUID(imap->_mbif._msgCount);

      imap->_msgUID.push_back(uid);
      imap->_headerOnly = false;
      _uid = uid;
      imap->_config->fetch.uid = _uid;
    }
  }
  else
  {

    if (imap->_config->fetch.uid.length() > 0)
    {
      uint32_t uid = atoi(imap->_config->fetch.uid.c_str());
      imap->_mbif._availableItems++;
      imap->_msgUID.push_back(uid);
    }
  }

  for (size_t i = 0; i < imap->_msgUID.size(); i++)
  {
    imap->_cMsgIdx = i;
    imap->_totalRead++;

    if (MailClient.getFreeHeap() - (imap->_config->limit.msg_size * (i + 1)) < ESP_MAIL_MIN_MEM)
    {
      if (imap->_debug)
        errorStatusCB(imap, MAIL_CLIENT_ERROR_OUT_OF_MEMORY);
      goto out;
    }

    if (imap->_readCallback)
    {
      readCount++;

      MB_String s = esp_mail_str_74;
      s += imap->_totalRead;

      if (imap->_uidSearch || imap->_config->fetch.uid.length() > 0)
        s += esp_mail_str_75;
      else
        s += esp_mail_str_76;

      s += imap->_msgUID[i];
      imapCB(imap, "", false);
      imapCB(imap, s.c_str(), false);
    }

    if (imap->_debug)
      debugInfoP(esp_mail_str_233);

    MB_String cmd;
    if (imap->_uidSearch || imap->_config->fetch.uid.length() > 0)
      cmd += esp_mail_str_142;
    else
      cmd += esp_mail_str_143;

    if (imap->_debug)
      debugInfoP(esp_mail_str_77);

    cmd += imap->_msgUID[i];

    cmd += esp_mail_str_147;
    if (!imap->_config->fetch.set_seen)
    {
      cmd += esp_mail_str_152;
      cmd += esp_mail_str_214;
    }
    cmd += esp_mail_str_218;

    cmd += esp_mail_str_144;
    cmd += esp_mail_str_219;
    if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_fetch_body_header;

    int err = IMAP_STATUS_BAD_COMMAND;
    if (imap->_headerOnly)
      err = IMAP_STATUS_IMAP_RESPONSE_FAILED;

    if (!handleIMAPResponse(imap, err, closeSession))
      return false;

    cHeader(imap)->flags = imap->getFlags(cHeader(imap)->message_no);

    if (!imap->_headerOnly)
    {
      imap->_cPartIdx = 0;

      // multipart
      if (cHeader(imap)->multipart)
      {
        struct esp_mail_imap_multipart_level_t mlevel;
        mlevel.level = 1;
        mlevel.fetch_rfc822_header = false;
        mlevel.append_body_text = false;
        imap->_multipart_levels.push_back(mlevel);

        if (!fetchMultipartBodyHeader(imap, i))
          return false;
      }
      else
      {
        // singlepart
        if (imap->_debug)
        {
          MB_String s = esp_mail_str_81;
          s += '1';
          esp_mail_debug(s.c_str());
        }

        cHeader(imap)->partNumStr.clear();
        if (!sendIMAPCommand(imap, i, 1))
          return false;

        imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_fetch_body_mime;
        if (!handleIMAPResponse(imap, IMAP_STATUS_BAD_COMMAND, closeSession))
          return false;
      }

      if (imap->_config->download.text || imap->_config->download.html || imap->_config->download.attachment || imap->_config->download.inlineImg)
      {

        if (mbfs->checkStorageReady(mbfs_type imap->_config->storage.type))
        {
          if (imap->_config->storage.type == esp_mail_file_storage_type_sd)
          {
            if (imap->_config->storage.saved_path.length() == 0)
              imap->_config->storage.saved_path = MBSTRING_FLASH_MCR("/");
            else
              mbfs->createDirs(imap->_config->storage.saved_path);
          }
        }
        else
          sendStorageNotReadyError(imap, imap->_config->storage.type);
      }

      if (cHeader(imap)->part_headers.size() > 0)
      {

        if (cHeader(imap)->attachment_count > 0)
          cHeader(imap)->hasAttachment = true;

        cHeader(imap)->sd_alias_file_count = 0;

        imap->_sdFileList.clear();
        if (!mbfs->longNameSupported())
          imap->_sdFileList = MBSTRING_FLASH_MCR("[");

        if (cHeader(imap)->attachment_count > 0 && imap->_readCallback)
        {
          MB_String s = esp_mail_str_34;
          s += esp_mail_str_78;
          s += cHeader(imap)->attachment_count;
          s += esp_mail_str_79;
          imapCB(imap, s.c_str(), false);

          int cnt = 0;

          for (size_t j = 0; j < cHeader(imap)->part_headers.size(); j++)
          {
            imap->_cPartIdx = j;
            if (!cPart(imap)->rfc822_part && cPart(imap)->attach_type != esp_mail_att_type_none)
            {
              cnt++;
              s = cnt;
              s += MBSTRING_FLASH_MCR(". ");
              s += cPart(imap)->filename;
              imapCB(imap, s.c_str(), false);
            }
          }
        }

        MB_String s1, s2;
        int _idx1 = 0;
        for (size_t j = 0; j < cHeader(imap)->part_headers.size(); j++)
        {
          imap->_cPartIdx = j;
          if (cPart(imap)->rfc822_part)
          {
            s1 = cPart(imap)->partNumStr;
            _idx1 = cPart(imap)->rfc822_msg_Idx;
          }
          else if (s1.length() > 0)
          {
            if (multipartMember(s1, cPart(imap)->partNumStr))
            {
              cPart(imap)->message_sub_type = esp_mail_imap_message_sub_type_rfc822;
              cPart(imap)->rfc822_msg_Idx = _idx1;
            }
          }

          if (cPart(imap)->multipart_sub_type == esp_mail_imap_multipart_sub_type_parallel)
            s2 = cPart(imap)->partNumStr;
          else if (s2.length() > 0)
          {
            if (multipartMember(s2, cPart(imap)->partNumStr))
            {
              cPart(imap)->attach_type = esp_mail_att_type_attachment;
              if (cPart(imap)->filename.length() == 0)
              {
                if (cPart(imap)->name.length() > 0)
                  cPart(imap)->filename = cPart(imap)->name;
                else
                {
                  cPart(imap)->filename = getRandomUID();
                  cPart(imap)->filename += esp_mail_str_40;
                }
              }
            }
          }
        }

        int acnt = 0;
        int ccnt = 0;

        for (size_t j = 0; j < cHeader(imap)->part_headers.size(); j++)
        {
          imap->_cPartIdx = j;

          if (cPart(imap)->rfc822_part || cPart(imap)->multipart_sub_type != esp_mail_imap_multipart_sub_type_none)
            continue;

          bool rfc822_body_subtype = cPart(imap)->message_sub_type == esp_mail_imap_message_sub_type_rfc822;

          if (cPart(imap)->attach_type == esp_mail_att_type_none || cPart(imap)->msg_type == esp_mail_msg_type_html || cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched)
          {

            bool ret = ((imap->_config->enable.rfc822 || imap->_config->download.rfc822) && rfc822_body_subtype) || (!rfc822_body_subtype && ((imap->_config->enable.text && (cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched)) || (imap->_config->enable.html && cPart(imap)->msg_type == esp_mail_msg_type_html) || (cPart(imap)->msg_type == esp_mail_msg_type_html && imap->_config->download.html) || ((cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched) && imap->_config->download.text)));
            if (!ret)
              continue;

            if ((imap->_config->download.rfc822 && rfc822_body_subtype) || (!rfc822_body_subtype && ((cPart(imap)->msg_type == esp_mail_msg_type_html && imap->_config->download.html) || ((cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched) && imap->_config->download.text))))
            {

              if (ccnt == 0)
              {
                imapCB(imap, "", false);
                imapCBP(imap, esp_mail_str_57, false);
              }

              if (imap->_debug)
              {
                if (cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched)
                  debugInfoP(esp_mail_str_59);
                else if (cPart(imap)->msg_type == esp_mail_msg_type_html)
                  debugInfoP(esp_mail_str_60);
              }
            }
            else
            {
              if (ccnt == 0)
              {
                imapCB(imap, "", false);
                imapCBP(imap, esp_mail_str_307, false);
              }

              if (imap->_debug)
              {
                if (cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched)
                  debugInfoP(esp_mail_str_308);
                else if (cPart(imap)->msg_type == esp_mail_msg_type_html)
                  debugInfoP(esp_mail_str_309);
              }
            }

            ccnt++;

            if (!sendIMAPCommand(imap, i, 2))
              return false;

            imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_fetch_body_text;
            if (!handleIMAPResponse(imap, IMAP_STATUS_IMAP_RESPONSE_FAILED, closeSession))
              return false;
          }
          else if (cPart(imap)->attach_type != esp_mail_att_type_none && (mbfs->flashReady() || mbfs->sdReady()))
          {

            if ((imap->_config->download.attachment && cPart(imap)->attach_type == esp_mail_att_type_attachment) || (imap->_config->download.inlineImg && cPart(imap)->attach_type == esp_mail_att_type_inline))
            {
              if (acnt == 0)
              {
                imapCB(imap, "", false);
                imapCBP(imap, esp_mail_str_80, false);
              }

              if (imap->_debug)
              {
                MB_String s = esp_mail_str_55;
                s += acnt + 1;
                s += MBSTRING_FLASH_MCR(" of ");
                s += cHeader(imap)->attachment_count;
                esp_mail_debug(s.c_str());
              }

              acnt++;
              if (cPart(imap)->octetLen <= (int)imap->_config->limit.attachment_size)
              {

                if (mbfs->flashReady() || mbfs->sdReady())
                {

                  if ((int)j < (int)cHeader(imap)->part_headers.size() - 1)
                    if (cHeader(imap)->part_headers[j + 1].octetLen > (int)imap->_config->limit.attachment_size)
                      cHeader(imap)->downloaded_bytes += cHeader(imap)->part_headers[j + 1].octetLen;

                  if (!sendIMAPCommand(imap, i, 3))
                    return false;

                  imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_fetch_body_attachment;
                  if (!handleIMAPResponse(imap, IMAP_STATUS_IMAP_RESPONSE_FAILED, closeSession))
                    return false;
                  delay(0);
                }
              }
              else
              {
                if ((int)j == (int)cHeader(imap)->part_headers.size() - 1)
                  cHeader(imap)->downloaded_bytes += cPart(imap)->octetLen;
              }
            }
          }
        }
      }

      if (imap->_config->download.header && !imap->_headerSaved)
      {
        if (imap->_readCallback)
        {
          imapCB(imap, "", false);
          imapCBP(imap, esp_mail_str_124, false);
        }
        saveHeader(imap, false);
        saveHeader(imap, true);
      }

      // save file list file
      if (imap->_sdFileList.length() > 0)
      {
        if (mbfs->open(cMSG(imap), mbfs_type imap->_config->storage.type, mb_fs_open_mode_write) > -1)
        {
          mbfs->print(mbfs_type imap->_config->storage.type, imap->_sdFileList.c_str());
          mbfs->close(mbfs_type imap->_config->storage.type);
        }
      }

      imap->_cMsgIdx++;
    }

    if (imap->_debug)
    {
      MB_String s = esp_mail_str_261;
      s += esp_mail_str_84;
      s += MailClient.getFreeHeap();
      esp_mail_debug(s.c_str());
    }
  }

out:

  if (readCount < imap->_msgUID.size())
  {
    imap->_mbif._availableItems = readCount;
    imap->_msgUID.erase(imap->_msgUID.begin() + readCount, imap->_msgUID.end());
  }

  if (closeSession)
  {
    if (!imap->closeSession())
      return false;
  }
  else
  {
    if (imap->_readCallback)
    {
      imapCB(imap, "", false);
      imapCBP(imap, esp_mail_str_87, false);
    }

    if (imap->_debug)
      debugInfoP(esp_mail_str_88);
  }

  if (imap->_readCallback)
    imapCB(imap, "", true);

  return true;
}
bool ESP_Mail_Client::getMultipartFechCmd(IMAPSession *imap, int msgIdx, MB_String &partText)
{
  if (imap->_multipart_levels.size() == 0)
    return false;

  int cLevel = imap->_multipart_levels.size() - 1;

  cHeader(imap)->partNumStr.clear();

  if (imap->_uidSearch || imap->_config->fetch.uid.length() > 0)
    partText = esp_mail_str_142;
  else
    partText = esp_mail_str_143;

  partText += imap->_msgUID[msgIdx];

  partText += esp_mail_str_147;
  if (!imap->_config->fetch.set_seen)
  {
    partText += esp_mail_str_152;
    partText += esp_mail_str_214;
  }
  partText += esp_mail_str_218;

  for (size_t i = 0; i < imap->_multipart_levels.size(); i++)
  {
    if (i > 0)
    {
      partText += esp_mail_str_152;
      cHeader(imap)->partNumStr += esp_mail_str_152;
    }

    partText += imap->_multipart_levels[i].level;
    cHeader(imap)->partNumStr += imap->_multipart_levels[i].level;
  }

  if (imap->_multipart_levels[cLevel].fetch_rfc822_header)
  {
    partText += esp_mail_str_152;
    partText += esp_mail_str_144;
    partText += esp_mail_str_219;
    imap->_multipart_levels[cLevel].append_body_text = true;
  }
  else
    partText += esp_mail_str_148;

  imap->_multipart_levels[cLevel].fetch_rfc822_header = false;

  return true;
}

bool ESP_Mail_Client::multipartMember(const MB_String &part, const MB_String &check)
{
  if (part.length() > check.length())
    return false;

  for (size_t i = 0; i < part.length(); i++)
    if (part[i] != check[i])
      return false;

  return true;
}

bool ESP_Mail_Client::fetchMultipartBodyHeader(IMAPSession *imap, int msgIdx)
{
  bool ret = true;

  if (!connected(imap))
  {
    closeTCPSession(imap);
    return false;
  }
  int cLevel = 0;

  do
  {

    struct esp_mail_message_part_info_t *_cpart = &cHeader(imap)->part_headers[cHeader(imap)->message_data_count - 1];
    bool rfc822_body_subtype = _cpart->message_sub_type == esp_mail_imap_message_sub_type_rfc822;

    MB_String cmd;

    if (!getMultipartFechCmd(imap, msgIdx, cmd))
      return true;

    if (imap->_debug)
    {
      MB_String s;
      if (imap->_multipart_levels.size() > 1)
        s = esp_mail_str_86;
      else
        s = esp_mail_str_81;
      s += cHeader(imap)->partNumStr;
      esp_mail_debug(s.c_str());
    }

    if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    imap->_imap_cmd = esp_mail_imap_cmd_fetch_body_mime;
    ret = handleIMAPResponse(imap, IMAP_STATUS_IMAP_RESPONSE_FAILED, false);

    _cpart = &cHeader(imap)->part_headers[cHeader(imap)->message_data_count - 1];
    rfc822_body_subtype = _cpart->message_sub_type == esp_mail_imap_message_sub_type_rfc822;
    cLevel = imap->_multipart_levels.size() - 1;

    if (ret)
    {

      if (_cpart->multipart)
      {
        if (_cpart->multipart_sub_type == esp_mail_imap_multipart_sub_type_parallel || _cpart->multipart_sub_type == esp_mail_imap_multipart_sub_type_alternative || _cpart->multipart_sub_type == esp_mail_imap_multipart_sub_type_related || _cpart->multipart_sub_type == esp_mail_imap_multipart_sub_type_mixed)
        {
          struct esp_mail_imap_multipart_level_t mlevel;
          mlevel.level = 1;
          mlevel.fetch_rfc822_header = false;
          mlevel.append_body_text = false;
          imap->_multipart_levels.push_back(mlevel);
          fetchMultipartBodyHeader(imap, msgIdx);
        }
        else
          imap->_multipart_levels[cLevel].level++;
      }
      else
      {
        if (rfc822_body_subtype)
        {
          // to get additional rfc822 message header
          imap->_multipart_levels[cLevel].fetch_rfc822_header = true;
          fetchMultipartBodyHeader(imap, msgIdx);
        }
        else
        {
          if (imap->_multipart_levels[cLevel].append_body_text)
          {
            // single part rfc822 message body, append TEXT to the body fetch command
            _cpart->partNumFetchStr += esp_mail_str_152;
            _cpart->partNumFetchStr += esp_mail_str_215;
            imap->_multipart_levels[cLevel].append_body_text = false;
          }
          imap->_multipart_levels[cLevel].level++;
        }
      }
    }

  } while (ret);

  imap->_multipart_levels.pop_back();

  if (imap->_multipart_levels.size() > 0)
  {
    cLevel = imap->_multipart_levels.size() - 1;
    imap->_multipart_levels[cLevel].level++;
  }

  return true;
}

bool ESP_Mail_Client::connected(IMAPSession *imap)
{
  return imap->client.connected();
}

bool ESP_Mail_Client::imapAuth(IMAPSession *imap)
{

  bool ssl = false;
  MB_String buf;
#if defined(ESP32) && defined(ESP32_TCP_CLIENT)
  imap->client.setDebugCallback(NULL);
#elif defined(ESP8266)

#endif

  if (imap->_config != nullptr)
  {
    if (imap->_config->fetch.uid.length() > 0)
      imap->_headerOnly = false;
    else
      imap->_headerOnly = true;
  }

  imap->_totalRead = 0;
  imap->_secure = true;
  bool secureMode = true;

#if defined(ESP32) && defined(ESP32_TCP_CLIENT)
  if (imap->_debug)
    imap->client.setDebugCallback(esp_mail_debug);
#elif defined(ESP8266) && defined(ESP8266_TCP_CLIENT)
  imap->client.txBufDivider = 16; // minimum, tx buffer size for ssl data and request command data
  imap->client.rxBufDivider = 1;
  if (imap->_config != nullptr)
  {
    if (!imap->_headerOnly && !imap->_config->enable.html && !imap->_config->enable.text && !imap->_config->download.attachment && !imap->_config->download.inlineImg && !imap->_config->download.html && !imap->_config->download.text)
      imap->client.rxBufDivider = 16; // minimum rx buffer size for only message header
  }
#endif

  if (imap->_sesson_cfg->server.port == esp_mail_imap_port_143)
  {
    imap->_secure = false;
    secureMode = false;
  }
  else
    secureMode = !imap->_sesson_cfg->secure.startTLS;

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)
  setSecure(imap->client, imap->_sesson_cfg, imap->_caCert);
#endif

  if (imap->_readCallback)
    imapCBP(imap, esp_mail_str_50, false);

  if (imap->_debug)
  {
    MB_String s = esp_mail_str_314;
    s += ESP_MAIL_VERSION;
    s += imap->client.fwVersion();
    esp_mail_debug(s.c_str());

#if defined(BOARD_HAS_PSRAM) && defined(MB_STRING_USE_PSRAM)
    if (ESP.getPsramSize() == 0)
    {
      s = esp_mail_str_353;
      esp_mail_debug(s.c_str());
    }
#endif

    debugInfoP(esp_mail_str_225);
    s = esp_mail_str_261;
    s += esp_mail_str_211;
    s += imap->_sesson_cfg->server.host_name;
    esp_mail_debug(s.c_str());
    s = esp_mail_str_261;
    s += esp_mail_str_201;
    s += imap->_sesson_cfg->server.port;
    esp_mail_debug(s.c_str());
  }

  imap->client.begin(imap->_sesson_cfg->server.host_name.c_str(), imap->_sesson_cfg->server.port);

  imap->client.ethDNSWorkAround();

  if (!imap->client.connect(secureMode, imap->_sesson_cfg->certificate.verify))
    return handleIMAPError(imap, IMAP_STATUS_SERVER_CONNECT_FAILED, false);

  imap->_tcpConnected = true;

  imap->client.setTimeout(TCP_CLIENT_DEFAULT_TCP_TIMEOUT_SEC);

  if (imap->_readCallback)
    imapCBP(imap, esp_mail_str_54, false);

  if (imap->_debug)
    debugInfoP(esp_mail_str_228);

init:

  if (!imap->checkCapability())
    return false;

  // start TLS when needed or the server issue
  if ((imap->_auth_capability.start_tls || imap->_sesson_cfg->secure.startTLS) && !ssl)
  {
    MB_String s;
    if (imap->_readCallback)
    {
      s += esp_mail_str_34;
      s += esp_mail_str_209;
      esp_mail_debug(s.c_str());
    }

    if (imap->_debug)
    {
      s = esp_mail_str_196;
      esp_mail_debug(s.c_str());
    }

    imapSendP(imap, esp_mail_str_311, false);
    imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_starttls;
    if (!handleIMAPResponse(imap, IMAP_STATUS_BAD_COMMAND, false))
      return false;

    if (imap->_debug)
    {
      debugInfoP(esp_mail_str_310);
    }

    // connect in secure mode
    // do ssl handshake

    if (!imap->client.connectSSL(imap->_sesson_cfg->certificate.verify))
      return handleIMAPError(imap, MAIL_CLIENT_ERROR_SSL_TLS_STRUCTURE_SETUP, false);

    // set the secure mode
    imap->_sesson_cfg->secure.startTLS = false;
    ssl = true;
    imap->_secure = true;

    // check the capabilitiy again
    goto init;
  }

  imap->clearMessageData();
  imap->_mailboxOpened = false;

  bool creds = imap->_sesson_cfg->login.email.length() > 0 && imap->_sesson_cfg->login.password.length() > 0;
  bool xoauth_auth = imap->_sesson_cfg->login.accessToken.length() > 0 && imap->_auth_capability.xoauth2;
  bool login_auth = creds;
  bool plain_auth = imap->_auth_capability.plain && creds;

  bool supported_auth = xoauth_auth || login_auth || plain_auth;

  if (!supported_auth)
    return handleIMAPError(imap, IMAP_STATUS_NO_SUPPORTED_AUTH, false);

  // rfc4959
  if (supported_auth)
  {
    if (imap->_readCallback)
    {
      imapCB(imap, "", false);
      imapCBP(imap, esp_mail_str_56, false);
    }
  }

  if (xoauth_auth)
  {
    if (!imap->_auth_capability.xoauth2)
      return handleIMAPError(imap, IMAP_STATUS_SERVER_OAUTH2_LOGIN_DISABLED, false);

    if (imap->_debug)
      debugInfoP(esp_mail_str_291);

    MB_String cmd = esp_mail_str_292;
    cmd += getEncodedToken(imap);
    if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_auth;
    if (!handleIMAPResponse(imap, IMAP_STATUS_LOGIN_FAILED, false))
      return false;
  }
  else if (login_auth)
  {

    if (imap->_debug)
      debugInfoP(esp_mail_str_229);

    MB_String cmd = esp_mail_str_130;
    cmd += imap->_sesson_cfg->login.email;
    cmd += esp_mail_str_131;
    cmd += imap->_sesson_cfg->login.password;

    if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_login;
    if (!handleIMAPResponse(imap, IMAP_STATUS_LOGIN_FAILED, true))
      return false;
  }
  else if (plain_auth)
  {
    if (imap->_debug)
      debugInfoP(esp_mail_str_290);

    int len = imap->_sesson_cfg->login.email.length() + imap->_sesson_cfg->login.password.length() + 2;
    uint8_t *tmp = (uint8_t *)newP(len);
    memset(tmp, 0, len);
    int p = 1;
    memcpy(tmp + p, imap->_sesson_cfg->login.email.c_str(), imap->_sesson_cfg->login.email.length());
    p += imap->_sesson_cfg->login.email.length() + 1;
    memcpy(tmp + p, imap->_sesson_cfg->login.password.c_str(), imap->_sesson_cfg->login.password.length());
    p += imap->_sesson_cfg->login.password.length();

    MB_String s = esp_mail_str_41;
    s += encodeBase64Str(tmp, p);
    delP(&tmp);

    if (imapSend(imap, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_login;
    if (!handleIMAPResponse(imap, IMAP_STATUS_LOGIN_FAILED, true))
      return false;
  }

  return true;
}

MB_String ESP_Mail_Client::getEncodedToken(IMAPSession *imap)
{
  MB_String raw = esp_mail_str_285;
  raw += imap->_sesson_cfg->login.email;
  raw += esp_mail_str_286;
  raw += imap->_sesson_cfg->login.accessToken;
  raw += esp_mail_str_287;
  MB_String s = encodeBase64Str((const unsigned char *)raw.c_str(), raw.length());
  return s;
}

bool ESP_Mail_Client::imapLogout(IMAPSession *imap)
{
  if (imap->_readCallback)
  {
    imapCBP(imap, esp_mail_str_85, false);
  }

  if (imap->_debug)
    debugInfoP(esp_mail_str_234);

  if (imapSendP(imap, esp_mail_str_146, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  imap->_imap_cmd = esp_mail_imap_cmd_logout;
  if (!handleIMAPResponse(imap, IMAP_STATUS_BAD_COMMAND, false))
    return false;

  if (imap->_readCallback)
  {
    imapCB(imap, "", false);
    imapCBP(imap, esp_mail_str_187, false);
  }

  if (imap->_debug)
    debugInfoP(esp_mail_str_235);

  return true;
}

void ESP_Mail_Client::errorStatusCB(IMAPSession *imap, int error)
{
  imap->_imapStatus.statusCode = error;
  MB_String s;
  if (imap->_readCallback)
  {
    s += esp_mail_str_53;
    s += imap->errorReason().c_str();
    imapCB(imap, s.c_str(), false);
  }

  if (imap->_debug)
  {
    s = esp_mail_str_185;
    s += imap->errorReason().c_str();
    esp_mail_debug(s.c_str());
  }
}

size_t ESP_Mail_Client::imapSendP(IMAPSession *imap, PGM_P v, bool newline)
{
  if (!reconnect(imap))
  {
    closeTCPSession(imap);
    return 0;
  }

  if (!connected(imap))
  {
    errorStatusCB(imap, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
    return 0;
  }

  if (!imap->_tcpConnected)
  {
    errorStatusCB(imap, MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED);
    return 0;
  }

  int len = 0;

  MB_String s = v;

  if (newline)
  {
    if (imap->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug(s.c_str());
    len = imap->client.println(s.c_str());
  }
  else
  {
    if (imap->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug_line(s.c_str(), false);
    len = imap->client.print(s.c_str());
  }

  if (len != (int)s.length() && len != (int)s.length() + 2)
  {
    errorStatusCB(imap, len);
    len = 0;
  }

  return len;
}

size_t ESP_Mail_Client::imapSend(IMAPSession *imap, const char *data, bool newline)
{
  if (!reconnect(imap))
  {
    closeTCPSession(imap);
    return 0;
  }

  if (!connected(imap))
  {
    errorStatusCB(imap, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
    return 0;
  }

  if (!imap->_tcpConnected)
  {
    errorStatusCB(imap, MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED);
    return 0;
  }

  int len = 0;

  if (newline)
  {
    if (imap->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug(data);
    len = imap->client.println(data);
  }
  else
  {
    if (imap->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug_line(data, false);
    len = imap->client.print(data);
  }

  if (len != (int)strlen(data) && len != (int)strlen(data) + 2)
  {
    errorStatusCB(imap, len);
    len = 0;
  }
  return len;
}

size_t ESP_Mail_Client::imapSend(IMAPSession *imap, int data, bool newline)
{
  if (!reconnect(imap))
  {
    closeTCPSession(imap);
    return 0;
  }

  if (!connected(imap))
  {
    errorStatusCB(imap, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
    return 0;
  }

  if (!imap->_tcpConnected)
  {
    errorStatusCB(imap, MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED);
    return 0;
  }

  int len = 0;

  if (newline)
  {
    if (imap->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug(num2Str(data, 0));
    len = imap->client.println(data);
  }
  else
  {
    if (imap->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug_line(num2Str(data, 0), false);
    len = imap->client.print(data);
  }

  if (len != (int)strlen(num2Str(data, 0)) && len != (int)strlen(num2Str(data, 0)) + 2)
  {
    errorStatusCB(imap, len);
    len = 0;
  }

  return len;
}

bool ESP_Mail_Client::mSetFlag(IMAPSession *imap, int msgUID, MB_StringPtr flag, uint8_t action, bool closeSession)
{
  if (!reconnect(imap))
    return false;

  if (!imap->_tcpConnected)
  {
    imap->_mailboxOpened = false;
    return false;
  }

  if (imap->_currentFolder.length() == 0)
  {
    if (imap->_readCallback)
      debugInfoP(esp_mail_str_153);

    if (imap->_debug)
    {
      MB_String e = esp_mail_str_185;
      e += esp_mail_str_151;
      esp_mail_debug(e.c_str());
    }
  }
  else
  {
    if (imap->_readOnlyMode || !imap->_mailboxOpened)
    {
      if (!imap->selectFolder(imap->_currentFolder.c_str(), false))
        return false;
    }
  }

  if (imap->_readCallback)
  {
    imapCB(imap, "", false);
    if (action == 0)
      debugInfoP(esp_mail_str_157);
    else if (action == 1)
      debugInfoP(esp_mail_str_155);
    else
      debugInfoP(esp_mail_str_154);
  }

  if (imap->_debug)
  {
    if (action == 0)
      debugInfoP(esp_mail_str_253);
    else if (action == 1)
      debugInfoP(esp_mail_str_254);
    else
      debugInfoP(esp_mail_str_255);
  }

  MB_String cmd = esp_mail_str_249;
  cmd += msgUID;
  if (action == 0)
    cmd += esp_mail_str_250;
  else if (action == 1)
    cmd += esp_mail_str_251;
  else
    cmd += esp_mail_str_252;

  cmd += flag;
  cmd += esp_mail_str_192;

  if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  imap->_imap_cmd = esp_mail_imap_cmd_store;

  if (!handleIMAPResponse(imap, IMAP_STATUS_PARSE_FLAG_FAILED, false))
    return false;

  if (closeSession)
    imap->closeSession();

  return true;
}

void ESP_Mail_Client::imapCBP(IMAPSession *imap, PGM_P info, bool success)
{
  MB_String s = info;
  imapCB(imap, s.c_str(), success);
}

void ESP_Mail_Client::imapCB(IMAPSession *imap, const char *info, bool success)
{
  imap->_cbData._info = info;
  imap->_cbData._success = success;
  imap->_readCallback(imap->_cbData);
}

int ESP_Mail_Client::getMSGNUM(IMAPSession *imap, char *buf, int bufLen, int &chunkIdx, bool &endSearch, int &nump, const char *key, const char *pc)
{
  int ret = -1;
  char c = 0;
  int idx = 0;
  int num = 0;
  while (imap->client.available() > 0 && idx < bufLen)
  {
    delay(0);

    ret = imap->client.read();

    if (ret > -1)
    {

      if (idx >= bufLen - 1)
        return idx;

      c = (char)ret;

      if (c == '\n')
        c = ' ';

      strcat_c(buf, c);
      idx++;

      if (chunkIdx == 0)
      {
        if (strcmp(buf, key) == 0)
        {
          chunkIdx++;
          return 0;
        }

        if (strposP(buf, esp_mail_imap_response_1, 0) > -1)
          goto end_search;
      }
      else
      {
        if (c == ' ')
        {
          imap->_mbif._searchCount++;
          if (imap->_config->enable.recent_sort)
          {
            uint32_t uid = atoi(buf);
            imap->_msgUID.push_back(uid);
            if (imap->_msgUID.size() > imap->_config->limit.search)
              imap->_msgUID.erase(imap->_msgUID.begin());
          }
          else
          {
            if (imap->_msgUID.size() < imap->_config->limit.search)
            {
              uint32_t uid = atoi(buf);
              imap->_msgUID.push_back(uid);
            }
          }

          if (imap->_debug)
          {
            num = (float)(100.0f * imap->_mbif._searchCount / imap->_mbif._msgCount);
            if (nump != num)
            {
              nump = num;
              searchReport(num, pc);
            }
          }

          chunkIdx++;
          return idx;
        }
        else if (c == '$')
        {
#if defined(MB_USE_STD_VECTOR)
          if (imap->_config->enable.recent_sort)
            std::sort(imap->_msgUID.begin(), imap->_msgUID.end(), compFunc);
#endif
          goto end_search;
        }
      }
    }
  }

  return idx;

end_search:

  endSearch = true;
  int read = imap->client.available();
  idx = imap->client.readBytes(buf + idx, read);

  return idx;
}

struct esp_mail_message_part_info_t *ESP_Mail_Client::cPart(IMAPSession *imap)
{
  return &cHeader(imap)->part_headers[imap->_cPartIdx];
}

struct esp_mail_message_header_t *ESP_Mail_Client::cHeader(IMAPSession *imap)
{
  return &imap->_headers[cIdx(imap)];
}

void ESP_Mail_Client::handleHeader(IMAPSession *imap, char *buf, int bufLen, int &chunkIdx, struct esp_mail_message_header_t &header, int &headerState, int &octetCount, bool caseSensitive)
{
  char *tmp = nullptr;
  if (chunkIdx == 0)
  {
    if (strposP(buf, esp_mail_str_324, 0, caseSensitive) != -1 && buf[0] == '*')
      chunkIdx++;

    tmp = subStr(buf, esp_mail_str_193, esp_mail_str_194, 0);
    if (tmp)
    {
      octetCount = 2;
      header.header_data_len = atoi(tmp);
      delP(&tmp);
    }

    tmp = subStr(buf, esp_mail_str_51, esp_mail_imap_response_7, 0);
    if (tmp)
    {
      header.message_no = atoi(tmp);
      delP(&tmp);
    }
  }
  else
  {
    if (octetCount > header.header_data_len + 2)
      return;

    if (strcmpP(buf, 0, esp_mail_str_10, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_from;
      tmp = subStr(buf, esp_mail_str_10, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_150, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_sender;
      tmp = subStr(buf, esp_mail_str_150, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_11, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_to;
      tmp = subStr(buf, esp_mail_str_11, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_12, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_cc;
      tmp = subStr(buf, esp_mail_str_12, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_24, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_subject;
      tmp = subStr(buf, esp_mail_str_24, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_46, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_return_path;
      tmp = subStr(buf, esp_mail_str_46, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_184, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_reply_to;
      tmp = subStr(buf, esp_mail_str_184, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_109, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_in_reply_to;
      tmp = subStr(buf, esp_mail_str_109, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_107, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_references;
      tmp = subStr(buf, esp_mail_str_107, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_134, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_comments;
      tmp = subStr(buf, esp_mail_str_134, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_145, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_keywords;
      tmp = subStr(buf, esp_mail_str_145, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_25, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_content_type;
      tmp = subStr(buf, esp_mail_str_25, esp_mail_str_97, 0, caseSensitive);
      if (tmp)
      {
        if (strpos(tmp, esp_mail_imap_multipart_sub_type_t::mixed, 0, caseSensitive) != -1)
          header.hasAttachment = true;

        setHeader(imap, buf, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_172, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_content_transfer_encoding;
      tmp = subStr(buf, esp_mail_str_172, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_190, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_accept_language;
      tmp = subStr(buf, esp_mail_str_190, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_191, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_content_language;
      tmp = subStr(buf, esp_mail_str_191, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_99, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_date;
      tmp = subStr(buf, esp_mail_str_99, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_101, caseSensitive))
    {
      headerState = esp_mail_imap_header_state::esp_mail_imap_state_msg_id;
      tmp = subStr(buf, esp_mail_str_101, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);
      }
    }
    chunkIdx++;
  }
}

void ESP_Mail_Client::setHeader(IMAPSession *imap, char *buf, struct esp_mail_message_header_t &header, int state)
{
  size_t i = 0;
  while (buf[i] == ' ')
  {
    i++;
    if (strlen(buf) <= i)
      return;
  }

  switch (state)
  {
  case esp_mail_imap_header_state::esp_mail_imap_state_from:
    header.header_fields.from += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_sender:
    header.header_fields.sender += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_to:
    header.header_fields.to += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_cc:
    header.header_fields.cc += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_subject:
    header.header_fields.subject += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_content_type:
    header.content_type += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_content_transfer_encoding:
    header.content_transfer_encoding += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_accept_language:
    header.accept_language += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_content_language:
    header.content_language += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_date:
    header.header_fields.date += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_msg_id:
    header.header_fields.messageID += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_return_path:
    header.header_fields.return_path += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_reply_to:
    header.header_fields.reply_to += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_in_reply_to:
    header.header_fields.in_reply_to += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_references:
    header.header_fields.references += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_comments:
    header.header_fields.comments += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_keywords:
    header.header_fields.keywords += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_char_set:
    header.char_set += &buf[i];
    break;
  case esp_mail_imap_header_state::esp_mail_imap_state_boundary:
    header.boundary += &buf[i];
    break;
  default:
    break;
  }
}

void ESP_Mail_Client::handlePartHeader(IMAPSession *imap, char *buf, int &chunkIdx, struct esp_mail_message_part_info_t &part, bool caseSensitive)
{
  char *tmp = nullptr;
  if (chunkIdx == 0)
  {
    tmp = subStr(buf, esp_mail_imap_response_7, NULL, 0, -1);
    if (tmp)
    {
      delP(&tmp);
      tmp = subStr(buf, esp_mail_str_193, esp_mail_str_194, 0);
      if (tmp)
      {
        chunkIdx++;
        part.octetLen = atoi(tmp);
        delP(&tmp);
      }
    }
  }
  else
  {
    if (strcmpP(buf, 0, esp_mail_str_25, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_25, esp_mail_str_97, 0, 0, caseSensitive);
      bool con_type = false;
      if (tmp)
      {
        con_type = true;
        part.content_type = tmp;
        delP(&tmp);
        int p1 = strposP(part.content_type.c_str(), esp_mail_imap_composite_media_type_t::multipart, 0, caseSensitive);
        if (p1 != -1)
        {
          p1 += strlen(esp_mail_imap_composite_media_type_t::multipart) + 1;
          part.multipart = true;
          // inline or embedded images
          if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::related, p1, caseSensitive) != -1)
            part.multipart_sub_type = esp_mail_imap_multipart_sub_type_related;
          // multiple text formats e.g. plain, html, enriched
          else if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::alternative, p1, caseSensitive) != -1)
            part.multipart_sub_type = esp_mail_imap_multipart_sub_type_alternative;
          // medias
          else if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::parallel, p1, caseSensitive) != -1)
            part.multipart_sub_type = esp_mail_imap_multipart_sub_type_parallel;
          // rfc822 encapsulated
          else if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::digest, p1, caseSensitive) != -1)
            part.multipart_sub_type = esp_mail_imap_multipart_sub_type_digest;
          else if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::report, p1, caseSensitive) != -1)
            part.multipart_sub_type = esp_mail_imap_multipart_sub_type_report;
          // others can be attachments
          else if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::mixed, p1, caseSensitive) != -1)
            part.multipart_sub_type = esp_mail_imap_multipart_sub_type_mixed;
        }

        p1 = strposP(part.content_type.c_str(), esp_mail_imap_composite_media_type_t::message, 0, caseSensitive);
        if (p1 != -1)
        {
          p1 += strlen(esp_mail_imap_composite_media_type_t::message) + 1;
          if (strpos(part.content_type.c_str(), esp_mail_imap_message_sub_type_t::rfc822, p1, caseSensitive) != -1)
            part.message_sub_type = esp_mail_imap_message_sub_type_rfc822;
          else if (strpos(part.content_type.c_str(), esp_mail_imap_message_sub_type_t::Partial, p1, caseSensitive) != -1)
            part.message_sub_type = esp_mail_imap_message_sub_type_partial;
          else if (strpos(part.content_type.c_str(), esp_mail_imap_message_sub_type_t::External_Body, p1, caseSensitive) != -1)
            part.message_sub_type = esp_mail_imap_message_sub_type_external_body;
          else if (strpos(part.content_type.c_str(), esp_mail_imap_message_sub_type_t::delivery_status, p1, caseSensitive) != -1)
            part.message_sub_type = esp_mail_imap_message_sub_type_delivery_status;
        }

        p1 = strpos(part.content_type.c_str(), esp_mail_imap_descrete_media_type_t::text, 0, caseSensitive);
        if (p1 != -1)
        {
          p1 += strlen(esp_mail_imap_descrete_media_type_t::text) + 1;
          if (strpos(part.content_type.c_str(), esp_mail_imap_media_text_sub_type_t::plain, p1, caseSensitive) != -1)
            part.msg_type = esp_mail_msg_type_plain;
          else if (strpos(part.content_type.c_str(), esp_mail_imap_media_text_sub_type_t::enriched, p1, caseSensitive) != -1)
            part.msg_type = esp_mail_msg_type_enriched;
          else if (strpos(part.content_type.c_str(), esp_mail_imap_media_text_sub_type_t::html, p1, caseSensitive) != -1)
            part.msg_type = esp_mail_msg_type_html;
          else
            part.msg_type = esp_mail_msg_type_plain;
        }
      }

      if (con_type)
      {
        if (part.msg_type == esp_mail_msg_type_plain || part.msg_type == esp_mail_msg_type_enriched)
        {
          tmp = subStr(buf, esp_mail_str_168, esp_mail_str_136, 0, 0, caseSensitive);
          if (tmp)
          {
            part.charset = tmp;
            delP(&tmp);
          }
          else
          {
            tmp = subStr(buf, esp_mail_str_169, NULL, 0, -1, caseSensitive);
            if (tmp)
            {
              part.charset = tmp;
              delP(&tmp);
            }
          }

          if (strposP(buf, esp_mail_str_275, 0, caseSensitive) > -1 || strposP(buf, esp_mail_str_270, 0, caseSensitive) > -1)
            part.plain_flowed = true;
          if (strposP(buf, esp_mail_str_259, 0, caseSensitive) > -1 || strposP(buf, esp_mail_str_257, 0, caseSensitive) > -1)
            part.plain_delsp = true;
        }

        if (part.charset.length() == 0)
        {
          tmp = subStr(buf, esp_mail_str_168, esp_mail_str_136, 0, 0, caseSensitive);
          if (tmp)
          {
            part.charset = tmp;
            delP(&tmp);
          }
          else
          {
            tmp = subStr(buf, esp_mail_str_169, NULL, 0, -1, caseSensitive);
            if (tmp)
            {
              part.charset = tmp;
              delP(&tmp);
            }
          }
        }

        tmp = subStr(buf, esp_mail_str_170, esp_mail_str_136, 0, 0, caseSensitive);
        if (tmp)
        {
          part.name = tmp;
          delP(&tmp);
        }
        else
        {
          tmp = subStr(buf, esp_mail_str_171, NULL, 0, -1, caseSensitive);
          if (tmp)
          {
            part.name = tmp;
            delP(&tmp);
          }
        }
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_172, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_172, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.content_transfer_encoding = tmp;

        if (strcmpP(tmp, 0, esp_mail_str_31))
          part.xencoding = esp_mail_msg_part_xencoding_base64;
        else if (strcmpP(tmp, 0, esp_mail_str_278))
          part.xencoding = esp_mail_msg_part_xencoding_qp;
        else if (strcmpP(tmp, 0, esp_mail_str_29))
          part.xencoding = esp_mail_msg_part_xencoding_7bit;

        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_174, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_174, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.descr = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_175, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_175, esp_mail_str_97, 0, 0, caseSensitive);
      if (tmp)
      {

        // don't count altenative part text and html as embedded contents
        if (cHeader(imap)->multipart_sub_type != esp_mail_imap_multipart_sub_type_alternative)
        {
          part.content_disposition = tmp;
          if (caseSensitive)
          {
            if (strcmp(tmp, esp_mail_imap_content_disposition_type_t::attachment) == 0)
              part.attach_type = esp_mail_att_type_attachment;
            else if (strcmp(tmp, esp_mail_imap_content_disposition_type_t::inline_) == 0)
              part.attach_type = esp_mail_att_type_inline;
          }
          else
          {
            if (strcasecmp(tmp, esp_mail_imap_content_disposition_type_t::attachment) == 0)
              part.attach_type = esp_mail_att_type_attachment;
            else if (strcasecmp(tmp, esp_mail_imap_content_disposition_type_t::inline_) == 0)
              part.attach_type = esp_mail_att_type_inline;
          }
        }
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_150, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_150, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.sender = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_10, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_10, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.from = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_11, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_11, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.to = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_12, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_12, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.cc = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_184, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_184, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.reply_to = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_134, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_134, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.comments = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_24, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_24, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.subject = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_101, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_101, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.messageID = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_46, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_46, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.return_path = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_99, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_99, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.date = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_145, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_145, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.keywords = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_109, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_109, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.in_reply_to = tmp;
        delP(&tmp);
      }
    }
    else if (strcmpP(buf, 0, esp_mail_str_107, caseSensitive))
    {
      tmp = subStr(buf, esp_mail_str_107, NULL, 0, -1, caseSensitive);
      if (tmp)
      {
        part.rfc822_header.references = tmp;
        delP(&tmp);
      }
    }

    if (part.content_disposition.length() > 0)
    {
      tmp = subStr(buf, esp_mail_str_176, esp_mail_str_136, 0, caseSensitive);
      if (tmp)
      {
        MB_String s = tmp;
        decodeHeader(s);
        part.filename = s;
        delP(&tmp);
      }
      else
      {
        tmp = subStr(buf, esp_mail_str_177, NULL, 0, -1, caseSensitive);
        if (tmp)
        {
          MB_String s = tmp;
          decodeHeader(s);
          part.filename = s;
          delP(&tmp);
        }
      }

      tmp = subStr(buf, esp_mail_str_178, esp_mail_str_97, 0, 0, caseSensitive);
      if (tmp)
      {
        part.attach_data_size = atoi(tmp);
        delP(&tmp);
        cHeader(imap)->total_attach_data_size += part.attach_data_size;
        part.sizeProp = true;
      }
      else
      {
        tmp = subStr(buf, esp_mail_str_178, NULL, 0, -1, caseSensitive);
        if (tmp)
        {
          part.attach_data_size = atoi(tmp);
          delP(&tmp);
          cHeader(imap)->total_attach_data_size += part.attach_data_size;
          part.sizeProp = true;
        }
      }

      tmp = subStr(buf, esp_mail_str_179, esp_mail_str_136, 0, 0, caseSensitive);
      if (tmp)
      {
        part.creation_date = tmp;
        delP(&tmp);
      }
      else
      {
        tmp = subStr(buf, esp_mail_str_180, NULL, 0, -1, caseSensitive);
        if (tmp)
        {
          part.creation_date = tmp;
          delP(&tmp);
        }
      }

      tmp = subStr(buf, esp_mail_str_181, esp_mail_str_136, 0, 0, caseSensitive);
      if (tmp)
      {
        part.modification_date = tmp;
        delP(&tmp);
      }
      else
      {
        tmp = subStr(buf, esp_mail_str_182, NULL, 0, -1, caseSensitive);
        if (tmp)
        {
          part.modification_date = tmp;
          delP(&tmp);
        }
      }
    }

    chunkIdx++;
  }
}

void ESP_Mail_Client::closeTCPSession(IMAPSession *imap)
{

  if (imap->_tcpConnected)
  {
    imap->client.stop();
    _lastReconnectMillis = millis();
  }
  imap->_tcpConnected = false;
}

bool ESP_Mail_Client::reconnect(IMAPSession *imap, unsigned long dataTime, bool downloadRequest)
{

  imap->client.setSession(imap->_sesson_cfg);

  bool status = imap->client.networkReady();

  if (dataTime > 0)
  {
    if (millis() - dataTime > (unsigned long)imap->client.tcpTimeout())
    {

      closeTCPSession(imap);

      if (imap->_headers.size() > 0)
      {
        if (downloadRequest)
        {
          errorStatusCB(imap, IMAP_STATUS_ERROR_DOWNLAD_TIMEOUT);
          if (cHeader(imap)->part_headers.size() > 0)
            cPart(imap)->download_error = imap->errorReason().c_str();
        }
        else
        {
          errorStatusCB(imap, MAIL_CLIENT_ERROR_READ_TIMEOUT);
          cHeader(imap)->error_msg = imap->errorReason().c_str();
        }
      }
      return false;
    }
  }

  if (!status)
  {

    if (imap->_tcpConnected)
      closeTCPSession(imap);

    if (imap->_mbif._idleTimeMs > 0 || imap->_imap_cmd == esp_mail_imap_command::esp_mail_imap_cmd_idle || imap->_imap_cmd == esp_mail_imap_command::esp_mail_imap_cmd_done)
    {
      // defer the polling error report
      if (millis() - imap->_last_polling_error_ms > 10000 && !imap->_tcpConnected)
      {
        imap->_last_polling_error_ms = millis();
        errorStatusCB(imap, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
      }
    }
    else
      errorStatusCB(imap, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);

    if (imap->_headers.size() > 0)
    {
      if (downloadRequest)
        cPart(imap)->download_error = imap->errorReason().c_str();
      else
        cHeader(imap)->error_msg = imap->errorReason().c_str();
    }

    if (millis() - _lastReconnectMillis > _reconnectTimeout && !imap->_tcpConnected)
    {
      if (imap->_sesson_cfg->network_connection_handler)
      {
        imap->client.disconnect();
        imap->_sesson_cfg->network_connection_handler();
      }
      else
        imap->client.networkReconnect();

      _lastReconnectMillis = millis();
    }

    status = imap->client.networkReady();
  }

  return status;
}

bool ESP_Mail_Client::handleIMAPResponse(IMAPSession *imap, int errCode, bool closeSession)
{

  if (!reconnect(imap))
    return false;

  esp_mail_imap_response_status imapResp = esp_mail_imap_response_status::esp_mail_imap_resp_unknown;
  char *response = nullptr;
  int readLen = 0;
  long dataTime = millis();
  int chunkBufSize = imap->client.available();
  int chunkIdx = 0;
  MB_String s;
  bool completedResponse = false;
  bool endSearch = false;
  struct esp_mail_message_header_t header;
  struct esp_mail_message_part_info_t part;

  MB_String filePath;
  bool downloadRequest = false;
  int octetCount = 0;
  int octetLength = 0;
  bool tmo = false;
  int headerState = 0;
  int scnt = 0;
  char *lastBuf = nullptr;
  char *tmp = nullptr;
  bool crLF = imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_text && cPart(imap)->xencoding == esp_mail_msg_part_xencoding_base64;

  while (imap->_tcpConnected && chunkBufSize <= 0)
  {
    if (!reconnect(imap, dataTime))
      return false;
    if (!connected(imap))
    {
      errorStatusCB(imap, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
      return false;
    }
    chunkBufSize = imap->client.available();
    delay(0);
  }

  dataTime = millis();

  if (chunkBufSize > 1)
  {
    if (imap->_imap_cmd == esp_mail_imap_cmd_examine)
    {
      imap->_mbif.clear();
      imap->_mbif._msgCount = 0;
      imap->_mbif._polling_status.argument.clear();
      imap->_mbif._polling_status.messageNum = 0;
      imap->_mbif._polling_status.type = imap_polling_status_type_undefined;
      imap->_mbif._idleTimeMs = 0;
      imap->_nextUID.clear();
    }

    if (imap->_imap_cmd == esp_mail_imap_cmd_search)
    {
      imap->_mbif._searchCount = 0;
      imap->_msgUID.clear();
    }

    chunkBufSize = 512;
    response = (char *)newP(chunkBufSize + 1);

    if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_inline)
      lastBuf = (char *)newP(BASE64_CHUNKED_LEN + 1);

    while (!completedResponse)
    {
      delay(0);
      if (!reconnect(imap, dataTime) || !connected(imap))
      {

        if (!connected(imap))
        {
          errorStatusCB(imap, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
          return false;
        }
        return false;
      }
      chunkBufSize = imap->client.available();

      if (chunkBufSize > 0)
      {
        chunkBufSize = 512;

        if (imap->_imap_cmd == esp_mail_imap_command::esp_mail_imap_cmd_search)
        {
          MB_String s1 = esp_mail_imap_response_6;
          MB_String s2 = esp_mail_str_92;
          readLen = getMSGNUM(imap, response, chunkBufSize, chunkIdx, endSearch, scnt, s1.c_str(), s2.c_str());
          imap->_mbif._availableItems = imap->_msgUID.size();
        }
        else
        {
          readLen = readLine(imap, response, chunkBufSize, crLF, octetCount);
        }

        if (readLen)
        {

          if (imap->_debugLevel > esp_mail_debug_level_1)
          {
            if (imap->_imap_cmd != esp_mail_imap_cmd_search && imap->_imap_cmd != esp_mail_imap_cmd_fetch_body_text && imap->_imap_cmd != esp_mail_imap_cmd_fetch_body_attachment && imap->_imap_cmd != esp_mail_imap_cmd_fetch_body_inline)
              esp_mail_debug((const char *)response);
          }

          if (imap->_imap_cmd != esp_mail_imap_cmd_search || (imap->_imap_cmd == esp_mail_imap_cmd_search && endSearch))
            imapResp = imapResponseStatus(imap, response);

          if (imapResp != esp_mail_imap_response_status::esp_mail_imap_resp_unknown)
          {

            if (imap->_debugLevel > esp_mail_debug_level_1)
            {
              if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_text || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_inline)
                esp_mail_debug((const char *)response);
            }

            if (imap->_imap_cmd == esp_mail_imap_cmd_custom && imap->_customCmdResCallback)
              imap->_customCmdResCallback((const char *)response);

            if (imap->_imap_cmd == esp_mail_imap_cmd_close)
              completedResponse = true;
            else
            {
              // some IMAP servers advertise CAPABILITY in their responses
              // try to read the next available response
              memset(response, 0, chunkBufSize);

              readLen = readLine(imap, response, chunkBufSize, true, octetCount);
              if (readLen)
              {
                completedResponse = false;
                imapResp = imapResponseStatus(imap, response);
                if (imapResp > esp_mail_imap_response_status::esp_mail_imap_resp_unknown)
                  completedResponse = true;
              }
              else
                completedResponse = true;
            }
          }
          else
          {
            if (imap->_imap_cmd == esp_mail_imap_cmd_auth)
            {
              if (authFailed(response, readLen, chunkIdx, 2))
                completedResponse = true;
            }
            else if (imap->_imap_cmd == esp_mail_imap_cmd_capability)
              handleCapability(imap, response, chunkIdx);
            else if (imap->_imap_cmd == esp_mail_imap_cmd_list)
              handleFolders(imap, response);
            else if (imap->_imap_cmd == esp_mail_imap_cmd_select || imap->_imap_cmd == esp_mail_imap_cmd_examine)
              handleExamine(imap, response);
            else if (imap->_imap_cmd == esp_mail_imap_cmd_get_uid)
              handleGetUID(imap, response);
            else if (imap->_imap_cmd == esp_mail_imap_cmd_get_flags)
              handleGetFlags(imap, response);
            else if (imap->_imap_cmd == esp_mail_imap_cmd_custom && imap->_customCmdResCallback)
              imap->_customCmdResCallback((const char *)response);
            else if (imap->_imap_cmd == esp_mail_imap_cmd_idle)
            {
              completedResponse = response[0] == '+';
              imapResp = esp_mail_imap_response_status::esp_mail_imap_resp_ok;

              imap->_last_host_check_ms = millis();
            }
            else if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_header)
            {
              if (headerState == 0)
                header.message_uid = cMSG(imap);

              int _st = headerState;
              handleHeader(imap, response, readLen, chunkIdx, header, headerState, octetCount, imap->_config->enable.header_case_sensitive);
              if (_st == headerState && headerState > 0 && octetCount <= header.header_data_len)
                setHeader(imap, response, header, headerState);
            }
            else if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_mime)
              handlePartHeader(imap, response, chunkIdx, part, imap->_config->enable.header_case_sensitive);
            else if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_text)
              decodeText(imap, response, readLen, chunkIdx, filePath, downloadRequest, octetLength, octetCount);
            else if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_inline)
            {

              if (cPart(imap)->xencoding == esp_mail_msg_part_xencoding_base64)
              {
                // multi-line chunked base64 string attachment handle
                if (octetCount < octetLength && readLen < BASE64_CHUNKED_LEN)
                {
                  if (strlen(lastBuf) > 0)
                  {
                    tmp = (char *)newP(readLen + strlen(lastBuf) + 2);
                    strcpy(tmp, lastBuf);
                    strcat(tmp, response);
                    readLen = strlen(tmp);
                    tmo = handleAttachment(imap, tmp, readLen, chunkIdx, filePath, downloadRequest, octetCount, octetLength);
                    delP(&tmp);
                    memset(lastBuf, 0, BASE64_CHUNKED_LEN + 1);
                    if (!tmo)
                      break;
                  }
                  else if (readLen < BASE64_CHUNKED_LEN + 1)
                    strcpy(lastBuf, response);
                }
                else
                {
                  tmo = handleAttachment(imap, response, readLen, chunkIdx, filePath, downloadRequest, octetCount, octetLength);
                  if (!tmo)
                    break;
                }
              }
              else
                tmo = handleAttachment(imap, response, readLen, chunkIdx, filePath, downloadRequest, octetCount, octetLength);
            }
            dataTime = millis();
          }
        }
        memset(response, 0, chunkBufSize);
      }
    }

    delP(&response);
    if (imap->_imap_cmd == esp_mail_imap_command::esp_mail_imap_cmd_search)
    {
      if (imap->_debug && scnt > 0 && scnt < 100)
      {
        MB_String s1 = esp_mail_str_92;
        searchReport(100, s1.c_str());
      }
    }
    if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment)
      delP(&lastBuf);
  }

  if ((imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_header && header.header_data_len == 0) || imapResp == esp_mail_imap_response_status::esp_mail_imap_resp_no)
  {
    if (imapResp == esp_mail_imap_response_status::esp_mail_imap_resp_no)
      imap->_imapStatus.statusCode = IMAP_STATUS_IMAP_RESPONSE_FAILED;
    else
      imap->_imapStatus.statusCode = IMAP_STATUS_NO_MESSAGE;

    if (imap->_readCallback && imap->_imap_cmd != esp_mail_imap_cmd_fetch_body_mime)
    {
      MB_String s = esp_mail_str_53;
      s += imap->errorReason();
      imapCB(imap, s.c_str(), false);
    }

    if (imap->_debug && imap->_imap_cmd != esp_mail_imap_cmd_fetch_body_mime)
    {
      MB_String s = esp_mail_str_185;
      s += imap->errorReason();
      esp_mail_debug_line(s.c_str(), true);
    }

    return false;
  }

  if (imapResp == esp_mail_imap_response_status::esp_mail_imap_resp_ok)
  {
    if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_header)
    {
      char *buf = (char *)newP(header.content_type.length() + 1);
      strcpy(buf, header.content_type.c_str());
      header.content_type.clear();

      tmp = subStr(buf, esp_mail_str_25, esp_mail_str_97, 0, 0, false);
      if (tmp)
      {
        headerState = esp_mail_imap_header_state::esp_mail_imap_state_content_type;
        setHeader(imap, tmp, header, headerState);
        delP(&tmp);

        int p1 = strposP(header.content_type.c_str(), esp_mail_imap_composite_media_type_t::multipart, 0);
        if (p1 != -1)
        {
          p1 += strlen(esp_mail_imap_composite_media_type_t::multipart) + 1;
          header.multipart = true;
          // inline or embedded images
          if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::related, p1) != -1)
            header.multipart_sub_type = esp_mail_imap_multipart_sub_type_related;
          // multiple text formats e.g. plain, html, enriched
          else if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::alternative, p1) != -1)
            header.multipart_sub_type = esp_mail_imap_multipart_sub_type_alternative;
          // medias
          else if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::parallel, p1) != -1)
            header.multipart_sub_type = esp_mail_imap_multipart_sub_type_parallel;
          // rfc822 encapsulated
          else if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::digest, p1) != -1)
            header.multipart_sub_type = esp_mail_imap_multipart_sub_type_digest;
          else if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::report, p1) != -1)
            header.multipart_sub_type = esp_mail_imap_multipart_sub_type_report;
          // others can be attachments
          else if (strpos(part.content_type.c_str(), esp_mail_imap_multipart_sub_type_t::mixed, p1) != -1)
            header.multipart_sub_type = esp_mail_imap_multipart_sub_type_mixed;
        }

        p1 = strposP(header.content_type.c_str(), esp_mail_imap_composite_media_type_t::message, 0);
        if (p1 != -1)
        {
          p1 += strlen(esp_mail_imap_composite_media_type_t::message) + 1;
          if (strpos(part.content_type.c_str(), esp_mail_imap_message_sub_type_t::rfc822, p1) != -1)
          {
            header.rfc822_part = true;
            header.message_sub_type = esp_mail_imap_message_sub_type_rfc822;
          }
          else if (strpos(part.content_type.c_str(), esp_mail_imap_message_sub_type_t::Partial, p1) != -1)
            header.message_sub_type = esp_mail_imap_message_sub_type_partial;
          else if (strpos(part.content_type.c_str(), esp_mail_imap_message_sub_type_t::External_Body, p1) != -1)
            header.message_sub_type = esp_mail_imap_message_sub_type_external_body;
          else if (strpos(part.content_type.c_str(), esp_mail_imap_message_sub_type_t::delivery_status, p1) != -1)
            header.message_sub_type = esp_mail_imap_message_sub_type_delivery_status;
        }

        tmp = subStr(buf, esp_mail_str_169, NULL, 0, -1, false);
        if (tmp)
        {
          headerState = esp_mail_imap_header_state::esp_mail_imap_state_char_set;
          setHeader(imap, tmp, header, headerState);
          delP(&tmp);
        }

        if (header.multipart)
        {
          if (strcmpP(buf, 0, esp_mail_str_277))
          {
            tmp = subStr(buf, esp_mail_str_277, esp_mail_str_136, 0, 0, false);
            if (tmp)
            {
              headerState = esp_mail_imap_header_state::esp_mail_imap_state_boundary;
              setHeader(imap, tmp, header, headerState);
              delP(&tmp);
            }
          }
        }
      }

      delP(&buf);

      decodeHeader(header.header_fields.messageID);
      decodeHeader(header.header_fields.from);
      decodeHeader(header.header_fields.sender);
      decodeHeader(header.header_fields.to);
      decodeHeader(header.header_fields.cc);
      decodeHeader(header.header_fields.bcc);
      decodeHeader(header.header_fields.subject);
      decodeHeader(header.header_fields.date);
      decodeHeader(header.header_fields.return_path);
      decodeHeader(header.header_fields.reply_to);
      decodeHeader(header.header_fields.in_reply_to);
      decodeHeader(header.header_fields.references);
      decodeHeader(header.header_fields.comments);
      decodeHeader(header.header_fields.keywords);
      imap->_headers.push_back(header);
    }

    if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_mime)
    {
      // expect the octet length in the response for the existent part
      if (part.octetLen > 0)
      {

        part.partNumStr = cHeader(imap)->partNumStr;
        part.partNumFetchStr = cHeader(imap)->partNumStr;
        if (cHeader(imap)->part_headers.size() > 0)
        {

          struct esp_mail_message_part_info_t *_part = &cHeader(imap)->part_headers[cHeader(imap)->part_headers.size() - 1];
          bool rfc822_body_subtype = _part->message_sub_type == esp_mail_imap_message_sub_type_rfc822;

          if (rfc822_body_subtype)
          {
            if (!_part->rfc822_part)
            {
              // additional rfc822 message header, store it to the rfc822 part header
              _part->rfc822_part = true;
              _part->rfc822_header = part.rfc822_header;
              imap->_rfc822_part_count++;
              _part->rfc822_msg_Idx = imap->_rfc822_part_count;
            }
          }
        }

        cHeader(imap)->part_headers.push_back(part);
        cHeader(imap)->message_data_count = cHeader(imap)->part_headers.size();

        if (part.msg_type == esp_mail_msg_type_plain || part.msg_type == esp_mail_msg_type_enriched || part.msg_type == esp_mail_msg_type_html || part.attach_type == esp_mail_att_type_none || (part.attach_type == esp_mail_att_type_attachment && imap->_config->download.attachment) || (part.attach_type == esp_mail_att_type_inline && imap->_config->download.inlineImg))
        {
          if (part.message_sub_type != esp_mail_imap_message_sub_type_rfc822)
          {
            if (part.attach_type != esp_mail_att_type_none && cHeader(imap)->multipart_sub_type != esp_mail_imap_multipart_sub_type_alternative)
              cHeader(imap)->attachment_count++;
          }
        }
      }
      else
      {
        // nonexistent part
        // return false to exit the loop without closing the connection
        if (closeSession)
          imap->closeSession();
        return false;
      }
    }

    if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_text || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_inline)
    {
      if (cPart(imap)->file_open_write)
        mbfs->close(mbfs_type imap->_config->storage.type);
    }

    if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_text)
      cPart(imap)->text[cPart(imap)->textLen] = 0;
  }
  else
  {
    // some server responses NO and should exit (false) from MIME feching loop without
    // closing the session
    if (imap->_imap_cmd != esp_mail_imap_cmd_fetch_body_mime)
      return handleIMAPError(imap, errCode, false);

    if (closeSession)
      imap->closeSession();
    return false;
  }

  return true;
}

void ESP_Mail_Client::addHeader(MB_String &s, const char *name, const MB_String &value, bool trim, bool json)
{
  if (json)
  {
    s += (s.length() > 0) ? MBSTRING_FLASH_MCR(",\"") : MBSTRING_FLASH_MCR("{\"");
    s += name;
    s += MBSTRING_FLASH_MCR("\":\"");
    if (trim)
    {
      MB_String t = value;
      t.replaceAll("\"", "");
      s += t;
    }
    else
      s += value;
    s += MBSTRING_FLASH_MCR("\"");
  }
  else
  {
    if (s.length() > 0)
      s += MBSTRING_FLASH_MCR("\r\n");
    s += name;
    s += MBSTRING_FLASH_MCR(": ");
    s += value;
  }
}

void ESP_Mail_Client::addHeader(MB_String &s, const char *name, int value, bool json)
{
  if (json)
  {
    s += (s.length() > 0) ? MBSTRING_FLASH_MCR(",\"") : MBSTRING_FLASH_MCR("{\"");
    s += name;
    s += MBSTRING_FLASH_MCR("\":");
    s += value;
  }
  else
  {
    if (s.length() > 0)
      s += MBSTRING_FLASH_MCR("\r\n");
    s += name;
    s += MBSTRING_FLASH_MCR(": ");
    s += value;
  }
}

void ESP_Mail_Client::saveHeader(IMAPSession *imap, bool json)
{

  MB_String headerFilePath;

  prepareFilePath(imap, headerFilePath, true);

  if (json)
    headerFilePath += esp_mail_str_347;
  else
    headerFilePath += esp_mail_str_203;

  prepareFileList(imap, headerFilePath);

  int sz = mbfs->open(headerFilePath, mbfs_type imap->_config->storage.type, mb_fs_open_mode_write);
  if (sz < 0)
  {
    if (imap->_debug)
    {
      imap->_imapStatus.statusCode = sz;
      imap->_imapStatus.text.clear();

      MB_String e = esp_mail_str_185;
      e += imap->errorReason().c_str();
      esp_mail_debug_line(e.c_str(), true);
    }
    return;
  }

  MB_String s;

  for (size_t i = 0; i < imap->_headers.size(); i++)
    addHeaderItem(s, &imap->_headers[i], json);

  if (json)
    s += MBSTRING_FLASH_MCR("]}");

  mbfs->print(mbfs_type imap->_config->storage.type, s.c_str());

  mbfs->close(mbfs_type imap->_config->storage.type);

  imap->_headerSaved = true;
}

void ESP_Mail_Client::addHeaderItem(MB_String &str, esp_mail_message_header_t *header, bool json)
{
  MB_String s;

  if (json)
  {
    if (str.length() > 0)
      str += MBSTRING_FLASH_MCR(",");
    else
      str = MBSTRING_FLASH_MCR("{\"Messages\":[");
  }

  addHeader(s, "Number", header->message_no, json);
  addHeader(s, "UID", header->message_uid, json);

  if (header->accept_language.length() > 0)
    addHeader(s, "Accept-Language", header->accept_language, false, json);

  if (header->content_language.length() > 0)
    addHeader(s, "Content-Language", header->content_language, false, json);

  addHeaders(s, &header->header_fields, json);

  for (size_t j = 0; j < header->part_headers.size(); j++)
  {
    if (header->part_headers[j].rfc822_part)
    {
      MB_String s1;
      addHeaders(s1, &header->part_headers[j].rfc822_header, json);

      if (json)
      {
        s += MBSTRING_FLASH_MCR(",\"RFC822\":");
        s += s1;
        s += MBSTRING_FLASH_MCR("}");
      }
      else
      {
        s += MBSTRING_FLASH_MCR("\r\n\r\nRFC822:\r\n");
        s += s1;
      }
    }
  }

  if (header->attachment_count > 0)
  {
    if (json)
    {
      s += MBSTRING_FLASH_MCR(",\"Attachments\":{\"Count\":");
      s += header->attachment_count;
      s += MBSTRING_FLASH_MCR(",\"Files\":[");
    }
    else
    {
      s += MBSTRING_FLASH_MCR("\r\n\r\nAttachments (");
      s += header->attachment_count;
      s += MBSTRING_FLASH_MCR(")\r\n");
    }

    int index = 0;
    for (size_t j = 0; j < header->part_headers.size(); j++)
    {

      if (header->part_headers[j].attach_type == esp_mail_att_type_none || header->part_headers[j].rfc822_part)
        continue;

      struct esp_mail_attachment_info_t att;
      att.filename = header->part_headers[j].filename.c_str();
      att.mime = header->part_headers[j].content_type.c_str();
      att.name = header->part_headers[j].name.c_str();
      att.size = header->part_headers[j].attach_data_size;
      att.creationDate = header->part_headers[j].creation_date.c_str();
      att.type = header->part_headers[j].attach_type;

      if (json)
      {
        if (index > 0)
          s += MBSTRING_FLASH_MCR(",");
        s += MBSTRING_FLASH_MCR("{\"Filename\":\"");
        s += att.filename;
        s += MBSTRING_FLASH_MCR("\"");
      }
      else
      {
        if (index > 0)
          s += MBSTRING_FLASH_MCR("\r\n");
        s += MBSTRING_FLASH_MCR("\r\n");
        s += MBSTRING_FLASH_MCR("Index: ");
        s += index + 1;
        addHeader(s, "Filename", att.filename, false, json);
      }

      addHeader(s, "Name", att.name, false, json);
      addHeader(s, "Size", att.size, false, json);
      addHeader(s, "MIME", att.mime, false, json);
      addHeader(s, "Type", att.type == esp_mail_att_type_attachment ? "attachment" : "inline", false, json);
      addHeader(s, "Creation Date", att.creationDate, false, json);

      if (json)
        s += MBSTRING_FLASH_MCR("}");

      index++;
    }

    if (json)
      s += MBSTRING_FLASH_MCR("]}");
  }

  if (json)
  {
    s += MBSTRING_FLASH_MCR("}");
  }

  str += s;
}

void ESP_Mail_Client::addHeaders(MB_String &s, esp_mail_imap_rfc822_msg_header_item_t *header, bool json)
{

  if (header->messageID.length() > 0)
    addHeader(s, "Message-ID", header->messageID, false, json);

  if (header->from.length() > 0)
    addHeader(s, "From", header->from, true, json);

  if (header->sender.length() > 0)
    addHeader(s, "Sender", header->sender, true, json);

  if (header->to.length() > 0)
    addHeader(s, "To", header->to, true, json);

  if (header->cc.length() > 0)
    addHeader(s, "CC", header->cc, true, json);

  if (header->date.length() > 0)
    addHeader(s, "Date", header->date, false, json);

  if (header->subject.length() > 0)
    addHeader(s, "Subject", header->subject, false, json);

  if (header->reply_to.length() > 0)
    addHeader(s, "Reply-To", header->reply_to, true, json);

  if (header->return_path.length() > 0)
    addHeader(s, "Return-Path", header->return_path, true, json);

  if (header->in_reply_to.length() > 0)
    addHeader(s, "In-Reply-To", header->in_reply_to, true, json);

  if (header->references.length() > 0)
    addHeader(s, "References", header->references, true, json);

  if (header->comments.length() > 0)
    addHeader(s, "Comments", header->comments, false, json);

  if (header->keywords.length() > 0)
    addHeader(s, "Keywords", header->keywords, false, json);
}

esp_mail_imap_response_status ESP_Mail_Client::imapResponseStatus(IMAPSession *imap, char *response)
{
  imap->_imapStatus.text.clear();
  if (strposP(response, esp_mail_imap_response_1, 0) > -1)
    return esp_mail_imap_response_status::esp_mail_imap_resp_ok;
  else if (strposP(response, esp_mail_imap_response_2, 0) > -1)
  {
    imap->_imapStatus.text = response;
    imap->_imapStatus.text = imap->_imapStatus.text.substr(strlen_P(esp_mail_imap_response_2));
    return esp_mail_imap_response_status::esp_mail_imap_resp_no;
  }
  else if (strposP(response, esp_mail_imap_response_3, 0) > -1)
  {
    imap->_imapStatus.text = response;
    imap->_imapStatus.text = imap->_imapStatus.text.substr(strlen_P(esp_mail_imap_response_3));
    return esp_mail_imap_response_status::esp_mail_imap_resp_bad;
  }
  return esp_mail_imap_response_status::esp_mail_imap_resp_unknown;
}

void ESP_Mail_Client::handleCapability(IMAPSession *imap, char *buf, int &chunkIdx)
{
  if (chunkIdx == 0)
  {
    if (strposP(buf, esp_mail_imap_response_10, 0) > -1)
    {
      if (strposP(buf, esp_mail_imap_response_11, 0) > -1)
        imap->_auth_capability.login = true;
      if (strposP(buf, esp_mail_imap_response_12, 0) > -1)
        imap->_auth_capability.plain = true;
      if (strposP(buf, esp_mail_imap_response_13, 0) > -1)
        imap->_auth_capability.xoauth2 = true;
      if (strposP(buf, esp_mail_imap_response_14, 0) > -1)
        imap->_auth_capability.start_tls = true;
      if (strposP(buf, esp_mail_imap_response_15, 0) > -1)
        imap->_auth_capability.cram_md5 = true;
      if (strposP(buf, esp_mail_imap_response_16, 0) > -1)
        imap->_auth_capability.digest_md5 = true;
      if (strposP(buf, esp_mail_imap_response_17, 0) > -1)
        imap->_read_capability.idle = true;
      if (strposP(buf, esp_mail_imap_response_18, 0) > -1)
        imap->_read_capability.imap4 = true;
      if (strposP(buf, esp_mail_imap_response_19, 0) > -1)
        imap->_read_capability.imap4rev1 = true;
    }
  }
}

void ESP_Mail_Client::handleFolders(IMAPSession *imap, char *buf)
{
  struct esp_mail_folder_info_t fd;
  char *tmp = nullptr;
  int p1 = strposP(buf, esp_mail_imap_response_4, 0);
  int p2 = 0;
  if (p1 != -1)
  {
    p1 = strposP(buf, esp_mail_str_198, 0);
    if (p1 != -1)
    {
      p2 = strposP(buf, esp_mail_str_192, p1 + 1);
      if (p2 != -1)
      {
        tmp = (char *)newP(p2 - p1);
        strncpy(tmp, buf + p1 + 1, p2 - p1 - 1);
        if (tmp[p2 - p1 - 2] == '\r')
          tmp[p2 - p1 - 2] = 0;
        fd.attributes = tmp;
        delP(&tmp);
      }
    }

    p1 = strposP(buf, esp_mail_str_136, 0);
    if (p1 != -1)
    {
      p2 = strposP(buf, esp_mail_str_136, p1 + 1);
      if (p2 != -1)
      {
        tmp = (char *)newP(p2 - p1);
        strncpy(tmp, buf + p1 + 1, p2 - p1 - 1);
        if (tmp[p2 - p1 - 2] == '\r')
          tmp[p2 - p1 - 2] = 0;
        fd.delimiter = tmp;
        delP(&tmp);
      }
    }

    p1 = strposP(buf, esp_mail_str_131, p2);
    if (p1 != -1)
    {
      p2 = strlen(buf);
      tmp = (char *)newP(p2 - p1);
      if (buf[p1 + 1] == '"')
        p1++;
      strncpy(tmp, buf + p1 + 1, p2 - p1 - 1);
      if (tmp[p2 - p1 - 2] == '\r')
        tmp[p2 - p1 - 2] = 0;
      if (tmp[strlen(tmp) - 1] == '"')
        tmp[strlen(tmp) - 1] = 0;
      fd.name = tmp;
      delP(&tmp);
    }
    imap->_folders.add(fd);
  }
}

bool ESP_Mail_Client::handleIdle(IMAPSession *imap)
{

  int chunkBufSize = 0;

  if (!reconnect(imap))
    return false;

  if (imap->client.connected())
    chunkBufSize = imap->client.available();
  else
    return false;

  if (chunkBufSize > 0)
  {
    chunkBufSize = 512;

    char *buf = (char *)newP(chunkBufSize + 1);

    int octetCount = 0;

    int readLen = MailClient.readLine(imap, buf, chunkBufSize, false, octetCount);

    if (readLen > 0)
    {

      if (imap->_debugLevel > esp_mail_debug_level_1)
        esp_mail_debug((const char *)buf);

      char *tmp = nullptr;
      int p1 = -1;
      bool exists = false;

      p1 = strposP(buf, esp_mail_str_199, 0);
      if (p1 != -1)
      {
        int numMsg = imap->_mbif._msgCount;
        tmp = (char *)newP(p1);
        strncpy(tmp, buf + 2, p1 - 1);
        imap->_mbif._msgCount = atoi(tmp);
        delP(&tmp);
        exists = true;
        imap->_mbif._folderChanged |= (int)imap->_mbif._msgCount != numMsg;
        if ((int)imap->_mbif._msgCount > numMsg)
        {
          imap->_mbif._polling_status.type = imap_polling_status_type_new_message;
          imap->_mbif._polling_status.messageNum = imap->_mbif._msgCount;
        }
        goto ex;
      }

      p1 = strposP(buf, esp_mail_str_333, 0);
      if (p1 != -1)
      {
        imap->_mbif._polling_status.type = imap_polling_status_type_remove_message;
        tmp = (char *)newP(p1);
        strncpy(tmp, buf + 2, p1 - 1);
        imap->_mbif._polling_status.messageNum = atoi(tmp);

        if (imap->_mbif._polling_status.messageNum == imap->_mbif._msgCount && imap->_mbif._nextUID > 0)
          imap->_mbif._nextUID--;

        delP(&tmp);
        imap->_mbif._folderChanged = true;
        goto ex;
      }

      p1 = strposP(buf, esp_mail_str_334, 0);
      if (p1 != -1)
      {
        tmp = (char *)newP(p1);
        strncpy(tmp, buf + 2, p1 - 1);
        imap->_mbif._recentCount = atoi(tmp);
        delP(&tmp);
        goto ex;
      }

      p1 = strposP(buf, esp_mail_imap_response_7, 0);
      if (p1 != -1)
      {
        tmp = (char *)newP(p1);
        strncpy(tmp, buf + 2, p1 - 1);
        imap->_mbif._polling_status.messageNum = atoi(tmp);
        delP(&tmp);

        imap->_mbif._polling_status.argument = buf;
        imap->_mbif._polling_status.argument.erase(0, p1 + 8);
        imap->_mbif._polling_status.argument.pop_back();
        imap->_mbif._folderChanged = true;
        imap->_mbif._polling_status.type = imap_polling_status_type_fetch_message;
        goto ex;
      }

    ex:

      imap->_mbif._floderChangedState = (imap->_mbif._folderChanged && exists) || imap->_mbif._polling_status.type == imap_polling_status_type_fetch_message;
    }

    delP(&buf);
  }

  size_t imap_idle_tmo = imap->_config->limit.imap_idle_timeout;

  if (imap_idle_tmo < 60 * 1000 || imap_idle_tmo > 29 * 60 * 1000)
    imap_idle_tmo = 10 * 60 * 1000;

  if (millis() - imap->_mbif._idleTimeMs > imap_idle_tmo)
  {
    if (imap->mStopListen(true))
      return imap->mListen(true);
    return false;
  }

  return true;
}

void ESP_Mail_Client::handleGetUID(IMAPSession *imap, char *buf)
{
  char *tmp = nullptr;
  int p1 = strposP(buf, esp_mail_str_342, 0);
  imap->_uid_tmp = 0;
  if (p1 != -1)
  {
    tmp = (char *)newP(20);
    strncpy(tmp, buf + p1 + strlen_P(esp_mail_str_342), strlen(buf) - p1 - strlen_P(esp_mail_str_342) - 1);
    imap->_uid_tmp = atoi(tmp);
    delP(&tmp);
    return;
  }
}

void ESP_Mail_Client::handleGetFlags(IMAPSession *imap, char *buf)
{
  char *tmp = nullptr;
  int p1 = strposP(buf, esp_mail_str_112, 0);
  if (p1 != -1)
  {
    int len = strlen(buf) - p1 - strlen_P(esp_mail_str_112);
    tmp = (char *)newP(len);
    strncpy(tmp, buf + p1 + strlen_P(esp_mail_str_112), strlen(buf) - p1 - strlen_P(esp_mail_str_112) - 2);
    imap->_flags_tmp = tmp;
    delP(&tmp);
    return;
  }
}

void ESP_Mail_Client::handleExamine(IMAPSession *imap, char *buf)
{
  char *tmp = NULL;
  int p1, p2;

  p1 = strposP(buf, esp_mail_str_199, 0);
  if (p1 != -1)
  {
    tmp = (char *)newP(p1);
    strncpy(tmp, buf + 2, p1 - 1);
    imap->_mbif._msgCount = atoi(tmp);
    delP(&tmp);
    return;
  }

  p1 = strposP(buf, esp_mail_str_334, 0);
  if (p1 != -1)
  {
    tmp = (char *)newP(p1);
    strncpy(tmp, buf + 2, p1 - 1);
    imap->_mbif._recentCount = atoi(tmp);
    delP(&tmp);
    return;
  }

  if (imap->_mbif._flags.size() == 0)
  {
    p1 = strposP(buf, esp_mail_imap_response_5, 0);
    if (p1 != -1)
    {
      p1 = strposP(buf, esp_mail_str_198, 0);
      if (p1 != -1)
      {
        p2 = strposP(buf, esp_mail_str_192, p1 + 1);
        if (p2 != -1)
        {
          tmp = (char *)newP(p2 - p1);
          strncpy(tmp, buf + p1 + 1, p2 - p1 - 1);
          char *stk = strP(esp_mail_str_131);
          MB_String content = tmp;
          MB_VECTOR<MB_String> tokens;
          splitTk(content, tokens, stk);
          for (size_t i = 0; i < tokens.size(); i++)
            imap->_mbif.addFlag(tokens[i].c_str());
          delP(&tmp);
          delP(&stk);
        }
      }
      return;
    }
  }

  if (imap->_nextUID.length() == 0)
  {
    p1 = strposP(buf, esp_mail_str_200, 0);
    if (p1 != -1)
    {
      p2 = strposP(buf, esp_mail_str_219, p1 + strlen_P(esp_mail_str_200));
      if (p2 != -1)
      {
        tmp = (char *)newP(p2 - p1 - strlen_P(esp_mail_str_200) + 1);
        strncpy(tmp, buf + p1 + strlen_P(esp_mail_str_200), p2 - p1 - strlen_P(esp_mail_str_200));
        imap->_nextUID = tmp;
        imap->_mbif._nextUID = atoi(tmp);
        delP(&tmp);
      }
      return;
    }
  }
}

bool ESP_Mail_Client::handleIMAPError(IMAPSession *imap, int err, bool ret)
{
  if (err < 0)
  {
    errorStatusCB(imap, err);

    if (imap->_headers.size() > 0)
    {
      if ((imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_inline) && (imap->_config->download.attachment || imap->_config->download.inlineImg))
      {
        if (cHeader(imap)->part_headers.size() > 0)
          cPart(imap)->download_error = imap->errorReason().c_str();
      }
      else
        cHeader(imap)->error_msg = imap->errorReason().c_str();

      cHeader(imap)->error = true;
    }
  }

  if (imap->_tcpConnected)
    closeTCPSession(imap);

  imap->_cbData.empty();

  return ret;
}

void ESP_Mail_Client::prepareFileList(IMAPSession *imap, MB_String &filePath)
{
#if defined(MBFS_SD_FS)
  if (!mbfs->longNameSupported())
  {
    cHeader(imap)->sd_alias_file_count++;
    MB_String alias = cMSG(imap);
    alias += MBSTRING_FLASH_MCR("_");
    alias += cHeader(imap)->sd_alias_file_count;

    if (imap->_sdFileList.length() > 0)
    {
      if (imap->_sdFileList[imap->_sdFileList.length() - 1] == ']')
      {
        imap->_sdFileList[imap->_sdFileList.length() - 1] = 0;
        imap->_sdFileList += MBSTRING_FLASH_MCR(",");
      }
    }
    imap->_sdFileList += MBSTRING_FLASH_MCR("{\"Renamed\":\"");
    imap->_sdFileList += alias;
    imap->_sdFileList += MBSTRING_FLASH_MCR("\",\"Original\":\"");
    imap->_sdFileList += filePath;
    imap->_sdFileList += MBSTRING_FLASH_MCR("\"}]");
    // rename the original file
    filePath = alias;
  }
#endif
}

bool ESP_Mail_Client::handleAttachment(IMAPSession *imap, char *buf, int bufLen, int &chunkIdx, MB_String &filePath, bool &downloadRequest, int &octetCount, int &octetLength)
{
  if (chunkIdx == 0)
  {
    char *tmp = subStr(buf, esp_mail_str_193, esp_mail_str_194, 0);
    if (tmp)
    {
      octetCount = 0; // CRLF counted from first line
      octetLength = atoi(tmp);
      delP(&tmp);
      chunkIdx++;
      cPart(imap)->octetCount = 0;
      cHeader(imap)->total_download_size += octetLength;
      imap->_lastProgress = -1;

      if (!cPart(imap)->file_open_write)
      {

        if (mbfs->checkStorageReady(mbfs_type imap->_config->storage.type))
        {

          downloadRequest = true;

          filePath.clear();

          filePath += imap->_config->storage.saved_path;
          filePath += esp_mail_str_202;
          filePath += cMSG(imap);

#if defined(MBFS_SD_FS)

          if (imap->_config->storage.type == esp_mail_file_storage_type_sd)
            mbfs->createDirs(filePath);

#endif

          filePath += esp_mail_str_202;

          filePath += cPart(imap)->filename;

          prepareFileList(imap, filePath);

          int sz = mbfs->open(filePath, mbfs_type imap->_config->storage.type, mb_fs_open_mode_write);

          if (sz < 0)
          {
            if (imap->_debug)
            {
              imap->_imapStatus.statusCode = sz;
              imap->_imapStatus.text.clear();

              MB_String e = esp_mail_str_185;
              e += imap->errorReason().c_str();
              esp_mail_debug_line(e.c_str(), true);
            }
          }

          cPart(imap)->file_open_write = true;
        }
        else
          sendStorageNotReadyError(imap, imap->_config->storage.type);
      }
    }
    return true;
  }

  if (octetLength == 0)
    return true;

  chunkIdx++;

  delay(0);

  if (cPart(imap)->octetCount <= octetLength)
  {
    if (cPart(imap)->octetCount + bufLen > octetLength)
    {
      bufLen = octetLength - cPart(imap)->octetCount;
      buf[bufLen] = 0;
      cPart(imap)->octetCount += bufLen;
    }
    else
      cPart(imap)->octetCount = octetCount;

    if (imap->_config->enable.download_status)
    {
      if (imap->_readCallback)
        downloadReport(imap, 100 * cPart(imap)->octetCount / octetLength);
    }

    if (cPart(imap)->xencoding == esp_mail_msg_part_xencoding_base64)
    {

      size_t olen = 0;
      unsigned char *decoded = decodeBase64((const unsigned char *)buf, bufLen, &olen);

      if (decoded)
      {

        if (!cPart(imap)->sizeProp)
        {
          cPart(imap)->attach_data_size += olen;
          cHeader(imap)->total_attach_data_size += cPart(imap)->attach_data_size;
        }
        int write = olen;
        if (mbfs->ready(mbfs_type imap->_config->storage.type))
          write = mbfs->write(mbfs_type imap->_config->storage.type, (uint8_t *)decoded, olen);
        delay(0);
        delP(&decoded);

        if (write != (int)olen)
          return false;
      }

      if (!reconnect(imap))
        return false;
    }
    else
    {
      // binary content
      if (!cPart(imap)->sizeProp)
      {
        cPart(imap)->attach_data_size += bufLen;
        cHeader(imap)->total_attach_data_size += cPart(imap)->attach_data_size;
      }

      int write = bufLen;
      if (mbfs->ready(mbfs_type imap->_config->storage.type))
        write = mbfs->write(mbfs_type imap->_config->storage.type, (uint8_t *)buf, bufLen);

      delay(0);

      if (write != bufLen)
        return false;

      if (!reconnect(imap))
        return false;
    }
  }
  return true;
}

void ESP_Mail_Client::downloadReport(IMAPSession *imap, int progress)
{
  if (progress > 100)
    progress = 100;
  if (imap->_readCallback && imap->_lastProgress != progress && (progress == 0 || progress == 100 || imap->_lastProgress + ESP_MAIL_PROGRESS_REPORT_STEP <= progress))
  {
    MB_String filePath = imap->_config->storage.saved_path;
    filePath += esp_mail_str_202;
    filePath += cMSG(imap);
    filePath += esp_mail_str_202;
    filePath += cPart(imap)->filename;

    MB_String s = esp_mail_str_90;
    s += esp_mail_str_131;
    s += MBSTRING_FLASH_MCR("\"");
    s += filePath;
    s += MBSTRING_FLASH_MCR("\"");
    s += esp_mail_str_91;
    s += progress;
    s += esp_mail_str_92;
    s += esp_mail_str_34;
    esp_mail_debug_line(s.c_str(), false);
    s.clear();
    imap->_lastProgress = progress;
  }
}

void ESP_Mail_Client::fetchReport(IMAPSession *imap, int progress, bool download)
{
  if (progress > 100)
    progress = 100;
  if (imap->_readCallback && imap->_lastProgress != progress && (progress == 0 || progress == 100 || imap->_lastProgress + ESP_MAIL_PROGRESS_REPORT_STEP <= progress))
  {
    MB_String s;

    if (download)
      s = esp_mail_str_90;
    else
      s = esp_mail_str_83;
    s += esp_mail_str_131;
    if (cPart(imap)->filename.length() > 0)
    {
      s += MBSTRING_FLASH_MCR("\"");
      s += cPart(imap)->filename;
      s += MBSTRING_FLASH_MCR("\"");
      s += esp_mail_str_91;
    }
    s += progress;
    s += esp_mail_str_92;
    s += esp_mail_str_34;
    esp_mail_debug_line(s.c_str(), false);
    s.clear();
    imap->_lastProgress = progress;
  }
}

void ESP_Mail_Client::searchReport(int progress, const char *percent)
{
  if (progress % ESP_MAIL_PROGRESS_REPORT_STEP == 0)
  {
    MB_String s = esp_mail_str_261;
    s += progress;
    s += percent;
    s += esp_mail_str_34;
    esp_mail_debug_line(s.c_str(), false);
  }
}

int ESP_Mail_Client::cMSG(IMAPSession *imap)
{
  return imap->_msgUID[cIdx(imap)];
}

int ESP_Mail_Client::cIdx(IMAPSession *imap)
{
  return imap->_cMsgIdx;
}

void ESP_Mail_Client::decodeText(IMAPSession *imap, char *buf, int bufLen, int &chunkIdx, MB_String &filePath, bool &downloadRequest, int &octetLength, int &octetCount)
{

  bool rfc822_body_subtype = cPart(imap)->message_sub_type == esp_mail_imap_message_sub_type_rfc822;
  if (chunkIdx == 0)
  {
    imap->_lastProgress = -1;
    char *tmp = subStr(buf, esp_mail_str_193, esp_mail_str_194, 0);
    if (tmp)
    {

      octetCount = 0;
      octetLength = atoi(tmp);
      delP(&tmp);
      chunkIdx++;
      cPart(imap)->octetLen = octetLength;
      cPart(imap)->octetCount = 0;

      bool dlMsg = (rfc822_body_subtype && imap->_config->download.rfc822) || (!rfc822_body_subtype && ((cPart(imap)->msg_type == esp_mail_msg_type_html && imap->_config->download.html) || ((cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched) && imap->_config->download.text)));
      if (dlMsg)
        prepareFilePath(imap, filePath, false);

      if (filePath.length() == 0)
      {
        if (!rfc822_body_subtype)
          filePath += esp_mail_str_67;
        else
        {
          filePath += esp_mail_str_82;
          filePath += esp_mail_str_131;
          filePath += esp_mail_str_67;
        }
      }
      cPart(imap)->filename = filePath;

      if (!cPart(imap)->file_open_write && dlMsg)
      {

        prepareFileList(imap, filePath);

        int sz = mbfs->open(filePath, mbfs_type imap->_config->storage.type, mb_fs_open_mode_write);
        if (sz > -1)
        {
          downloadRequest = true;
          cPart(imap)->file_open_write = true;
        }
        else
        {
          imap->_imapStatus.statusCode = sz;
          imap->_imapStatus.text.clear();

          if (imap->_debug)
          {
            MB_String e = esp_mail_str_185;
            e += imap->errorReason().c_str();
            esp_mail_debug_line(e.c_str(), true);
          }
        }
      }

      return;
    }
    else
    {
      if (imap->_debug)
      {
        MB_String s1 = esp_mail_str_280;
        esp_mail_debug_line(s1.c_str(), false);
      }
    }
  }

  delay(0);

  if (octetLength == 0)
    return;

  bool enableDownloads = (imap->_config->download.rfc822 && rfc822_body_subtype) || (!rfc822_body_subtype && ((cPart(imap)->msg_type == esp_mail_msg_type_html && imap->_config->download.html) || ((cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched) && imap->_config->download.text)));

  if (imap->_config->download.rfc822 || imap->_config->download.html || imap->_config->download.text || (rfc822_body_subtype && imap->_config->enable.rfc822) || (!rfc822_body_subtype && ((cPart(imap)->msg_type == esp_mail_msg_type_html && imap->_config->enable.html) || ((cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched) && imap->_config->enable.text))))
  {

    if (cPart(imap)->octetCount + bufLen > octetLength)
    {
      bufLen = octetLength - cPart(imap)->octetCount;
      buf[bufLen] = 0;
      cPart(imap)->octetCount += bufLen;
    }
    else
      cPart(imap)->octetCount = octetCount;

    if (imap->_readCallback)
      fetchReport(imap, 100 * cPart(imap)->octetCount / octetLength, enableDownloads);

    if (cPart(imap)->octetCount <= octetLength)
    {
      bool hrdBrk = cPart(imap)->xencoding != esp_mail_msg_part_xencoding_base64 && cPart(imap)->octetCount < octetLength;

      // remove soft break for QP
      if (bufLen <= QP_ENC_MSG_LEN && buf[bufLen - 1] == '=' && cPart(imap)->xencoding == esp_mail_msg_part_xencoding_qp)
      {
        hrdBrk = false;
        buf[bufLen - 1] = 0;
        bufLen--;
      }

      size_t olen = 0;
      char *decoded = nullptr;
      bool newC = true;
      if (cPart(imap)->xencoding == esp_mail_msg_part_xencoding_base64)
      {
        decoded = (char *)decodeBase64((const unsigned char *)buf, bufLen, &olen);
      }
      else if (cPart(imap)->xencoding == esp_mail_msg_part_xencoding_qp)
      {
        decoded = (char *)newP(bufLen + 10);
        decodeQP(buf, decoded);
        olen = strlen(decoded);
      }
      else if (cPart(imap)->xencoding == esp_mail_msg_part_xencoding_7bit)
      {
        decoded = decode7Bit(buf);
        olen = strlen(decoded);
      }
      else
      {
        // 8bit and binary
        newC = false;
        decoded = buf;
        olen = bufLen;
      }

      if (decoded)
      {

        if ((rfc822_body_subtype && imap->_config->enable.rfc822) || (!rfc822_body_subtype && ((cPart(imap)->msg_type == esp_mail_msg_type_html && imap->_config->enable.html) || ((cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched) && imap->_config->enable.text))))
        {

          if (getEncodingFromCharset(cPart(imap)->charset.c_str()) == esp_mail_char_decoding_scheme_iso8859_1)
          {
            int ilen = olen;
            int olen2 = (ilen + 1) * 2;
            unsigned char *tmp = (unsigned char *)newP(olen2);
            decodeLatin1_UTF8(tmp, &olen2, (unsigned char *)decoded, &ilen);
            delP(&decoded);
            olen = olen2;
            decoded = (char *)tmp;
          }
          else if (getEncodingFromCharset(cPart(imap)->charset.c_str()) == esp_mail_char_decoding_scheme_tis620)
          {
            char *out = (char *)newP((olen + 1) * 3);
            decodeTIS620_UTF8(out, decoded, olen);
            olen = strlen(out);
            delP(&decoded);
            decoded = out;
          }

          if (cPart(imap)->text.length() < imap->_config->limit.msg_size)
          {

            if (cPart(imap)->text.length() + olen < imap->_config->limit.msg_size)
            {
              cPart(imap)->textLen += olen;
              cPart(imap)->text.append(decoded, olen);
              if (hrdBrk)
              {
                cPart(imap)->text += MBSTRING_FLASH_MCR("\r\n");
                cPart(imap)->textLen += 2;
              }
            }
            else
            {
              int d = imap->_config->limit.msg_size - cPart(imap)->text.length();
              cPart(imap)->textLen += d;
              if (d > 0)
                cPart(imap)->text.append(decoded, d);

              if (hrdBrk)
              {
                cPart(imap)->text += MBSTRING_FLASH_MCR("\r\n");
                cPart(imap)->textLen += 2;
              }
            }
          }
        }

        if (filePath.length() > 0 && downloadRequest)
        {
          if (mbfs->ready(mbfs_type imap->_config->storage.type))
          {
            if (olen > 0)
              mbfs->write(mbfs_type imap->_config->storage.type, (uint8_t *)decoded, olen);
            if (hrdBrk)
              mbfs->write(mbfs_type imap->_config->storage.type, (uint8_t *)MBSTRING_FLASH_MCR("\r\n"), 2);
          }
        }

        if (newC)
          delP(&decoded);
      }
    }
  }
}

void ESP_Mail_Client::prepareFilePath(IMAPSession *imap, MB_String &filePath, bool header)
{
  bool rfc822_body_subtype = cPart(imap)->message_sub_type == esp_mail_imap_message_sub_type_rfc822;
  MB_String fpath = imap->_config->storage.saved_path;
  fpath += esp_mail_str_202;
  fpath += cMSG(imap);

#if defined(MBFS_SD_FS)
  if (imap->_config->storage.type == esp_mail_file_storage_type_sd)
    mbfs->createDirs(fpath);
#endif

  if (!header)
  {
    if (!rfc822_body_subtype)
    {
      fpath += esp_mail_str_161;
      if (cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched)
        fpath += esp_mail_str_95;
      else if (cPart(imap)->msg_type == esp_mail_msg_type_html)
        fpath += esp_mail_str_94;
    }
    else
    {
      fpath += esp_mail_str_163;

      if (cPart(imap)->rfc822_msg_Idx > 0)
        fpath += cPart(imap)->rfc822_msg_Idx;

      if (cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched)
        fpath += esp_mail_str_95;
      else if (cPart(imap)->msg_type == esp_mail_msg_type_html)
        fpath += esp_mail_str_94;
      else
        // possible rfc822 encapsulated message which cannot fetch its header
        fpath += esp_mail_str_40;
    }
  }

  filePath = fpath;
}

IMAPSession::IMAPSession(Client *client)
{
  setClient(client);
}

IMAPSession::IMAPSession()
{
}

IMAPSession::~IMAPSession()
{
  empty();
#if defined(ESP32) || defined(ESP8266)
  _caCert.reset();
  _caCert = nullptr;
#endif
}

bool IMAPSession::closeSession()
{
  if (!_tcpConnected)
    return false;

  if (_mbif._idleTimeMs > 0)
    mStopListen(false);

#if !defined(ESP8266)
  /**
   * The strange behavior in ESP8266 SSL client, BearSSLWiFiClientSecure
   * The client disposed without memory released after the server close
   * the connection due to LOGOUT command, which caused the memory leaks.
   */
  if (!MailClient.imapLogout(this))
    return false;
#endif
  return MailClient.handleIMAPError(this, 0, true);
}

bool IMAPSession::connect(ESP_Mail_Session *session, IMAP_Config *config)
{
  if (client.type() == esp_mail_client_type_custom)
  {
#if !defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT)
    return MailClient.handleIMAPError(this, MAIL_CLIENT_ERROR_CUSTOM_CLIENT_DISABLED, false);
#endif
    if (!client.isInitialized())
      return MailClient.handleIMAPError(this, TCP_CLIENT_ERROR_NOT_INITIALIZED, false);
  }

  if (_tcpConnected)
    MailClient.closeTCPSession(this);

  _sesson_cfg = session;
  _config = config;

  if (session)
  {
    if (session->time.ntp_server.length() > 0)
      MailClient.setTime(session->time.gmt_offset, session->time.day_light_offset, session->time.ntp_server.c_str(), true);
  }

#if defined(ESP32) || defined(ESP8266)

  _caCert = nullptr;

  if (strlen(_sesson_cfg->certificate.cert_data) > 0)
    _caCert = std::shared_ptr<const char>(_sesson_cfg->certificate.cert_data);

#endif

  return MailClient.imapAuth(this);
}

void IMAPSession::debug(int level)
{
  if (level > esp_mail_debug_level_0)
  {
    if (level > esp_mail_debug_level_3)
      level = esp_mail_debug_level_1;
    _debugLevel = level;
    _debug = true;
  }
  else
  {
    _debugLevel = esp_mail_debug_level_0;
    _debug = false;
  }
}

String IMAPSession::errorReason()
{
  MB_String ret;

  if (_imapStatus.text.length() > 0)
    return _imapStatus.text.c_str();

  switch (_imapStatus.statusCode)
  {
  case IMAP_STATUS_SERVER_CONNECT_FAILED:
    ret += esp_mail_str_38;
    break;
  case MAIL_CLIENT_ERROR_CONNECTION_CLOSED:
    ret += esp_mail_str_221;
    break;
  case MAIL_CLIENT_ERROR_READ_TIMEOUT:
    ret += esp_mail_str_258;
    break;
  case MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED:
    ret += esp_mail_str_305;
    break;
  case IMAP_STATUS_NO_MESSAGE:
    ret += esp_mail_str_306;
    break;
  case IMAP_STATUS_ERROR_DOWNLAD_TIMEOUT:
    ret += esp_mail_str_93;
    break;
  case MAIL_CLIENT_ERROR_SSL_TLS_STRUCTURE_SETUP:
    ret += esp_mail_str_132;
    break;
  case IMAP_STATUS_CLOSE_MAILBOX_FAILED:
    ret += esp_mail_str_188;
    break;
  case IMAP_STATUS_OPEN_MAILBOX_FAILED:
    ret += esp_mail_str_281;
    break;
  case IMAP_STATUS_LIST_MAILBOXS_FAILED:
    ret += esp_mail_str_62;
    break;
  case IMAP_STATUS_NO_SUPPORTED_AUTH:
    ret += esp_mail_str_42;
    break;
  case IMAP_STATUS_CHECK_CAPABILITIES_FAILED:
    ret += esp_mail_str_63;
    break;
  case MAIL_CLIENT_ERROR_OUT_OF_MEMORY:
    ret += esp_mail_str_186;
    break;
  case IMAP_STATUS_NO_MAILBOX_FOLDER_OPENED:
    ret += esp_mail_str_153;
    break;

  case TCP_CLIENT_ERROR_CONNECTION_REFUSED:
    ret += esp_mail_str_345;
    break;
  case TCP_CLIENT_ERROR_NOT_INITIALIZED:
    ret += esp_mail_str_346;
    break;

  case MAIL_CLIENT_ERROR_CUSTOM_CLIENT_DISABLED:
    ret += esp_mail_str_352;
    break;

  case MB_FS_ERROR_FILE_IO_ERROR:
    ret += esp_mail_str_282;
    break;

#if defined(MBFS_FLASH_FS) || defined(MBFS_SD_FS)

  case MB_FS_ERROR_FLASH_STORAGE_IS_NOT_READY:
    ret += esp_mail_str_348;
    break;

  case MB_FS_ERROR_SD_STORAGE_IS_NOT_READY:
    ret += esp_mail_str_349;
    break;

  case MB_FS_ERROR_FILE_STILL_OPENED:
    ret += esp_mail_str_350;
    break;

  case MB_FS_ERROR_FILE_NOT_FOUND:
    ret += esp_mail_str_351;
    break;

#endif

  default:
    break;
  }
  return ret.c_str();
}

bool IMAPSession::mSelectFolder(MB_StringPtr folderName, bool readOnly)
{
  if (_tcpConnected)
  {
    if (!openFolder(folderName, readOnly))
      return false;
  }
  else
  {
    _currentFolder = folderName;
  }

  return true;
}

void IMAPSession::setClient(Client *client)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.setClient(client);
#endif
}

void IMAPSession::connectionRequestCallback(ConnectionRequestCallback connectCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.connectionRequestCallback(connectCB);
#endif
}

void IMAPSession::connectionUpgradeRequestCallback(ConnectionUpgradeRequestCallback upgradeCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.connectionUpgradeRequestCallback(upgradeCB);
#endif
}

void IMAPSession::networkConnectionRequestCallback(NetworkConnectionRequestCallback networkConnectionCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.networkConnectionRequestCallback(networkConnectionCB);
#endif
}

void IMAPSession::networkStatusRequestCallback(NetworkStatusRequestCallback networkStatusCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.networkStatusRequestCallback(networkStatusCB);
#endif
}

void IMAPSession::setNetworkStatus(bool status)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.setNetworkStatus(status);
#endif
}

bool IMAPSession::mOpenFolder(MB_StringPtr folderName, bool readOnly)
{
  if (!_tcpConnected)
    return false;
  if (readOnly)
    return openMailbox(folderName, esp_mail_imap_auth_mode::esp_mail_imap_mode_examine, true);
  else
    return openMailbox(folderName, esp_mail_imap_auth_mode::esp_mail_imap_mode_select, true);
}

bool IMAPSession::getFolders(FoldersCollection &folders)
{
  if (!_tcpConnected)
    return false;
  return getMailboxes(folders);
}

bool IMAPSession::mCloseFolder(MB_StringPtr folderName)
{
  if (!_tcpConnected)
    return false;
  return closeMailbox();
}

bool IMAPSession::mListen(bool recon)
{
  // no folder opened or IDLE was not supported
  if (_currentFolder.length() == 0 || !_read_capability.idle)
  {
    _mbif._floderChangedState = false;
    _mbif._folderChanged = false;
    return false;
  }

  if (!MailClient.reconnect(this))
    return false;

  if (!_tcpConnected)
  {
    if (!_sesson_cfg || !_config)
      return false;

    // re-authenticate after session closed
    if (!MailClient.imapAuth(this))
    {
      MailClient.closeTCPSession(this);
      return false;
    }

    // re-open folder
    if (!selectFolder(_currentFolder.c_str()))
      return false;
  }

  if (_mbif._idleTimeMs == 0)
  {
    _mbif._polling_status.messageNum = 0;
    _mbif._polling_status.type = imap_polling_status_type_undefined;
    _mbif._polling_status.argument.clear();
    _mbif._recentCount = 0;
    _mbif._folderChanged = false;

    MB_String s;

    if (!recon)
    {

      if (_readCallback)
      {
        s += esp_mail_str_336;
        s += _currentFolder;
        s += esp_mail_str_337;
        MailClient.imapCB(this, "", false);
        MailClient.imapCB(this, s.c_str(), false);
      }

      if (_debug)
        MailClient.debugInfoP(esp_mail_str_335);
    }

    if (MailClient.imapSendP(this, esp_mail_str_331, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_idle;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
      return false;

    if (!recon)
    {
      if (_readCallback)
      {
        s = esp_mail_str_338;
        MailClient.imapCB(this, s.c_str(), false);
      }

      if (_debug)
      {
        if (MailClient.Time.clockReady() && (_readCallback || _debug))
        {
          s += esp_mail_str_343;
          s += MailClient.Time.getDateTimeString();
        }
        esp_mail_debug(s.c_str());
      }
    }

    _mbif._idleTimeMs = millis();
  }
  else
  {

    if (_mbif._floderChangedState)
    {
      _mbif._floderChangedState = false;
      _mbif._folderChanged = false;
      _mbif._polling_status.messageNum = 0;
      _mbif._polling_status.type = imap_polling_status_type_undefined;
      _mbif._polling_status.argument.clear();
      _mbif._recentCount = 0;
    }

    size_t imap_idle_tmo = _config->limit.imap_idle_timeout;

    if (imap_idle_tmo < 60 * 1000 || imap_idle_tmo > 29 * 60 * 1000)
      imap_idle_tmo = 10 * 60 * 1000;

    size_t host_check_interval = _config->limit.imap_idle_host_check_interval;

    if (host_check_interval < 30 * 1000 || host_check_interval > imap_idle_tmo)
      host_check_interval = 60 * 1000;

    if (millis() - _last_host_check_ms > host_check_interval && _tcpConnected)
    {
      _last_host_check_ms = millis();

      IPAddress ip;

      if (client.hostByName(_sesson_cfg->server.host_name.c_str(), ip) != 1)
      {
        closeSession();
        _mbif._idleTimeMs = millis();
        return false;
      }
    }

    return MailClient.handleIdle(this);
  }

  return true;
}

bool IMAPSession::mStopListen(bool recon)
{
  _mbif._idleTimeMs = 0;
  _mbif._floderChangedState = false;
  _mbif._folderChanged = false;
  _mbif._polling_status.messageNum = 0;
  _mbif._polling_status.type = imap_polling_status_type_undefined;
  _mbif._polling_status.argument.clear();
  _mbif._recentCount = 0;

  if (!_tcpConnected || _currentFolder.length() == 0 || !_read_capability.idle)
    return false;

  if (MailClient.imapSendP(this, esp_mail_str_332, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_done;
  if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
    return false;

  if (!recon)
  {
    if (_readCallback)
    {
      MB_String s = esp_mail_str_340;
      MailClient.imapCB(this, s.c_str(), false);
    }

    if (_debug)
      MailClient.debugInfoP(esp_mail_str_341);
  }

  return true;
}

bool IMAPSession::folderChanged()
{
  return _mbif._floderChangedState;
}

void IMAPSession::checkUID()
{
  if (MailClient.strcmpP(_config->fetch.uid.c_str(), 0, esp_mail_str_140) || MailClient.strcmpP(_config->fetch.uid.c_str(), 0, esp_mail_str_212) ||
      MailClient.strcmpP(_config->fetch.uid.c_str(), 0, esp_mail_str_213) || MailClient.strcmpP(_config->fetch.uid.c_str(), 0, esp_mail_str_214) || MailClient.strcmpP(_config->fetch.uid.c_str(), 0, esp_mail_str_215) ||
      MailClient.strcmpP(_config->fetch.uid.c_str(), 0, esp_mail_str_216) || MailClient.strcmpP(_config->fetch.uid.c_str(), 0, esp_mail_str_217) || MailClient.strcmpP(_config->fetch.uid.c_str(), 0, esp_mail_str_218) ||
      MailClient.strcmpP(_config->fetch.uid.c_str(), 0, esp_mail_str_219) || MailClient.strcmpP(_config->fetch.uid.c_str(), 0, esp_mail_str_220))
    _config->fetch.uid = MBSTRING_FLASH_MCR("*");
}

void IMAPSession::checkPath()
{
  MB_String path = _config->storage.saved_path;
  if (path[0] != '/')
  {
    path = "/";
    path += _config->storage.saved_path;
    path = path.c_str();
  }
}

bool IMAPSession::headerOnly()
{
  return _headerOnly;
}

struct esp_mail_imap_msg_list_t IMAPSession::data()
{
  struct esp_mail_imap_msg_list_t ret;

  for (size_t i = 0; i < _headers.size(); i++)
  {
    if (MailClient.getFreeHeap() < ESP_MAIL_MIN_MEM)
      continue;

    struct esp_mail_imap_msg_item_t itm;

    itm.UID = _headers[i].message_uid;
    itm.msgNo = _headers[i].message_no;
    itm.ID = _headers[i].header_fields.messageID.c_str();
    itm.from = _headers[i].header_fields.from.c_str();
    itm.sender = _headers[i].header_fields.sender.c_str();
    itm.to = _headers[i].header_fields.to.c_str();
    itm.cc = _headers[i].header_fields.cc.c_str();
    itm.subject = _headers[i].header_fields.subject.c_str();
    itm.date = _headers[i].header_fields.date.c_str();
    itm.return_path = _headers[i].header_fields.return_path.c_str();
    itm.reply_to = _headers[i].header_fields.reply_to.c_str();
    itm.in_reply_to = _headers[i].header_fields.in_reply_to.c_str();
    itm.references = _headers[i].header_fields.references.c_str();
    itm.comments = _headers[i].header_fields.comments.c_str();
    itm.keywords = _headers[i].header_fields.keywords.c_str();
    itm.flags = _headers[i].flags.c_str();
    itm.acceptLang = _headers[i].accept_language.c_str();
    itm.contentLang = _headers[i].content_language.c_str();
    itm.hasAttachment = _headers[i].hasAttachment;
    itm.fetchError = _headers[i].error_msg.c_str();

    getMessages(i, itm);

    getRFC822Messages(i, itm);

    ret.msgItems.push_back(itm);
  }

  return ret;
}

SelectedFolderInfo IMAPSession::selectedFolder()
{
  return _mbif;
}

void IMAPSession::callback(imapStatusCallback imapCallback)
{
  _readCallback = imapCallback;
}

void IMAPSession::setSystemTime(time_t ts)
{
  this->client.setSystemTime(ts);
}

void IMAPSession::getMessages(uint16_t messageIndex, struct esp_mail_imap_msg_item_t &msg)
{
  msg.text.content = "";
  msg.text.charSet = "";
  msg.text.content_type = "";
  msg.text.transfer_encoding = "";
  msg.html.content = "";
  msg.html.charSet = "";
  msg.html.content_type = "";
  msg.html.transfer_encoding = "";

  if (messageIndex < _headers.size())
  {
    int sz = _headers[messageIndex].part_headers.size();
    if (sz > 0)
    {
      for (int i = 0; i < sz; i++)
      {
        if (!_headers[messageIndex].part_headers[i].rfc822_part && _headers[messageIndex].part_headers[i].message_sub_type != esp_mail_imap_message_sub_type_rfc822)
        {
          if (_headers[messageIndex].part_headers[i].attach_type == esp_mail_att_type_none)
          {
            if (_headers[messageIndex].part_headers[i].msg_type == esp_mail_msg_type_plain || _headers[messageIndex].part_headers[i].msg_type == esp_mail_msg_type_enriched)
            {
              msg.text.content = _headers[messageIndex].part_headers[i].text.c_str();
              msg.text.charSet = _headers[messageIndex].part_headers[i].charset.c_str();
              msg.text.content_type = _headers[messageIndex].part_headers[i].content_type.c_str();
              msg.text.transfer_encoding = _headers[messageIndex].part_headers[i].content_transfer_encoding.c_str();
            }

            if (_headers[messageIndex].part_headers[i].msg_type == esp_mail_msg_type_html)
            {
              msg.html.content = _headers[messageIndex].part_headers[i].text.c_str();
              msg.html.charSet = _headers[messageIndex].part_headers[i].charset.c_str();
              msg.html.content_type = _headers[messageIndex].part_headers[i].content_type.c_str();
              msg.html.transfer_encoding = _headers[messageIndex].part_headers[i].content_transfer_encoding.c_str();
            }
          }
          else
          {
            struct esp_mail_attachment_info_t att;
            att.filename = _headers[messageIndex].part_headers[i].filename.c_str();
            att.mime = _headers[messageIndex].part_headers[i].content_type.c_str();
            att.name = _headers[messageIndex].part_headers[i].name.c_str();
            att.size = _headers[messageIndex].part_headers[i].attach_data_size;
            att.creationDate = _headers[messageIndex].part_headers[i].creation_date.c_str();
            att.type = _headers[messageIndex].part_headers[i].attach_type;
            msg.attachments.push_back(att);
          }
        }
      }
    }
  }
}

void IMAPSession::getRFC822Messages(uint16_t messageIndex, struct esp_mail_imap_msg_item_t &msg)
{
  if (messageIndex < _headers.size())
  {
    int sz = _headers[messageIndex].part_headers.size();
    int partIdx = 0;
    int cIdx = 0;
    IMAP_MSG_Item *_rfc822 = nullptr;
    if (sz > 0)
    {
      for (int i = 0; i < sz; i++)
      {
        if (_headers[messageIndex].part_headers[i].message_sub_type == esp_mail_imap_message_sub_type_rfc822)
        {
          if (_headers[messageIndex].part_headers[i].rfc822_part)
          {
            if (partIdx > 0)
              msg.rfc822.push_back(*_rfc822);
            cIdx = i;
            partIdx++;
            _rfc822 = new IMAP_MSG_Item();

            _rfc822->from = _headers[messageIndex].part_headers[i].rfc822_header.from.c_str();
            _rfc822->sender = _headers[messageIndex].part_headers[i].rfc822_header.sender.c_str();
            _rfc822->to = _headers[messageIndex].part_headers[i].rfc822_header.to.c_str();
            _rfc822->cc = _headers[messageIndex].part_headers[i].rfc822_header.cc.c_str();
            _rfc822->return_path = _headers[messageIndex].part_headers[i].rfc822_header.return_path.c_str();
            _rfc822->reply_to = _headers[messageIndex].part_headers[i].rfc822_header.reply_to.c_str();
            _rfc822->subject = _headers[messageIndex].part_headers[i].rfc822_header.subject.c_str();
            _rfc822->comments = _headers[messageIndex].part_headers[i].rfc822_header.comments.c_str();
            _rfc822->keywords = _headers[messageIndex].part_headers[i].rfc822_header.keywords.c_str();
            _rfc822->in_reply_to = _headers[messageIndex].part_headers[i].rfc822_header.in_reply_to.c_str();
            _rfc822->references = _headers[messageIndex].part_headers[i].rfc822_header.references.c_str();
            _rfc822->date = _headers[messageIndex].part_headers[i].rfc822_header.date.c_str();
            _rfc822->ID = _headers[messageIndex].part_headers[i].rfc822_header.messageID.c_str();
            _rfc822->flags = _headers[messageIndex].part_headers[i].rfc822_header.flags.c_str();
            _rfc822->text.charSet = "";
            _rfc822->text.content_type = "";
            _rfc822->text.transfer_encoding = "";
            _rfc822->html.charSet = "";
            _rfc822->html.content_type = "";
            _rfc822->html.transfer_encoding = "";
          }
          else
          {
            if (MailClient.multipartMember(_headers[messageIndex].part_headers[cIdx].partNumStr, _headers[messageIndex].part_headers[i].partNumStr))
            {
              if (_headers[messageIndex].part_headers[i].attach_type == esp_mail_att_type_none)
              {
                if (_headers[messageIndex].part_headers[i].msg_type == esp_mail_msg_type_plain || _headers[messageIndex].part_headers[i].msg_type == esp_mail_msg_type_enriched)
                {
                  _rfc822->text.charSet = _headers[messageIndex].part_headers[i].charset.c_str();
                  _rfc822->text.content_type = _headers[messageIndex].part_headers[i].content_type.c_str();
                  _rfc822->text.content = _headers[messageIndex].part_headers[i].text.c_str();
                  _rfc822->text.transfer_encoding = _headers[messageIndex].part_headers[i].content_transfer_encoding.c_str();
                }
                if (_headers[messageIndex].part_headers[i].msg_type == esp_mail_msg_type_html)
                {
                  _rfc822->html.charSet = _headers[messageIndex].part_headers[i].charset.c_str();
                  _rfc822->html.content_type = _headers[messageIndex].part_headers[i].content_type.c_str();
                  _rfc822->html.content = _headers[messageIndex].part_headers[i].text.c_str();
                  _rfc822->html.transfer_encoding = _headers[messageIndex].part_headers[i].content_transfer_encoding.c_str();
                }
              }
              else
              {
                struct esp_mail_attachment_info_t att;
                att.filename = _headers[messageIndex].part_headers[i].filename.c_str();
                att.mime = _headers[messageIndex].part_headers[i].content_type.c_str();
                att.name = _headers[messageIndex].part_headers[i].name.c_str();
                att.size = _headers[messageIndex].part_headers[i].attach_data_size;
                att.creationDate = _headers[messageIndex].part_headers[i].creation_date.c_str();
                att.type = _headers[messageIndex].part_headers[i].attach_type;
                _rfc822->attachments.push_back(att);
              }
            }
          }
        }
      }

      if ((int)msg.rfc822.size() < partIdx && _rfc822 != nullptr)
        msg.rfc822.push_back(*_rfc822);
    }
  }
}

bool IMAPSession::closeMailbox()
{

  if (!MailClient.reconnect(this))
    return false;

  MB_String s;

  if (_readCallback)
  {
    s += esp_mail_str_210;
    s += _currentFolder;
    s += esp_mail_str_96;
    MailClient.imapCB(this, "", false);
    MailClient.imapCB(this, s.c_str(), false);
  }

  if (_debug)
    MailClient.debugInfoP(esp_mail_str_197);

  if (MailClient.imapSendP(this, esp_mail_str_195, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;
  _imap_cmd = esp_mail_imap_cmd_close;
  if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_CLOSE_MAILBOX_FAILED, false))
    return false;

  _currentFolder.clear();
  _mailboxOpened = false;

  return true;
}

bool IMAPSession::openMailbox(MB_StringPtr folder, esp_mail_imap_auth_mode mode, bool waitResponse)
{

  MB_String _folder = folder;

  if (!MailClient.reconnect(this))
    return false;

  if (_folder.length() == 0)
    return false;

  // The SELECT/EXAMINE command automatically deselects any currently selected mailbox
  // before attempting the new selection (RFC3501 p.33)

  // folder should not close for re-selection otherwise the server returned * BAD Command Argument Error. 12

  bool sameFolder = strcmp(_currentFolder.c_str(), _folder.c_str()) == 0;

  // guards 5 seconds to prevent accidently frequently select the same folder with the same mode
  if (_mailboxOpened && sameFolder && millis() - _lastSameFolderOpenMillis < 5000)
  {
    if ((_readOnlyMode && mode == esp_mail_imap_mode_examine) || (!_readOnlyMode && mode == esp_mail_imap_mode_select))
      return true;
  }

  if (!sameFolder)
    _currentFolder = folder;

  MB_String s;
  if (_readCallback)
  {
    s += esp_mail_str_61;
    s += _currentFolder;
    s += esp_mail_str_96;
    MailClient.imapCB(this, "", false);
    MailClient.imapCB(this, s.c_str(), false);
  }

  if (_debug)
    MailClient.debugInfoP(esp_mail_str_248);

  if (mode == esp_mail_imap_mode_examine)
  {
    s = esp_mail_str_135;
    _imap_cmd = esp_mail_imap_cmd_examine;
  }
  else if (mode == esp_mail_imap_mode_select)
  {
    s = esp_mail_str_247;
    _imap_cmd = esp_mail_imap_cmd_select;
  }
  s += _currentFolder;
  s += esp_mail_str_136;
  if (MailClient.imapSend(this, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  _lastSameFolderOpenMillis = millis();

  if (waitResponse)
  {

    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_OPEN_MAILBOX_FAILED, false))
      return false;
  }

  if (mode == esp_mail_imap_mode_examine)
    _readOnlyMode = true;
  else if (mode == esp_mail_imap_mode_select)
    _readOnlyMode = false;

  _mailboxOpened = true;

  return true;
}

bool IMAPSession::getMailboxes(FoldersCollection &folders)
{
  _folders.clear();

  if (_readCallback)
  {
    MailClient.imapCB(this, "", false);
    MailClient.imapCBP(this, esp_mail_str_58, false);
  }

  if (_debug)
    MailClient.debugInfoP(esp_mail_str_230);

  if (MailClient.imapSendP(this, esp_mail_str_133, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;
  _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_list;
  if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_LIST_MAILBOXS_FAILED, false))
    return false;

  folders = _folders;
  return true;
}

bool IMAPSession::checkCapability()
{
  if (_readCallback)
  {
    MailClient.imapCB(this, "", false);
    MailClient.imapCBP(this, esp_mail_str_64, false);
  }

  if (_debug)
    MailClient.debugInfoP(esp_mail_str_65);

  if (MailClient.imapSendP(this, esp_mail_str_2, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_capability;
  if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_CHECK_CAPABILITIES_FAILED, false))
    return false;

  return true;
}

bool IMAPSession::mCreateFolder(MB_StringPtr folderName)
{
  if (_debug)
  {
    esp_mail_debug("");
    MailClient.debugInfoP(esp_mail_str_320);
  }

  MB_String cmd = esp_mail_str_322;
  cmd += folderName;

  if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_create;
  if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
    return false;

  return true;
}

int IMAPSession::getUID(int msgNum)
{
  if (_currentFolder.length() == 0)
    return 0;

  MB_String cmd = esp_mail_str_143;
  cmd += msgNum;
  cmd += esp_mail_str_138;

  MB_String s;

  if (_readCallback)
  {
    s += esp_mail_str_156;
    MailClient.imapCB(this, s.c_str(), false);
  }

  if (_debug)
    MailClient.debugInfoP(esp_mail_str_189);

  if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return 0;

  _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_get_uid;
  if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
    return 0;

  if (_readCallback || _debug)
  {

    if (_readCallback)
    {
      s = esp_mail_str_274;
      s += _uid_tmp;
      MailClient.imapCB(this, s.c_str(), false);
    }

    if (_debug)
    {
      s = esp_mail_str_111;
      s += _uid_tmp;
      esp_mail_debug(s.c_str());
    }
  }

  return _uid_tmp;
}

const char *IMAPSession::getFlags(int msgNum)
{
  _flags_tmp.clear();
  if (_currentFolder.length() == 0)
    return _flags_tmp.c_str();

  MB_String cmd = esp_mail_str_143;
  cmd += msgNum;
  cmd += esp_mail_str_273;

  MB_String s;

  if (_readCallback)
  {
    s = esp_mail_str_279;
    MailClient.imapCB(this, s.c_str(), false);
  }

  if (_debug)
    MailClient.debugInfoP(esp_mail_str_105);

  if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return _flags_tmp.c_str();

  _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_get_flags;
  MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false);

  return _flags_tmp.c_str();
}

bool IMAPSession::mSendCustomCommand(MB_StringPtr cmd, imapResponseCallback callback)
{
  if (_currentFolder.length() == 0)
    return false;

  _customCmdResCallback = callback;

  MB_String _cmd = cmd;

  if (MailClient.imapSend(this, _cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_custom;
  if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
    return false;

  return true;
}

bool IMAPSession::mDeleteFolder(MB_StringPtr folderName)
{
  if (_debug)
  {
    esp_mail_debug("");
    MailClient.debugInfoP(esp_mail_str_321);
  }

  MB_String cmd = esp_mail_str_323;
  cmd += folderName;

  if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_delete;
  if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
    return false;

  return true;
}

bool IMAPSession::deleteMessages(MessageList *toDelete, bool expunge)
{
  if (toDelete->_list.size() > 0)
  {

    if (!selectFolder(_currentFolder.c_str(), false))
      return false;

    if (_debug)
    {
      esp_mail_debug("");
      MailClient.debugInfoP(esp_mail_str_316);
    }

    MB_String cmd = esp_mail_str_249;
    for (size_t i = 0; i < toDelete->_list.size(); i++)
    {
      if (i > 0)
        cmd += esp_mail_str_263;
      cmd += toDelete->_list[i];
    }
    cmd += esp_mail_str_315;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_store;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
      return false;

    if (expunge)
    {
      if (MailClient.imapSendP(this, esp_mail_str_317, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

      _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_expunge;
      if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;
    }
  }

  return true;
}

bool IMAPSession::mCopyMessages(MessageList *toCopy, MB_StringPtr dest)
{
  if (toCopy->_list.size() > 0)
  {

    if (!selectFolder(_currentFolder.c_str(), false))
      return false;

    if (_debug)
    {
      MB_String s = esp_mail_str_318;
      s += dest;
      esp_mail_debug(s.c_str());
    }

    MB_String cmd = esp_mail_str_319;
    for (size_t i = 0; i < toCopy->_list.size(); i++)
    {
      if (i > 0)
        cmd += esp_mail_str_263;
      cmd += toCopy->_list[i];
    }
    cmd += esp_mail_str_131;
    cmd += dest;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_store;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
      return false;
  }

  return true;
}

void IMAPSession::empty()
{
  _nextUID.clear();
  _sdFileList.clear();
  clearMessageData();
}

String IMAPSession::fileList()
{
  return _sdFileList.c_str();
}

void IMAPSession::clearMessageData()
{
  for (size_t i = 0; i < _headers.size(); i++)
  {
    _headers[i].part_headers.clear();
  }
  _headers.clear();
  _msgUID.clear();
  _folders.clear();
  _mbif._flags.clear();
  _mbif._searchCount = 0;
  _flags_tmp.clear();
}

IMAP_Status::IMAP_Status()
{
}
IMAP_Status::~IMAP_Status()
{
  empty();
}

const char *IMAP_Status::info()
{
  return _info.c_str();
}

bool IMAP_Status::success()
{
  return _success;
}

void IMAP_Status::empty()
{
  _info.clear();
}

#endif

#if defined(ENABLE_SMTP)

int ESP_Mail_Client::readLine(SMTPSession *smtp, char *buf, int bufLen, bool crlf, int &count)
{
  int ret = -1;
  char c = 0;
  char _c = 0;
  int idx = 0;
  if (!smtp->client.connected())
    return idx;
  while (smtp->client.available() && idx < bufLen)
  {
    ret = smtp->client.read();
    if (ret > -1)
    {
      if (idx >= bufLen - 1)
        return idx;

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
    }
    if (!smtp->client.connected())
      return idx;
  }
  return idx;
}

void ESP_Mail_Client::mimeFromFile(const char *name, MB_String &mime)
{
  MB_String ext = name;
  size_t p = ext.find_last_of(".");
  if (p != MB_String::npos)
  {
    ext = ext.substr(p, ext.length() - p);
    if (ext.length() > 0)
      getMIME(ext.c_str(), mime);
  }
}

void ESP_Mail_Client::getMIME(const char *ext, MB_String &mime)
{
  mime.clear();
  for (int i = 0; i < esp_mail_file_extension_maxType; i++)
  {
    if (strcmp_P(ext, mimeinfo[i].endsWith) == 0)
    {
      mime += mimeinfo[i].mimeType;
      break;
    }
  }
}

MB_String ESP_Mail_Client::getEncodedToken(SMTPSession *smtp)
{
  MB_String raw = esp_mail_str_285;
  raw += smtp->_sesson_cfg->login.email;
  raw += esp_mail_str_286;
  raw += smtp->_sesson_cfg->login.accessToken;
  raw += esp_mail_str_287;
  return encodeBase64Str((const unsigned char *)raw.c_str(), raw.length());
}

bool ESP_Mail_Client::smtpAuth(SMTPSession *smtp)
{

  if (!reconnect(smtp))
    return false;

  bool ssl = false;
  smtp->_secure = true;
  bool secureMode = true;

  MB_String s;

#if defined(ESP32) && defined(ESP32_TCP_CLIENT)
  smtp->client.setDebugCallback(NULL);
#elif defined(ESP8266) && defined(ESP8266_TCP_CLIENT)
  smtp->client.rxBufDivider = 16; // minimum rx buffer for smtp status response
  smtp->client.txBufDivider = 8;  // medium tx buffer for faster attachment/inline data transfer
#endif

  if (smtp->_sesson_cfg->server.port == esp_mail_smtp_port_25)
  {
    smtp->_secure = false;
    secureMode = false;
  }
  else
  {
    if (smtp->_sesson_cfg->server.port == esp_mail_smtp_port_587)
      smtp->_sesson_cfg->secure.startTLS = true;

    secureMode = !smtp->_sesson_cfg->secure.startTLS;

    // to prevent to send the connection upgrade command when some server promotes
    // the starttls capability even the current connection was already secured.
    if (smtp->_sesson_cfg->server.port == esp_mail_smtp_port_465)
      ssl = true;
  }

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)
  setSecure(smtp->client, smtp->_sesson_cfg, smtp->_caCert);
#endif

  // Server connection attempt: no status code
  if (smtp->_sendCallback)
    smtpCBP(smtp, esp_mail_str_120);

  if (smtp->_debug)
  {
    s = esp_mail_str_314;
    s += ESP_MAIL_VERSION;
    s += smtp->client.fwVersion();
    esp_mail_debug(s.c_str());

#if defined(BOARD_HAS_PSRAM) && defined(MB_STRING_USE_PSRAM)
    if (ESP.getPsramSize() == 0)
    {
      s = esp_mail_str_353;
      esp_mail_debug(s.c_str());
    }
#endif

    debugInfoP(esp_mail_str_236);
    s = esp_mail_str_261;
    s += esp_mail_str_211;
    s += smtp->_sesson_cfg->server.host_name;
    esp_mail_debug(s.c_str());
    s = esp_mail_str_261;
    s += esp_mail_str_201;
    s += smtp->_sesson_cfg->server.port;
    esp_mail_debug(s.c_str());
  }
#if defined(ESP32) && defined(ESP32_TCP_CLIENT)
  if (smtp->_debug)
    smtp->client.setDebugCallback(esp_mail_debug);
#endif

  smtp->client.begin(smtp->_sesson_cfg->server.host_name.c_str(), smtp->_sesson_cfg->server.port);

  smtp->client.ethDNSWorkAround();

  if (!smtp->client.connect(secureMode, smtp->_sesson_cfg->certificate.verify))
    return handleSMTPError(smtp, SMTP_STATUS_SERVER_CONNECT_FAILED);

  // server connected
  smtp->_tcpConnected = true;

  if (smtp->_debug)
    debugInfoP(esp_mail_str_238);

  if (smtp->_sendCallback)
  {
    smtpCB(smtp, "");
    smtpCBP(smtp, esp_mail_str_121);
  }

  smtp->client.setTimeout(TCP_CLIENT_DEFAULT_TCP_TIMEOUT_SEC);

  smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_initial_state;

  // expected status code 220 for ready to service
  if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_220, SMTP_STATUS_SMTP_GREETING_GET_RESPONSE_FAILED))
    return false;

init:

  // Sending greeting hello response
  if (smtp->_sendCallback)
  {
    smtpCB(smtp, "");
    smtpCBP(smtp, esp_mail_str_122);
  }

  if (smtp->_debug)
    debugInfoP(esp_mail_str_239);

  s = esp_mail_str_6;
  if (smtp->_sesson_cfg->login.user_domain.length() > 0)
    s += smtp->_sesson_cfg->login.user_domain;
  else
    s += esp_mail_str_44;

  if (smtpSendP(smtp, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_greeting;

  if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_250, 0))
  {
    s = esp_mail_str_5;
    if (smtp->_sesson_cfg->login.user_domain.length() > 0)
      s += smtp->_sesson_cfg->login.user_domain;
    else
      s += esp_mail_str_44;

    if (smtpSend(smtp, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;
    if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_250, SMTP_STATUS_SMTP_GREETING_SEND_ACK_FAILED))
      return false;
    smtp->_send_capability.esmtp = false;
    smtp->_auth_capability.login = true;
  }
  else
    smtp->_send_capability.esmtp = true;

  // start TLS when needed
  if ((smtp->_auth_capability.start_tls || smtp->_sesson_cfg->secure.startTLS) && !ssl)
  {
    // send starttls command
    if (smtp->_sendCallback)
    {
      smtpCB(smtp, "");
      smtpCBP(smtp, esp_mail_str_209);
    }

    if (smtp->_debug)
    {
      s = esp_mail_str_196;
      esp_mail_debug(s.c_str());
    }

    // expected status code 250 for complete the request
    // some server returns 220 to restart to initial state
    smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_start_tls;
    smtpSendP(smtp, esp_mail_str_311, false);
    if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_250, SMTP_STATUS_SMTP_GREETING_SEND_ACK_FAILED))
      return false;

    if (smtp->_debug)
    {
      debugInfoP(esp_mail_str_310);
    }

    // connect in secure mode
    // do ssl handshake
    if (!smtp->client.connectSSL(smtp->_sesson_cfg->certificate.verify))
      return handleSMTPError(smtp, MAIL_CLIENT_ERROR_SSL_TLS_STRUCTURE_SETUP);

    // set the secure mode
    smtp->_sesson_cfg->secure.startTLS = false;
    ssl = true;
    smtp->_secure = true;

    // return to initial state if the response status is 220.
    if (smtp->_smtpStatus.respCode == esp_mail_smtp_status_code_220)
      goto init;
  }

  bool creds = smtp->_sesson_cfg->login.email.length() > 0 && smtp->_sesson_cfg->login.password.length() > 0;
  bool xoauth_auth = smtp->_sesson_cfg->login.accessToken.length() > 0 && smtp->_auth_capability.xoauth2;
  bool login_auth = smtp->_auth_capability.login && creds;
  bool plain_auth = smtp->_auth_capability.plain && creds;

  if (xoauth_auth || login_auth || plain_auth)
  {
    if (smtp->_sendCallback)
    {
      smtpCB(smtp, "", false);
      smtpCBP(smtp, esp_mail_str_56, false);
    }

    // log in
    if (xoauth_auth)
    {
      if (smtp->_debug)
        debugInfoP(esp_mail_str_288);

      if (!smtp->_auth_capability.xoauth2)
        return handleSMTPError(smtp, SMTP_STATUS_SERVER_OAUTH2_LOGIN_DISABLED, false);

      if (smtpSendP(smtp, esp_mail_str_289, false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

      if (smtpSend(smtp, getEncodedToken(smtp).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

      smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_auth;
      if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_235, SMTP_STATUS_AUTHEN_FAILED))
        return false;

      return true;
    }
    else if (plain_auth)
    {

      if (smtp->_debug)
        debugInfoP(esp_mail_str_241);

      if (smtp->_debug)
      {
        s = esp_mail_str_261;
        s += smtp->_sesson_cfg->login.email;
        esp_mail_debug(s.c_str());

        s += esp_mail_str_131;
        for (size_t i = 0; i < smtp->_sesson_cfg->login.password.length(); i++)
          s += esp_mail_str_183;
        esp_mail_debug(s.c_str());
      }

      // rfc4616
      int len = smtp->_sesson_cfg->login.email.length() + smtp->_sesson_cfg->login.password.length() + 2;
      uint8_t *tmp = (uint8_t *)newP(len);
      memset(tmp, 0, len);
      int p = 1;
      memcpy(tmp + p, smtp->_sesson_cfg->login.email.c_str(), smtp->_sesson_cfg->login.email.length());
      p += smtp->_sesson_cfg->login.email.length() + 1;
      memcpy(tmp + p, smtp->_sesson_cfg->login.password.c_str(), smtp->_sesson_cfg->login.password.length());
      p += smtp->_sesson_cfg->login.password.length();

      MB_String s = esp_mail_str_45;
      s += esp_mail_str_131;
      s += encodeBase64Str(tmp, p);
      delP(&tmp);

      if (smtpSend(smtp, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

      smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_auth_plain;
      if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_235, SMTP_STATUS_USER_LOGIN_FAILED))
        return false;

      return true;
    }
    else if (login_auth)
    {
      if (smtp->_debug)
        debugInfoP(esp_mail_str_240);

      if (smtpSendP(smtp, esp_mail_str_4, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

      if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_334, SMTP_STATUS_AUTHEN_FAILED))
        return false;

      if (smtp->_debug)
      {
        s = esp_mail_str_261;
        s += smtp->_sesson_cfg->login.email;
        esp_mail_debug(s.c_str());
      }

      if (smtpSend(smtp, encodeBase64Str((const unsigned char *)smtp->_sesson_cfg->login.email.c_str(), smtp->_sesson_cfg->login.email.length()).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

      smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_login_user;
      if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_334, SMTP_STATUS_USER_LOGIN_FAILED))
        return false;

      if (smtp->_debug)
      {
        s = esp_mail_str_261;
        for (size_t i = 0; i < smtp->_sesson_cfg->login.password.length(); i++)
          s += esp_mail_str_183;
        esp_mail_debug(s.c_str());
      }

      if (smtpSend(smtp, encodeBase64Str((const unsigned char *)smtp->_sesson_cfg->login.password.c_str(), smtp->_sesson_cfg->login.password.length()).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

      smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_login_psw;
      if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_235, SMTP_STATUS_PASSWORD_LOGIN_FAILED))
        return false;

      return true;
    }
  }

  return true;
}

bool ESP_Mail_Client::connected(SMTPSession *smtp)
{
  return smtp->client.connected();
}

bool ESP_Mail_Client::setSendingResult(SMTPSession *smtp, SMTP_Message *msg, bool result)
{
  if (result)
    smtp->_sentSuccessCount++;
  else
    smtp->_sentFailedCount++;

  if (smtp->_sendCallback)
  {
    SMTP_Result status;
    status.completed = result;
    smtp->client.setSystemTime(Time.getCurrentTimestamp());
    status.timestamp = smtp->client.getTime();
    status.subject = msg->subject.c_str();
    status.recipients = msg->_rcp[0].email.c_str();

    smtp->sendingResult.add(&status);

    smtp->_cbData._sentSuccess = smtp->_sentSuccessCount;
    smtp->_cbData._sentFailed = smtp->_sentFailedCount;
  }

  return result;
}

bool ESP_Mail_Client::sendMail(SMTPSession *smtp, SMTP_Message *msg, bool closeSession)
{

  if (msg->html.content.length() > 0 || strlen(msg->html.nonCopyContent) > 0 || msg->html.blob.size > 0 || msg->html.file.name.length() > 0)
    msg->type |= esp_mail_msg_type_html;

  if (msg->text.content.length() > 0 || strlen(msg->text.nonCopyContent) > 0 || msg->text.blob.size > 0 || msg->text.file.name.length() > 0)
    msg->type |= esp_mail_msg_type_plain;

  for (size_t i = 0; i < msg->_rfc822.size(); i++)
  {
    if (msg->_rfc822[i].html.content.length() > 0)
      msg->_rfc822[i].type |= esp_mail_msg_type_html;

    if (msg->_rfc822[i].text.content.length() > 0)
      msg->_rfc822[i].type |= esp_mail_msg_type_plain;
  }

  return mSendMail(smtp, msg, closeSession);
}

size_t ESP_Mail_Client::numAtt(SMTPSession *smtp, esp_mail_attach_type type, SMTP_Message *msg)
{
  size_t count = 0;
  for (size_t i = 0; i < msg->_att.size(); i++)
  {
    if (msg->_att[i]._int.att_type == type)
      count++;
  }
  return count;
}

bool ESP_Mail_Client::checkEmail(SMTPSession *smtp, SMTP_Message *msg)
{
  bool validRecipient = false;

  if (!validEmail(msg->sender.email.c_str()))
  {
    errorStatusCB(smtp, SMTP_STATUS_NO_VALID_SENDER_EXISTED);
    return setSendingResult(smtp, msg, false);
  }

  for (uint8_t i = 0; i < msg->_rcp.size(); i++)
  {
    if (validEmail(msg->_rcp[i].email.c_str()))
      validRecipient = true;
  }

  if (!validRecipient)
  {
    errorStatusCB(smtp, SMTP_STATUS_NO_VALID_RECIPIENTS_EXISTED);
    return setSendingResult(smtp, msg, false);
  }

  return true;
}

bool ESP_Mail_Client::mSendMail(SMTPSession *smtp, SMTP_Message *msg, bool closeSession)
{

  smtp->_smtpStatus.statusCode = 0;
  smtp->_smtpStatus.respCode = 0;
  smtp->_smtpStatus.text.clear();
  bool rfc822MSG = false;

  if (!checkEmail(smtp, msg))
    return false;

  smtp->_chunkedEnable = false;
  smtp->_chunkCount = 0;

  // new session
  if (!smtp->_tcpConnected)
  {
    if (!smtpAuth(smtp))
    {
      closeTCPSession(smtp);
      return setSendingResult(smtp, msg, false);
    }
    smtp->_sentSuccessCount = 0;
    smtp->_sentFailedCount = 0;
    smtp->sendingResult.clear();
  }
  else
  {
    // reuse session
    if (smtp->_sendCallback)
    {
      smtpCB(smtp, "");
      if (smtp->_sentSuccessCount || smtp->_sentFailedCount)
        smtpCBP(smtp, esp_mail_str_267);
      else
        smtpCBP(smtp, esp_mail_str_208);
    }

    if (smtp->_debug)
    {
      if (smtp->_sentSuccessCount || smtp->_sentFailedCount)
        debugInfoP(esp_mail_str_268);
      else
        debugInfoP(esp_mail_str_207);
    }
  }

  if (smtp->_sendCallback)
  {
    smtpCB(smtp, "");
    smtpCBP(smtp, esp_mail_str_125);
  }

  if (smtp->_debug)
    debugInfoP(esp_mail_str_242);

  MB_String buf;
  MB_String buf2;
  checkBinaryData(smtp, msg);

  if (msg->priority >= esp_mail_smtp_priority_high && msg->priority <= esp_mail_smtp_priority_low)
  {
    buf2 += esp_mail_str_17;
    buf2 += (int)msg->priority;
    buf2 += esp_mail_str_34;

    if (msg->priority == esp_mail_smtp_priority_high)
    {
      buf2 += esp_mail_str_18;
      buf2 += esp_mail_str_21;
    }
    else if (msg->priority == esp_mail_smtp_priority_normal)
    {
      buf2 += esp_mail_str_19;
      buf2 += esp_mail_str_22;
    }
    else if (msg->priority == esp_mail_smtp_priority_low)
    {
      buf2 += esp_mail_str_20;
      buf2 += esp_mail_str_23;
    }
  }

  buf2 += esp_mail_str_10;
  buf2 += esp_mail_str_131;

  if (msg->sender.name.length() > 0)
    buf2 += msg->sender.name;

  buf2 += esp_mail_str_14;
  buf2 += msg->sender.email;
  buf2 += esp_mail_str_15;
  buf2 += esp_mail_str_34;

  buf = esp_mail_str_8;
  buf += esp_mail_str_14;
  buf += msg->sender.email;
  buf += esp_mail_str_15;

  if (smtp->_send_capability.binaryMIME && smtp->_send_capability.chunking && msg->enable.chunking && (msg->text._int.binary || msg->html._int.binary))
    buf += esp_mail_str_104;

  if (smtpSend(smtp, buf.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return setSendingResult(smtp, msg, false);

  smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_send_header_sender;
  if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_SENDER_FAILED))
    return setSendingResult(smtp, msg, false);

  for (uint8_t i = 0; i < msg->_rcp.size(); i++)
  {
    if (i == 0)
    {
      buf2 += esp_mail_str_11;
      buf2 += esp_mail_str_131;
      if (msg->_rcp[i].name.length() > 0)
        buf2 += msg->_rcp[i].name;

      buf2 += esp_mail_str_14;
      buf2 += msg->_rcp[i].email;
      buf2 += esp_mail_str_15;
    }
    else
    {
      if (msg->_rcp[i].name.length() > 0)
      {
        buf2 += esp_mail_str_263;
        buf2 += msg->_rcp[i].name;
        buf2 += esp_mail_str_14;
      }
      else
        buf2 += esp_mail_str_13;
      buf2 += msg->_rcp[i].email;
      buf2 += esp_mail_str_15;
    }

    if (i == msg->_rcp.size() - 1)
      buf2 += esp_mail_str_34;

    buf.clear();
    // only address
    buf += esp_mail_str_9;
    buf += esp_mail_str_14;
    buf += msg->_rcp[i].email;
    buf += esp_mail_str_15;

    // rfc3461, rfc3464
    if (smtp->_send_capability.dsn)
    {
      if (msg->response.notify != esp_mail_smtp_notify::esp_mail_smtp_notify_never)
      {
        buf += esp_mail_str_262;
        int opcnt = 0;
        if (msg->response.notify == esp_mail_smtp_notify::esp_mail_smtp_notify_success)
        {
          if (opcnt > 0)
            buf += esp_mail_str_263;
          buf += esp_mail_str_264;
          opcnt++;
        }
        if (msg->response.notify == esp_mail_smtp_notify::esp_mail_smtp_notify_failure)
        {
          if (opcnt > 0)
            buf += esp_mail_str_263;
          buf += esp_mail_str_265;
          opcnt++;
        }
        if (msg->response.notify == esp_mail_smtp_notify::esp_mail_smtp_notify_delay)
        {
          if (opcnt > 0)
            buf += esp_mail_str_263;
          buf += esp_mail_str_266;
          opcnt++;
        }
      }
    }

    if (smtpSend(smtp, buf.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return setSendingResult(smtp, msg, false);

    smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_send_header_recipient;
    if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_RECIPIENT_FAILED))
      return setSendingResult(smtp, msg, false);
  }

  for (uint8_t i = 0; i < msg->_cc.size(); i++)
  {
    if (i == 0)
    {
      buf2 += esp_mail_str_12;
      buf2 += esp_mail_str_131;
      buf2 += esp_mail_str_14;
      buf2 += msg->_cc[i].email;
      buf2 += esp_mail_str_15;
    }
    else
    {
      buf2 += esp_mail_str_13;
      buf2 += msg->_cc[i].email;
      buf2 += esp_mail_str_15;
    }

    if (i == msg->_cc.size() - 1)
      buf2 += esp_mail_str_34;

    buf.clear();

    buf += esp_mail_str_9;
    buf += esp_mail_str_14;
    buf += msg->_cc[i].email;
    buf += esp_mail_str_15;

    if (smtpSend(smtp, buf.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return setSendingResult(smtp, msg, false);

    smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_send_header_recipient;
    if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_RECIPIENT_FAILED))
      return setSendingResult(smtp, msg, false);
  }

  for (uint8_t i = 0; i < msg->_bcc.size(); i++)
  {
    buf = esp_mail_str_9;
    buf += esp_mail_str_14;
    buf += msg->_bcc[i].email;
    buf += esp_mail_str_15;
    if (smtpSend(smtp, buf.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return setSendingResult(smtp, msg, false);
    smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_send_header_recipient;
    if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_RECIPIENT_FAILED))
      return setSendingResult(smtp, msg, false);
  }

  if (smtp->_sendCallback)
  {
    smtpCB(smtp, "");
    smtpCBP(smtp, esp_mail_str_126);
  }

  if (smtp->_debug)
    debugInfoP(esp_mail_str_243);

  if (smtp->_send_capability.chunking && msg->enable.chunking)
  {
    smtp->_chunkedEnable = true;
    if (!bdat(smtp, msg, buf2.length(), false))
      return false;
  }
  else
  {
    if (smtpSendP(smtp, esp_mail_str_16, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return setSendingResult(smtp, msg, false);

    smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_send_body;
    if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_354, SMTP_STATUS_SEND_BODY_FAILED))
      return setSendingResult(smtp, msg, false);
  }

  if (smtpSend(smtp, buf2.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return setSendingResult(smtp, msg, false);

  MB_String s = esp_mail_str_24;
  s += esp_mail_str_131;
  s += msg->subject;
  s += esp_mail_str_34;

  bool dateHdr = false;

  if (msg->_hdr.size() > 0)
  {
    for (uint8_t k = 0; k < msg->_hdr.size(); k++)
    {
      s += msg->_hdr[k];
      s += esp_mail_str_34;

      if (strcmpP(msg->_hdr[k].c_str(), 0, esp_mail_str_99, false))
        dateHdr = true;
    }
  }

  if (!dateHdr)
  {
    smtp->client.setSystemTime(Time.getCurrentTimestamp());
    time_t now = smtp->client.getTime();
    if (now > ESP_TIME_DEFAULT_TS)
    {
      s += esp_mail_str_99;
      s += esp_mail_str_131;
      s += Time.getDateTimeString();
      s += esp_mail_str_34;
    }
  }

  if (msg->response.reply_to.length() > 0)
  {
    s += esp_mail_str_184;
    s += esp_mail_str_131;
    s += esp_mail_str_14;
    s += msg->response.reply_to;
    s += esp_mail_str_15;
    s += esp_mail_str_34;
  }

  if (msg->response.return_path.length() > 0)
  {
    s += esp_mail_str_46;
    s += esp_mail_str_131;
    s += esp_mail_str_14;
    s += msg->response.return_path;
    s += esp_mail_str_15;
    s += esp_mail_str_34;
  }

  if (msg->in_reply_to.length() > 0)
  {
    s += esp_mail_str_109;
    s += esp_mail_str_131;
    s += msg->in_reply_to;
    s += esp_mail_str_34;
  }

  if (msg->references.length() > 0)
  {
    s += esp_mail_str_107;
    s += esp_mail_str_131;
    s += msg->references;
    s += esp_mail_str_34;
  }

  if (msg->comments.length() > 0)
  {
    s += esp_mail_str_134;
    s += esp_mail_str_131;
    s += msg->comments;
    s += esp_mail_str_34;
  }

  if (msg->keywords.length() > 0)
  {
    s += esp_mail_str_145;
    s += esp_mail_str_131;
    s += msg->keywords;
    s += esp_mail_str_34;
  }

  if (msg->messageID.length() > 0)
  {
    s += esp_mail_str_101;
    s += esp_mail_str_131;
    s += esp_mail_str_14;
    s += msg->messageID;
    s += esp_mail_str_15;
    s += esp_mail_str_34;
  }

  s += esp_mail_str_3;

  if (!bdat(smtp, msg, s.length(), false))
    return false;

  if (smtpSend(smtp, s.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return setSendingResult(smtp, msg, false);

  return sendMSGData(smtp, msg, closeSession, rfc822MSG);
}

bool ESP_Mail_Client::sendMSGData(SMTPSession *smtp, SMTP_Message *msg, bool closeSession, bool rfc822MSG)
{
  MB_String s;
  MB_String mixed = getBoundary(15);
  MB_String alt = getBoundary(15);

  if (numAtt(smtp, esp_mail_att_type_attachment, msg) == 0 && msg->_parallel.size() == 0 && msg->_rfc822.size() == 0)
  {
    if (msg->type == (esp_mail_msg_type_plain | esp_mail_msg_type_html | esp_mail_msg_type_enriched) || numAtt(smtp, esp_mail_att_type_inline, msg) > 0)
    {
      if (!sendMSG(smtp, msg, alt))
        return setSendingResult(smtp, msg, false);
    }
    else if (msg->type != esp_mail_msg_type_none)
    {
      if (!sendPartText(smtp, msg, msg->type, ""))
        return setSendingResult(smtp, msg, false);
    }
  }
  else
  {
    s = esp_mail_str_1;
    s += mixed;
    s += esp_mail_str_35;

    s += esp_mail_str_33;
    s += mixed;
    s += esp_mail_str_34;

    if (!bdat(smtp, msg, s.length(), false))
      return false;

    if (smtpSend(smtp, s.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return setSendingResult(smtp, msg, false);

    if (!sendMSG(smtp, msg, alt))
      return setSendingResult(smtp, msg, false);

    if (!bdat(smtp, msg, 2, false))
      return false;

    if (smtpSendP(smtp, esp_mail_str_34, false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return setSendingResult(smtp, msg, false);

    if (smtp->_sendCallback)
    {
      smtpCB(smtp, "");
      smtpCBP(smtp, esp_mail_str_127);
    }

    if (smtp->_debug)
      debugInfoP(esp_mail_str_244);

    if (smtp->_sendCallback && numAtt(smtp, esp_mail_att_type_attachment, msg) > 0)
      esp_mail_debug("");

    if (!sendAttachments(smtp, msg, mixed))
      return setSendingResult(smtp, msg, false);

    if (!sendParallelAttachments(smtp, msg, mixed))
      return setSendingResult(smtp, msg, false);

    if (!sendRFC822Msg(smtp, msg, mixed, closeSession, msg->_rfc822.size() > 0))
      return setSendingResult(smtp, msg, false);

    s = esp_mail_str_33;
    s += mixed;
    s += esp_mail_str_33;

    if (!bdat(smtp, msg, s.length(), false))
      return false;

    if (smtpSend(smtp, s.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return setSendingResult(smtp, msg, false);
  }

  if (!rfc822MSG)
  {
    if (smtp->_sendCallback)
    {
      smtpCB(smtp, "");
      smtpCBP(smtp, esp_mail_str_303);
    }

    if (smtp->_debug)
      debugInfoP(esp_mail_str_304);

    if (smtp->_chunkedEnable)
    {

      if (!bdat(smtp, msg, 0, true))
        return false;

      smtp->_smtp_cmd = esp_mail_smtp_cmd_chunk_termination;
      if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_BODY_FAILED))
        return false;
    }
    else
    {
      if (smtpSendP(smtp, esp_mail_str_37, false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return setSendingResult(smtp, msg, false);

      smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_send_body;
      if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_BODY_FAILED))
        return setSendingResult(smtp, msg, false);
    }

    setSendingResult(smtp, msg, true);

    if (closeSession)
      if (!smtp->closeSession())
        return false;
  }

  return true;
}

bool ESP_Mail_Client::sendRFC822Msg(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary, bool closeSession, bool rfc822MSG)
{
  if (msg->_rfc822.size() == 0)
    return true;
  MB_String buf;
  for (uint8_t i = 0; i < msg->_rfc822.size(); i++)
  {
    buf.clear();
    getRFC822PartHeader(smtp, buf, boundary);

    getRFC822MsgEnvelope(smtp, &msg->_rfc822[i], buf);

    if (!bdat(smtp, msg, buf.length(), false))
      return false;

    if (smtpSend(smtp, buf.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    if (!sendMSGData(smtp, &msg->_rfc822[i], closeSession, rfc822MSG))
      return false;
  }

  return true;
}

void ESP_Mail_Client::getRFC822MsgEnvelope(SMTPSession *smtp, SMTP_Message *msg, MB_String &buf)
{
  if (msg->date.length() > 0)
  {
    buf += esp_mail_str_99;
    buf += msg->date;
    buf += esp_mail_str_34;
  }
  else
  {
    smtp->client.setSystemTime(Time.getCurrentTimestamp());
    time_t now = smtp->client.getTime();
    if (now > ESP_TIME_DEFAULT_TS)
    {
      buf += esp_mail_str_99;
      buf += esp_mail_str_131;
      buf += Time.getDateTimeString();
      buf += esp_mail_str_34;
    }
  }

  if (msg->from.email.length() > 0)
  {
    buf += esp_mail_str_10;
    buf += esp_mail_str_131;

    if (msg->from.name.length() > 0)
      buf += msg->from.name;

    buf += esp_mail_str_14;
    buf += msg->from.email;
    buf += esp_mail_str_15;
    buf += esp_mail_str_34;
  }

  if (msg->sender.email.length() > 0)
  {
    buf += esp_mail_str_150;
    buf += esp_mail_str_131;

    if (msg->sender.name.length() > 0)
      buf += msg->sender.name;

    buf += esp_mail_str_14;
    buf += msg->sender.email;
    buf += esp_mail_str_15;
    buf += esp_mail_str_34;
  }

  if (msg->response.reply_to.length() > 0)
  {
    buf += esp_mail_str_184;
    buf += esp_mail_str_131;
    buf += esp_mail_str_14;
    buf += msg->response.reply_to;
    buf += esp_mail_str_15;
    buf += esp_mail_str_34;
  }

  if (msg->response.return_path.length() > 0)
  {
    buf += esp_mail_str_46;
    buf += esp_mail_str_131;
    buf += esp_mail_str_14;
    buf += msg->response.return_path;
    buf += esp_mail_str_15;
    buf += esp_mail_str_34;
  }

  for (uint8_t i = 0; i < msg->_rcp.size(); i++)
  {
    if (i == 0)
    {
      buf += esp_mail_str_11;
      buf += esp_mail_str_131;
      if (msg->_rcp[i].name.length() > 0)
        buf += msg->_rcp[i].name;

      buf += esp_mail_str_14;
      buf += msg->_rcp[i].email;
      buf += esp_mail_str_15;
    }
    else
    {
      if (msg->_rcp[i].name.length() > 0)
      {
        buf += esp_mail_str_263;
        buf += msg->_rcp[i].name;
        buf += esp_mail_str_14;
      }
      else
        buf += esp_mail_str_13;
      buf += msg->_rcp[i].email;
      buf += esp_mail_str_15;
    }

    if (i == msg->_rcp.size() - 1)
      buf += esp_mail_str_34;
  }

  for (uint8_t i = 0; i < msg->_cc.size(); i++)
  {
    if (i == 0)
    {
      buf += esp_mail_str_12;
      buf += esp_mail_str_131;
      buf += esp_mail_str_14;
      buf += msg->_cc[i].email;
      buf += esp_mail_str_15;
    }
    else
    {
      buf += esp_mail_str_13;
      buf += msg->_cc[i].email;
      buf += esp_mail_str_15;
    }

    if (i == msg->_cc.size() - 1)
      buf += esp_mail_str_34;
  }

  for (uint8_t i = 0; i < msg->_bcc.size(); i++)
  {
    if (i == 0)
    {
      buf += esp_mail_str_149;
      buf += esp_mail_str_14;
      buf += msg->_bcc[i].email;
      buf += esp_mail_str_15;
    }
    else
    {
      buf += esp_mail_str_13;
      buf += msg->_bcc[i].email;
      buf += esp_mail_str_15;
    }

    if (i == msg->_bcc.size() - 1)
      buf += esp_mail_str_34;
  }

  if (msg->subject.length() > 0)
  {
    buf += esp_mail_str_24;
    buf += esp_mail_str_131;
    buf += msg->subject;
    buf += esp_mail_str_34;
  }

  if (msg->keywords.length() > 0)
  {
    buf += esp_mail_str_145;
    buf += esp_mail_str_131;
    buf += msg->keywords;
    buf += esp_mail_str_34;
  }

  if (msg->comments.length() > 0)
  {
    buf += esp_mail_str_134;
    buf += esp_mail_str_131;
    buf += msg->comments;
    buf += esp_mail_str_34;
  }

  if (msg->in_reply_to.length() > 0)
  {
    buf += esp_mail_str_109;
    buf += esp_mail_str_131;
    buf += msg->in_reply_to;
    buf += esp_mail_str_34;
  }

  if (msg->references.length() > 0)
  {
    buf += esp_mail_str_107;
    buf += esp_mail_str_131;
    buf += msg->references;
    buf += esp_mail_str_34;
  }

  if (msg->messageID.length() > 0)
  {
    buf += esp_mail_str_101;
    buf += esp_mail_str_131;
    buf += esp_mail_str_14;
    buf += msg->messageID;
    buf += esp_mail_str_15;
    buf += esp_mail_str_34;
  }
}

bool ESP_Mail_Client::bdat(SMTPSession *smtp, SMTP_Message *msg, int len, bool last)
{
  if (!smtp->_chunkedEnable || !msg->enable.chunking)
    return true;

  smtp->_chunkCount++;

  MB_String bdat = esp_mail_str_106;
  bdat += len;
  if (last)
    bdat += esp_mail_str_173;

  if (smtpSend(smtp, bdat.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return setSendingResult(smtp, msg, false);

  if (!smtp->_send_capability.pipelining)
  {
    smtp->_smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_send_body;
    if (!handleSMTPResponse(smtp, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_BODY_FAILED))
      return setSendingResult(smtp, msg, false);
    smtp->_chunkCount = 0;
  }
  return true;
}

void ESP_Mail_Client::checkBinaryData(SMTPSession *smtp, SMTP_Message *msg)
{
  if (msg->type & esp_mail_msg_type_plain || msg->type == esp_mail_msg_type_enriched || msg->type & esp_mail_msg_type_html)
  {
    if ((msg->type & esp_mail_msg_type_plain || msg->type == esp_mail_msg_type_enriched) > 0)
    {
      if (msg->text.transfer_encoding.length() > 0)
      {
        if (strcmp(msg->text.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_binary) == 0)
        {
          msg->text._int.binary = true;
        }
      }
    }

    if ((msg->type & esp_mail_msg_type_html) > 0)
    {
      if (msg->html.transfer_encoding.length() > 0)
      {
        if (strcmp(msg->html.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_binary) == 0)
        {
          msg->html._int.binary = true;
        }
      }
    }
  }

  for (size_t i = 0; i < msg->_att.size(); i++)
  {
    if (strcmpP(msg->_att[i].descr.transfer_encoding.c_str(), 0, esp_mail_str_166))
    {
      msg->_att[i]._int.binary = true;
    }
  }
}

bool ESP_Mail_Client::sendBlob(SMTPSession *smtp, SMTP_Message *msg, SMTP_Attachment *att)
{
  if (strcmp(att->descr.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0 && strcmp(att->descr.transfer_encoding.c_str(), att->descr.content_encoding.c_str()) != 0)
  {
    if (!sendBase64(smtp, msg, (const unsigned char *)att->blob.data, att->blob.size, att->_int.flash_blob, att->descr.filename.c_str(), smtp->_sendCallback != NULL))
      return false;
    return true;
  }
  else
  {
    if (att->blob.size > 0)
    {
      if (strcmp(att->descr.content_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0)
      {
        if (!sendBase64Raw(smtp, msg, att->blob.data, att->blob.size, att->_int.flash_blob, att->descr.filename.c_str(), smtp->_sendCallback != NULL))
          return false;
        return true;
      }
      else
      {

        size_t chunkSize = ESP_MAIL_CLIENT_STREAM_CHUNK_SIZE;
        size_t writeLen = 0;
        uint8_t *buf = (uint8_t *)newP(chunkSize);
        while (writeLen < att->blob.size)
        {
          if (writeLen > att->blob.size - chunkSize)
            chunkSize = att->blob.size - writeLen;

          if (!bdat(smtp, msg, chunkSize, false))
            break;
          memcpy_P(buf, att->blob.data, chunkSize);
          if (smtpSend(smtp, buf, chunkSize) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            break;

          if (smtp->_sendCallback)
            uploadReport(att->descr.filename.c_str(), smtp->_lastProgress, 100 * writeLen / att->blob.size);

          writeLen += chunkSize;
        }
        delP(&buf);

        if (smtp->_sendCallback)
          uploadReport(att->descr.filename.c_str(), smtp->_lastProgress, 100);

        return writeLen >= att->blob.size;
      }
    }
  }
  return false;
}

bool ESP_Mail_Client::sendFile(SMTPSession *smtp, SMTP_Message *msg, SMTP_Attachment *att)
{
  smtp->_lastProgress = -1;

  if (strcmp(att->descr.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0 && strcmp(att->descr.transfer_encoding.c_str(), att->descr.content_encoding.c_str()) != 0)
  {
    if (!sendBase64Stream(smtp, msg, att->file.storage_type, att->descr.filename.c_str(), smtp->_sendCallback != NULL))
      return false;
    return true;
  }
  else
  {
    if (mbfs->size(mbfs_type att->file.storage_type) > 0)
    {
      if (strcmp(att->descr.content_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0)
      {
        if (!sendBase64StreamRaw(smtp, msg, att->file.storage_type, att->descr.filename.c_str(), smtp->_sendCallback != NULL))
          return false;
        return true;
      }
      else
      {
        int chunkSize = ESP_MAIL_CLIENT_STREAM_CHUNK_SIZE;
        int writeLen = 0;

        int fileSize = mbfs->size(mbfs_type att->file.storage_type);

        if (fileSize < chunkSize)
          chunkSize = fileSize;

        uint8_t *buf = (uint8_t *)newP(chunkSize);
        while (writeLen < fileSize && mbfs->available(mbfs_type att->file.storage_type))
        {
          if (writeLen > fileSize - chunkSize)
            chunkSize = fileSize - writeLen;
          int readLen = mbfs->read(mbfs_type att->file.storage_type, buf, chunkSize);
          if (readLen != chunkSize)
          {
            errorStatusCB(smtp, MB_FS_ERROR_FILE_IO_ERROR);
            break;
          }

          if (!bdat(smtp, msg, chunkSize, false))
            break;

          if (smtpSend(smtp, buf, chunkSize) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            break;

          if (smtp->_sendCallback)
            uploadReport(att->descr.filename.c_str(), smtp->_lastProgress, 100 * writeLen / fileSize);
          writeLen += chunkSize;
        }
        delP(&buf);
        if (smtp->_sendCallback)
          uploadReport(att->descr.filename.c_str(), smtp->_lastProgress, 100);
        return writeLen == fileSize;
      }
    }
    return false;
  }
  return false;
}

bool ESP_Mail_Client::sendParallelAttachments(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary)
{
  if (msg->_parallel.size() == 0)
    return true;

  MB_String parallel = getBoundary(15);
  MB_String buf = esp_mail_str_33;
  buf += boundary;
  buf += esp_mail_str_34;

  buf += esp_mail_str_28;
  buf += parallel;
  buf += esp_mail_str_35;

  if (!bdat(smtp, msg, buf.length(), false))
    return false;

  if (smtpSend(smtp, buf.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return setSendingResult(smtp, msg, false);

  if (!sendAttachments(smtp, msg, parallel, true))
    return setSendingResult(smtp, msg, false);

  buf = esp_mail_str_33;
  buf += parallel;
  buf += esp_mail_str_33;

  if (!bdat(smtp, msg, buf.length(), false))
    return false;

  if (smtpSend(smtp, buf.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return setSendingResult(smtp, msg, false);

  return true;
}

bool ESP_Mail_Client::sendAttachments(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary, bool parallel)
{
  MB_String s;
  MB_String buf;
  int cnt = 0;

  SMTP_Attachment *att = nullptr;

  size_t sz = msg->_att.size();
  if (parallel)
    sz = msg->_parallel.size();

  for (size_t i = 0; i < sz; i++)
  {
    if (parallel)
      att = &msg->_parallel[i];
    else
      att = &msg->_att[i];

    if (att->_int.att_type == esp_mail_att_type_attachment)
    {
      s = esp_mail_str_261;
      s += att->descr.filename;

      if (smtp->_sendCallback)
      {
        if (cnt > 0)
          smtpCB(smtp, "");
        smtpCB(smtp, att->descr.filename.c_str());
      }

      if (smtp->_debug)
        esp_mail_debug(s.c_str());

      cnt++;

      if (att->file.storage_type == esp_mail_file_storage_type_none)
      {
        if (!att->blob.data)
          continue;

        if (att->blob.size == 0)
          continue;

        if (smtp->_sendCallback)
          smtpCB(smtp, att->descr.filename.c_str());

        if (smtp->_debug)
          esp_mail_debug(s.c_str());

        buf.clear();
        getAttachHeader(buf, boundary, att, att->blob.size);

        if (!bdat(smtp, msg, buf.length(), false))
          return false;

        if (smtpSend(smtp, buf.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
          return false;

        if (!sendBlob(smtp, msg, att))
          return false;

        if (!bdat(smtp, msg, 2, false))
          return false;

        if (smtpSendP(smtp, esp_mail_str_34, false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
          return false;
      }
      else
      {

        if (!mbfs->checkStorageReady(mbfs_type att->file.storage_type))
        {
          sendStorageNotReadyError(smtp, att->file.storage_type);
          continue;
        }

        if (openFileRead(smtp, msg, att, s, buf, boundary, false))
        {

          if (!sendFile(smtp, msg, att))
            return false;

          if (!bdat(smtp, msg, 2, false))
            return false;

          if (smtpSendP(smtp, esp_mail_str_34, false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return false;
        }
      }
    }
  }
  return true;
}

bool ESP_Mail_Client::openFileRead(SMTPSession *smtp, SMTP_Message *msg, SMTP_Attachment *att, MB_String &s, MB_String &buf, const MB_String &boundary, bool inlined)
{
  int sz = -1;
  MB_String filepath;

  if (att->file.path.length() > 0)
  {
    if (att->file.path[0] != '/')
      filepath = esp_mail_str_202;
    filepath += att->file.path;
  }

  sz = mbfs->open(filepath, mbfs_type att->file.storage_type, mb_fs_open_mode_read);
  if (sz < 0)
  {

    if (strlen(att->descr.filename.c_str()) > 0)
    {
      filepath.clear();
      if (att->descr.filename[0] != '/')
        filepath = esp_mail_str_202;
      filepath += att->descr.filename;
    }

    sz = mbfs->open(filepath, mbfs_type att->file.storage_type, mb_fs_open_mode_read);
  }

  if (sz < 0)
  {
    if (smtp->_sendCallback)
      debugInfoP(esp_mail_str_158);

    if (smtp->_debug)
    {
      smtp->_smtpStatus.statusCode = sz;
      smtp->_smtpStatus.text.clear();

      MB_String e = esp_mail_str_185;
      e += smtp->errorReason().c_str();
      esp_mail_debug_line(e.c_str(), true);
    }
  }
  else
  {

    buf.clear();

    if (inlined)
      getInlineHeader(buf, boundary, att, sz);
    else
      getAttachHeader(buf, boundary, att, sz);

    if (!bdat(smtp, msg, buf.length(), false))
      return false;

    if (smtpSend(smtp, buf.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    return true;
  }

  return false;
}

bool ESP_Mail_Client::openFileRead2(SMTPSession *smtp, SMTP_Message *msg, const char *path, esp_mail_file_storage_type storageType)
{

  MB_String filepath;

  if (strlen(path) > 0)
  {
    if (path[0] != '/')
      filepath = esp_mail_str_202;
    filepath += path;
  }

  int sz = mbfs->open(filepath, mbfs_type storageType, mb_fs_open_mode_read);
  if (sz < 0)
  {
    if (smtp->_sendCallback)
      debugInfoP(esp_mail_str_158);

    if (smtp->_debug)
    {
      smtp->_smtpStatus.statusCode = sz;
      smtp->_smtpStatus.text.clear();

      MB_String e = esp_mail_str_185;
      e += smtp->errorReason().c_str();
      esp_mail_debug_line(e.c_str(), true);
    }

    return false;
  }

  return true;
}

#if defined(ENABLE_SMTP)
void ESP_Mail_Client::sendStorageNotReadyError(SMTPSession *smtp, esp_mail_file_storage_type storageType)
{

#if defined(MBFS_FLASH_FS) || defined(MBFS_SD_FS)

  if (smtp->_sendCallback)
  {
    if (storageType == esp_mail_file_storage_type_flash)
      debugInfoP(esp_mail_str_348);
    else if (storageType == esp_mail_file_storage_type_sd)
      debugInfoP(esp_mail_str_349);
  }

  if (smtp->_debug)
  {
    MB_String e = esp_mail_str_185;
    if (storageType == esp_mail_file_storage_type_flash)
      e += esp_mail_str_348;
    else if (storageType == esp_mail_file_storage_type_sd)
      e += esp_mail_str_349;
    esp_mail_debug(e.c_str());
  }
#endif
}
#endif

#if defined(ENABLE_IMAP)
void ESP_Mail_Client::sendStorageNotReadyError(IMAPSession *imap, esp_mail_file_storage_type storageType)
{

#if defined(MBFS_FLASH_FS) || defined(MBFS_SD_FS)
  if (imap->_debug)
  {
    MB_String e = esp_mail_str_185;
    if (storageType == esp_mail_file_storage_type_flash)
      e += esp_mail_str_348;
    else if (storageType == esp_mail_file_storage_type_sd)
      e += esp_mail_str_349;
    esp_mail_debug(e.c_str());
  }
#endif
}
#endif

bool ESP_Mail_Client::sendInline(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary, byte type)
{
  size_t num = numAtt(smtp, esp_mail_att_type_inline, msg) > 0;

  if (num > 0)
  {
    if (smtp->_sendCallback)
    {
      smtpCB(smtp, "");
      smtpCBP(smtp, esp_mail_str_167);
    }

    if (smtp->_debug)
      debugInfoP(esp_mail_str_271);
  }

  MB_String buf;
  MB_String related = getBoundary(15);
  int cnt = 0;
  SMTP_Attachment *att = nullptr;

  MB_String s = esp_mail_str_33;
  s += boundary;
  s += esp_mail_str_34;

  s += esp_mail_str_298;
  s += related;
  s += esp_mail_str_35;

  if (!bdat(smtp, msg, s.length(), false))
    return false;

  if (smtpSend(smtp, s.c_str()) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  if (!sendPartText(smtp, msg, type, related.c_str()))
    return false;

  if (smtp->_sendCallback && numAtt(smtp, esp_mail_att_type_inline, msg) > 0)
    esp_mail_debug("");

  if (num > 0)
  {
    for (uint8_t i = 0; i < msg->_att.size(); i++)
    {
      att = &msg->_att[i];
      if (att->_int.att_type == esp_mail_att_type_inline)
      {
        s = esp_mail_str_261;
        s += att->descr.filename;

        if (smtp->_sendCallback)
        {
          if (cnt > 0)
            smtpCB(smtp, "");
          smtpCB(smtp, att->descr.filename.c_str());
        }

        if (smtp->_debug)
          esp_mail_debug(s.c_str());

        cnt++;

        if (att->file.storage_type == esp_mail_file_storage_type_none)
        {
          if (!att->blob.data)
            continue;

          if (att->blob.size == 0)
            continue;

          if (smtp->_sendCallback)
            smtpCB(smtp, att->descr.filename.c_str());

          if (smtp->_debug)
            esp_mail_debug(s.c_str());

          buf.clear();
          getInlineHeader(buf, related, att, att->blob.size);

          if (!bdat(smtp, msg, buf.length(), false))
            return false;

          if (smtpSend(smtp, buf.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return false;

          if (!sendBlob(smtp, msg, att))
            return false;

          if (!bdat(smtp, msg, 2, false))
            return false;

          if (smtpSendP(smtp, esp_mail_str_34, false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return false;
        }
        else
        {

          if (!mbfs->checkStorageReady(mbfs_type att->file.storage_type))
          {
            sendStorageNotReadyError(smtp, att->file.storage_type);
            continue;
          }

          if (openFileRead(smtp, msg, att, s, buf, related, true))
          {
            if (!sendFile(smtp, msg, att))
              return false;

            if (!bdat(smtp, msg, 2, false))
              return false;

            if (smtpSendP(smtp, esp_mail_str_34, false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
              return false;
          }
        }
      }
    }
  }

  s = esp_mail_str_34;
  s += esp_mail_str_33;
  s += related;
  s += esp_mail_str_33;
  s += esp_mail_str_34;

  if (!bdat(smtp, msg, s.length(), false))
    return false;

  if (smtpSend(smtp, s.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  return true;
}

void ESP_Mail_Client::errorStatusCB(SMTPSession *smtp, int error)
{
  smtp->_smtpStatus.statusCode = error;
  MB_String s;

  if (smtp->_sendCallback)
  {
    s += esp_mail_str_53;
    s += smtp->errorReason().c_str();
    smtpCB(smtp, s.c_str(), false);
  }

  if (smtp->_debug)
  {
    s = esp_mail_str_185;
    s += smtp->errorReason().c_str();
    esp_mail_debug(s.c_str());
  }
}

size_t ESP_Mail_Client::smtpSendP(SMTPSession *smtp, PGM_P v, bool newline)
{
  if (!reconnect(smtp))
  {
    closeTCPSession(smtp);
    return 0;
  }

  if (!connected(smtp))
  {
    errorStatusCB(smtp, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
    return 0;
  }

  if (!smtp->_tcpConnected)
  {
    errorStatusCB(smtp, MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED);
    return 0;
  }

  int len = 0;

  MB_String s1 = v;

  if (newline)
  {
    if (smtp->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug(s1.c_str());
    len = smtp->client.println(s1.c_str());
  }
  else
  {
    if (smtp->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug_line(s1.c_str(), false);
    len = smtp->client.print(s1.c_str());
  }

  if (len != (int)s1.length() && len != (int)s1.length() + 2)
  {
    errorStatusCB(smtp, len);
    len = 0;
  }

  return len;
}

size_t ESP_Mail_Client::smtpSend(SMTPSession *smtp, const char *data, bool newline)
{
  if (!reconnect(smtp))
  {
    closeTCPSession(smtp);
    return 0;
  }

  if (!connected(smtp))
  {
    errorStatusCB(smtp, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
    return 0;
  }

  if (!smtp->_tcpConnected)
  {
    errorStatusCB(smtp, MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED);
    return 0;
  }

  int len = 0;

  if (newline)
  {
    if (smtp->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug(data);
    len = smtp->client.println(data);
  }
  else
  {
    if (smtp->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug_line(data, false);
    len = smtp->client.print(data);
  }

  if (len != (int)strlen(data) && len != (int)strlen(data) + 2)
  {
    errorStatusCB(smtp, len);
    len = 0;
  }

  return len;
}

size_t ESP_Mail_Client::smtpSend(SMTPSession *smtp, int data, bool newline)
{
  if (!reconnect(smtp))
  {
    closeTCPSession(smtp);
    return 0;
  }

  if (!connected(smtp))
  {
    errorStatusCB(smtp, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
    return 0;
  }

  if (!smtp->_tcpConnected)
  {
    errorStatusCB(smtp, MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED);
    return 0;
  }

  int len = 0;

  if (newline)
  {
    if (smtp->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug(num2Str(data, 0));
    len = smtp->client.println(data);
  }
  else
  {
    if (smtp->_debugLevel > esp_mail_debug_level_2)
      esp_mail_debug_line(num2Str(data, 0), false);
    len = smtp->client.print(data);
  }

  if (len != (int)strlen(num2Str(data, 0)) && len != (int)strlen(num2Str(data, 0)) + 2)
  {
    errorStatusCB(smtp, len);
    len = 0;
  }

  return len;
}

size_t ESP_Mail_Client::smtpSend(SMTPSession *smtp, uint8_t *data, size_t size)
{
  if (!reconnect(smtp))
  {
    closeTCPSession(smtp);
    return 0;
  }

  if (!connected(smtp))
  {
    errorStatusCB(smtp, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
    return 0;
  }

  if (!smtp->_tcpConnected)
  {
    errorStatusCB(smtp, MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED);
    return 0;
  }

  size_t len = 0;

  len = smtp->client.write(data, size);

  if (len != size)
  {
    errorStatusCB(smtp, len);
    len = 0;
  }

  return len;
}

bool ESP_Mail_Client::handleSMTPError(SMTPSession *smtp, int err, bool ret)
{
  if (err < 0)
    errorStatusCB(smtp, err);

  if (smtp->_tcpConnected)
    closeTCPSession(smtp);

  return ret;
}

bool ESP_Mail_Client::sendPartText(SMTPSession *smtp, SMTP_Message *msg, uint8_t type, const char *boundary)
{
  MB_String header;

  if (strlen(boundary) > 0)
  {
    header += esp_mail_str_33;
    header += boundary;
    header += esp_mail_str_34;
  }

  if (type == esp_mail_msg_type_plain || type == esp_mail_msg_type_enriched)
  {
    if (msg->text.content_type.length() > 0)
    {
      header += esp_mail_str_25;
      header += esp_mail_str_131;
      header += msg->text.content_type;

      if (msg->text.charSet.length() > 0)
      {
        header += esp_mail_str_97;
        header += esp_mail_str_131;
        header += esp_mail_str_168;
        header += msg->text.charSet;
        header += esp_mail_str_136;
      }

      if (msg->text.flowed)
      {
        header += esp_mail_str_97;
        header += esp_mail_str_131;
        header += esp_mail_str_270;

        header += esp_mail_str_97;
        header += esp_mail_str_131;
        header += esp_mail_str_110;
      }

      if (msg->text.embed.enable)
      {
        header += esp_mail_str_26;
        header += esp_mail_str_164;
        header += esp_mail_str_136;
        char *tmp = getRandomUID();
        msg->text._int.cid = tmp;
        delP(&tmp);
      }

      header += esp_mail_str_34;
    }

    if (msg->text.transfer_encoding.length() > 0)
    {
      header += esp_mail_str_272;
      header += msg->text.transfer_encoding;
      header += esp_mail_str_34;
    }
  }
  else if (type == esp_mail_msg_type_html)
  {
    if (msg->text.content_type.length() > 0)
    {
      header += esp_mail_str_25;
      header += esp_mail_str_131;
      header += msg->html.content_type;

      if (msg->html.charSet.length() > 0)
      {
        header += esp_mail_str_97;
        header += esp_mail_str_131;
        header += esp_mail_str_168;
        header += msg->html.charSet;
        header += esp_mail_str_136;
      }
      if (msg->html.embed.enable)
      {
        header += esp_mail_str_26;
        header += esp_mail_str_159;
        header += esp_mail_str_136;
        char *tmp = getRandomUID();
        msg->html._int.cid = tmp;
        delP(&tmp);
      }
      header += esp_mail_str_34;
    }

    if (msg->html.transfer_encoding.length() > 0)
    {
      header += esp_mail_str_272;
      header += msg->html.transfer_encoding;
      header += esp_mail_str_34;
    }
  }

  if ((type == esp_mail_msg_type_html && msg->html.embed.enable) || ((type == esp_mail_msg_type_plain || type == esp_mail_msg_type_enriched) && msg->text.embed.enable))
  {

    if ((type == esp_mail_msg_type_plain || type == esp_mail_msg_type_enriched) && msg->text.embed.enable)
    {
      if (msg->text.embed.type == esp_mail_smtp_embed_message_type_attachment)
        header += esp_mail_str_30;
      else if (msg->text.embed.type == esp_mail_smtp_embed_message_type_inline)
        header += esp_mail_str_299;

      if (msg->text.embed.filename.length() > 0)
        header += msg->text.embed.filename;
      else
        header += esp_mail_str_164;
      header += esp_mail_str_36;

      if (msg->text.embed.type == esp_mail_smtp_embed_message_type_inline)
      {
        header += esp_mail_str_300;
        if (msg->text.embed.filename.length() > 0)
          header += msg->text.embed.filename;
        else
          header += esp_mail_str_159;
        header += esp_mail_str_34;

        header += esp_mail_str_301;
        header += msg->text._int.cid;
        header += esp_mail_str_15;
        header += esp_mail_str_34;
      }
    }
    else if (type == esp_mail_msg_type_html && msg->html.embed.enable)
    {
      if (msg->html.embed.type == esp_mail_smtp_embed_message_type_attachment)
        header += esp_mail_str_30;
      else if (msg->html.embed.type == esp_mail_smtp_embed_message_type_inline)
        header += esp_mail_str_299;

      if (msg->html.embed.filename.length() > 0)
        header += msg->html.embed.filename;
      else
        header += esp_mail_str_159;
      header += esp_mail_str_36;

      if (msg->html.embed.type == esp_mail_smtp_embed_message_type_inline)
      {
        header += esp_mail_str_300;
        if (msg->html.embed.filename.length() > 0)
          header += msg->html.embed.filename;
        else
          header += esp_mail_str_159;
        header += esp_mail_str_34;

        header += esp_mail_str_301;
        header += msg->html._int.cid;
        header += esp_mail_str_15;
        header += esp_mail_str_34;
      }
    }
  }

  header += esp_mail_str_34;


  bool rawBlob = (msg->text.blob.size > 0 && (type == esp_mail_msg_type_plain || type == esp_mail_msg_type_enriched)) || (msg->html.blob.size > 0 && type == esp_mail_msg_type_html);
  bool rawFile = (msg->text.file.name.length() > 0 && (type == esp_mail_msg_type_plain || type == esp_mail_msg_type_enriched)) || (msg->html.file.name.length() > 0 && type == esp_mail_msg_type_html);
  bool rawContent = ((type == esp_mail_msg_type_plain || type == esp_mail_msg_type_enriched) && msg->text.content.length() > 0) || (type == esp_mail_msg_type_html && msg->html.content.length() > 0);
  bool nonCopyContent = ((type == esp_mail_msg_type_plain || type == esp_mail_msg_type_enriched) && strlen(msg->text.nonCopyContent) > 0) || (type == esp_mail_msg_type_html && strlen(msg->html.nonCopyContent) > 0);
 
  if (rawBlob || rawFile || nonCopyContent)
  {
    if (!bdat(smtp, msg, header.length(), false))
      return false;

    if (smtpSend(smtp, header.c_str()) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    header.clear();

    if (rawBlob || nonCopyContent)
    {
      if (!sendBlobBody(smtp, msg, type))
        return false;
    }
    else if (rawFile)
    {
      if (!sendFileBody(smtp, msg, type))
        return false;
    }
  }
  else if (rawContent)
    encodingText(smtp, msg, type, header);

  header += esp_mail_str_34;

  if (strlen(boundary) > 0)
    header += esp_mail_str_34;

  if (!bdat(smtp, msg, header.length(), false))
    return false;

  if (smtpSend(smtp, header.c_str()) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    return false;

  return true;
}

bool ESP_Mail_Client::sendBlobBody(SMTPSession *smtp, SMTP_Message *msg, uint8_t type)
{

  smtp->_lastProgress = -1;

  if (msg->text.blob.size == 0 && msg->html.blob.size == 0 && strlen(msg->text.nonCopyContent) == 0 && strlen(msg->html.nonCopyContent) == 0)
    return true;

  bool ret = true;
  int bufLen = 512;
  size_t pos = 0;

  const uint8_t *raw = NULL;
  int len = 0;
  bool base64 = false;

  if ((type == esp_mail_msg_type_plain || type == esp_mail_msg_type_enriched))
  {
    if (strlen(msg->text.nonCopyContent) > 0)
    {
      raw = (const uint8_t *)msg->text.nonCopyContent;
      len = strlen(msg->text.nonCopyContent);
    }
    else
    {
      raw = msg->text.blob.data;
      len = msg->text.blob.size;
    }
    base64 = msg->text.transfer_encoding.length() > 0 && strcmp(msg->text.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0;
  }
  else if (type == esp_mail_msg_type_html)
  {
    if (strlen(msg->html.nonCopyContent) > 0)
    {
      raw = (const uint8_t *)msg->html.nonCopyContent;
      len = strlen(msg->html.nonCopyContent);
    }
    else
    {
      raw = msg->html.blob.data;
      len = msg->html.blob.size;
    }
    base64 = msg->html.transfer_encoding.length() > 0 && strcmp(msg->html.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0;
  }

  if (base64)
  {
    MB_String s1 = esp_mail_str_325;
    return sendBase64(smtp, msg, (const unsigned char *)raw, len, true, s1.c_str(), smtp->_sendCallback != NULL);
  }

  int available = len;
  int sz = len;
  uint8_t *buf = (uint8_t *)newP(bufLen + 1);
  while (available)
  {
    if (available > bufLen)
      available = bufLen;

    memcpy_P(buf, raw + pos, available);

    if (!bdat(smtp, msg, available, false))
    {
      ret = false;
      break;
    }

    if (smtpSend(smtp, buf, available) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
    {
      ret = false;
      break;
    }

    pos += available;
    len -= available;
    available = len;
    if (smtp->_sendCallback)
    {
      MB_String s1 = esp_mail_str_325;
      uploadReport(s1.c_str(), smtp->_lastProgress, 100 * pos / sz);
    }
  }
  delP(&buf);

  return ret;
}

bool ESP_Mail_Client::sendFileBody(SMTPSession *smtp, SMTP_Message *msg, uint8_t type)
{
  smtp->_lastProgress = -1;

  if (msg->text.file.name.length() == 0 && msg->html.file.name.length() == 0)
    return true;

  bool ret = true;
  int chunkSize = ESP_MAIL_CLIENT_STREAM_CHUNK_SIZE;
  int writeLen = 0;

  if (type == esp_mail_msg_type_plain || type == esp_mail_msg_type_enriched)
  {

    if (!openFileRead2(smtp, msg, msg->text.file.name.c_str(), msg->text.file.type))
      return false;

    if (msg->text.transfer_encoding.length() > 0)
    {
      if (strcmp(msg->text.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0)
      {
        MB_String s1 = esp_mail_str_326;
        return sendBase64Stream(smtp, msg, msg->text.file.type, s1.c_str(), smtp->_sendCallback != NULL);
      }
    }

    int fileSize = mbfs->size(mbfs_type msg->text.file.type);

    if (fileSize > 0)
    {

      if (fileSize < chunkSize)
        chunkSize = fileSize;
      uint8_t *buf = (uint8_t *)newP(chunkSize);
      while (writeLen < fileSize && mbfs->available(mbfs_type msg->text.file.type))
      {
        if (writeLen > fileSize - chunkSize)
          chunkSize = fileSize - writeLen;
        int readLen = mbfs->read(mbfs_type msg->text.file.type, buf, chunkSize);

        if (readLen != chunkSize)
        {
          errorStatusCB(smtp, MB_FS_ERROR_FILE_IO_ERROR);
          break;
        }

        if (!bdat(smtp, msg, chunkSize, false))
        {
          ret = false;
          break;
        }

        if (smtpSend(smtp, buf, chunkSize) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        {
          ret = false;
          break;
        }

        if (smtp->_sendCallback)
        {
          MB_String s1 = esp_mail_str_326;
          uploadReport(s1.c_str(), smtp->_lastProgress, 100 * writeLen / fileSize);
        }
        writeLen += chunkSize;
      }
      delP(&buf);
      if (smtp->_sendCallback)
      {
        MB_String s1 = esp_mail_str_326;
        uploadReport(s1.c_str(), smtp->_lastProgress, 100);
      }

      return ret && writeLen == fileSize;
    }
  }
  else if (type == esp_mail_message_type::esp_mail_msg_type_html)
  {

    if (!openFileRead2(smtp, msg, msg->html.file.name.c_str(), msg->html.file.type))
      return false;

    if (msg->html.transfer_encoding.length() > 0)
    {
      if (strcmp(msg->html.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0)
      {
        MB_String s1 = esp_mail_str_326;
        return sendBase64Stream(smtp, msg, msg->html.file.type, s1.c_str(), smtp->_sendCallback != NULL);
      }
    }

    int fileSize = mbfs->size(mbfs_type msg->html.file.type);

    if (fileSize > 0)
    {

      if (fileSize < chunkSize)
        chunkSize = fileSize;
      uint8_t *buf = (uint8_t *)newP(chunkSize);
      while (writeLen < fileSize && mbfs->available(mbfs_type msg->html.file.type))
      {
        if (writeLen > fileSize - chunkSize)
          chunkSize = fileSize - writeLen;
        int readLen = mbfs->read(mbfs_type msg->html.file.type, buf, chunkSize);

        if (readLen != chunkSize)
        {
          errorStatusCB(smtp, MB_FS_ERROR_FILE_IO_ERROR);
          break;
        }

        if (!bdat(smtp, msg, chunkSize, false))
        {
          ret = false;
          break;
        }

        if (smtpSend(smtp, buf, chunkSize) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        {
          ret = false;
          break;
        }

        if (smtp->_sendCallback)
        {
          MB_String s1 = esp_mail_str_326;
          uploadReport(s1.c_str(), smtp->_lastProgress, 100 * writeLen / fileSize);
        }
        writeLen += chunkSize;
      }

      delP(&buf);

      if (smtp->_sendCallback)
      {
        MB_String s1 = esp_mail_str_326;
        uploadReport(s1.c_str(), smtp->_lastProgress, 100);
      }

      return ret && writeLen == fileSize;
    }
  }

  return false;
}

void ESP_Mail_Client::encodingText(SMTPSession *smtp, SMTP_Message *msg, uint8_t type, MB_String &content)
{
  if (type == esp_mail_msg_type_plain || type == esp_mail_msg_type_enriched)
  {
    MB_String s = msg->text.content;

    if (msg->text.flowed)
      formatFlowedText(s);

    if (msg->text.transfer_encoding.length() > 0)
    {
      if (strcmp(msg->text.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0)
        content += encodeBase64Str((const unsigned char *)s.c_str(), s.length());
      else if (strcmp(msg->text.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_qp) == 0)
      {
        char *out = (char *)newP(s.length() * 3 + 1);
        encodeQP(s.c_str(), out);
        content += out;
        delP(&out);
      }
      else
        content += s;
    }
    else
      content += s;
  }
  else if (type == esp_mail_message_type::esp_mail_msg_type_html)
  {
    char *tmp = nullptr;
    MB_String s = msg->html.content;
    MB_String fnd, rep;
    SMTP_Attachment *att = nullptr;
    for (uint8_t i = 0; i < msg->_att.size(); i++)
    {
      att = &msg->_att[i];
      if (att->_int.att_type == esp_mail_att_type_inline)
      {
        MB_String filename(att->descr.filename);

        size_t found = filename.find_last_of("/\\");
        if (found != MB_String::npos)
          filename = filename.substr(found + 1);

        fnd = esp_mail_str_136;
        fnd += filename;
        fnd += esp_mail_str_136;

        rep = esp_mail_str_136;
        rep += esp_mail_str_302;
        if (att->descr.content_id.length() > 0)
          rep += att->descr.content_id;
        else
          rep += att->_int.cid;
        rep += esp_mail_str_136;

        tmp = strReplace((char *)s.c_str(), (char *)fnd.c_str(), (char *)rep.c_str());
        s = tmp;
        delP(&tmp);
      }
    }

    if (msg->html.transfer_encoding.length() > 0)
    {
      if (strcmp(msg->html.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0)
        content += encodeBase64Str((const unsigned char *)s.c_str(), s.length());
      else if (strcmp(msg->html.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_qp) == 0)
      {
        char *out = (char *)newP(msg->html.content.length() * 3 + 1);
        encodeQP(msg->html.content.c_str(), out);
        content += out;
        delP(&out);
      }
      else
        content += s;
    }
    else
      content += s;
    s.clear();
  }
}

void ESP_Mail_Client::encodeQP(const char *buf, char *out)
{
  int n = 0, p = 0, pos = 0;
  for (n = 0; *buf; buf++)
  {
    if (n >= 73 && *buf != 10 && *buf != 13)
    {
      p = sprintf(out + pos, "=\r\n");
      pos += p;
      n = 0;
    }

    if (*buf == 10 || *buf == 13)
    {
      strcat_c(out, *buf);
      pos++;
      n = 0;
    }
    else if (*buf < 32 || *buf == 61 || *buf > 126)
    {
      p = sprintf(out + pos, "=%02X", (unsigned char)*buf);
      n += p;
      pos += p;
    }
    else if (*buf != 32 || (*(buf + 1) != 10 && *(buf + 1) != 13))
    {
      strcat_c(out, *buf);
      n++;
      pos++;
    }
    else
    {
      p = sprintf(out + pos, "=20");
      n += p;
      pos += p;
    }
  }
}

/** Add the soft line break to the long text line (rfc 3676)
 * and add Format=flowed parameter in the plain text content-type header.
 * We use the existing white space as a part of this soft line break
 * and set delSp="no" parameter to the header.
 *
 * Some servers are not rfc 3676 compliant.
 * This causes the text lines are wrapped instead of joined.
 *
 * Some mail clients trim the space before the line break
 * which makes the soft line break cannot be seen.
 */
void ESP_Mail_Client::formatFlowedText(MB_String &content)
{
  int count = 0;
  MB_String qms;
  int j = 0;
  MB_VECTOR<MB_String> tokens;
  char *stk = strP(esp_mail_str_34);
  char *qm = strP(esp_mail_str_15);
  splitTk(content, tokens, stk);
  content.clear();
  for (size_t i = 0; i < tokens.size(); i++)
  {
    if (tokens[i].length() > 0)
    {
      j = 0;
      qms.clear();
      while (tokens[i][j] == qm[0])
      {
        qms += qm;
        j++;
      }
      softBreak(tokens[i], qms.c_str());
      if (count > 0)
        content += stk;
      content += tokens[i];
    }
    else if (count > 0)
      content += stk;
    count++;
  }

  delP(&stk);
  delP(&qm);
  tokens.clear();
}

void ESP_Mail_Client::softBreak(MB_String &content, const char *quoteMarks)
{
  size_t len = 0;
  char *stk = strP(esp_mail_str_131);
  MB_VECTOR<MB_String> tokens;
  splitTk(content, tokens, stk);
  content.clear();
  for (size_t i = 0; i < tokens.size(); i++)
  {
    if (tokens[i].length() > 0)
    {
      if (len + tokens[i].length() + 3 > FLOWED_TEXT_LEN)
      {
        /* insert soft crlf */
        content += stk;
        content += esp_mail_str_34;

        /* insert quote marks */
        if (strlen(quoteMarks) > 0)
          content += quoteMarks;
        content += tokens[i];
        len = tokens[i].length();
      }
      else
      {
        if (len > 0)
        {
          content += stk;
          len += strlen(stk);
        }
        content += tokens[i];
        len += tokens[i].length();
      }
    }
  }
  delP(&stk);
  tokens.clear();
}

bool ESP_Mail_Client::sendMSG(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary)
{
  MB_String alt = getBoundary(15);
  MB_String s;

  if (numAtt(smtp, esp_mail_att_type_inline, msg) > 0)
  {
    s += esp_mail_str_297;
    s += alt;
    s += esp_mail_str_35;

    if (!bdat(smtp, msg, s.length(), false))
      return false;

    if (smtpSend(smtp, s.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;

    if (msg->type == esp_mail_msg_type_plain || msg->type == esp_mail_msg_type_enriched || msg->type == esp_mail_msg_type_html)
    {
      if (!sendInline(smtp, msg, alt, msg->type))
        return false;
    }
    else if (msg->type == (esp_mail_msg_type_html | esp_mail_msg_type_enriched | esp_mail_msg_type_plain))
    {
      if (!sendPartText(smtp, msg, esp_mail_msg_type_plain, alt.c_str()))
        return false;
      if (!sendInline(smtp, msg, alt, esp_mail_msg_type_html))
        return false;
    }

    s = esp_mail_str_33;
    s += alt;
    s += esp_mail_str_33;
    s += esp_mail_str_34;

    if (!bdat(smtp, msg, s.length(), false))
      return false;

    if (smtpSend(smtp, s.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      return false;
  }
  else
  {
    if (msg->type == esp_mail_msg_type_plain || msg->type == esp_mail_msg_type_enriched || msg->type == esp_mail_msg_type_html)
    {
      if (!sendPartText(smtp, msg, msg->type, ""))
        return false;
    }
    else if (msg->type == (esp_mail_msg_type_html | esp_mail_msg_type_enriched | esp_mail_msg_type_plain))
    {
      s = esp_mail_str_33;
      s += boundary;
      s += esp_mail_str_34;
      s += esp_mail_str_297;
      s += alt;
      s += esp_mail_str_35;

      if (!bdat(smtp, msg, s.length(), false))
        return false;

      if (smtpSend(smtp, s.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

      if (!sendPartText(smtp, msg, esp_mail_msg_type_plain, alt.c_str()))
        return false;

      if (!sendPartText(smtp, msg, esp_mail_msg_type_html, alt.c_str()))
        return false;
    }
  }
  return true;
}

void ESP_Mail_Client::getInlineHeader(MB_String &header, const MB_String &boundary, SMTP_Attachment *inlineAttach, size_t size)
{
  header += esp_mail_str_33;
  header += boundary;
  header += esp_mail_str_34;

  header += esp_mail_str_25;
  header += esp_mail_str_131;

  if (inlineAttach->descr.mime.length() == 0)
  {
    MB_String mime;
    mimeFromFile(inlineAttach->descr.filename.c_str(), mime);
    if (mime.length() > 0)
      header += mime;
    else
      header += esp_mail_str_32;
  }
  else
    header += inlineAttach->descr.mime;

  header += esp_mail_str_26;

  MB_String filename = inlineAttach->descr.filename;

  size_t found = filename.find_last_of("/\\");

  if (found != MB_String::npos)
    filename = filename.substr(found + 1);

  header += filename;
  header += esp_mail_str_36;

  header += esp_mail_str_299;
  header += filename;
  header += esp_mail_str_327;
  header += size;
  header += esp_mail_str_34;

  header += esp_mail_str_300;
  header += filename;
  header += esp_mail_str_34;

  header += esp_mail_str_301;
  if (inlineAttach->descr.content_id.length() > 0)
    header += inlineAttach->descr.content_id;
  else
    header += inlineAttach->_int.cid;

  header += esp_mail_str_15;

  header += esp_mail_str_34;

  if (inlineAttach->descr.transfer_encoding.length() > 0)
  {
    header += esp_mail_str_272;
    header += inlineAttach->descr.transfer_encoding;
    header += esp_mail_str_34;
  }
  header += esp_mail_str_34;

  filename.clear();
}

void ESP_Mail_Client::getAttachHeader(MB_String &header, const MB_String &boundary, SMTP_Attachment *attach, size_t size)
{
  header += esp_mail_str_33;
  header += boundary;
  header += esp_mail_str_34;

  header += esp_mail_str_25;
  header += esp_mail_str_131;

  if (attach->descr.mime.length() == 0)
  {
    MB_String mime;
    mimeFromFile(attach->descr.filename.c_str(), mime);
    if (mime.length() > 0)
      header += mime;
    else
      header += esp_mail_str_32;
  }
  else
    header += attach->descr.mime;

  header += esp_mail_str_26;

  MB_String filename = attach->descr.filename;

  size_t found = filename.find_last_of("/\\");
  if (found != MB_String::npos)
    filename = filename.substr(found + 1);

  header += filename;
  header += esp_mail_str_36;

  if (!attach->_int.parallel)
  {
    header += esp_mail_str_30;
    header += filename;
    header += esp_mail_str_327;
    header += size;
    header += esp_mail_str_34;
  }

  if (attach->descr.transfer_encoding.length() > 0)
  {
    header += esp_mail_str_272;
    header += attach->descr.transfer_encoding;
    header += esp_mail_str_34;
  }

  header += esp_mail_str_34;
}

void ESP_Mail_Client::getRFC822PartHeader(SMTPSession *smtp, MB_String &header, const MB_String &boundary)
{
  header += esp_mail_str_33;
  header += boundary;
  header += esp_mail_str_34;

  header += esp_mail_str_25;
  header += esp_mail_str_131;

  header += esp_mail_str_123;
  header += esp_mail_str_34;

  header += esp_mail_str_98;

  header += esp_mail_str_34;
}

void ESP_Mail_Client::smtpCBP(SMTPSession *smtp, PGM_P info, bool success)
{
  MB_String s = info;
  smtpCB(smtp, s.c_str(), success);
}

void ESP_Mail_Client::smtpCB(SMTPSession *smtp, const char *info, bool success)
{
  smtp->_cbData._info = info;
  smtp->_cbData._success = success;
  smtp->_sendCallback(smtp->_cbData);
}

bool ESP_Mail_Client::sendBase64Raw(SMTPSession *smtp, SMTP_Message *msg, const uint8_t *data, size_t len, bool flashMem, const char *filename, bool report)
{
  bool ret = false;

  smtp->_lastProgress = -1;

  size_t chunkSize = (BASE64_CHUNKED_LEN * UPLOAD_CHUNKS_NUM) + (2 * UPLOAD_CHUNKS_NUM);
  size_t bufIndex = 0;
  size_t dataIndex = 0;

  if (len < chunkSize)
    chunkSize = len;

  uint8_t *buf = (uint8_t *)newP(chunkSize);
  memset(buf, 0, chunkSize);

  if (report)
    uploadReport(filename, smtp->_lastProgress, dataIndex);

  while (dataIndex < len - 1)
  {

    size_t size = 4;
    if (dataIndex + size > len - 1)
      size = len - dataIndex;

    if (flashMem)
      memcpy_P(buf + bufIndex, data + dataIndex, size);
    else
      memcpy(buf + bufIndex, data + dataIndex, size);

    dataIndex += size;
    bufIndex += size;

    if (bufIndex + 1 == BASE64_CHUNKED_LEN)
    {
      if (bufIndex + 2 < chunkSize)
      {
        buf[bufIndex++] = 0x0d;
        buf[bufIndex++] = 0x0a;
      }
    }

    if (bufIndex + 1 >= chunkSize - size)
    {
      if (!bdat(smtp, msg, bufIndex + 1, false))
        goto ex;

      if (smtpSend(smtp, buf, bufIndex + 1) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        goto ex;

      bufIndex = 0;
    }

    if (report)
      uploadReport(filename, smtp->_lastProgress, 100 * dataIndex / len);
  }

  if (report)
    uploadReport(filename, smtp->_lastProgress, 100);

  ret = true;
ex:
  if (report)
    esp_mail_debug("");
  delP(&buf);
  return ret;
}

bool ESP_Mail_Client::sendBase64Stream(SMTPSession *smtp, SMTP_Message *msg, esp_mail_file_storage_type storageType, const char *filename, bool report)
{
  bool ret = false;

  smtp->_lastProgress = -1;

  int fileSize = mbfs->size(mbfs_type storageType);

  if (!fileSize)
  {
    errorStatusCB(smtp, MB_FS_ERROR_FILE_IO_ERROR);
    return false;
  }

  size_t chunkSize = (BASE64_CHUNKED_LEN * UPLOAD_CHUNKS_NUM) + (2 * UPLOAD_CHUNKS_NUM);
  size_t byteAdded = 0;
  size_t byteSent = 0;

  unsigned char *buf = (unsigned char *)newP(chunkSize);
  memset(buf, 0, chunkSize);

  size_t len = fileSize;
  size_t fbufIndex = 0;
  unsigned char *fbuf = (unsigned char *)newP(3);

  int dByte = 0;
  int bcnt = 0;

  if (report)
    uploadReport(filename, smtp->_lastProgress, bcnt);

  while (mbfs->available(mbfs_type storageType))
  {
    memset(fbuf, 0, 3);
    if (len - fbufIndex >= 3)
    {
      bcnt += 3;
      size_t readLen = mbfs->read(mbfs_type storageType, fbuf, 3);
      if (readLen != 3)
      {
        errorStatusCB(smtp, MB_FS_ERROR_FILE_IO_ERROR);
        break;
      }

      buf[byteAdded++] = b64_index_table[fbuf[0] >> 2];
      buf[byteAdded++] = b64_index_table[((fbuf[0] & 0x03) << 4) | (fbuf[1] >> 4)];
      buf[byteAdded++] = b64_index_table[((fbuf[1] & 0x0f) << 2) | (fbuf[2] >> 6)];
      buf[byteAdded++] = b64_index_table[fbuf[2] & 0x3f];
      dByte += 4;
      if (dByte == BASE64_CHUNKED_LEN)
      {
        if (byteAdded + 1 < chunkSize)
        {
          buf[byteAdded++] = 0x0d;
          buf[byteAdded++] = 0x0a;
        }
        dByte = 0;
      }
      if (byteAdded >= chunkSize - 4)
      {
        byteSent += byteAdded;

        if (!bdat(smtp, msg, byteAdded, false))
          goto ex;

        if (smtpSend(smtp, buf, byteAdded) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
          goto ex;

        memset(buf, 0, chunkSize);
        byteAdded = 0;
      }
      fbufIndex += 3;

      if (report)
        uploadReport(filename, smtp->_lastProgress, 100 * bcnt / len);
    }
    else
    {
      size_t readLen = mbfs->read(mbfs_type storageType, fbuf, len - fbufIndex);
      if (readLen != len - fbufIndex)
      {
        errorStatusCB(smtp, MB_FS_ERROR_FILE_IO_ERROR);
        break;
      }
    }
  }

  mbfs->close(mbfs_type storageType);
  if (byteAdded > 0)
  {
    if (!bdat(smtp, msg, byteAdded, false))
      goto ex;

    if (smtpSend(smtp, buf, byteAdded) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      goto ex;
  }

  if (len - fbufIndex > 0)
  {
    memset(buf, 0, chunkSize);
    byteAdded = 0;
    buf[byteAdded++] = b64_index_table[fbuf[0] >> 2];
    if (len - fbufIndex == 1)
    {
      buf[byteAdded++] = b64_index_table[(fbuf[0] & 0x03) << 4];
      buf[byteAdded++] = '=';
    }
    else
    {
      buf[byteAdded++] = b64_index_table[((fbuf[0] & 0x03) << 4) | (fbuf[1] >> 4)];
      buf[byteAdded++] = b64_index_table[(fbuf[1] & 0x0f) << 2];
    }
    buf[byteAdded++] = '=';

    if (!bdat(smtp, msg, byteAdded, false))
      goto ex;

    if (smtpSend(smtp, buf, byteAdded) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      goto ex;
  }
  ret = true;

  if (report)
    uploadReport(filename, smtp->_lastProgress, 100);

ex:
  delP(&buf);
  delP(&fbuf);
  mbfs->close(mbfs_type storageType);
  return ret;
}

bool ESP_Mail_Client::sendBase64StreamRaw(SMTPSession *smtp, SMTP_Message *msg, esp_mail_file_storage_type storageType, const char *filename, bool report)
{
  bool ret = false;

  smtp->_lastProgress = -1;

  int fileSize = mbfs->size(mbfs_type storageType);

  if (!fileSize)
  {
    errorStatusCB(smtp, MB_FS_ERROR_FILE_IO_ERROR);
    return false;
  }

  size_t chunkSize = (BASE64_CHUNKED_LEN * UPLOAD_CHUNKS_NUM) + (2 * UPLOAD_CHUNKS_NUM);
  size_t bufIndex = 0;
  size_t dataIndex = 0;

  size_t len = fileSize;

  if (len < chunkSize)
    chunkSize = len;

  uint8_t *buf = (uint8_t *)newP(chunkSize);
  memset(buf, 0, chunkSize);

  uint8_t *fbuf = (uint8_t *)newP(4);

  if (report)
    uploadReport(filename, smtp->_lastProgress, bufIndex);

  while (mbfs->available(mbfs_type storageType))
  {

    if (dataIndex < len - 1)
    {
      size_t size = 4;
      if (dataIndex + size > len - 1)
        size = len - dataIndex;

      size_t readLen = mbfs->read(mbfs_type storageType, fbuf, size);
      if (readLen != size)
      {
        errorStatusCB(smtp, MB_FS_ERROR_FILE_IO_ERROR);
        break;
      }

      memcpy(buf + bufIndex, fbuf, size);

      bufIndex += size;

      if (bufIndex + 1 == BASE64_CHUNKED_LEN)
      {
        if (bufIndex + 2 < chunkSize)
        {
          buf[bufIndex++] = 0x0d;
          buf[bufIndex++] = 0x0a;
        }
      }

      if (bufIndex + 1 >= chunkSize - size)
      {
        if (!bdat(smtp, msg, bufIndex + 1, false))
          goto ex;

        if (smtpSend(smtp, buf, bufIndex + 1) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
          goto ex;

        bufIndex = 0;
      }

      if (report)
        uploadReport(filename, smtp->_lastProgress, 100 * dataIndex / len);
    }
  }

  mbfs->close(mbfs_type storageType);
  ret = true;

  if (report)
    uploadReport(filename, smtp->_lastProgress, 100);

ex:
  delP(&buf);
  delP(&fbuf);
  mbfs->close(mbfs_type storageType);
  return ret;
}

void ESP_Mail_Client::handleAuth(SMTPSession *smtp, char *buf)
{
  if (strposP(buf, esp_mail_smtp_response_1, 0) > -1)
  {
    if (strposP(buf, esp_mail_smtp_response_2, 0) > -1)
      smtp->_auth_capability.login = true;
    if (strposP(buf, esp_mail_smtp_response_3, 0) > -1)
      smtp->_auth_capability.plain = true;
    if (strposP(buf, esp_mail_smtp_response_4, 0) > -1)
      smtp->_auth_capability.xoauth2 = true;
    if (strposP(buf, esp_mail_smtp_response_11, 0) > -1)
      smtp->_auth_capability.cram_md5 = true;
    if (strposP(buf, esp_mail_smtp_response_12, 0) > -1)
      smtp->_auth_capability.digest_md5 = true;
  }
  else if (strposP(buf, esp_mail_smtp_response_5, 0) > -1)
    smtp->_auth_capability.start_tls = true;
  else if (strposP(buf, esp_mail_smtp_response_6, 0) > -1)
    smtp->_send_capability._8bitMIME = true;
  else if (strposP(buf, esp_mail_smtp_response_7, 0) > -1)
    smtp->_send_capability.binaryMIME = true;
  else if (strposP(buf, esp_mail_smtp_response_8, 0) > -1)
    smtp->_send_capability.chunking = true;
  else if (strposP(buf, esp_mail_smtp_response_9, 0) > -1)
    smtp->_send_capability.utf8 = true;
  else if (strposP(buf, esp_mail_smtp_response_10, 0) > -1)
    smtp->_send_capability.pipelining = true;
  else if (strposP(buf, esp_mail_smtp_response_13, 0) > -1)
    smtp->_send_capability.dsn = true;
}

bool ESP_Mail_Client::handleSMTPResponse(SMTPSession *smtp, esp_mail_smtp_status_code respCode, int errCode)
{
  if (!reconnect(smtp))
    return false;

  bool ret = false;
  char *response = nullptr;
  int readLen = 0;
  long dataTime = millis();
  int chunkBufSize = 0;
  MB_String s, r;
  int chunkIndex = 0;
  int count = 0;
  bool completedResponse = false;
  smtp->_smtpStatus.statusCode = 0;
  smtp->_smtpStatus.respCode = 0;
  smtp->_smtpStatus.text.clear();
  uint8_t minResLen = 5;
  struct esp_mail_smtp_response_status_t status;

  chunkBufSize = smtp->client.available();

  while (smtp->_tcpConnected && chunkBufSize <= 0)
  {
    if (!reconnect(smtp, dataTime))
      return false;
    if (!connected(smtp))
    {
      errorStatusCB(smtp, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
      return false;
    }
    chunkBufSize = smtp->client.available();
    delay(0);
  }

  dataTime = millis();

  if (chunkBufSize > 1)
  {
    while (!completedResponse)
    {
      delay(0);

      if (!reconnect(smtp, dataTime))
        return false;

      if (!connected(smtp))
      {
        errorStatusCB(smtp, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
        return false;
      }

      chunkBufSize = smtp->client.available();

      if (chunkBufSize <= 0)
        break;

      if (chunkBufSize > 0)
      {
        chunkBufSize = 512;
        response = (char *)newP(chunkBufSize + 1);

        readLen = readLine(smtp, response, chunkBufSize, false, count);

        if (readLen)
        {
          if (smtp->_smtp_cmd != esp_mail_smtp_command::esp_mail_smtp_cmd_initial_state)
          {
            // sometimes server sent multiple lines response
            // sometimes rx buffer may not ready for a while
            if (strlen(response) < minResLen)
            {
              r += response;
              chunkBufSize = 0;
              while (chunkBufSize == 0)
              {
                delay(0);
                if (!reconnect(smtp, dataTime))
                  return false;
                chunkBufSize = smtp->client.available();
              }
            }
            else
            {
              if (r.length() > 0)
              {
                r += response;
                memset(response, 0, chunkBufSize);
                strcpy(response, r.c_str());
              }

              if (smtp->_debugLevel > esp_mail_debug_level_1)
                esp_mail_debug((const char *)response);
            }

            if (smtp->_smtp_cmd == esp_mail_smtp_command::esp_mail_smtp_cmd_greeting)
              handleAuth(smtp, response);
          }

          getResponseStatus(response, respCode, 0, status);

          // get the status code again for unexpected return code
          if (smtp->_smtp_cmd == esp_mail_smtp_command::esp_mail_smtp_cmd_start_tls || status.respCode == 0)
            getResponseStatus(response, esp_mail_smtp_status_code_0, 0, status);

          ret = respCode == status.respCode;
          smtp->_smtpStatus = status;

          if (status.respCode > 0 && (status.respCode < 400 || status.respCode == respCode))
            ret = true;

          if (smtp->_debug && strlen(response) >= minResLen)
          {
            s = esp_mail_str_260;
            if (smtp->_smtpStatus.respCode != esp_mail_smtp_status_code_334)
              s += response;
            else
            {
              // base64 response
              size_t olen;
              char *decoded = (char *)decodeBase64((const unsigned char *)status.text.c_str(), status.text.length(), &olen);
              if (decoded && olen > 0)
              {
                olen += s.length();
                s += decoded;
                s[olen] = 0;
                delP(&decoded);
              }
            }
            esp_mail_debug(s.c_str());
            r.clear();
          }

          completedResponse = smtp->_smtpStatus.respCode > 0 && status.text.length() > minResLen;

          if (smtp->_smtp_cmd == esp_mail_smtp_command::esp_mail_smtp_cmd_auth && smtp->_smtpStatus.respCode == esp_mail_smtp_status_code_334)
          {
            if (authFailed(response, readLen, chunkIndex, 4))
            {
              smtp->_smtpStatus.statusCode = -1;
              ret = false;
            }
          }

          chunkIndex++;

          if (smtp->_chunkedEnable && smtp->_smtp_cmd == esp_mail_smtp_command::esp_mail_smtp_cmd_chunk_termination)
            completedResponse = smtp->_chunkCount == chunkIndex;
        }
        delP(&response);
      }
    }

    if (!ret)
      handleSMTPError(smtp, errCode, false);
  }

  return ret;
}

void ESP_Mail_Client::getResponseStatus(const char *buf, esp_mail_smtp_status_code respCode, int beginPos, struct esp_mail_smtp_response_status_t &status)
{
  MB_String s;
  char *tmp = nullptr;
  int p1 = 0;
  if (respCode > esp_mail_smtp_status_code_0)
  {
    s = respCode;
    s += esp_mail_str_131;
    p1 = strpos(buf, (const char *)s.c_str(), beginPos);
  }

  if (p1 != -1)
  {
    int ofs = s.length() - 2;
    if (ofs < 0)
      ofs = 1;

    int p2 = strposP(buf, esp_mail_str_131, p1 + ofs);

    if (p2 < 4 && p2 > -1)
    {
      tmp = (char *)newP(p2 + 1);
      memcpy(tmp, &buf[p1], p2);
      status.respCode = atoi(tmp);
      delP(&tmp);

      p1 = p2 + 1;
      p2 = strlen(buf);
      if (p2 > p1)
      {
        tmp = (char *)newP(p2 + 1);
        memcpy(tmp, &buf[p1], p2 - p1);
        status.text = tmp;
        delP(&tmp);
      }
    }
  }
}

void ESP_Mail_Client::closeTCPSession(SMTPSession *smtp)
{
  if (smtp->_tcpConnected)
  {
    smtp->client.stop();
    _lastReconnectMillis = millis();
  }
  smtp->_tcpConnected = false;
}

bool ESP_Mail_Client::reconnect(SMTPSession *smtp, unsigned long dataTime)
{
  smtp->client.setSession(smtp->_sesson_cfg);

  bool status = smtp->client.networkReady();

  if (dataTime > 0)
  {
    if (millis() - dataTime > (unsigned long)smtp->client.tcpTimeout())
    {
      closeTCPSession(smtp);
      errorStatusCB(smtp, MAIL_CLIENT_ERROR_READ_TIMEOUT);
      return false;
    }
  }

  if (!status)
  {
    if (smtp->_tcpConnected)
      closeTCPSession(smtp);

    errorStatusCB(smtp, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);

    if (millis() - _lastReconnectMillis > _reconnectTimeout && !smtp->_tcpConnected)
    {
      smtp->client.networkReconnect();

      _lastReconnectMillis = millis();
    }

    status = smtp->client.networkReady();
  }

  return status;
}

void ESP_Mail_Client::uploadReport(const char *filename, int &lastProgress, int progress)
{
  if (progress > 100)
    progress = 100;
  if (lastProgress != progress && (progress == 0 || progress == 100 || lastProgress + ESP_MAIL_PROGRESS_REPORT_STEP <= progress))
  {
    MB_String s = esp_mail_str_160;
    s += MBSTRING_FLASH_MCR("\"");
    s += filename;
    s += MBSTRING_FLASH_MCR("\"");
    s += esp_mail_str_91;
    s += progress;
    s += esp_mail_str_92;
    s += esp_mail_str_34;
    esp_mail_debug_line(s.c_str(), false);
    lastProgress = progress;
  }
}

MB_String ESP_Mail_Client::getBoundary(size_t len)
{
  MB_String tmp = boundary_table;
  char *buf = (char *)newP(len);
  if (len)
  {
    --len;
    buf[0] = tmp[0];
    buf[1] = tmp[1];
    for (size_t n = 2; n < len; n++)
    {
      int key = rand() % (int)(tmp.length() - 1);
      buf[n] = tmp[key];
    }
    buf[len] = '\0';
  }
  MB_String s = buf;
  delP(&buf);
  return s;
}

bool ESP_Mail_Client::sendBase64(SMTPSession *smtp, SMTP_Message *msg, const unsigned char *data, size_t len, bool flashMem, const char *filename, bool report)
{
  bool ret = false;
  smtp->_lastProgress = -1;
  const unsigned char *end, *in;

  end = data + len;
  in = data;

  size_t chunkSize = (BASE64_CHUNKED_LEN * UPLOAD_CHUNKS_NUM) + (2 * UPLOAD_CHUNKS_NUM);
  size_t byteAdded = 0;
  size_t byteSent = 0;

  int dByte = 0;
  unsigned char *buf = (unsigned char *)newP(chunkSize);
  memset(buf, 0, chunkSize);

  unsigned char *tmp = (unsigned char *)newP(3);
  int bcnt = 0;

  if (report)
    uploadReport(filename, smtp->_lastProgress, bcnt);

  while (end - in >= 3)
  {
    memset(tmp, 0, 3);
    if (flashMem)
      memcpy_P(tmp, in, 3);
    else
      memcpy(tmp, in, 3);
    bcnt += 3;

    buf[byteAdded++] = b64_index_table[tmp[0] >> 2];
    buf[byteAdded++] = b64_index_table[((tmp[0] & 0x03) << 4) | (tmp[1] >> 4)];
    buf[byteAdded++] = b64_index_table[((tmp[1] & 0x0f) << 2) | (tmp[2] >> 6)];
    buf[byteAdded++] = b64_index_table[tmp[2] & 0x3f];
    dByte += 4;
    if (dByte == BASE64_CHUNKED_LEN)
    {
      if (byteAdded + 1 < chunkSize)
      {
        buf[byteAdded++] = 0x0d;
        buf[byteAdded++] = 0x0a;
      }
      dByte = 0;
    }
    if (byteAdded >= chunkSize - 4)
    {
      byteSent += byteAdded;

      if (!bdat(smtp, msg, byteAdded, false))
        goto ex;

      if (smtpSend(smtp, buf, byteAdded) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        goto ex;
      memset(buf, 0, chunkSize);
      byteAdded = 0;
    }
    in += 3;

    if (report)
      uploadReport(filename, smtp->_lastProgress, 100 * bcnt / len);
  }

  if (byteAdded > 0)
  {
    if (!bdat(smtp, msg, byteAdded, false))
      goto ex;

    if (smtpSend(smtp, buf, byteAdded) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      goto ex;
  }

  if (end - in)
  {
    memset(buf, 0, chunkSize);
    byteAdded = 0;
    memset(tmp, 0, 3);
    if (flashMem)
    {
      if (end - in == 1)
        memcpy_P(tmp, in, 1);
      else
        memcpy_P(tmp, in, 2);
    }
    else
    {
      if (end - in == 1)
        memcpy(tmp, in, 1);
      else
        memcpy(tmp, in, 2);
    }

    buf[byteAdded++] = b64_index_table[tmp[0] >> 2];
    if (end - in == 1)
    {
      buf[byteAdded++] = b64_index_table[(tmp[0] & 0x03) << 4];
      buf[byteAdded++] = '=';
    }
    else
    {
      buf[byteAdded++] = b64_index_table[((tmp[0] & 0x03) << 4) | (tmp[1] >> 4)];
      buf[byteAdded++] = b64_index_table[(tmp[1] & 0x0f) << 2];
    }
    buf[byteAdded++] = '=';

    if (!bdat(smtp, msg, byteAdded, false))
      goto ex;

    if (smtpSend(smtp, buf, byteAdded) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
      goto ex;
    memset(buf, 0, chunkSize);
  }

  uploadReport(filename, smtp->_lastProgress, 100);

  ret = true;
ex:
  if (report)
    esp_mail_debug("");
  delP(&tmp);
  delP(&buf);
  return ret;
}

MB_FS *ESP_Mail_Client::getMBFS()
{
  return mbfs;
}

SMTPSession::SMTPSession(Client *client)
{
  setClient(client);
}

SMTPSession::SMTPSession()
{
}

SMTPSession::~SMTPSession()
{
  closeSession();
#if defined(ESP32) || defined(ESP8266)
  _caCert.reset();
  _caCert = nullptr;
#endif
}

bool SMTPSession::connect(ESP_Mail_Session *config)
{
  if (client.type() == esp_mail_client_type_custom)
  {
#if !defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT)
    return MailClient.handleSMTPError(this, MAIL_CLIENT_ERROR_CUSTOM_CLIENT_DISABLED);
#endif
    if (!client.isInitialized())
      return MailClient.handleSMTPError(this, TCP_CLIENT_ERROR_NOT_INITIALIZED);
  }

  if (_tcpConnected)
    MailClient.closeTCPSession(this);

  if (config)
  {
    if (config->time.ntp_server.length() > 0)
      MailClient.setTime(config->time.gmt_offset, config->time.day_light_offset, config->time.ntp_server.c_str(), true);
  }

  _sesson_cfg = config;
#if defined(ESP32) || defined(ESP8266)
  _caCert = nullptr;
  if (strlen(_sesson_cfg->certificate.cert_data) > 0)
    _caCert = std::shared_ptr<const char>(_sesson_cfg->certificate.cert_data);
#endif

  return MailClient.smtpAuth(this);
}

void SMTPSession::debug(int level)
{
  if (level > esp_mail_debug_level_0)
  {
    if (level > esp_mail_debug_level_3)
      level = esp_mail_debug_level_1;
    _debugLevel = level;
    _debug = true;
  }
  else
  {
    _debugLevel = esp_mail_debug_level_0;
    _debug = false;
  }
}

String SMTPSession::errorReason()
{
  MB_String ret;

  switch (_smtpStatus.statusCode)
  {
  case SMTP_STATUS_SERVER_CONNECT_FAILED:
    ret += esp_mail_str_38;
    break;
  case SMTP_STATUS_SMTP_GREETING_GET_RESPONSE_FAILED:
    ret += esp_mail_str_39;
    break;
  case SMTP_STATUS_SMTP_GREETING_SEND_ACK_FAILED:
    ret += esp_mail_str_39;
    break;
  case SMTP_STATUS_AUTHEN_NOT_SUPPORT:
    ret += esp_mail_str_42;
    break;
  case SMTP_STATUS_AUTHEN_FAILED:
    ret += esp_mail_str_43;
    break;
  case SMTP_STATUS_USER_LOGIN_FAILED:
    ret += esp_mail_str_43;
    break;
  case SMTP_STATUS_PASSWORD_LOGIN_FAILED:
    ret += esp_mail_str_47;
    break;
  case SMTP_STATUS_SEND_HEADER_SENDER_FAILED:
    ret += esp_mail_str_48;
    break;
  case SMTP_STATUS_SEND_HEADER_RECIPIENT_FAILED:
    ret += esp_mail_str_222;
    break;
  case SMTP_STATUS_SEND_BODY_FAILED:
    ret += esp_mail_str_49;
    break;
  case MAIL_CLIENT_ERROR_CONNECTION_CLOSED:
    ret += esp_mail_str_221;
    break;
  case MAIL_CLIENT_ERROR_READ_TIMEOUT:
    ret += esp_mail_str_258;
    break;
  case SMTP_STATUS_SERVER_OAUTH2_LOGIN_DISABLED:
    ret += esp_mail_str_293;
    break;
  case MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED:
    ret += esp_mail_str_305;
    break;
  case MAIL_CLIENT_ERROR_SSL_TLS_STRUCTURE_SETUP:
    ret += esp_mail_str_132;
    break;
  case SMTP_STATUS_NO_VALID_RECIPIENTS_EXISTED:
    ret += esp_mail_str_206;
    break;
  case SMTP_STATUS_NO_VALID_SENDER_EXISTED:
    ret += esp_mail_str_205;
    break;
  case MAIL_CLIENT_ERROR_OUT_OF_MEMORY:
    ret += esp_mail_str_186;
    break;
  case SMTP_STATUS_NO_SUPPORTED_AUTH:
    ret += esp_mail_str_42;
    break;
  case TCP_CLIENT_ERROR_SEND_DATA_FAILED:
    ret += esp_mail_str_344;
    break;
  case TCP_CLIENT_ERROR_CONNECTION_REFUSED:
    ret += esp_mail_str_345;
    break;
  case TCP_CLIENT_ERROR_NOT_INITIALIZED:
    ret += esp_mail_str_346;
    break;
  case MAIL_CLIENT_ERROR_CUSTOM_CLIENT_DISABLED:
    ret += esp_mail_str_352;
    break;
  case MB_FS_ERROR_FILE_IO_ERROR:
    ret += esp_mail_str_282;
    break;

#if defined(MBFS_FLASH_FS) || defined(MBFS_SD_FS)

  case MB_FS_ERROR_FLASH_STORAGE_IS_NOT_READY:
    ret += esp_mail_str_348;
    break;

  case MB_FS_ERROR_SD_STORAGE_IS_NOT_READY:
    ret += esp_mail_str_349;
    break;

  case MB_FS_ERROR_FILE_STILL_OPENED:
    ret += esp_mail_str_350;
    break;

  case MB_FS_ERROR_FILE_NOT_FOUND:
    ret += esp_mail_str_351;
    break;

#endif
  default:
    break;
  }

  if (_smtpStatus.text.length() > 0 && ret.length() == 0)
  {
    ret = esp_mail_str_312;
    ret += _smtpStatus.respCode;
    ret += esp_mail_str_313;
    ret += _smtpStatus.text;
    return ret.c_str();
  }
  return ret.c_str();
}

bool SMTPSession::closeSession()
{
  if (!_tcpConnected)
    return false;

  if (_sendCallback)
  {
    _cbData._sentSuccess = _sentSuccessCount;
    _cbData._sentFailed = _sentFailedCount;
    MailClient.smtpCB(this, "");
    MailClient.smtpCBP(this, esp_mail_str_128);
  }

  if (_debug)
    MailClient.debugInfoP(esp_mail_str_245);

  bool ret = true;

/* Sign out */
#if defined(ESP32)
  /**
   * The strange behavior in ESP8266 SSL client, BearSSLWiFiClientSecure
   * The client disposed without memory released after the server close
   * the connection due to QUIT command, which caused the memory leaks.
   */
  MailClient.smtpSendP(this, esp_mail_str_7, true);
  _smtp_cmd = esp_mail_smtp_cmd_logout;
  ret = MailClient.handleSMTPResponse(this, esp_mail_smtp_status_code_221, SMTP_STATUS_SEND_BODY_FAILED);
#endif

  if (ret)
  {

    if (_sendCallback)
    {
      MailClient.smtpCB(this, "");
      MailClient.smtpCBP(this, esp_mail_str_129, false);
    }

    if (_debug)
      MailClient.debugInfoP(esp_mail_str_246);

    if (_sendCallback)
      MailClient.smtpCB(this, "", true);
  }

  return MailClient.handleSMTPError(this, 0, ret);
}

void SMTPSession::callback(smtpStatusCallback smtpCallback)
{
  _sendCallback = smtpCallback;
}

void SMTPSession::setSystemTime(time_t ts)
{
  this->client.setSystemTime(ts);
}

void SMTPSession::setClient(Client *client)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.setClient(client);
#endif
}

void SMTPSession::connectionRequestCallback(ConnectionRequestCallback connectCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.connectionRequestCallback(connectCB);
#endif
}

void SMTPSession::connectionUpgradeRequestCallback(ConnectionUpgradeRequestCallback upgradeCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.connectionUpgradeRequestCallback(upgradeCB);
#endif
}

void SMTPSession::networkConnectionRequestCallback(NetworkConnectionRequestCallback networkConnectionCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.networkConnectionRequestCallback(networkConnectionCB);
#endif
}

void SMTPSession::networkStatusRequestCallback(NetworkStatusRequestCallback networkStatusCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.networkStatusRequestCallback(networkStatusCB);
#endif
}

void SMTPSession::setNetworkStatus(bool status)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
  this->client.setNetworkStatus(status);
#endif
}

SMTP_Status::SMTP_Status()
{
}

SMTP_Status::~SMTP_Status()
{
  empty();
}

const char *SMTP_Status::info()
{
  return _info.c_str();
}

bool SMTP_Status::success()
{
  return _success;
}

size_t SMTP_Status::completedCount()
{
  return _sentSuccess;
}

size_t SMTP_Status::failedCount()
{
  return _sentFailed;
}

void SMTP_Status::empty()
{
  _info.clear();
}

#endif

ESP_Mail_Client MailClient = ESP_Mail_Client();

#endif /* ESP_Mail_Client_CPP */