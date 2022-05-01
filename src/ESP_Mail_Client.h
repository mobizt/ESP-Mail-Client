#ifndef ESP_Mail_Client_H
#define ESP_Mail_Client_H

#define ESP_MAIL_VERSION "2.1.4"

/**
 * Mail Client Arduino Library for Espressif's ESP32 and ESP8266 and SAMD21 with u-blox NINA-W102 WiFi/Bluetooth module
 *
 *   Version:   2.1.4
 *   Released:  May 1, 2022
 *
 *   Updates:
 * - Add support NTP time synching timed out debug.
 *
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

#include <Arduino.h>
#include "extras/RFC2047.h"
#include <time.h>
#include <ctype.h>
#if !defined(__AVR__)
#include <algorithm>
#include <string>
#include <vector>
#endif

#include "extras/ESPTimeHelper/ESPTimeHelper.h"
#include "extras/MIMEInfo.h"

#if defined(ESP32) || defined(ESP8266)

#define UPLOAD_CHUNKS_NUM 12
#define ESP_MAIL_PRINTF ESP_MAIL_DEFAULT_DEBUG_PORT.printf

#if defined(ESP32)

#include <WiFi.h>
#include <ETH.h>
#define ESP_MAIL_MIN_MEM 70000

#elif defined(ESP8266)

#include <ESP8266WiFi.h>
#define SD_CS_PIN 15
#define ESP_MAIL_MIN_MEM 4000

#endif

#else

#undef min
#undef max
#define ESP_MAIL_MIN_MEM 3000
#define UPLOAD_CHUNKS_NUM 5

#include "extras/mb_print/mb_print.h"

extern "C" __attribute__((weak)) void
mb_print_putchar(char c)
{
  ESP_MAIL_DEFAULT_DEBUG_PORT.print(c);
}

#define ESP_MAIL_PRINTF mb_print_printf

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char *sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif // __arm__

#endif

#include "wcs/ESP_TCP_Clients.h"

using namespace mb_string;

class IMAPSession;
class SMTPSession;
class SMTP_Status;
class DownloadProgress;
class MessageData;

#if defined(ENABLE_IMAP)

class MessageList
{
public:
  friend class IMAPSession;
  MessageList(){};
  ~MessageList() { clear(); };
  void add(int uid)
  {
    if (uid > 0)
      _list.push_back(uid);
  }

  void clear() { _list.clear(); }

private:
  MB_VECTOR<int> _list;
};

/* The class that provides the info of selected or opened mailbox folder */
class SelectedFolderInfo
{
public:
  friend class ESP_Mail_Client;
  friend class IMAPSession;
  SelectedFolderInfo(){};
  ~SelectedFolderInfo() { clear(); };

  /* Get the flags count for this mailbox */
  size_t flagCount() { return _flags.size(); };

  /* Get the numbers of messages in this mailbox */
  size_t msgCount() { return _msgCount; };

  /* Get the numbers of messages in this mailbox that recent flag was set */
  size_t recentCount() { return _recentCount; };

  /* Get the order of message in number of message in this mailbox that reoved */
  /**
   * The IMAP_Polling_Status has the properties e.g. type, messageNum, and argument.
   *
   * The type property is the type of status e.g.imap_polling_status_type_undefined, imap_polling_status_type_new_message,
   * imap_polling_status_type_remove_message, and imap_polling_status_type_fetch_message.
   *
   * The messageNum property is message number or order from the total number of message that added, fetched or deleted.
   *
   * The argument property is the argument of commands e.g. FETCH
   */
  IMAP_Polling_Status pollingStatus() { return _polling_status; };

  /* Get the predict next message UID */
  size_t nextUID() { return _nextUID; };

  /* Get the index of first unseen message */
  size_t unseenIndex() { return _unseenMsgIndex; };

  /* Get the numbers of messages from search result based on the search criteria
   */
  size_t searchCount() { return _searchCount; };

  /* Get the numbers of messages to be stored in the ressult */
  size_t availableMessages() { return _availableItems; };

  /* Get the flag argument at the specified index */
  String flag(size_t index)
  {
    if (index < _flags.size())
      return _flags[index].c_str();
    return "";
  }

private:
  void addFlag(const char *flag)
  {
    MB_String s = flag;
    _flags.push_back(s);
  };
  void clear()
  {
    for (size_t i = 0; i < _flags.size(); i++)
      _flags[i].clear();
    _flags.clear();
  }
  size_t _msgCount = 0;
  size_t _recentCount = 0;
  size_t _nextUID = 0;
  size_t _unseenMsgIndex = 0;
  size_t _searchCount = 0;
  size_t _availableItems = 0;
  unsigned long _idleTimeMs = 0;
  bool _folderChanged = false;
  bool _floderChangedState = false;
  IMAP_Polling_Status _polling_status;
  MB_VECTOR<MB_String> _flags;
};

/* The class that provides the list of FolderInfo e.g. name, attributes and
 * delimiter */
class FoldersCollection
{
public:
  friend class ESP_Mail_Client;
  friend class IMAPSession;
  FoldersCollection(){};
  ~FoldersCollection() { clear(); };
  size_t size() { return _folders.size(); };

  struct esp_mail_folder_info_item_t info(size_t index)
  {
    struct esp_mail_folder_info_item_t fd;
    if (index < _folders.size())
    {
      fd.name = _folders[index].name.c_str();
      fd.attributes = _folders[index].attributes.c_str();
      fd.delimiter = _folders[index].delimiter.c_str();
    }
    return fd;
  }

private:
  void add(struct esp_mail_folder_info_t &fd) { _folders.push_back(fd); };
  void clear()
  {
    for (size_t i = 0; i < _folders.size(); i++)
    {
      if (_folders[i].name.length() > 0)
        _folders[i].name.clear();
      if (_folders[i].attributes.length() > 0)
        _folders[i].attributes.clear();
      if (_folders[i].delimiter.length() > 0)
        _folders[i].delimiter.clear();
    }
    _folders.clear();
  }
  MB_VECTOR<esp_mail_folder_info_t> _folders;
};

/* The class that provides the status of message feching and searching */
class IMAP_Status
{
public:
  IMAP_Status();
  ~IMAP_Status();
  const char *info();
  bool success();
  void empty();
  friend class IMAPSession;

  MB_String _info;
  bool _success = false;
};

typedef void (*imapStatusCallback)(IMAP_Status);
typedef void (*imapResponseCallback)(const char *);

#endif

#if defined(ENABLE_SMTP)

/* The SMTP message class */
class SMTP_Message
{
public:
  SMTP_Message(){};
  ~SMTP_Message() { clear(); };

  void resetAttachItem(SMTP_Attachment &att)
  {
    att.blob.size = 0;
    att.blob.data = nullptr;
    att.file.path.clear();
    att.file.storage_type = esp_mail_file_storage_type_none;
    att.descr.name.clear();
    att.descr.filename.clear();
    att.descr.transfer_encoding.clear();
    att.descr.content_encoding.clear();
    att.descr.mime.clear();
    att.descr.content_id.clear();
    att._int.att_type = esp_mail_att_type_none;
    att._int.index = 0;
    att._int.msg_uid = 0;
    att._int.flash_blob = false;
    att._int.binary = false;
    att._int.parallel = false;
    att._int.cid.clear();
  }

  void clear()
  {
    sender.name.clear();
    sender.email.clear();
    subject.clear();
    text.charSet.clear();
    text.content.clear();
    text.content_type.clear();
    text.embed.enable = false;
    html.charSet.clear();
    html.content.clear();
    html.content_type.clear();
    html.embed.enable = false;
    response.reply_to.clear();
    response.notify = esp_mail_smtp_notify::esp_mail_smtp_notify_never;
    priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;

    for (size_t i = 0; i < _rcp.size(); i++)
    {
      _rcp[i].name.clear();
      _rcp[i].email.clear();
    }

    for (size_t i = 0; i < _cc.size(); i++)
      _cc[i].email.clear();

    for (size_t i = 0; i < _bcc.size(); i++)
      _bcc[i].email.clear();

    for (size_t i = 0; i < _hdr.size(); i++)
      _hdr[i].clear();

    for (size_t i = 0; i < _att.size(); i++)
    {
      _att[i].descr.filename.clear();
      _att[i].blob.data = nullptr;
      _att[i].descr.mime.clear();
      _att[i].descr.name.clear();
      _att[i].blob.size = 0;
      _att[i].descr.transfer_encoding.clear();
      _att[i].file.path.clear();
      _att[i].file.storage_type = esp_mail_file_storage_type_none;
    }

    for (size_t i = 0; i < _parallel.size(); i++)
    {
      _parallel[i].descr.filename.clear();
      _parallel[i].blob.data = nullptr;
      _parallel[i].descr.mime.clear();
      _parallel[i].descr.name.clear();
      _parallel[i].blob.size = 0;
      _parallel[i].descr.transfer_encoding.clear();
      _parallel[i].file.path.clear();
      _parallel[i].file.storage_type = esp_mail_file_storage_type_none;
    }
    _rcp.clear();
    _cc.clear();
    _bcc.clear();
    _hdr.clear();
    _att.clear();
    _parallel.clear();
  }

  /** Clear all the inline images
   */
  void clearInlineimages()
  {
    for (int i = (int)_att.size() - 1; i >= 0; i--)
    {
      if (_att[i]._int.att_type == esp_mail_att_type_inline)
        _att.erase(_att.begin() + i);
    }
  };

  /* Clear all the attachments */
  void clearAttachments()
  {
    for (int i = (int)_att.size() - 1; i >= 0; i--)
    {
      if (_att[i]._int.att_type == esp_mail_att_type_attachment)
        _att.erase(_att.begin() + i);
    }

    for (int i = (int)_parallel.size() - 1; i >= 0; i--)
      _parallel.erase(_parallel.begin() + i);
  };

  /** Clear all rfc822 message attachment
   */
  void clearRFC822Messages()
  {
    for (int i = (int)_rfc822.size() - 1; i >= 0; i--)
    {
      _rfc822[i].clear();
      _rfc822.erase(_rfc822.begin() + i);
    }
  };

  /** Clear the primary recipient mailboxes
   */
  void clearRecipients() { _rcp.clear(); };

  /** Clear the Carbon-copy recipient mailboxes
   */
  void clearCc() { _cc.clear(); };

  /** Clear the Blind-carbon-copy recipient mailboxes
   */
  void clearBcc() { _bcc.clear(); };

  /** Clear the custom message headers
   */
  void clearHeader() { _hdr.clear(); };

  /** Add attachment to the message
   *
   * @param att The SMTP_Attachment data item
   */
  void addAttachment(SMTP_Attachment &att)
  {
    att._int.att_type = esp_mail_att_type_attachment;
    att._int.parallel = false;
    att._int.flash_blob = true;
    _att.push_back(att);
  };

  /** Add parallel attachment to the message
   *
   * @param att The SMTP_Attachment data item
   */
  void addParallelAttachment(SMTP_Attachment &att)
  {
    att._int.att_type = esp_mail_att_type_attachment;
    att._int.parallel = true;
    att._int.flash_blob = true;
    _parallel.push_back(att);
  };

  /** Add inline image to the message
   *
   * @param att The SMTP_Attachment data item
   */
  void addInlineImage(SMTP_Attachment &att)
  {
    att._int.flash_blob = true;
    att._int.parallel = false;
    att._int.att_type = esp_mail_att_type_inline;
    char *tmp;
#if defined(BOARD_HAS_PSRAM) && defined(MB_STRING_USE_PSRAM)

    if ((tmp = (char *)ps_malloc(36)) == 0)
      return;

#else

    if ((tmp = (char *)malloc(36)) == 0)
      return;

#endif
    memset(tmp, 0, 36);
    sprintf(tmp, (const char *)MBSTRING_FLASH_MCR("%d"), (int)random(2000, 4000));
    att._int.cid = tmp;
    free(tmp);
    _att.push_back(att);
  };

  /** Add rfc822 message to the message
   *
   * @param msg The RFC822_Message class object
   */
  void addMessage(SMTP_Message &msg) { _rfc822.push_back(msg); }

  /** Add the primary recipient mailbox to the message
   *
   * @param name The name of primary recipient
   * @param email The Email address of primary recipient
   */
  template <typename T1 = const char *, typename T2 = const char *>
  void addRecipient(T1 name, T2 email)
  {
    struct esp_mail_smtp_recipient_t rcp;
    rcp.name = toStringPtr(name);
    rcp.email = toStringPtr(email);
    _rcp.push_back(rcp);
  };

  /** Add Carbon-copy recipient mailbox
   *
   * @param email The Email address of the secondary recipient
   */
  template <typename T = const char *>
  void addCc(T email)
  {
    struct esp_mail_smtp_recipient_address_t cc;
    cc.email = toStringPtr(email);
    _cc.push_back(cc);
  };

  /** Add Blind-carbon-copy recipient mailbox
   *
   * @param email The Email address of the tertiary recipient
   */
  template <typename T = const char *>
  void addBcc(T email)
  {
    struct esp_mail_smtp_recipient_address_t bcc;
    bcc.email = toStringPtr(email);
    _bcc.push_back(bcc);
  };

  /** Add the custom header to the message
   *
   * @param hdr The header name and value
   */
  template <typename T = const char *>
  void addHeader(T hdr)
  {
    _hdr.push_back(MB_String().setPtr(toStringPtr(hdr)));
  };

  /* The message author config */
  struct esp_mail_email_info_t sender;

  /* The topic of message */
  MB_String subject;

  /* The message type */
  byte type = esp_mail_msg_type_none;

  /* The PLAIN text message */
  struct esp_mail_plain_body_t text;

  /* The HTML text message */
  struct esp_mail_html_body_t html;

  /* The response config */
  struct esp_mail_smtp_msg_response_t response;

  /* The priority of the message */
  esp_mail_smtp_priority priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;

  /* The enable options */
  struct esp_mail_smtp_enable_option_t enable;

  /* The message from config */
  struct esp_mail_email_info_t from;

  /* The message identifier */
  MB_String messageID;

  /* The keywords or phrases, separated by commas */
  MB_String keywords;

  /* The comments about message */
  MB_String comments;

  /* The date of message */
  MB_String date;

  /* The field that contains the parent's message ID of the message to which this one is a reply */
  MB_String in_reply_to;

  /* The field that contains the parent's references (if any) and followed by the parent's message ID (if any) of the message to which this one is a reply */
  MB_String references;

private:
  friend class ESP_Mail_Client;
  MB_VECTOR<struct esp_mail_smtp_recipient_t> _rcp;
  MB_VECTOR<struct esp_mail_smtp_recipient_address_t> _cc;
  MB_VECTOR<struct esp_mail_smtp_recipient_address_t> _bcc;
  MB_VECTOR<MB_String> _hdr;
  MB_VECTOR<SMTP_Attachment> _att;
  MB_VECTOR<SMTP_Attachment> _parallel;
  MB_VECTOR<SMTP_Message> _rfc822;
};

class SMTP_Status
{
public:
  friend class SMTPSession;
  friend class ESP_Mail_Client;

  SMTP_Status();
  ~SMTP_Status();
  const char *info();
  bool success();
  void empty();
  size_t completedCount();
  size_t failedCount();

private:
  MB_String _info;
  bool _success = false;
  size_t _sentSuccess = 0;
  size_t _sentFailed = 0;
};

typedef void (*smtpStatusCallback)(SMTP_Status);

#endif

class ESP_Mail_Client
{

public:
  ESP_Mail_Client()
  {
    mbfs = new MB_FS();
  };
  ~ESP_Mail_Client()
  {
    if (mbfs)
      delete mbfs;
  };

#if defined(ENABLE_SMTP)
  /** Sending Email through the SMTP server
   *
   * @param smtp The pointer to SMTP session object which holds the data and the
   * TCP client.
   * @param msg The pointer to SMTP_Message class which contains the header,
   * body, and attachments.
   * @param closeSession The option to Close the SMTP session after sent.
   * @return The boolean value indicates the success of operation.
   */
  bool sendMail(SMTPSession *smtp, SMTP_Message *msg, bool closeSession = true);
#endif

#if defined(ENABLE_IMAP)
  /** Reading Email through IMAP server.
   *
   * @param imap The pointer to IMAP session object which holds the data and
   the TCP client.

   * @param closeSession The option to close the IMAP session after fetching or
   searching the Email.
   * @return The boolean value indicates the success of operation.
  */
  bool readMail(IMAPSession *imap, bool closeSession = true);

  /** Set the argument to the Flags for the specified message.
   *
   * @param imap The pointer to IMAP session object which holds the data and the
   * TCP client.
   * @param msgUID The UID of the message.
   * @param flags The flag list to set.
   * @param closeSession The option to close the IMAP session after set flag.
   * @return The boolean value indicates the success of operation.
   */
  template <typename T = const char *>
  bool setFlag(IMAPSession *imap, int msgUID, T flags, bool closeSession) { return mSetFlag(imap, msgUID, toStringPtr(flags), 0, closeSession); }

  /** Add the argument to the Flags for the specified message.
   *
   * @param imap The pointer to IMAP session object which holds the data and the
   * TCP client.
   * @param msgUID The UID of the message.
   * @param flags The flag list to set.
   * @param closeSession The option to close the IMAP session after add flag.
   * @return The boolean value indicates the success of operation.
   */
  template <typename T = const char *>
  bool addFlag(IMAPSession *imap, int msgUID, T flags, bool closeSession) { return mSetFlag(imap, msgUID, toStringPtr(flags), 1, closeSession); }

  /** Remove the argument from the Flags for the specified message.
   *
   * @param imap The pointer to IMAP session object which holds the data and the
   * TCP client.
   * @param msgUID The UID of the message that flags to be removed.
   * @param flags The flag list to remove.
   * @param closeSession The option to close the IMAP session after remove flag.
   * @return The boolean value indicates the success of operation.
   */
  template <typename T = const char *>
  bool removeFlag(IMAPSession *imap, int msgUID, T flags, bool closeSession) { return mSetFlag(imap, msgUID, toStringPtr(flags), 2, closeSession); }
#endif

#if defined(MBFS_SD_FS) && defined(MBFS_CARD_TYPE_SD)

  /** SD card config with GPIO pins.
   *
   * @param ss SPI Chip/Slave Select pin.
   * @param sck SPI Clock pin.
   * @param miso SPI MISO pin.
   * @param mosi SPI MOSI pin.
   * @return Boolean type status indicates the success of the operation.
   */
  bool sdBegin(int8_t ss = -1, int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1);

#if defined(ESP8266)

  /** SD card config with SD FS configurations (ESP8266 only).
   *
   * @param ss SPI Chip/Slave Select pin.
   * @param sdFSConfig The pointer to SDFSConfig object (ESP8266 only).
   * @return Boolean type status indicates the success of the operation.
   */
  bool sdBegin(SDFSConfig *sdFSConfig);

#endif

#if defined(ESP32)
  /** SD card config with chip select and SPI configuration (ESP32 only).
   *
   * @param ss SPI Chip/Slave Select pin.
   * @param spiConfig The pointer to SPIClass object for SPI configuartion (ESP32 only).
   * @return Boolean type status indicates the success of the operation.
   */
  bool sdBegin(int8_t ss, SPIClass *spiConfig = nullptr);
#endif

#if defined(MBFS_ESP32_SDFAT_ENABLED) || defined(MBFS_SDFAT_ENABLED)
  /** SD card config with SdFat SPI and pins configurations (ESP32 with SdFat included only).
   *
   * @param sdFatSPIConfig The pointer to SdSpiConfig object for SdFat SPI configuration.
   * @param ss SPI Chip/Slave Select pin.
   * @param sck SPI Clock pin.
   * @param miso SPI MISO pin.
   * @param mosi SPI MOSI pin.
   * @return Boolean type status indicates the success of the operation.
   */
  bool sdBegin(SdSpiConfig *sdFatSPIConfig, int8_t ss = -1, int8_t sck = -1, int8_t miso = -1, int8_t mosi = -1);
#endif

#endif

#if defined(ESP32) && defined(MBFS_SD_FS) && defined(MBFS_CARD_TYPE_SD_MMC)
  /** Initialize the SD_MMC card (ESP32 only).
   *
   * @param mountpoint The mounting point.
   * @param mode1bit Allow 1 bit data line (SPI mode).
   * @param format_if_mount_failed Format SD_MMC card if mount failed.
   * @return The boolean value indicates the success of operation.
   */
  bool sdMMCBegin(const char *mountpoint = "/sdcard", bool mode1bit = false, bool format_if_mount_failed = false);
#endif

  /** Get free Heap memory.
   *
   * @return Free memory amount in byte.
   */
  int getFreeHeap();


  ESPTimeHelper Time;

private:
  friend class SMTPSession;
  friend class IMAPSession;

  MB_FS *mbfs = nullptr;
  bool _clockReady = false;
  time_t ts = 0;

#if defined(ENABLE_SMTP) || defined(ENABLE_IMAP)

  unsigned long _lastReconnectMillis = 0;
  uint16_t _reconnectTimeout = ESP_MAIL_NETWORK_RECONNECT_TIMEOUT;

  char *strReplace(char *orig, char *rep, char *with);
  char *strReplaceP(char *buf, PGM_P key, PGM_P value);
  bool authFailed(char *buf, int bufLen, int &chunkIdx, int ofs);
  void createDirs(MB_String dirs);

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)
  void setSecure(ESP_MAIL_TCP_CLIENT &client, ESP_Mail_Session *session, std::shared_ptr<const char> caCert);
#endif

  size_t getReservedLen(size_t len);
  void debugInfoP(PGM_P info);
  bool validEmail(const char *s);
  char *getRandomUID();
  void splitTk(MB_String &str, MB_VECTOR<MB_String> &tk, const char *delim);
  unsigned char *decodeBase64(const unsigned char *src, size_t len, size_t *out_len);
  MB_String encodeBase64Str(const unsigned char *src, size_t len);
  MB_String encodeBase64Str(uint8_t *src, size_t len);

  char *subStr(const char *buf, PGM_P beginH, PGM_P endH, int beginPos, int endPos = 0, bool caseSensitive = true);
  void strcat_c(char *str, char c);
  int strpos(const char *haystack, const char *needle, int offset, bool caseSensitive = true);
  void *newP(size_t len);
  void delP(void *ptr);
  char *newS(char *p, size_t len);
  char *newS(char *p, size_t len, char *d);
  bool strcmpP(const char *buf, int ofs, PGM_P beginH, bool caseSensitive = true);
  int strposP(const char *buf, PGM_P beginH, int ofs, bool caseSensitive = true);
  char *strP(PGM_P pgm);
  void setTime(float gmt_offset, float day_light_offset, const char *ntp_server, bool wait);
#endif

#if defined(ENABLE_SMTP)
  int readLine(SMTPSession *smtp, char *buf, int bufLen, bool crlf, int &count);
  void encodeQP(const char *buf, char *out);
  void formatFlowedText(MB_String &content);
  void softBreak(MB_String &content, const char *quoteMarks);
  void getMIME(const char *ext, MB_String &mime);
  void mimeFromFile(const char *name, MB_String &mime);
  MB_String getBoundary(size_t len);
  bool mSendMail(SMTPSession *smtp, SMTP_Message *msg, bool closeSession = true);
  bool reconnect(SMTPSession *smtp, unsigned long dataTime = 0);
  void closeTCPSession(SMTPSession *smtp);
  void errorStatusCB(SMTPSession *smtp, int error);
  size_t smtpSendP(SMTPSession *smtp, PGM_P v, bool newline = false);
  size_t smtpSend(SMTPSession *smtp, const char *data, bool newline = false);
  size_t smtpSend(SMTPSession *smtp, int data, bool newline = false);
  size_t smtpSend(SMTPSession *smtp, uint8_t *data, size_t size);
  bool handleSMTPError(SMTPSession *smtp, int err, bool ret = false);
  bool sendParallelAttachments(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary);
  bool sendAttachments(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary, bool parallel = false);
  bool sendMSGData(SMTPSession *smtp, SMTP_Message *msg, bool closeSession, bool rfc822MSG);
  bool sendRFC822Msg(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary, bool closeSession, bool rfc822MSG);
  void getRFC822MsgEnvelope(SMTPSession *smtp, SMTP_Message *msg, MB_String &buf);
  bool bdat(SMTPSession *smtp, SMTP_Message *msg, int len, bool last);
  void checkBinaryData(SMTPSession *smtp, SMTP_Message *msg);
  bool sendBlob(SMTPSession *smtp, SMTP_Message *msg, SMTP_Attachment *att);
  bool sendFile(SMTPSession *smtp, SMTP_Message *msg, SMTP_Attachment *att);
  bool openFileRead(SMTPSession *smtp, SMTP_Message *msg, SMTP_Attachment *att, MB_String &s, MB_String &buf, const MB_String &boundary, bool inlined);
  bool openFileRead2(SMTPSession *smtp, SMTP_Message *msg, const char *path, esp_mail_file_storage_type storageType);
  bool sendInline(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary, byte type);
  void sendStorageNotReadyError(SMTPSession *smtp, esp_mail_file_storage_type storageType);
  size_t numAtt(SMTPSession *smtp, esp_mail_attach_type type, SMTP_Message *msg);
  bool checkEmail(SMTPSession *smtp, SMTP_Message *msg);
  bool sendPartText(SMTPSession *smtp, SMTP_Message *msg, byte type, const char *boundary);
  bool sendMSG(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary);
  void getAttachHeader(MB_String &header, const MB_String &boundary, SMTP_Attachment *attach, size_t size);
  void getRFC822PartHeader(SMTPSession *smtp, MB_String &header, const MB_String &boundary);
  void getInlineHeader(MB_String &header, const MB_String &boundary, SMTP_Attachment *inlineAttach, size_t size);
  bool sendBlobBody(SMTPSession *smtp, SMTP_Message *msg, uint8_t type);
  bool sendFileBody(SMTPSession *smtp, SMTP_Message *msg, uint8_t type);
  void encodingText(SMTPSession *smtp, SMTP_Message *msg, uint8_t type, MB_String &content);
  bool sendBase64(SMTPSession *smtp, SMTP_Message *msg, const unsigned char *data, size_t len, bool flashMem, const char *filename, bool report);
  bool sendBase64Raw(SMTPSession *smtp, SMTP_Message *msg, const uint8_t *data, size_t len, bool flashMem, const char *filename, bool report);
  bool sendBase64Stream(SMTPSession *smtp, SMTP_Message *msg, esp_mail_file_storage_type storageType, const char *filename, bool report);
  bool sendBase64StreamRaw(SMTPSession *smtp, SMTP_Message *msg, esp_mail_file_storage_type storageType, const char *filename, bool report);
  void smtpCBP(SMTPSession *smtp, PGM_P info, bool success = false);
  void smtpCB(SMTPSession *smtp, const char *info, bool success = false);
  void getResponseStatus(const char *buf, esp_mail_smtp_status_code respCode, int beginPos, struct esp_mail_smtp_response_status_t &status);
  void handleAuth(SMTPSession *smtp, char *buf);
  MB_String getEncodedToken(SMTPSession *smtp);
  bool connected(SMTPSession *smtp);
  bool setSendingResult(SMTPSession *smtp, SMTP_Message *msg, bool result);
  bool smtpAuth(SMTPSession *smtp);
  bool handleSMTPResponse(SMTPSession *smtp, esp_mail_smtp_status_code respCode, int errCode);
  void uploadReport(const char *filename, int &lastProgress, int progress);
  MB_FS *getMBFS();
  int setTimestamp(time_t ts);
#endif

#if defined(ENABLE_IMAP)

      RFC2047_Decoder RFC2047Decoder;
  int readLine(IMAPSession *imap, char *buf, int bufLen, bool crlf, int &count);
  bool multipartMember(const MB_String &part, const MB_String &check);
  int decodeChar(const char *s);
  void decodeQP(const char *buf, char *out);
  char *decode7Bit(char *buf);
  esp_mail_char_decoding_scheme getEncodingFromCharset(const char *enc);
  void decodeHeader(MB_String &headerField);
  int decodeLatin1_UTF8(unsigned char *out, int *outlen, const unsigned char *in, int *inlen);
  void decodeTIS620_UTF8(char *out, const char *in, size_t len);
  bool reconnect(IMAPSession *imap, unsigned long dataTime = 0, bool downloadRequestuest = false);
  void closeTCPSession(IMAPSession *imap);
  bool getMultipartFechCmd(IMAPSession *imap, int msgIdx, MB_String &partText);
  bool fetchMultipartBodyHeader(IMAPSession *imap, int msgIdx);
  bool connected(IMAPSession *imap);
  bool imapAuth(IMAPSession *imap);
  bool sendIMAPCommand(IMAPSession *imap, int msgIndex, int cmdCase);
  void errorStatusCB(IMAPSession *imap, int error);
  size_t imapSendP(IMAPSession *imap, PGM_P v, bool newline = false);
  size_t imapSend(IMAPSession *imap, const char *data, bool nwline = false);
  size_t imapSend(IMAPSession *imap, int data, bool newline = false);
  MB_String getEncodedToken(IMAPSession *imap);
  bool imapLogout(IMAPSession *imap);
  void imapCBP(IMAPSession *imap, PGM_P info, bool success);
  void imapCB(IMAPSession *imap, const char *info, bool success);
  void sendStorageNotReadyError(IMAPSession *imap, esp_mail_file_storage_type storageType);
  int getMSGNUM(IMAPSession *imap, char *buf, int bufLen, int &chunkIdx, bool &endSearch, int &nump, const char *key, const char *pc);
  void handleHeader(IMAPSession *imap, char *buf, int bufLen, int &chunkIdx, struct esp_mail_message_header_t &header, int &headerState, int &octetCount, bool caseSensitive = true);
  void setHeader(IMAPSession *imap, char *buf, struct esp_mail_message_header_t &header, int state);
  void handlePartHeader(IMAPSession *imap, char *buf, int &chunkIdx, struct esp_mail_message_part_info_t &part, bool caseSensitive = true);

  struct esp_mail_message_part_info_t *cPart(IMAPSession *imap);
  struct esp_mail_message_header_t *cHeader(IMAPSession *imap);
  bool handleIMAPResponse(IMAPSession *imap, int errCode, bool closeSession);
  void downloadReport(IMAPSession *imap, int progress);
  void fetchReport(IMAPSession *imap, int progress, bool html);
  void searchReport(int progress, const char *percent);
  int cMSG(IMAPSession *imap);
  int cIdx(IMAPSession *imap);
  esp_mail_imap_response_status imapResponseStatus(IMAPSession *imap, char *response);
  void addHeaderItem(MB_String &str, esp_mail_message_header_t *header, bool json);
  void addHeaders(MB_String &s, esp_mail_imap_rfc822_msg_header_item_t *header, bool json);
  void addHeader(MB_String &s, const char *name, const MB_String &value, bool trim, bool json);
  void addHeader(MB_String &s, const char *name, int value, bool json);
  void saveHeader(IMAPSession *imap, bool json);
  void prepareFilePath(IMAPSession *imap, MB_String &filePath, bool header);
  void decodeText(IMAPSession *imap, char *buf, int bufLen, int &chunkIdx, MB_String &filePath, bool &downloadRequest, int &octetLength, int &readDataLen);
  bool handleAttachment(IMAPSession *imap, char *buf, int bufLen, int &chunkIdx, MB_String &filePath, bool &downloadRequest, int &octetCount, int &octetLength);
  void handleFolders(IMAPSession *imap, char *buf);
  void prepareFileList(IMAPSession *imap, MB_String &filePath);
  void handleCapability(IMAPSession *imap, char *buf, int &chunkIdx);
  bool handleIdle(IMAPSession *imap);
  void handleGetUID(IMAPSession *imap, char *buf);
  void handleGetFlags(IMAPSession *imap, char *buf);
  void handleExamine(IMAPSession *imap, char *buf);
  bool handleIMAPError(IMAPSession *imap, int err, bool ret);
  bool mSetFlag(IMAPSession *imap, int msgUID, MB_StringPtr flags, uint8_t action, bool closeSession);

#endif
};

#if defined(ENABLE_IMAP)

class IMAPSession
{
public:
  IMAPSession(Client *client);
  IMAPSession();
  ~IMAPSession();

  /** Assign custom Client from Arduino Clients.
   *
   * @param client The pointer to Arduino Client derived class e.g. WiFiClient, WiFiClientSecure, EthernetClient or GSMClient.
   */
  void setClient(Client *client);

  /** Assign the callback function to handle the server connection for custom Client.
   *
   * @param connectionCB The function that handles the server connection.
   */
  void connectionRequestCallback(ConnectionRequestCallback connectionCB);

  /** Assign the callback function to handle the server upgrade connection for custom Client.
   *
   * @param upgradeCB The function that handles existing connection upgrade.
   */
  void connectionUpgradeRequestCallback(ConnectionUpgradeRequestCallback upgradeCB);

  /** Assign the callback function to handle the network connection for custom Client.
   *
   * @param networkConnectionCB The function that handles the network connection.
   */
  void networkConnectionRequestCallback(NetworkConnectionRequestCallback networkConnectionCB);

  /** Assign the callback function to handle the network connection status acknowledgement.
   *
   * @param networkStatusCB The function that handle the network connection status acknowledgement.
   */
  void networkStatusRequestCallback(NetworkStatusRequestCallback networkStatusCB);

  /** Set the network status acknowledgement.
   *
   * @param status The network status.
   */
  void setNetworkStatus(bool status);

  /** Begin the IMAP server connection.
   *
   * @param session The pointer to ESP_Mail_Session structured data that keeps
   * the server and log in details.
   * @param config The pointer to IMAP_Config structured data that keeps the
   * operation options.
   * @return The boolean value which indicates the success of operation.
   */
  bool connect(ESP_Mail_Session *session, IMAP_Config *config);

  /** Close the IMAP session.
   *
   * @return The boolean value which indicates the success of operation.
   */
  bool closeSession();

  /** Set to enable the debug.
   *
   * @param level The level to enable the debug message
   * level = 0, no debug
   * level = 1, basic debug
   * level = 2, full debug 1
   * level = 333, full debug 2
   */
  void debug(int level);

  /** Get the list of all the mailbox folders since the TCP session was opened
   * and user was authenticated.
   *
   * @param folders The FoldersCollection class that contains the collection of
   * the
   * FolderInfo structured data.
   * @return The boolean value which indicates the success of operation.
   */
  bool getFolders(FoldersCollection &folders);

  /** Select or open the mailbox folder to search or fetch the message inside.
   *
   * @param folderName The known mailbox folder name. The default name is INBOX.
   * @param readOnly The option to open the mailbox for read only. Set this
   * option to false when you wish
   * to modify the Flags using the setFlag, addFlag and removeFlag functions.
   * @return The boolean value which indicates the success of operation.
   *
   * @note: the function will exit immediately and return true if the time since previous success folder selection (open)
   * with the same readOnly mode, is less than 5 seconds.
   */
  template <typename T = const char *>
  bool selectFolder(T folderName, bool readOnly = true) { return mSelectFolder(toStringPtr(folderName), readOnly); }

  /** Open the mailbox folder to read or search the mesages.
   *
   * @param folderName The name of known mailbox folder to be opened.
   * @param readOnly The option to open the mailbox for reading only. Set this
   * option to false when you wish
   * to modify the flags using the setFlag, addFlag and removeFlag functions.
   * @return The boolean value which indicates the success of operation.
   *
   * @note: the function will exit immediately and return true if the time since previous success folder selection (open)
   * with the same readOnly mode, is less than 5 seconds.
   */
  template <typename T = const char *>
  bool openFolder(T folderName, bool readOnly = true) { return mOpenFolder(toStringPtr(folderName), readOnly); }

  /** Close the mailbox folder that was opened.
   *
   * @param folderName The known mailbox folder name.
   * @return The boolean value which indicates the success of operation.
   */
  template <typename T = const char *>
  bool closeFolder(T folderName) { return mCloseFolder(toStringPtr(folderName)); }

  /** Create folder.
   *
   * @param folderName The name of folder to create.
   * @return The boolean value which indicates the success of operation.
   */
  template <typename T = const char *>
  bool createFolder(T folderName) { return mCreateFolder(toStringPtr(folderName)); }

  /** Delete folder.
   *
   * @param folderName The name of folder to delete.
   * @return The boolean value which indicates the success of operation.
   */
  template <typename T = const char *>
  bool deleteFolder(T folderName) { return mDeleteFolder(toStringPtr(folderName)); }

  /** Get UID number in selected or opened mailbox.
   *
   * @param msgNum The message number or order in the total message numbers.
   * @return UID number in selected or opened mailbox.
   *
   * @note Returns 0 when fail to get UID.
   */
  int getUID(int msgNum);

  /** Get message flags in selected or opened mailbox.
   *
   * @param msgNum The message number or order in the total message numbers.
   * @return Message flags in selected or opened mailbox.
   *
   * @note Returns empty string when fail to get flags.
   */
  const char *getFlags(int msgNum);

  /** Send the custom IMAP command and get the result via callback.
   *
   * @param cmd The command string.
   * @param callback The function that accepts the pointer to const char (const char*) as parameter.
   * @return he boolean value which indicates the success of operation.
   *
   * @note imap.connect and imap.selectFolder or imap.openFolder are needed to call once prior to call this function.
   */
  template <typename T = const char *>
  bool sendCustomCommand(T cmd, imapResponseCallback callback) { return mSendCustomCommand(toStringPtr(cmd), callback); }

  /** Copy the messages to the defined mailbox folder.
   *
   * @param toCopy The pointer to the MessageListList class that contains the
   * list of messages to copy.
   * @param dest The destination folder that the messages to copy to.
   * @return The boolean value which indicates the success of operation.
   */
  template <typename T = const char *>
  bool copyMessages(MessageList *toCopy, T dest) { return mCopyMessages(toCopy, toStringPtr(dest)); }

  /** Delete the messages in the opened mailbox folder.
   *
   * @param toDelete The pointer to the MessageListList class that contains the
   * list of messages to delete.
   * @param expunge The boolean option to expunge all messages.
   * @return The boolean value which indicates the success of operation.
   */
  bool deleteMessages(MessageList *toDelete, bool expunge = false);

  /** Listen for the selected or open mailbox for updates.
   * @return The boolean value which indicates the success of operation.
   */
  bool listen() { return mListen(false); };

  /** Stop listen for the mailbox for updates.
   * @return The boolean value which indicates the success of operation.
   */
  bool stopListen() { return mStopListen(false); };

  /** Check for the selected or open mailbox updates.
   * @return The boolean value which indicates the changes status of mailbox.
   */
  bool folderChanged();

  /** Assign the callback function that returns the operating status when
   * fetching or reading the Email.
   *
   * @param imapCallback The function that accepts the imapStatusCallback as
   * parameter.
   */
  void callback(imapStatusCallback imapCallback);

  /** Determine if no message body contained in the search result and only the
   * message header is available.
   */
  bool headerOnly();

  /** Get the message list from search or fetch the Emails
   *
   * @return The IMAP_MSG_List structured data which contains text and html
   * contents,
   * attachments, inline images, embedded rfc822 messages details for each
   * message.
   */
  IMAP_MSG_List data();

  /** Get the details of the selected or opned mailbox folder
   *
   * @return The SelectedFolderInfo class which contains the info about flags,
   * total messages, next UID,
   * search count and the available messages count.
   */
  SelectedFolderInfo selectedFolder();

  /** Get the error details when readingg the Emails
   *
   * @return The string of error details.
   */
  String errorReason();

  /** Clear all the cache data stored in the IMAP session object.
   */
  void empty();

  /** Get the JSON string of file name list of files that stored in SD card.
   *
   * @return The JSON string of filenames.
   * @note This will available only when standard SD library was used and file storage is SD.
   */
  String fileList();

  /** Set the current timestamp.
   *
   * @param ts The current timestamp.
   */
  void setSystemTime(time_t ts);

  friend class ESP_Mail_Client;
  friend class foldderList;

private:
  void clearMessageData();
  void checkUID();
  void checkPath();
  void getMessages(uint16_t messageIndex, struct esp_mail_imap_msg_item_t &msg);
  void getRFC822Messages(uint16_t messageIndex, struct esp_mail_imap_msg_item_t &msg);
  bool closeMailbox();
  bool openMailbox(MB_StringPtr folder, esp_mail_imap_auth_mode mode, bool waitResponse);
  bool getMailboxes(FoldersCollection &flders);
  bool checkCapability();
  bool mListen(bool recon);
  bool mStopListen(bool recon);
  bool mSendCustomCommand(MB_StringPtr cmd, imapResponseCallback callback);
  bool mDeleteFolder(MB_StringPtr folderName);
  bool mCreateFolder(MB_StringPtr folderName);
  bool mCopyMessages(MessageList *toCopy, MB_StringPtr dest);
  bool mCloseFolder(MB_StringPtr folderName);
  bool mOpenFolder(MB_StringPtr folderName, bool readOnly);
  bool mSelectFolder(MB_StringPtr folderName, bool readOnly);

  bool _tcpConnected = false;
  unsigned long _last_polling_error_ms = 0;
  unsigned long _last_host_check_ms = 0;
  struct esp_mail_imap_response_status_t _imapStatus;
  int _cMsgIdx = 0;
  int _cPartIdx = 0;
  int _totalRead = 0;
  MB_VECTOR<struct esp_mail_message_header_t> _headers;

  esp_mail_imap_command _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_login;
  MB_VECTOR<struct esp_mail_imap_multipart_level_t> _multipart_levels;
  int _rfc822_part_count = 0;
  bool _unseen = false;
  bool _readOnlyMode = true;
  struct esp_mail_auth_capability_t _auth_capability;
  struct esp_mail_imap_capability_t _read_capability;
  ESP_Mail_Session *_sesson_cfg;
  MB_String _currentFolder;
  bool _mailboxOpened = false;
  unsigned long _lastSameFolderOpenMillis = 0;
  MB_String _nextUID;
  MB_String _unseenMsgIndex;
  MB_String _flags_tmp;
  MB_String _sdFileList;

  struct esp_mail_imap_read_config_t *_config = nullptr;

  bool _headerOnly = true;
  bool _uidSearch = false;
  bool _headerSaved = false;
  bool _debug = false;
  int _debugLevel = 0;
  bool _secure = false;
  imapStatusCallback _readCallback = NULL;
  imapResponseCallback _customCmdResCallback = NULL;

  MB_VECTOR<uint32_t> _msgUID;
  FoldersCollection _folders;
  SelectedFolderInfo _mbif;
  int _uid_tmp = 0;
  int _lastProgress = -1;
  int _certType = -1;
#if defined(ESP32) || defined(ESP8266)
  std::shared_ptr<const char> _caCert = nullptr;
#endif

  ESP_MAIL_TCP_CLIENT client;

  IMAP_Status _cbData;
};

#endif

#if defined(ENABLE_SMTP)

class SendingResult
{
private:
  MB_VECTOR<SMTP_Result> _result;

  void add(SMTP_Result *r)
  {
    _result.push_back(*r);
  }

public:
  friend class SMTPSession;
  friend class ESP_Mail_Client;
  SendingResult(){};
  ~SendingResult() { clear(); };

  void clear()
  {
    for (size_t i = 0; i < _result.size(); i++)
    {
      _result[i].recipients.clear();
      _result[i].subject.clear();
      _result[i].timestamp = 0;
      _result[i].completed = false;
    }
    _result.clear();
  }

  SMTP_Result getItem(size_t index)
  {
    SMTP_Result r;
    if (index < _result.size())
      return _result[index];
    return r;
  }
  size_t size() { return _result.size(); };
};

class SMTPSession
{
public:
  SMTPSession(Client *client);
  SMTPSession();
  ~SMTPSession();

  /** Assign custom Client from Arduino Clients.
   *
   * @param client The pointer to Arduino Client derived class e.g. WiFiClient, WiFiClientSecure, EthernetClient or GSMClient.
   */
  void setClient(Client *client);

  /** Assign the callback function to handle the server connection for custom Client.
   *
   * @param connectionCB The function that handles the server connection.
   */
  void connectionRequestCallback(ConnectionRequestCallback connectionCB);

  /** Assign the callback function to handle the server upgrade connection for custom Client.
   *
   * @param upgradeCB The function that handles existing connection upgrade.
   */
  void connectionUpgradeRequestCallback(ConnectionUpgradeRequestCallback upgradeCB);

  /** Assign the callback function to handle the network connection for custom Client.
   *
   * @param networkConnectionCB The function that handles the network connection.
   */
  void networkConnectionRequestCallback(NetworkConnectionRequestCallback networkConnectionCB);

  /** Assign the callback function to handle the network connection status acknowledgement.
   *
   * @param networkStatusCB The function that handle the network connection status acknowledgement.
   */
  void networkStatusRequestCallback(NetworkStatusRequestCallback networkStatusCB);

  /** Set the network status acknowledgement.
   *
   * @param status The network status.
   */
  void setNetworkStatus(bool status);

  /** Begin the SMTP server connection.
   *
   * @param session The pointer to ESP_Mail_Session structured data that keeps
   * the server and log in details.
   * @return The boolean value indicates the success of operation.
   */
  bool connect(ESP_Mail_Session *session);

  /** Close the SMTP session.
   *
   */
  bool closeSession();

  /** Set to enable the debug.
   *
   * @param level The level to enable the debug message
   * level = 0, no debugging
   * level = 1, basic level debugging
   */
  void debug(int level);

  /** Get the error details when sending the Email
   *
   * @return The string of error details.
   */
  String errorReason();

  /** Set the Email sending status callback function.
   *
   * @param smtpCallback The callback function that accept the
   * smtpStatusCallback param.
   */
  void callback(smtpStatusCallback smtpCallback);

  /** Set the current timestamp.
   *
   * @param ts The current timestamp.
   */
  void setSystemTime(time_t ts);

  SendingResult sendingResult;

  friend class ESP_Mail_Client;

private:
  bool _tcpConnected = false;
  struct esp_mail_smtp_response_status_t _smtpStatus;
  int _sentSuccessCount = 0;
  int _sentFailedCount = 0;
  bool _chunkedEnable = false;
  int _chunkCount = 0;

  esp_mail_smtp_command _smtp_cmd = esp_mail_smtp_command::esp_mail_smtp_cmd_greeting;
  struct esp_mail_auth_capability_t _auth_capability;
  struct esp_mail_smtp_capability_t _send_capability;
  ESP_Mail_Session *_sesson_cfg = NULL;

  bool _debug = false;
  int _debugLevel = 0;
  bool _secure = false;
  smtpStatusCallback _sendCallback = NULL;

  SMTP_Status _cbData;
  struct esp_mail_smtp_msg_type_t _msgType;
  int _lastProgress = -1;

  int _certType = -1;
#if defined(ESP32) || defined(ESP8266)
  std::shared_ptr<const char> _caCert = nullptr;
#endif

  ESP_MAIL_TCP_CLIENT client;
};

#endif

extern ESP_Mail_Client MailClient;

#endif // ESP_Mail_Client_H
