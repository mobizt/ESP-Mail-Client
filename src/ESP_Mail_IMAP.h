
#ifndef ESP_MAIL_IMAP_H
#define ESP_MAIL_IMAP_H

/**
 * Mail Client Arduino Library for Espressif's ESP32 and ESP8266, Raspberry Pi RP2040 Pico, and SAMD21 with u-blox NINA-W102 WiFi/Bluetooth module
 *
 * Created January 26, 2023
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

#include "ESP_Mail_Client_Version.h"
#include "ESP_Mail_Client.h"

#if defined(ENABLE_IMAP)

int ESP_Mail_Client::decodeChar(const char *s)
{
    return 16 * hexval(*(s + 1)) + hexval(*(s + 2));
}

void ESP_Mail_Client::decodeQP_UTF8(const char *buf, char *out)
{
    char *tmp = strP(esp_mail_str_295);
    int idx = 0;
    while (*buf)
    {
        if (*buf != '=')
            out[idx++] = *buf++;
        else if (*(buf + 1) == '\r' && *(buf + 2) == '\n')
            buf += 3;
        else if (*(buf + 1) == '\n')
            buf += 2;
        else if (!strchr(tmp, *(buf + 1)))
            out[idx++] = *buf++;
        else if (!strchr(tmp, *(buf + 2)))
            out[idx++] = *buf++;
        else
        {
            out[idx++] = decodeChar(buf);
            buf += 3;
        }
    }
    delP(&tmp);
}

char *ESP_Mail_Client::decode7Bit_UTF8(char *buf)
{
    MB_String s;

    // only non NULL and 7-bit ASCII are allowed

    // rfc2045 section 2.7

    size_t len = buf ? strlen(buf) : 0;

    for (size_t i = 0; i < len; i++)
    {
        if (buf[i] > 0 && buf[i] < 128 && i < 998)
            s.append(1, buf[i]);
    }

    // some special chars can't send in 7bit unless encoded as queoted printable string
    char *decoded = (char *)newP(s.length() + 10);
    decodeQP_UTF8(s.c_str(), decoded);
    s.clear();
    return decoded;
}

char *ESP_Mail_Client::decode8Bit_UTF8(char *buf)
{
    MB_String s;

    // only non NULL and less than 998 octet length are allowed

    // rfc2045 section 2.8

    size_t len = buf ? strlen(buf) : 0;

    for (size_t i = 0; i < len; i++)
    {
        if (buf[i] > 0 && i < 998)
            s.append(1, buf[i]);
    }

    char *decoded = (char *)newP(s.length() + 1);
    strcpy(decoded, s.c_str());
    s.clear();
    return decoded;
}

void ESP_Mail_Client::decodeHeader(IMAPSession *imap, MB_String &headerField)
{

    size_t p1 = 0, p2 = 0;
    MB_String headerEnc;

    while (headerField[p1] == ' ' && p1 < headerField.length() - 1)
        p1++;

    if (headerField[p1] == '=' && headerField[p1 + 1] == '?')
    {
        p2 = headerField.find('?', p1 + 2);
        if (p2 != MB_String::npos)
        {
            headerEnc = headerField.substr(p1 + 2, p2 - p1 - 2);
        }
    }

    int bufSize = headerField.length() + 10;
    char *buf = (char *)newP(bufSize);

    // Content Q and B decodings
    RFC2047Decoder.decode(mbfs, buf, headerField.c_str(), bufSize);

    // Char set decoding
    if (imap->_charDecCallback)
    {
        IMAP_Decoding_Info decoding;

        decoding.charset = headerEnc.c_str();
        decoding.data = buf;
        decoding.type = IMAP_Decoding_Info::message_part_type_header;

        imap->_charDecCallback(&decoding);

        if (decoding.decodedString.length() > 0)
        {
            char *buf2 = (char *)newP(decoding.decodedString.length() + 1);
            strcpy(buf2, decoding.decodedString.c_str());
            delP(&buf);
            buf = buf2;
        }
    }
    else if (getEncodingFromCharset(headerEnc.c_str()) == esp_mail_char_decoding_scheme_iso8859_1)
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

    if (strposP(enc, esp_mail_str_237, 0, false) > -1 || strposP(enc, esp_mail_str_231, 0, false) > -1 || strposP(enc, esp_mail_str_226, 0, false) > -1)
        scheme = esp_mail_char_decoding_scheme_tis620;
    else if (strposP(enc, esp_mail_str_227, 0, false) > -1)
        scheme = esp_mail_char_decoding_scheme_iso8859_1;
    else if (strpos(enc, "utf-8", 0, false) > -1)
        scheme = esp_mail_char_decoding_scheme_utf_8;

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
    if (imap->_uidSearch || imap->_imap_msg_num[msgIndex].type == esp_mail_imap_msg_num_type_uid)
        cmd += imap->prependTag(esp_mail_str_27, esp_mail_str_142);
    else
        cmd += imap->prependTag(esp_mail_str_27, esp_mail_str_143);

    cmd += imap->_imap_msg_num[msgIndex].value;

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

#if defined(MB_MCU_ESP)

    int cmem = MailClient.getFreeHeap();

    if (cmem < ESP_MAIL_MIN_MEM)
    {
        if (imap->_debug)
        {
            esp_mail_debug_print();
            errorStatusCB(imap, MAIL_CLIENT_ERROR_OUT_OF_MEMORY);
        }
        goto out;
    }
#endif

    // new session
    if (!imap->_tcpConnected)
    {
        // authenticate new

        bool ssl = false;

        if (!imap->connect(ssl))
        {
            closeTCPSession(imap);
            return false;
        }

        if (!imapAuth(imap, ssl))
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

        if (imap->_config->fetch.sequence_set.string.length() > 0 || imap->_config->fetch.uid.length() > 0 || imap->_config->fetch.number.length() > 0)
            imap->_headerOnly = false;
        else
            imap->_headerOnly = true;
    }

    imap->_rfc822_part_count = 0;
    imap->_mbif._availableItems = 0;
    imap->_imap_msg_num.clear();
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
                imapCB(imap, esp_mail_str_66, true, false);

            if (imap->_debug)
                esp_mail_debug_print(esp_mail_str_232, true);

            if (strposP(imap->_config->search.criteria.c_str(), esp_mail_str_137, 0) != -1)
            {
                imap->_uidSearch = true;
                command += esp_mail_str_138;
            }

            command += esp_mail_str_139;

            imap->_config->search.criteria.trim();

            MB_String tag = esp_mail_str_27;
            tag += esp_mail_str_131;

            // Remove reserved tag used internal
            if (strpos(imap->_config->search.criteria.c_str(), tag.c_str(), 0, true) == 0)
                imap->_config->search.criteria.erase(0, tag.length());

            for (size_t i = 0; i < imap->_config->search.criteria.length(); i++)
            {
                if (imap->_config->search.criteria[i] != ' ' && imap->_config->search.criteria[i] != '\r' && imap->_config->search.criteria[i] != '\n')
                    buf.append(1, imap->_config->search.criteria[i]);

                if (imap->_config->search.criteria[i] == ' ')
                {

                    if ((imap->_uidSearch && strcmp(buf.c_str(), pgm2Str(esp_mail_str_140)) == 0) || (imap->_unseen && buf.find(pgm2Str(esp_mail_str_224)) != MB_String::npos))
                        buf.clear();

                    if (strcmp(buf.c_str(), pgm2Str(esp_mail_str_141)) != 0 && buf.length() > 0)
                    {
                        command += esp_mail_str_131;
                        command += buf;
                    }

                    buf.clear();
                }
            }

            if (imap->_unseen && strpos(imap->_config->search.criteria.c_str(), pgm2Str(esp_mail_str_223), 0) == -1)
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
                imapCB(imap, s.c_str(), false, false);

                if (imap->_imap_msg_num.size() > 0)
                {
                    s = esp_mail_str_69;
                    s += imap->_mbif._searchCount;
                    s += esp_mail_str_70;
                    imapCB(imap, s.c_str(), false, false);

                    s = esp_mail_str_71;
                    s += imap->_imap_msg_num.size();
                    s += esp_mail_str_70;
                    imapCB(imap, s.c_str(), false, false);
                }
                else
                    imapCB(imap, esp_mail_str_72, false, false);
            }
        }
        else
        {
            if (imap->_readCallback)
                imapCB(imap, esp_mail_str_73, false, false);

            imap->_mbif._availableItems++;

            esp_mail_imap_msg_num_t msg_num;
            msg_num.type = esp_mail_imap_msg_num_type_number;
            msg_num.value = (uint32_t)imap->_mbif._msgCount;

            imap->_imap_msg_num.push_back(msg_num);
            imap->_headerOnly = false;
            imap->_config->fetch.number = imap->_mbif._msgCount;
        }
    }
    else
    {

        if (imap->_config->fetch.sequence_set.string.length() > 0)
        {
            imap->_headerOnly = imap->_config->fetch.sequence_set.headerOnly;
            imap->mFetchSequenceSet();

            imap->_mbif._availableItems = imap->_imap_msg_num.size();
        }
        else
        {
            if (imap->_config->fetch.uid.length() > 0)
            {
                imap->_mbif._availableItems++;

                esp_mail_imap_msg_num_t msg_num;
                msg_num.type = esp_mail_imap_msg_num_type_uid;
                msg_num.value = (uint32_t)atoi(imap->_config->fetch.uid.c_str());

                imap->_imap_msg_num.push_back(msg_num);
            }

            if (imap->_config->fetch.number.length() > 0)
            {
                imap->_mbif._availableItems++;

                esp_mail_imap_msg_num_t msg_num;
                msg_num.type = esp_mail_imap_msg_num_type_number;
                msg_num.value = (uint32_t)atoi(imap->_config->fetch.number.c_str());

                imap->_imap_msg_num.push_back(msg_num);
            }
        }
    }

    for (size_t i = 0; i < imap->_imap_msg_num.size(); i++)
    {
        imap->_cMsgIdx = i;
        imap->_totalRead++;

#if defined(MB_MCU_ESP)
        if (MailClient.getFreeHeap() - (imap->_config->limit.msg_size * (i + 1)) < ESP_MAIL_MIN_MEM)
        {
            if (imap->_debug)
                errorStatusCB(imap, MAIL_CLIENT_ERROR_OUT_OF_MEMORY);
            goto out;
        }
#endif

        if (imap->_readCallback)
        {
            readCount++;

            MB_String s = esp_mail_str_74;
            s += imap->_totalRead;

            if (imap->_uidSearch || imap->_imap_msg_num[i].type == esp_mail_imap_msg_num_type_uid)
                s += esp_mail_str_75;
            else
                s += esp_mail_str_76;

            s += imap->_imap_msg_num[i].value;
            imapCB(imap, s.c_str(), true, false);
        }

        if (imap->_debug)
            esp_mail_debug_print(esp_mail_str_233, true);

        MB_String cmd;
        if (imap->_uidSearch || imap->_imap_msg_num[i].type == esp_mail_imap_msg_num_type_uid)
            cmd += imap->prependTag(esp_mail_str_27, esp_mail_str_142);
        else
            cmd += imap->prependTag(esp_mail_str_27, esp_mail_str_143);

        if (imap->_debug)
            esp_mail_debug_print(esp_mail_str_77, true);

        cmd += imap->_imap_msg_num[i].value;

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

        if (imap->_imap_msg_num[i].type == esp_mail_imap_msg_num_type_number)
        {

            cHeader(imap)->message_uid = imap->getUID(cHeader(imap)->message_no);
        }

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
                // single part
                if (imap->_debug)
                {
                    MB_String s = esp_mail_str_81;
                    s += 1;
                    esp_mail_debug_print(s.c_str(), true);
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
                    if (imap->_config->storage.saved_path.length() == 0)
                        imap->_config->storage.saved_path = esp_mail_str_202;
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
                    imap->_sdFileList = esp_mail_str_218;

                if (cHeader(imap)->attachment_count > 0 && imap->_readCallback)
                {
                    MB_String s = esp_mail_str_34;
                    s += esp_mail_str_78;
                    s += cHeader(imap)->attachment_count;
                    s += esp_mail_str_79;
                    imapCB(imap, s.c_str(), false, false);

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
                            imapCB(imap, s.c_str(), false, false);
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

                    bool rfc822_body_subtype = cPart(imap)->message_sub_type == esp_mail_imap_message_sub_type_rfc822 && cPart(imap)->attach_type != esp_mail_att_type_attachment;

                    if (cPart(imap)->attach_type == esp_mail_att_type_none && (cPart(imap)->msg_type == esp_mail_msg_type_html || cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched))
                    {

                        bool ret = ((imap->_config->enable.rfc822 || imap->_config->download.rfc822) && rfc822_body_subtype) || (!rfc822_body_subtype && ((imap->_config->enable.text && (cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched)) || (imap->_config->enable.html && cPart(imap)->msg_type == esp_mail_msg_type_html) || (cPart(imap)->msg_type == esp_mail_msg_type_html && imap->_config->download.html) || ((cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched) && imap->_config->download.text)));
                        if (!ret)
                            continue;

                        if ((imap->_config->download.rfc822 && rfc822_body_subtype) || (!rfc822_body_subtype && ((cPart(imap)->msg_type == esp_mail_msg_type_html && imap->_config->download.html) || ((cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched) && imap->_config->download.text))))
                        {

                            if (ccnt == 0)
                                imapCB(imap, esp_mail_str_57, true, false);

                            if (imap->_debug)
                            {
                                if (cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched)
                                    esp_mail_debug_print(esp_mail_str_59, true);
                                else if (cPart(imap)->msg_type == esp_mail_msg_type_html)
                                    esp_mail_debug_print(esp_mail_str_60, true);
                            }
                        }
                        else
                        {
                            if (ccnt == 0)
                                imapCB(imap, esp_mail_str_307, true, false);

                            if (imap->_debug)
                            {
                                if (cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched)
                                    esp_mail_debug_print(esp_mail_str_308, true);
                                else if (cPart(imap)->msg_type == esp_mail_msg_type_html)
                                    esp_mail_debug_print(esp_mail_str_309, true);
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
                                imapCB(imap, esp_mail_str_80, true, false);

                            if (imap->_debug)
                            {
                                MB_String s = esp_mail_str_55;
                                s += acnt + 1;
                                s += MBSTRING_FLASH_MCR(" of ");
                                s += cHeader(imap)->attachment_count;
                                esp_mail_debug_print(s.c_str(), true);
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
                                    idle();
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
                    imapCB(imap, esp_mail_str_124, true, false);

                saveHeader(imap, false);
                saveHeader(imap, true);
            }

            // save files list file
            if (imap->_sdFileList.length() > 0)
            {
                MB_String filepath = cHeader(imap)->message_uid;
                filepath += mimeinfo[3].endsWith; // .txt
                if (mbfs->open(filepath, mbfs_type imap->_config->storage.type, mb_fs_open_mode_write) > -1)
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
            esp_mail_debug_print(s.c_str(), true);
        }
    }
#if defined(MB_MCU_ESP)
out:
#endif
    if (readCount < imap->_imap_msg_num.size())
    {
        imap->_mbif._availableItems = readCount;
        imap->_imap_msg_num.erase(imap->_imap_msg_num.begin() + readCount, imap->_imap_msg_num.end());
    }

    if (closeSession)
    {
        if (!imap->closeSession())
            return false;
    }
    else
    {
        if (imap->_readCallback)
            imapCB(imap, esp_mail_str_87, true, false);

        if (imap->_debug)
            esp_mail_debug_print(esp_mail_str_88, true);
    }

    if (imap->_readCallback)
        imapCB(imap, "", false, true);

    return true;
}

bool ESP_Mail_Client::getMultipartFechCmd(IMAPSession *imap, int msgIdx, MB_String &partText)
{
    if (imap->_multipart_levels.size() == 0)
        return false;

    int cLevel = imap->_multipart_levels.size() - 1;

    cHeader(imap)->partNumStr.clear();

    if (imap->_uidSearch || imap->_imap_msg_num[msgIdx].type == esp_mail_imap_msg_num_type_uid)
        partText = imap->prependTag(esp_mail_str_27, esp_mail_str_142);
    else
        partText = imap->prependTag(esp_mail_str_27, esp_mail_str_143);

    partText += imap->_imap_msg_num[msgIdx].value;

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

bool ESP_Mail_Client::multipartMember(const MB_String &parent, const MB_String &child)
{
    if (parent.length() > child.length())
        return false;

    for (size_t i = 0; i < parent.length(); i++)
        if (parent[i] != child[i])
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

    // slower than BODYSTRUCTURE parsing but sure
    do
    {

        struct esp_mail_message_part_info_t *_cpart = &cHeader(imap)->part_headers[cHeader(imap)->message_data_count - 1];
        bool rfc822_body_subtype = _cpart->message_sub_type == esp_mail_imap_message_sub_type_rfc822 && _cpart->attach_type != esp_mail_att_type_attachment;

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
            esp_mail_debug_print(s.c_str(), true);
        }

        // Try fetching the part and its sub parts hierarchically
        // Some sub part may not exist at the current multipart level
        if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return false;

        imap->_imap_cmd = esp_mail_imap_cmd_fetch_body_mime;
        ret = handleIMAPResponse(imap, IMAP_STATUS_IMAP_RESPONSE_FAILED, false);

        _cpart = &cHeader(imap)->part_headers[cHeader(imap)->message_data_count - 1];
        rfc822_body_subtype = _cpart->message_sub_type == esp_mail_imap_message_sub_type_rfc822 && _cpart->attach_type != esp_mail_att_type_attachment;
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
                    // Get additional rfc822 message header
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

bool ESP_Mail_Client::imapAuth(IMAPSession *imap, bool &ssl)
{

non_authenticated:

    // capabilities may change after TLS negotiation
    if (!imap->checkCapabilities())
        return false;

    // start TLS when needed or the server issue
    if ((imap->_auth_capability.start_tls || imap->_sesson_cfg->secure.startTLS) && !ssl)
    {
        MB_String s;
        if (imap->_readCallback)
            imapCB(imap, esp_mail_str_209, true, false);

        if (imap->_debug)
            esp_mail_debug_print(esp_mail_str_196, true);

        s = imap->prependTag(esp_mail_str_27, esp_mail_str_311);

        imapSend(imap, s.c_str(), false);

        // rfc2595 section 3.1
        imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_starttls;
        if (!handleIMAPResponse(imap, IMAP_STATUS_BAD_COMMAND, false))
            return false;

        if (imap->_debug)
            esp_mail_debug_print(esp_mail_str_310, true);

        // connect in secure mode
        // do TLS handshake

        if (!imap->client.connectSSL(imap->_sesson_cfg->certificate.verify))
            return handleIMAPError(imap, MAIL_CLIENT_ERROR_SSL_TLS_STRUCTURE_SETUP, false);

        // set the secure mode
        imap->_sesson_cfg->secure.startTLS = false;
        ssl = true;
        imap->_secure = true;

        // check the capabilitiy again to prevent the man in the middle attack
        goto non_authenticated;
    }

    imap->clearMessageData();
    imap->_mailboxOpened = false;

    bool creds = imap->_sesson_cfg->login.email.length() > 0 && imap->_sesson_cfg->login.password.length() > 0;
    bool sasl_auth_oauth = imap->_sesson_cfg->login.accessToken.length() > 0 && imap->_auth_capability.xoauth2;
    bool sasl_login = creds;
    bool sasl_auth_plain = imap->_auth_capability.plain && creds;

    bool supported_sasl = sasl_auth_oauth || sasl_login || sasl_auth_plain;

    if (!supported_sasl)
        return handleIMAPError(imap, IMAP_STATUS_NO_SUPPORTED_AUTH, false);

    // rfc4959
    if (supported_sasl)
    {
        if (imap->_readCallback)
            imapCB(imap, esp_mail_str_56, true, false);
    }

    if (sasl_auth_oauth)
    {

        if (!imap->_auth_capability.xoauth2)
        {
            handleIMAPError(imap, IMAP_STATUS_SERVER_OAUTH2_LOGIN_DISABLED, false);
            return false;
        }

        if (imap->_debug)
            esp_mail_debug_print(esp_mail_str_291, true);

        MB_String cmd = imap->prependTag(esp_mail_str_27, esp_mail_str_292);

        if (imap->_auth_capability.sasl_ir)
        {
            cmd += esp_mail_str_131;
            cmd += getXOAUTH2String(imap->_sesson_cfg->login.email, imap->_sesson_cfg->login.accessToken);
            if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;
        }
        else
        {
            if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;

            imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_sasl_auth_oauth;
            if (!handleIMAPResponse(imap, IMAP_STATUS_LOGIN_FAILED, true))
                return false;

            cmd = getXOAUTH2String(imap->_sesson_cfg->login.email, imap->_sesson_cfg->login.accessToken);

            if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;
        }

        imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_sasl_auth_oauth;
        if (!handleIMAPResponse(imap, IMAP_STATUS_LOGIN_FAILED, false))
            return false;
    }
    else if (sasl_auth_plain)
    {

        if (imap->_debug)
            esp_mail_debug_print(esp_mail_str_290, true);

        int len = imap->_sesson_cfg->login.email.length() + imap->_sesson_cfg->login.password.length() + 2;
        uint8_t *tmp = (uint8_t *)newP(len);
        memset(tmp, 0, len);
        int p = 1;
        memcpy(tmp + p, imap->_sesson_cfg->login.email.c_str(), imap->_sesson_cfg->login.email.length());
        p += imap->_sesson_cfg->login.email.length() + 1;
        memcpy(tmp + p, imap->_sesson_cfg->login.password.c_str(), imap->_sesson_cfg->login.password.length());
        p += imap->_sesson_cfg->login.password.length();

        MB_String s = imap->prependTag(esp_mail_str_27, esp_mail_str_41);

        if (imap->_auth_capability.sasl_ir)
        {
            s += esp_mail_str_131;
            s += encodeBase64Str(tmp, p);
            delP(&tmp);

            if (imapSend(imap, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;
        }
        else
        {

            if (imapSend(imap, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;

            imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_sasl_auth_plain;
            if (!handleIMAPResponse(imap, IMAP_STATUS_LOGIN_FAILED, true))
                return false;

            s = encodeBase64Str(tmp, p);
            delP(&tmp);

            if (imapSend(imap, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;
        }

        imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_sasl_auth_plain;
        if (!handleIMAPResponse(imap, IMAP_STATUS_LOGIN_FAILED, true))
            return false;
    }
    else if (sasl_login)
    {

        if (imap->_debug)
            esp_mail_debug_print(esp_mail_str_229, true);

        MB_String cmd = imap->prependTag(esp_mail_str_27, esp_mail_str_130);
        cmd += imap->_sesson_cfg->login.email;
        cmd += esp_mail_str_131;
        cmd += imap->_sesson_cfg->login.password;

        if (imapSend(imap, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return false;

        imap->_imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_sasl_login;
        if (!handleIMAPResponse(imap, IMAP_STATUS_LOGIN_FAILED, true))
            return false;
    }

    // auto capabilities after login?
    if (!imap->_read_capability.auto_caps)
    {
        if (!imap->checkCapabilities())
            return false;
    }

    return true;
}

bool ESP_Mail_Client::imapLogout(IMAPSession *imap)
{
    if (imap->_readCallback)
        imapCB(imap, esp_mail_str_85, false, false);

    if (imap->_debug)
        esp_mail_debug_print(esp_mail_str_234, true);

    if (imapSend(imap, imap->prependTag(esp_mail_str_27, esp_mail_str_146).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    imap->_imap_cmd = esp_mail_imap_cmd_logout;
    if (!handleIMAPResponse(imap, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    if (imap->_readCallback)
        imapCB(imap, esp_mail_str_187, true, false);

    if (imap->_debug)
        esp_mail_debug_print(esp_mail_str_235, true);

    return true;
}

void ESP_Mail_Client::errorStatusCB(IMAPSession *imap, int error)
{
    imap->_imapStatus.statusCode = error;
    MB_String s;
    if (imap->_readCallback && !imap->_customCmdResCallback)
    {
        s += esp_mail_str_53;
        s += imap->errorReason().c_str();
        imapCB(imap, s.c_str(), false, false);
    }

    if (imap->_debug && !imap->_customCmdResCallback)
    {
        s = esp_mail_str_185;
        s += imap->errorReason().c_str();
        esp_mail_debug_print(s.c_str(), true);
    }
}

size_t ESP_Mail_Client::imapSend(IMAPSession *imap, PGM_P data, bool newline)
{
    int sent = 0;

    if (!reconnect(imap))
    {
        closeTCPSession(imap);
        return sent;
    }

    if (!connected(imap))
    {
        errorStatusCB(imap, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
        return sent;
    }

    if (!imap->_tcpConnected)
    {
        errorStatusCB(imap, MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED);
        return sent;
    }

    MB_String s = data;

    int toSend = newline ? s.length() + 2 : s.length();

    if (imap->_debugLevel > esp_mail_debug_level_maintener && !imap->_customCmdResCallback)
        esp_mail_debug_print(s.c_str(), newline);

    sent = newline ? imap->client.println(s.c_str()) : imap->client.print(s.c_str());

    if (sent != toSend)
    {
        errorStatusCB(imap, sent);
        sent = 0;
    }

    return sent;
}

size_t ESP_Mail_Client::imapSend(IMAPSession *imap, int data, bool newline)
{
    MB_String s = data;
    return imapSend(imap, s.c_str(), newline);
}

size_t ESP_Mail_Client::imapSend(IMAPSession *imap, uint8_t *data, size_t size)
{
    int sent = 0;

    if (!reconnect(imap))
    {
        closeTCPSession(imap);
        return sent;
    }

    if (!connected(imap))
    {
        errorStatusCB(imap, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
        return sent;
    }

    if (!imap->_tcpConnected)
    {
        errorStatusCB(imap, MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED);
        return sent;
    }

    sent = imap->client.write(data, size);

    if (sent != (int)size)
    {
        errorStatusCB(imap, sent);
        sent = 0;
    }

    return sent;
}

bool ESP_Mail_Client::mSetFlag(IMAPSession *imap, MB_StringPtr sequenceSet, MB_StringPtr flag, esp_mail_imap_store_flag_type type, bool closeSession, bool silent, bool UID)
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
            imapCB(imap, esp_mail_str_153, false, false);

        if (imap->_debug)
        {
            MB_String e = esp_mail_str_185;
            e += esp_mail_str_151;
            esp_mail_debug_print(e.c_str(), true);
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
        if (type == esp_mail_imap_store_flag_type_set)
            imapCB(imap, esp_mail_str_157, true, false);
        else if (type == esp_mail_imap_store_flag_type_add)
            imapCB(imap, esp_mail_str_155, true, false);
        else
            imapCB(imap, esp_mail_str_154, true, false);
    }

    if (imap->_debug)
    {
        if (type == esp_mail_imap_store_flag_type_set)
            esp_mail_debug_print(esp_mail_str_253, true);
        else if (type == esp_mail_imap_store_flag_type_add)
            esp_mail_debug_print(esp_mail_str_254, true);
        else
            esp_mail_debug_print(esp_mail_str_255, true);
    }

    MB_String cmd;

    if (UID)
    {
        cmd = imap->prependTag(esp_mail_str_27, esp_mail_str_137);
        cmd += esp_mail_str_249;
    }
    else
        cmd = imap->prependTag(esp_mail_str_27, esp_mail_str_249);

    cmd += sequenceSet;

    if (type == esp_mail_imap_store_flag_type_set)
        cmd += esp_mail_str_250;
    else if (type == esp_mail_imap_store_flag_type_add)
        cmd += esp_mail_str_251;
    else
        cmd += esp_mail_str_252;

    if (silent)
        cmd += esp_mail_str_384;

    cmd += esp_mail_str_131;

    cmd += esp_mail_str_198;

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

void ESP_Mail_Client::imapCB(IMAPSession *imap, PGM_P info, bool prependCRLF, bool success)
{
    imap->_cbData._info.clear();

    if (prependCRLF)
        imap->_cbData._info = esp_mail_str_34;

    imap->_cbData._info += info;
    imap->_cbData._success = success;
    if (imap->_readCallback && !imap->_customCmdResCallback)
        imap->_readCallback(imap->_cbData);
}

int ESP_Mail_Client::parseSearchResponse(IMAPSession *imap, char *buf, int bufLen, int &chunkIdx, PGM_P tag, bool &endSearch, int &nump, const char *key, const char *pc)
{
    int ret = -1;
    char c = 0;
    int idx = 0;
    int num = 0;

    size_t tagLen = strlen_P(tag);
    MB_String _tag = tag;

    while (imap->client.available() > 0 && idx < bufLen)
    {
        idle();

        ret = imap->client.read();

        if (ret > -1)
        {

            if (idx >= bufLen - 1)
                return idx;

            c = (char)ret;

            if (c == '\n')
                c = ' ';

            buf[idx++] = c;

            if (chunkIdx == 0)
            {
                if (strcmp(buf, key) == 0)
                {
                    chunkIdx++;
                    return 0;
                }

                MB_String s = imap->prependTag(esp_mail_str_27, esp_mail_imap_response_1);

                if (strpos(buf, s.c_str(), 0) > -1)
                    goto end_search;
            }
            else
            {
                if (c == ' ')
                {
                    imap->_mbif._searchCount++;
                    if (imap->_config->enable.recent_sort)
                    {
                        esp_mail_imap_msg_num_t msg_num;
                        msg_num.type = imap->_uidSearch ? esp_mail_imap_msg_num_type_uid : esp_mail_imap_msg_num_type_number;
                        msg_num.value = (uint32_t)atoi(buf);

                        imap->_imap_msg_num.push_back(msg_num);

                        if (imap->_imap_msg_num.size() > imap->_config->limit.search)
                            imap->_imap_msg_num.erase(imap->_imap_msg_num.begin());
                    }
                    else
                    {
                        if (imap->_imap_msg_num.size() < imap->_config->limit.search)
                        {
                            esp_mail_imap_msg_num_t msg_num;
                            msg_num.type = imap->_uidSearch ? esp_mail_imap_msg_num_type_uid : esp_mail_imap_msg_num_type_number;
                            msg_num.value = (uint32_t)atoi(buf);

                            imap->_imap_msg_num.push_back(msg_num);
                        }
                    }

                    if (imap->_debug)
                    {
                        num = (float)(100.0f * imap->_mbif._searchCount / imap->_mbif._msgCount);
                        if (nump != num)
                        {
                            nump = num;
                            searchReport(imap, num, pc);
                        }
                    }

                    chunkIdx++;
                    return idx;
                }
                else if (idx >= (int)tagLen)
                {
                    if (strpos(buf, _tag.c_str(), 0, false) > -1)
                    {
#if defined(MB_USE_STD_VECTOR)
                        if (imap->_config->enable.recent_sort)
                            std::sort(imap->_imap_msg_num.begin(), imap->_imap_msg_num.end(), compareMore);
#else
                        if (imap->_config->enable.recent_sort)
                            numDecSort(imap->_imap_msg_num);
#endif
                        goto end_search;
                    }
                }
            }
        }
    }

    return idx;

end_search:

    endSearch = true;
    int read = imap->client.available();
    read = imap->client.readBytes(buf + idx, read);
    return idx + read;
}

#if !defined(MB_USE_STD_VECTOR)
void ESP_Mail_Client::numDecSort(MB_VECTOR<struct esp_mail_imap_msg_num_t> &arr)
{

    struct esp_mail_imap_msg_num_t tmp;

    for (size_t i = 0; i < arr.size(); ++i)
    {
        for (size_t j = i + 1; j < arr.size(); ++j)
        {
            if (arr[i].value < arr[j].value)
            {
                tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
    }
}
#endif

struct esp_mail_message_part_info_t *ESP_Mail_Client::cPart(IMAPSession *imap)
{
    return &cHeader(imap)->part_headers[imap->_cPartIdx];
}

struct esp_mail_message_header_t *ESP_Mail_Client::cHeader(IMAPSession *imap)
{
    return &imap->_headers[cIdx(imap)];
}

bool ESP_Mail_Client::parseHeaderState(IMAPSession *imap, const char *buf, PGM_P beginH, bool caseSensitive, struct esp_mail_message_header_t &header, int &headerState, esp_mail_imap_header_state state)
{
    if (strcmpP(buf, 0, beginH, caseSensitive))
    {
        headerState = state;
        char *tmp = subStr(buf, beginH, NULL, 0, -1, caseSensitive);
        if (tmp)
        {
            setHeader(imap, tmp, header, headerState);
            delP(&tmp);

            return true;
        }
    }

    return false;
}

void ESP_Mail_Client::parseHeaderResponse(IMAPSession *imap, char *buf, int bufLen, int &chunkIdx, struct esp_mail_message_header_t &header, int &headerState, int &octetCount, bool caseSensitive)
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

        chunkIdx++;

        if (parseHeaderState(imap, buf, esp_mail_str_10, caseSensitive, header, headerState, esp_mail_imap_state_from))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_150, caseSensitive, header, headerState, esp_mail_imap_state_sender))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_11, caseSensitive, header, headerState, esp_mail_imap_state_to))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_12, caseSensitive, header, headerState, esp_mail_imap_state_cc))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_24, caseSensitive, header, headerState, esp_mail_imap_state_subject))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_46, caseSensitive, header, headerState, esp_mail_imap_state_return_path))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_184, caseSensitive, header, headerState, esp_mail_imap_state_reply_to))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_109, caseSensitive, header, headerState, esp_mail_imap_state_in_reply_to))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_107, caseSensitive, header, headerState, esp_mail_imap_state_references))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_134, caseSensitive, header, headerState, esp_mail_imap_state_comments))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_145, caseSensitive, header, headerState, esp_mail_imap_state_keywords))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_172, caseSensitive, header, headerState, esp_mail_imap_state_content_transfer_encoding))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_190, caseSensitive, header, headerState, esp_mail_imap_state_accept_language))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_191, caseSensitive, header, headerState, esp_mail_imap_state_content_language))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_99, caseSensitive, header, headerState, esp_mail_imap_state_date))
            return;

        if (parseHeaderState(imap, buf, esp_mail_str_101, caseSensitive, header, headerState, esp_mail_imap_state_msg_id))
            return;

        if (strcmpP(buf, 0, esp_mail_str_25, caseSensitive))
        {
            headerState = esp_mail_imap_state_content_type;
            tmp = subStr(buf, esp_mail_str_25, esp_mail_str_97, 0, caseSensitive);
            if (tmp)
            {
                if (strpos(tmp, esp_mail_imap_multipart_sub_type_t::mixed, 0, caseSensitive) != -1)
                    header.hasAttachment = true;

                setHeader(imap, buf, header, headerState);
                delP(&tmp);
            }
        }
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
    case esp_mail_imap_state_from:
        header.header_fields.from += &buf[i];
        break;
    case esp_mail_imap_state_sender:
        header.header_fields.sender += &buf[i];
        break;
    case esp_mail_imap_state_to:
        header.header_fields.to += &buf[i];
        break;
    case esp_mail_imap_state_cc:
        header.header_fields.cc += &buf[i];
        break;
    case esp_mail_imap_state_subject:
        header.header_fields.subject += &buf[i];
        break;
    case esp_mail_imap_state_content_type:
        header.content_type += &buf[i];
        break;
    case esp_mail_imap_state_content_transfer_encoding:
        header.content_transfer_encoding += &buf[i];
        break;
    case esp_mail_imap_state_accept_language:
        header.accept_language += &buf[i];
        break;
    case esp_mail_imap_state_content_language:
        header.content_language += &buf[i];
        break;
    case esp_mail_imap_state_date:
        header.header_fields.date += &buf[i];
        break;
    case esp_mail_imap_state_msg_id:
        header.header_fields.messageID += &buf[i];
        break;
    case esp_mail_imap_state_return_path:
        header.header_fields.return_path += &buf[i];
        break;
    case esp_mail_imap_state_reply_to:
        header.header_fields.reply_to += &buf[i];
        break;
    case esp_mail_imap_state_in_reply_to:
        header.header_fields.in_reply_to += &buf[i];
        break;
    case esp_mail_imap_state_references:
        header.header_fields.references += &buf[i];
        break;
    case esp_mail_imap_state_comments:
        header.header_fields.comments += &buf[i];
        break;
    case esp_mail_imap_state_keywords:
        header.header_fields.keywords += &buf[i];
        break;
    case esp_mail_imap_state_char_set:
        header.char_set += &buf[i];
        break;
    case esp_mail_imap_state_boundary:
        header.boundary += &buf[i];
        break;
    default:
        break;
    }
}

bool ESP_Mail_Client::getDecodedHeader(IMAPSession *imap, const char *buf, PGM_P beginH, MB_String &out, bool caseSensitive)
{
    if (getHeader(buf, beginH, out, caseSensitive))
    {
        // decode header text
        decodeHeader(imap, out);
        return true;
    }

    return false;
}

void ESP_Mail_Client::parsePartHeaderResponse(IMAPSession *imap, const char *buf, int &chunkIdx, struct esp_mail_message_part_info_t &part, int &octetCount, bool caseSensitive)
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
                octetCount = 2;
                delP(&tmp);
            }
        }
    }
    else
    {
        MB_String value, old_value;
        bool valueStored = false;
        chunkIdx++;

        // if all octets read

        if (octetCount > part.octetLen)
        {

            // Is inline attachment without content id or name or filename?
            // It is supposed to be the inline message txt content, reset attach type to none

            if (part.attach_type == esp_mail_att_type_inline && part.CID.length() == 0)
                part.attach_type = esp_mail_att_type_none;

            // Is attachment file extension missing?
            // append extension

            if (part.attach_type == esp_mail_att_type_inline || part.attach_type == esp_mail_att_type_attachment)
            {
                if (part.filename.length() > 0 && part.filename.find('.') == MB_String::npos)
                {
                    MB_String ext;
                    getExtfromMIME(part.content_type.c_str(), ext);
                    part.filename += ext;
                }
            }

            return;
        }

        // Content header field parse
        if (strcmpP(buf, 0, esp_mail_str_180, caseSensitive))
        {

            // Content-Type
            if (strcmpP(buf, 0, esp_mail_str_25, caseSensitive))
            {

                part.cur_content_hdr = esp_mail_message_part_info_t::content_header_field_type;
                resetStringPtr(part);

                tmp = subStr(buf, esp_mail_str_25, esp_mail_str_97, 0, 0, caseSensitive);
                if (tmp)
                {
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
            }

            // Content-Description
            if (getDecodedHeader(imap, buf, esp_mail_str_174, part.descr, caseSensitive))
            {
                part.cur_content_hdr = esp_mail_message_part_info_t::content_header_field_description;

                tmp = subStr(buf, esp_mail_str_174, NULL, 0, -1, caseSensitive);
                if (tmp)
                {
                    value = tmp;
                    delP(&tmp);

                    part.stringPtr = toAddr(part.content_description);
                    value.trim();
                    if (value.length() == 0)
                        return;
                }
            }

            // Content-ID
            if (strcmpP(buf, 0, esp_mail_str_171, caseSensitive))
            {
                tmp = subStr(buf, esp_mail_str_171, NULL, 0, -1, caseSensitive);
                if (tmp)
                {
                    part.CID = tmp;
                    delP(&tmp);
                    part.CID.trim();

                    if (part.CID[0] == '<')
                        part.CID.erase(0, 1);

                    if (part.CID[part.CID.length() - 1] == '>')
                        part.CID.erase(part.CID.length() - 1, 1);

                    // if inline attachment file name was not assigned
                    if (part.attach_type == esp_mail_att_type_inline && part.filename.length() == 0)
                    {
                        // set filename from content id and append extension later

                        part.filename = part.CID;
                        part.name = part.filename;
                    }
                }

                part.cur_content_hdr = esp_mail_message_part_info_t::content_header_field_id;
                resetStringPtr(part);
            }

            // Content-Disposition
            if (strcmpP(buf, 0, esp_mail_str_175, caseSensitive))
            {

                part.cur_content_hdr = esp_mail_message_part_info_t::content_header_field_disposition;
                resetStringPtr(part);

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

            // Content-Transfer-Encoding
            if (strcmpP(buf, 0, esp_mail_str_172, caseSensitive))
            {
                // store last text field

                part.cur_content_hdr = esp_mail_message_part_info_t::content_header_field_transfer_enc;
                resetStringPtr(part);

                tmp = subStr(buf, esp_mail_str_172, NULL, 0, -1, caseSensitive);
                if (tmp)
                {

                    part.content_transfer_encoding = tmp;

                    if (strcmpP(tmp, 0, esp_mail_str_31))
                        part.xencoding = esp_mail_msg_xencoding_base64;
                    else if (strcmpP(tmp, 0, esp_mail_str_278))
                        part.xencoding = esp_mail_msg_xencoding_qp;
                    else if (strcmpP(tmp, 0, esp_mail_str_29))
                        part.xencoding = esp_mail_msg_xencoding_7bit;
                    else if (strcmpP(tmp, 0, esp_mail_str_358))
                        part.xencoding = esp_mail_msg_xencoding_8bit;
                    else if (strcmpP(tmp, 0, esp_mail_str_365))
                        part.xencoding = esp_mail_msg_xencoding_binary;

                    delP(&tmp);
                }
            }
        }
        else
        {

            if (part.cur_content_hdr == esp_mail_message_part_info_t::content_header_field_none)
            {

                resetStringPtr(part);

                if (getDecodedHeader(imap, buf, esp_mail_str_150, part.rfc822_header.sender, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_10, part.rfc822_header.from, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_11, part.rfc822_header.to, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_12, part.rfc822_header.cc, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_184, part.rfc822_header.reply_to, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_134, part.rfc822_header.comments, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_24, part.rfc822_header.subject, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_101, part.rfc822_header.messageID, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_46, part.rfc822_header.return_path, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_99, part.rfc822_header.date, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_145, part.rfc822_header.keywords, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_109, part.rfc822_header.in_reply_to, caseSensitive))
                    return;

                if (getDecodedHeader(imap, buf, esp_mail_str_107, part.rfc822_header.references, caseSensitive))
                    return;
            }
        }

        // parse content type header sub type properties

        if (part.cur_content_hdr == esp_mail_message_part_info_t::content_header_field_type)
        {

            if (part.msg_type == esp_mail_msg_type_plain || part.msg_type == esp_mail_msg_type_enriched)
            {

                if (getPartHeaderProperties(imap, buf, esp_mail_str_168, esp_mail_str_136, false, value, old_value, part.stringEnc, caseSensitive))
                {
                    part.charset = value;
                    resetStringPtr(part);
                }

                if (strposP(buf, esp_mail_str_275, 0, caseSensitive) > -1 || strposP(buf, esp_mail_str_270, 0, caseSensitive) > -1)
                {
                    part.plain_flowed = true;
                    resetStringPtr(part);
                }

                if (strposP(buf, esp_mail_str_259, 0, caseSensitive) > -1 || strposP(buf, esp_mail_str_257, 0, caseSensitive) > -1)
                {
                    part.plain_delsp = true;
                    resetStringPtr(part);
                }
            }

            if (part.charset.length() == 0)
            {
                if (getPartHeaderProperties(imap, buf, esp_mail_str_168, esp_mail_str_136, false, value, old_value, part.stringEnc, caseSensitive))
                {
                    part.charset = value;
                    resetStringPtr(part);
                }
            }

            if (getPartHeaderProperties(imap, buf, esp_mail_str_170, esp_mail_str_136, false, value, old_value, part.stringEnc, caseSensitive))
            {
                part.stringPtr = toAddr(part.name);
                value.trim();
                if (value.length() == 0)
                    return;
            }
        }

        // parse content disposition header sub type properties

        if (part.cur_content_hdr == esp_mail_message_part_info_t::content_header_field_disposition && part.content_disposition.length() > 0)
        {

            // filename prop
            if (getPartHeaderProperties(imap, buf, esp_mail_str_176, esp_mail_str_136, false, value, old_value, part.stringEnc, caseSensitive))
            {
                part.stringPtr = toAddr(part.filename);
                value.trim();
                if (value.length() == 0)
                    return;
            }

            // size prop
            if (getPartHeaderProperties(imap, buf, esp_mail_str_178, esp_mail_str_97, true, value, old_value, part.stringEnc, caseSensitive))
            {
                part.attach_data_size = atoi(value.c_str());
                cHeader(imap)->total_attach_data_size += part.attach_data_size;
                part.sizeProp = true;

                if (!valueStored && old_value.length() > 0)
                    valueStored = storeStringPtr(imap, part.stringPtr, old_value, buf);
                resetStringPtr(part);
            }

            // creation date prop
            if (getPartHeaderProperties(imap, buf, esp_mail_str_179, esp_mail_str_136, false, value, old_value, part.stringEnc, caseSensitive))
            {
                part.creation_date = value;
                if (!valueStored && old_value.length() > 0)
                    valueStored = storeStringPtr(imap, part.stringPtr, old_value, buf);
                resetStringPtr(part);
            }

            // mod date prop
            if (getPartHeaderProperties(imap, buf, esp_mail_str_181, esp_mail_str_136, false, value, old_value, part.stringEnc, caseSensitive))
            {
                part.modification_date = value;
                if (!valueStored && old_value.length() > 0)
                    valueStored = storeStringPtr(imap, part.stringPtr, old_value, buf);
                resetStringPtr(part);
            }
        }

        if (!valueStored && (part.cur_content_hdr == esp_mail_message_part_info_t::content_header_field_description || part.cur_content_hdr == esp_mail_message_part_info_t::content_header_field_type || part.cur_content_hdr == esp_mail_message_part_info_t::content_header_field_disposition))
            storeStringPtr(imap, part.stringPtr, value, buf);
    }
}

void ESP_Mail_Client::resetStringPtr(struct esp_mail_message_part_info_t &part)
{
    part.stringPtr = 0;
    part.stringEnc = esp_mail_char_decoding_scheme_default;
}

int ESP_Mail_Client::countChar(const char *buf, char find)
{
    if (!buf)
        return 0;

    int count = 0;

    for (size_t i = 0; i < strlen(buf); i++)
    {
        if (buf[i] == find)
            count++;
    }

    return count;
}

bool ESP_Mail_Client::storeStringPtr(IMAPSession *imap, uint32_t addr, MB_String &value, const char *buf)
{
    if (addr)
    {

        MB_String *a = addrTo<MB_String *>(addr);

        MB_String s = value.length() > 0 ? value : buf;

        // is value string contains double quotes?
        // trim it
        if (countChar(s.c_str(), '"') == 2)
            s.trim();

        if (s[0] == '"')
            s.erase(0, 1);

        if (s[s.length() - 1] == '"')
            s.erase(s.length() - 1, 1);

        decodeHeader(imap, s);

        *a += s;

        return true;
    }

    return false;
}

bool ESP_Mail_Client::getPartHeaderProperties(IMAPSession *imap, const char *buf, PGM_P p, PGM_P e, bool num, MB_String &value, MB_String &old_value, esp_mail_char_decoding_scheme &scheme, bool caseSensitive)
{

    MB_String str = p;
    str += esp_mail_str_177;
    if (!num)
        str += esp_mail_str_136;

    char *tmp = subStr(buf, str.c_str(), e, 0, 0, caseSensitive);
    if (!tmp)
    {
        str = p;
        str += esp_mail_str_177;
        tmp = subStr(buf, str.c_str(), e, 0, 0, caseSensitive);

        if (tmp)
        {
            // other sub headers found?
            int p2 = strposP(tmp, esp_mail_str_97, 0, caseSensitive);
            if (p2 > -1)
            {
                delP(&tmp);
                tmp = subStr(buf, str.c_str(), esp_mail_str_97, 0, 0, caseSensitive);
            }
        }
        else
        {
            // Extended notation rfc5987
            str = p;
            str += esp_mail_str_183;
            int p2 = strpos(buf, str.c_str(), 0, caseSensitive);
            if (p2 > -1)
            {
                int p3 = strposP(buf, esp_mail_str_183, p2 + str.length() + 1, caseSensitive);
                if (p3 > -1 && p3 < (int)strlen(buf))
                {
                    MB_String charset;

                    p3 += 2;

                    int p4 = strpos(buf, "'", p3, caseSensitive);
                    if (p4 > -1)
                    {

                        scheme = getEncodingFromCharset(buf);
                        int c1 = p4 + 1;
                        p4 = strpos(buf, "'", p4 + 1, caseSensitive);
                        int c2 = p4;
                        if (c2 > -1)
                        {
                            charset.append(buf + c1, c2 - c1);
                        }
                        p3 = p4 + 1;
                    }

                    int len = strlen(buf) - p3;
                    tmp = (char *)newP(len + 1);

                    if (buf[strlen(buf) - 1] == ';')
                        len--;

                    memcpy(tmp, &buf[p3], len);

                    if (scheme == esp_mail_char_decoding_scheme_utf_8)
                    {
                        char *buf2 = urlDecode(tmp);
                        delP(&tmp);
                        tmp = buf2;
                    }
                    else if (imap->_charDecCallback)
                    {

                        IMAP_Decoding_Info decoding;

                        decoding.charset = charset.c_str();
                        decoding.data = tmp;
                        decoding.type = IMAP_Decoding_Info::message_part_type_header;

                        imap->_charDecCallback(&decoding);

                        if (decoding.decodedString.length() > 0)
                        {
                            char *buf2 = (char *)newP(decoding.decodedString.length() + 1);
                            strcpy(buf2, decoding.decodedString.c_str());
                            delP(&tmp);
                            tmp = buf2;
                        }
                    }
                    else if (scheme == esp_mail_char_decoding_scheme_iso8859_1)
                    {
                        int ilen = strlen(tmp);
                        int olen = (ilen + 1) * 2;
                        char *buf2 = (char *)newP(olen);
                        decodeLatin1_UTF8((unsigned char *)buf2, &olen, (unsigned char *)tmp, &ilen);
                        delP(&tmp);
                        tmp = buf2;
                    }
                    else if (scheme == esp_mail_char_decoding_scheme_tis620)
                    {
                        int ilen = strlen(tmp);
                        char *buf2 = (char *)newP((ilen + 1) * 3);
                        decodeTIS620_UTF8(buf2, tmp, ilen);
                        delP(&tmp);
                        tmp = buf2;
                    }
                }
            }
        }
    }

    if (tmp)
    {
        old_value = value;
        value = tmp;
        delP(&tmp);
        return true;
    }

    return false;
}

char *ESP_Mail_Client::urlDecode(const char *str)
{
    int d = 0; /* whether or not the string is decoded */

    char *dStr = (char *)newP(strlen(str) + 1);
    char eStr[] = "00"; /* for a hex code */

    strcpy(dStr, str);

    while (!d)
    {
        d = 1;
        size_t i; /* the counter for the string */

        for (i = 0; i < strlen(dStr); ++i)
        {

            if (dStr[i] == '%')
            {
                if (dStr[i + 1] == 0)
                    return dStr;

                if (isxdigit(dStr[i + 1]) && isxdigit(dStr[i + 2]))
                {

                    d = 0;

                    /* combine the next to numbers into one */
                    eStr[0] = dStr[i + 1];
                    eStr[1] = dStr[i + 2];

                    /* convert it to decimal */
                    long int x = strtol(eStr, NULL, 16);

                    /* remove the hex */
                    memmove(&dStr[i + 1], &dStr[i + 3], strlen(&dStr[i + 3]) + 1);

                    dStr[i] = x;
                }
            }
        }
    }

    return dStr;
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

    networkStatus = imap->client.networkReady();

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
            else
            {
                if (imap->_debug)
                    esp_mail_debug_print(esp_mail_str_371, true);
            }

            return false;
        }
    }

    if (!networkStatus)
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
                // dummy
                imap->client.disconnect();
                imap->_sesson_cfg->network_connection_handler();
            }
            else
            {
                if (MailClient.networkAutoReconnect)
                    MailClient.resumeNetwork(&(imap->client));
            }

            _lastReconnectMillis = millis();
        }

        networkStatus = imap->client.networkReady();
    }

    return networkStatus;
}

// Check if response from basic client is actually TLS alert
// This response may return after basic Client sent plain text packet over SSL/TLS
void ESP_Mail_Client::checkTLSAlert(IMAPSession *imap, const char *response)
{

    /**
     * SSL Record Byte 0
     *
     * SSL3_RT_CHANGE_CIPHER_SPEC      0x14
     * SSL3_RT_ALERT                   0x15
     * SSL3_RT_HANDSHAKE               0x16
     * SSL3_RT_APPLICATION_DATA        0x17
     * TLS1_RT_HEARTBEAT               0x18
     *
     * SSL Record Alert Value
     * SSL3_AD_CLOSE_NOTIFY            0x00
     * TLS1_AD_PROTOCOL_VERSION        0x46
     *
     */
    uint8_t data0 = response[0];
    if (!imap->client.tlsErr() && data0 == 0x15)
    {
        imap->client.set_tlsErrr(true);

        int proto = imap->client.getProtocol(imap->_sesson_cfg->server.port);

        if (proto == (int)esp_mail_protocol_ssl || proto == (int)esp_mail_protocol_tls)
        {
            if (imap->_debug)
                esp_mail_debug_print(esp_mail_str_204, true);
        }
    }
}

bool ESP_Mail_Client::handleIMAPResponse(IMAPSession *imap, int errCode, bool closeSession)
{

    if (!reconnect(imap))
        return false;

    esp_mail_imap_response_status imapResp = esp_mail_imap_resp_unknown;
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
    imap->_lastProgress = -1;

    // Flag used for CRLF inclusion in response reading in case 8bit/binary attachment and base64 encoded message
    bool crLF = imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_text && (cPart(imap)->xencoding == esp_mail_msg_xencoding_base64 || cPart(imap)->xencoding == esp_mail_msg_xencoding_binary);
    crLF |= imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment && cPart(imap)->xencoding != esp_mail_msg_xencoding_base64;

    // custom cmd IDLE?, waiting incoming server response
    if (chunkBufSize == 0 && imap->_prev_imap_custom_cmd == imap->_imap_custom_cmd && imap->_imap_custom_cmd == esp_mail_imap_cmd_idle)
    {
        if (!reconnect(imap))
            return false;

        return true;
    }

    while (imap->_tcpConnected && chunkBufSize <= 0)
    {

        if (!reconnect(imap, dataTime))
            return false;

        if (!connected(imap))
        {
#if defined(ESP32)
            if (imap->_imap_cmd == esp_mail_imap_cmd_logout) // suppress the error due to server closes the connection immediately in ESP32 core v2.0.4
                return true;
#endif
            errorStatusCB(imap, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);

            return false;
        }
        chunkBufSize = imap->client.available();
        idle();
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
            imap->_unseenMsgIndex.clear();
        }

        if (imap->_imap_cmd == esp_mail_imap_cmd_search)
        {
            imap->_mbif._searchCount = 0;
            imap->_imap_msg_num.clear();
        }

        // response buffer
        chunkBufSize = ESP_MAIL_CLIENT_RESPONSE_BUFFER_SIZE;
        response = (char *)newP(chunkBufSize + 1);

        if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_inline)
            lastBuf = (char *)newP(BASE64_CHUNKED_LEN + 1);

        while (!completedResponse) // looking for operation finishing
        {
            idle();

            if (imap->_imap_cmd == esp_mail_imap_cmd_append || (imap->_imap_custom_cmd == esp_mail_imap_cmd_append && imap->_imap_cmd == esp_mail_imap_cmd_custom && imap->_customCmdResCallback))
            {
                // No waiting time out for APPEND
                dataTime = millis();
            }

            if (!reconnect(imap, dataTime) || !connected(imap))
            {

                if (!connected(imap))
                {

#if defined(ESP32)
                    if (imap->_imap_cmd == esp_mail_imap_cmd_logout) // suppress the error due to server closes the connection immediately in ESP32 core v2.0.4
                        return true;
#endif

                    errorStatusCB(imap, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
                    return false;
                }
                return false;
            }

            chunkBufSize = imap->client.available();

            if (chunkBufSize > 0)
            {
                chunkBufSize = ESP_MAIL_CLIENT_RESPONSE_BUFFER_SIZE;

                if (imap->_imap_cmd == esp_mail_imap_command::esp_mail_imap_cmd_search)
                {
                    readLen = parseSearchResponse(imap, response, chunkBufSize, chunkIdx, esp_mail_str_27, endSearch, scnt, pgm2Str(esp_mail_imap_response_6), pgm2Str(esp_mail_str_92));
                    imap->_mbif._availableItems = imap->_imap_msg_num.size();
                }
                else
                {
                    // response read as chunk ended with CRLF or complete buffer size
                    int o = octetCount;
                    readLen = readLine(&(imap->client), response, chunkBufSize, crLF, octetCount);
                    if (readLen == 0 && o != octetCount && octetCount <= octetLength && cPart(imap)->xencoding != esp_mail_msg_xencoding_base64 && cPart(imap)->xencoding != esp_mail_msg_xencoding_binary && cPart(imap)->xencoding != esp_mail_msg_xencoding_qp)
                    {
                        strcpy(response, "\r\n");
                        readLen = 2;
                    }
                }

                if (readLen)
                {
                    checkTLSAlert(imap, response);

                    if (imap->_debugLevel > esp_mail_debug_level_basic && !imap->_customCmdResCallback)
                    {
                        if (imap->_imap_cmd != esp_mail_imap_cmd_search && imap->_imap_cmd != esp_mail_imap_cmd_fetch_body_text && imap->_imap_cmd != esp_mail_imap_cmd_fetch_body_attachment && imap->_imap_cmd != esp_mail_imap_cmd_fetch_body_inline)
                            esp_mail_debug_print((const char *)response, true);
                    }

                    if (imap->_imap_cmd != esp_mail_imap_cmd_search || (imap->_imap_cmd == esp_mail_imap_cmd_search && endSearch))
                        imapResp = imapResponseStatus(imap, response, esp_mail_str_27);

                    if (imapResp != esp_mail_imap_resp_unknown)
                    {

                        // We've got the right response,
                        // prepare to exit

                        completedResponse = true;

                        if (imap->_debugLevel > esp_mail_debug_level_basic && !imap->_customCmdResCallback)
                        {
                            if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_text || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_inline)
                                esp_mail_debug_print((const char *)response, true);
                        }

                        // auto capabilities?
                        if (imap->_imap_cmd == esp_mail_imap_cmd_sasl_login || imap->_imap_cmd == esp_mail_imap_cmd_sasl_auth_oauth || imap->_imap_cmd == esp_mail_imap_cmd_sasl_auth_plain)
                        {
                            int i = 0;
                            if (parseCapabilityResponse(imap, response, i))
                                imap->_read_capability.auto_caps = true;
                        }

                        while (imap->client.available())
                        {
                            readLen = readLine(&(imap->client), response, chunkBufSize, true, octetCount);
                            if (readLen)
                            {
                                if (imap->_debugLevel > esp_mail_debug_level_basic && !imap->_customCmdResCallback)
                                {
                                    if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_text || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_inline)
                                        esp_mail_debug_print((const char *)response, true);
                                }
                                // auto capabilities?
                                if (imap->_imap_cmd == esp_mail_imap_cmd_sasl_login || imap->_imap_cmd == esp_mail_imap_cmd_sasl_auth_oauth || imap->_imap_cmd == esp_mail_imap_cmd_sasl_auth_plain)
                                {
                                    int i = 0;
                                    parseCapabilityResponse(imap, response, i);
                                }
                            }
                        }
                    }
                    else
                    {

                        // No response ever parsed

                        if (imap->_imap_cmd == esp_mail_imap_cmd_custom && imap->_customCmdResCallback)
                        {

                            imapResp = imapResponseStatus(imap, response, imap->_imapStatus.tag.c_str());

                            // get response or custom cmd APPEND or custom cmd IDLE?
                            if (imapResp > esp_mail_imap_resp_unknown || strposP(imap->_cmd.c_str(), esp_mail_str_360, 0, false) > -1 || imap->_imap_custom_cmd == esp_mail_imap_cmd_idle)
                                completedResponse = true;

                            imap->_imapStatus.text = response;

                            imap->_customCmdResCallback(imap->_imapStatus);

                            if (completedResponse)
                            {
                                delP(&response);
                                delP(&lastBuf);
                                return true;
                            }
                        }
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_append)
                        {
                            imapResp = esp_mail_imap_resp_ok;
                            completedResponse = true;
                        }
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_sasl_auth_plain || imap->_imap_cmd == esp_mail_imap_cmd_sasl_auth_oauth)
                        {
                            if (imap->_imap_cmd == esp_mail_imap_cmd_sasl_auth_oauth)
                            {
                                if (authFailed(response, readLen, chunkIdx, 2))
                                    completedResponse = true;
                            }

                            // multiline auth SASL (no SASL-IR extension supported)
                            if (strcmp(response, pgm2Str(esp_mail_str_385)) == 0)
                            {
                                imapResp = esp_mail_imap_resp_ok;
                                completedResponse = true;
                            }
                        }
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_capability)
                            parseCapabilityResponse(imap, response, chunkIdx);
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_list)
                            parseFoldersResponse(imap, response, true);
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_lsub)
                            parseFoldersResponse(imap, response, false);
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_select || imap->_imap_cmd == esp_mail_imap_cmd_examine)
                            parseExamineResponse(imap, response);
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_get_uid)
                            parseGetUIDResponse(imap, response);
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_get_flags)
                            parseGetFlagsResponse(imap, response);
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_get_quota)
                            parseGetQuotaResponse(imap, response);
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_get_quota_root)
                        {
                            parseGetQuotaRootsResponse(imap, response);
                            imap->_quota_tmp.clear();
                            parseGetQuotaResponse(imap, response);
                            if (imap->_quota_tmp.length() > 0)
                            {
                                imap->_quota_root_tmp += (char)':';
                                imap->_quota_root_tmp += imap->_quota_tmp;
                            }
                        }
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_get_acl || imap->_imap_cmd == esp_mail_imap_cmd_my_rights)
                            parseGetACLResponse(imap, response, imap->_imap_cmd);
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_namespace)
                            parseNamespaceResponse(imap, response);
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_idle)
                        {
                            completedResponse = response[0] == '+';
                            imapResp = esp_mail_imap_resp_ok;

                            imap->_last_host_check_ms = millis();
                        }
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_sequence_set)
                        {
                            parFetchSequenceSetResponse(imap, response);
                        }
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_header)
                        {

                            if (headerState == 0 && cMSG(imap).type == esp_mail_imap_msg_num_type_uid)
                                header.message_uid = cMSG(imap).value;

                            int _st = headerState;
                            parseHeaderResponse(imap, response, readLen, chunkIdx, header, headerState, octetCount, imap->_config->enable.header_case_sensitive);
                            if (_st == headerState && headerState > 0 && octetCount <= header.header_data_len)
                                setHeader(imap, response, header, headerState);
                        }
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_mime)
                            parsePartHeaderResponse(imap, response, chunkIdx, part, octetCount, imap->_config->enable.header_case_sensitive);
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_text)
                            decodeText(imap, response, readLen, chunkIdx, filePath, downloadRequest, octetLength, octetCount);
                        else if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment || imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_inline)
                        {

                            if (cPart(imap)->xencoding == esp_mail_msg_xencoding_base64)
                            {
                                // Multi-line chunked base64 string attachment handle
                                if (octetCount < octetLength && readLen < BASE64_CHUNKED_LEN)
                                {
                                    if (strlen(lastBuf) > 0)
                                    {
                                        tmp = (char *)newP(readLen + strlen(lastBuf) + 2);
                                        strcpy(tmp, lastBuf);
                                        strcat(tmp, response);
                                        readLen = strlen(tmp);
                                        tmo = parseAttachmentResponse(imap, tmp, readLen, chunkIdx, filePath, downloadRequest, octetCount, octetLength);
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
                                    tmo = parseAttachmentResponse(imap, response, readLen, chunkIdx, filePath, downloadRequest, octetCount, octetLength);
                                    if (!tmo)
                                        break;
                                }
                            }
                            else
                                tmo = parseAttachmentResponse(imap, response, readLen, chunkIdx, filePath, downloadRequest, octetCount, octetLength);
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
                searchReport(imap, 100, pgm2Str(esp_mail_str_92));
            }
        }

        if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_attachment)
            delP(&lastBuf);
    }

    if ((imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_header && header.header_data_len == 0) || imapResp == esp_mail_imap_resp_no)
    {
        // We don't get any response

        if (imapResp == esp_mail_imap_resp_no)
            imap->_imapStatus.statusCode = IMAP_STATUS_IMAP_RESPONSE_FAILED;
        else
            imap->_imapStatus.statusCode = IMAP_STATUS_NO_MESSAGE;

        if (imap->_readCallback && imap->_imap_cmd != esp_mail_imap_cmd_fetch_body_mime)
        {
            MB_String s = esp_mail_str_53;
            s += imap->errorReason();
            imapCB(imap, s.c_str(), false, false);
        }

        if (imap->_debug && imap->_imap_cmd != esp_mail_imap_cmd_fetch_body_mime)
        {
            MB_String s = esp_mail_str_185;
            s += imap->errorReason();
            esp_mail_debug_print(s.c_str(), true);
        }

        return false;
    }

    // We've got OK or NO responses

    if (imapResp == esp_mail_imap_resp_ok)
    {
        // Response OK

        if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_header)
        {
            // Headers management

            char *buf = (char *)newP(header.content_type.length() + 1);
            strcpy(buf, header.content_type.c_str());
            header.content_type.clear();

            tmp = subStr(buf, esp_mail_str_25, esp_mail_str_97, 0, 0, false);
            if (tmp)
            {
                headerState = esp_mail_imap_state_content_type;
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
                    headerState = esp_mail_imap_state_char_set;
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
                            headerState = esp_mail_imap_state_boundary;
                            setHeader(imap, tmp, header, headerState);
                            delP(&tmp);
                        }
                    }
                }
            }

            delP(&buf);

            // Decode the headers fields
            decodeHeader(imap, header.header_fields.messageID);
            decodeHeader(imap, header.header_fields.from);
            decodeHeader(imap, header.header_fields.sender);
            decodeHeader(imap, header.header_fields.to);
            decodeHeader(imap, header.header_fields.cc);
            decodeHeader(imap, header.header_fields.bcc);
            decodeHeader(imap, header.header_fields.subject);
            decodeHeader(imap, header.header_fields.date);
            decodeHeader(imap, header.header_fields.return_path);
            decodeHeader(imap, header.header_fields.reply_to);
            decodeHeader(imap, header.header_fields.in_reply_to);
            decodeHeader(imap, header.header_fields.references);
            decodeHeader(imap, header.header_fields.comments);
            decodeHeader(imap, header.header_fields.keywords);
            imap->_headers.push_back(header);
        }

        if (imap->_imap_cmd == esp_mail_imap_cmd_fetch_body_mime)
        {
            // Expect the octet length in the response for the existent part
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
                    if (part.attach_type == esp_mail_att_type_attachment || part.message_sub_type != esp_mail_imap_message_sub_type_rfc822)
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

        // Response NO

        // Some server responses NO and should exit (false) from MIME feching loop without
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
        s += esp_mail_str_136;
    }
    else
    {
        if (s.length() > 0)
            s += esp_mail_str_34;
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
            s += esp_mail_str_34;
        s += name;
        s += MBSTRING_FLASH_MCR(": ");
        s += value;
    }
}

void ESP_Mail_Client::saveHeader(IMAPSession *imap, bool json)
{

    MB_String headerFilePath;

    prepareFilePath(imap, headerFilePath, true);

    headerFilePath += json ? esp_mail_str_347 : esp_mail_str_203;

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
            esp_mail_debug_print(e.c_str(), true);
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

    addRFC822Headers(s, &header->header_fields, json);

    for (size_t j = 0; j < header->part_headers.size(); j++)
    {
        if (header->part_headers[j].rfc822_part)
        {
            MB_String s1;
            addRFC822Headers(s1, &header->part_headers[j].rfc822_header, json);

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

            if (json)
            {
                if (index > 0)
                    s += MBSTRING_FLASH_MCR(",");
                s += MBSTRING_FLASH_MCR("{\"Filename\":\"");
                s += header->part_headers[j].filename;
                s += esp_mail_str_136;
            }
            else
            {
                if (index > 0)
                    s += esp_mail_str_34;
                s += esp_mail_str_34;
                s += MBSTRING_FLASH_MCR("Index: ");
                s += index + 1;
                addHeader(s, "Filename", header->part_headers[j].filename, false, json);
            }

            addHeader(s, "Name", header->part_headers[j].name.c_str(), false, json);
            addHeader(s, "Size", header->part_headers[j].attach_data_size, false, json);
            addHeader(s, "MIME", header->part_headers[j].content_type.c_str(), false, json);
            addHeader(s, "Type", header->part_headers[j].attach_type == esp_mail_att_type_attachment ? "attachment" : "inline", false, json);
            addHeader(s, "Description", header->part_headers[j].content_description.c_str(), false, json);
            addHeader(s, "Creation Date", header->part_headers[j].creation_date.c_str(), false, json);

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

void ESP_Mail_Client::addRFC822Headers(MB_String &s, esp_mail_imap_rfc822_msg_header_item_t *header, bool json)
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

esp_mail_imap_response_status ESP_Mail_Client::imapResponseStatus(IMAPSession *imap, char *response, PGM_P tag)
{
    imap->_imapStatus.clear(false);

    MB_String s1 = imap->prependTag(tag, esp_mail_imap_response_1);
    MB_String s2 = imap->prependTag(tag, esp_mail_imap_response_2);
    MB_String s3 = imap->prependTag(tag, esp_mail_imap_response_3);

    if (strpos(response, s1.c_str(), 0) > -1)
    {
        s1 = esp_mail_imap_response_1;
        s1.trim();
        imap->_imapStatus.status = s1;
        imap->_imapStatus.completed = true;
        return esp_mail_imap_resp_ok;
    }
    else if (strpos(response, s2.c_str(), 0) > -1)
    {
        imap->_imapStatus.text = response;
        imap->_imapStatus.text = imap->_imapStatus.text.substr(s2.length());
        s2 = esp_mail_imap_response_2;
        s2.trim();
        imap->_imapStatus.status = s2;
        imap->_imapStatus.completed = true;
        return esp_mail_imap_resp_no;
    }
    else if (strpos(response, s3.c_str(), 0) > -1)
    {
        imap->_imapStatus.text = response;
        imap->_imapStatus.text = imap->_imapStatus.text.substr(s3.length());

        s3 = esp_mail_imap_response_3;
        s3.trim();
        imap->_imapStatus.status = s3;
        imap->_imapStatus.completed = true;
        return esp_mail_imap_resp_bad;
    }
    return esp_mail_imap_resp_unknown;
}

bool ESP_Mail_Client::parseCapabilityResponse(IMAPSession *imap, const char *buf, int &chunkIdx)
{
    if (chunkIdx == 0)
    {

        if (strposP(buf, esp_mail_imap_response_10, 0) > -1 || strposP(buf, esp_mail_imap_response_20, 0) > -1)
        {
            if (strposP(buf, esp_mail_imap_response_11, 0) > -1)
            {
                imap->_auth_capability.login = false;
                imap->_read_capability.logindisable = true;
            }
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

            // rfc4959
            if (strposP(buf, esp_mail_imap_response_28, 0) > -1)
                imap->_auth_capability.sasl_ir = true;

            if (strposP(buf, esp_mail_imap_response_17, 0) > -1)
                imap->_read_capability.idle = true;
            if (strposP(buf, esp_mail_imap_response_18, 0) > -1)
                imap->_read_capability.imap4 = true;
            if (strposP(buf, esp_mail_imap_response_19, 0) > -1)
                imap->_read_capability.imap4rev1 = true;
            if (strposP(buf, esp_mail_imap_response_21, 0) > -1)
                imap->_read_capability.acl = true;
            if (strposP(buf, esp_mail_imap_response_22, 0) > -1)
                imap->_read_capability.binary = true;
            if (strposP(buf, esp_mail_imap_response_23, 0) > -1)
                imap->_read_capability.multiappend = true;
            if (strposP(buf, esp_mail_imap_response_24, 0) > -1)
                imap->_read_capability.uidplus = true;
            if (strposP(buf, esp_mail_imap_response_25, 0) > -1)
                imap->_read_capability.literal_plus = true;
            if (strposP(buf, esp_mail_imap_response_26, 0) > -1)
                imap->_read_capability.literal_minus = true;
            if (strposP(buf, esp_mail_imap_response_29, 0) > -1)
                imap->_read_capability.move = true;
            if (strposP(buf, esp_mail_imap_response_32, 0) > -1)
                imap->_read_capability.quota = true;
            if (strposP(buf, esp_mail_imap_response_35, 0) > -1)
                imap->_read_capability.name_space = true;
            if (strposP(buf, esp_mail_imap_response_37, 0) > -1)
                imap->_read_capability.enable = true;

            return true;
        }
    }

    return false;
}

void ESP_Mail_Client::parseFoldersResponse(IMAPSession *imap, char *buf, bool list)
{
    struct esp_mail_folder_info_t fd;
    char *tmp = nullptr;
    int p1 = list ? strposP(buf, esp_mail_imap_response_4, 0) : strposP(buf, esp_mail_imap_response_27, 0);
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

bool ESP_Mail_Client::parseIdleResponse(IMAPSession *imap)
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
        chunkBufSize = ESP_MAIL_CLIENT_RESPONSE_BUFFER_SIZE;

        char *buf = (char *)newP(chunkBufSize + 1);

        int octetCount = 0;

        int readLen = MailClient.readLine(&(imap->client), buf, chunkBufSize, false, octetCount);

        if (readLen > 0)
        {

            if (imap->_debugLevel > esp_mail_debug_level_basic)
                esp_mail_debug_print((const char *)buf, true);

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

void ESP_Mail_Client::parseGetUIDResponse(IMAPSession *imap, char *buf)
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

void ESP_Mail_Client::parseGetFlagsResponse(IMAPSession *imap, char *buf)
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

void ESP_Mail_Client::parseGetQuotaResponse(IMAPSession *imap, char *buf)
{
    char *tmp = nullptr;

    int p1 = strposP(buf, esp_mail_imap_response_30, 0);
    if (p1 != -1)
    {
        int len = strlen(buf) - p1 - strlen_P(esp_mail_imap_response_30);
        tmp = (char *)newP(len);
        strncpy(tmp, buf + p1 + strlen_P(esp_mail_imap_response_30), strlen(buf) - p1 - strlen_P(esp_mail_imap_response_30) - 2);
        imap->_quota_tmp = tmp;
        delP(&tmp);
    }
}

void ESP_Mail_Client::parseGetQuotaRootsResponse(IMAPSession *imap, char *buf)
{
    char *tmp = nullptr;

    int p1 = strposP(buf, esp_mail_imap_response_31, 0);
    if (p1 != -1)
    {
        int p2 = strposP(buf, esp_mail_str_131, p1 + strlen_P(esp_mail_imap_response_31));
        if (p2 != -1)
        {
            int len = strlen(buf) - p2 - 1;
            tmp = (char *)newP(len);
            strncpy(tmp, buf + p2 + 1, strlen(buf) - p2 - 1);
            if (imap->_quota_root_tmp.length() > 0)
                imap->_quota_root_tmp += esp_mail_str_263;
            imap->_quota_root_tmp += tmp;
            delP(&tmp);
        }
    }
}

void ESP_Mail_Client::parseGetACLResponse(IMAPSession *imap, char *buf, esp_mail_imap_command cmd)
{
    char *tmp = nullptr;
    int p1 = cmd == esp_mail_imap_cmd_get_acl ? strposP(buf, esp_mail_imap_response_33, 0) : strposP(buf, esp_mail_imap_response_34, 0);

    if (p1 != -1)
    {
        int p2 = cmd == esp_mail_imap_cmd_get_acl ? strposP(buf, esp_mail_str_131, p1 + strlen_P(esp_mail_imap_response_33) + 1) : strposP(buf, esp_mail_str_131, p1 + strlen_P(esp_mail_imap_response_34) + 1);
        if (p2 != -1)
        {
            int len = strlen(buf) - p2 - 1;
            tmp = (char *)newP(len);
            strncpy(tmp, buf + p2 + 1, strlen(buf) - p2 - 1);
            imap->_acl_tmp = tmp;
            delP(&tmp);
        }
    }
}

void ESP_Mail_Client::parseNamespaceResponse(IMAPSession *imap, char *buf)
{
    char *tmp = nullptr;
    int p1 = strposP(buf, esp_mail_imap_response_36, 0);
    if (p1 != -1)
    {
        int len = strlen(buf) - p1 - strlen_P(esp_mail_imap_response_36);
        tmp = (char *)newP(len);
        strncpy(tmp, buf + p1 + strlen_P(esp_mail_imap_response_36), strlen(buf) - p1 - strlen_P(esp_mail_imap_response_36));
        imap->_ns_tmp += tmp;
        delP(&tmp);
    }
}

void ESP_Mail_Client::parFetchSequenceSetResponse(IMAPSession *imap, char *buf)
{
    char *tmp = nullptr;
    int p1 = strposP(buf, esp_mail_str_342, 0);
    if (p1 != -1)
    {
        int len = strlen(buf) - p1 - strlen_P(esp_mail_str_342);
        tmp = (char *)newP(len);
        strncpy(tmp, buf + p1 + strlen_P(esp_mail_str_342), strlen(buf) - p1 - strlen_P(esp_mail_str_342) - 1);

        esp_mail_imap_msg_num_t msg_num;
        msg_num.type = esp_mail_imap_msg_num_type_uid;
        msg_num.value = (uint32_t)atoi(tmp);

        imap->_imap_msg_num.push_back(msg_num);

        if (imap->_imap_msg_num.size() > imap->_config->limit.fetch)
            imap->_imap_msg_num.erase(imap->_imap_msg_num.begin());

        delP(&tmp);
        return;
    }
}

void ESP_Mail_Client::parseExamineResponse(IMAPSession *imap, char *buf)
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
                    splitToken(content, tokens, stk);
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

    if (imap->_unseenMsgIndex.length() == 0)
    {
        p1 = strposP(buf, esp_mail_str_354, 0);
        if (p1 != -1)
        {
            p2 = strposP(buf, esp_mail_str_219, p1 + strlen_P(esp_mail_str_354));
            if (p2 != -1)
            {
                tmp = (char *)newP(p2 - p1 - strlen_P(esp_mail_str_354) + 1);
                strncpy(tmp, buf + p1 + strlen_P(esp_mail_str_354), p2 - p1 - strlen_P(esp_mail_str_354));
                imap->_unseenMsgIndex = tmp;
                imap->_mbif._unseenMsgIndex = atoi(tmp);
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
        MB_String alias = cHeader(imap)->message_uid;
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

bool ESP_Mail_Client::parseAttachmentResponse(IMAPSession *imap, char *buf, int bufLen, int &chunkIdx, MB_String &filePath, bool &downloadRequest, int &octetCount, int &octetLength)
{
    if (chunkIdx == 0)
    {
        char *tmp = subStr(buf, esp_mail_str_193, esp_mail_str_194, 0);
        if (tmp)
        {
            chunkIdx++;
            octetCount = 0; // CRLF counted from first line
            octetLength = atoi(tmp);
            delP(&tmp);
            cPart(imap)->octetLen = octetLength;
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
                    filePath += cHeader(imap)->message_uid;

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
                            esp_mail_debug_print(e.c_str(), true);
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

    idle();

    if (octetLength == 0)
        return true;

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

        if (cPart(imap)->octetCount > octetLength)
            return true;

        if (cPart(imap)->xencoding == esp_mail_msg_xencoding_base64)
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

                sendStreamCB(imap, (void *)decoded, olen, chunkIdx, false);

                int write = olen;
                if (mbfs->ready(mbfs_type imap->_config->storage.type))
                    write = mbfs->write(mbfs_type imap->_config->storage.type, (uint8_t *)decoded, olen);
                idle();
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

            sendStreamCB(imap, (void *)buf, bufLen, chunkIdx, false);

            int write = bufLen;
            if (mbfs->ready(mbfs_type imap->_config->storage.type))
                write = mbfs->write(mbfs_type imap->_config->storage.type, (uint8_t *)buf, bufLen);

            idle();

            if (write != bufLen)
                return false;

            if (!reconnect(imap))
                return false;
        }
    }

    chunkIdx++;
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
        filePath += cHeader(imap)->message_uid;
        filePath += esp_mail_str_202;
        filePath += cPart(imap)->filename;

        MB_String s = esp_mail_str_90;
        s += esp_mail_str_131;
        s += esp_mail_str_136;
        s += filePath;
        s += esp_mail_str_136;
        s += esp_mail_str_91;
        s += progress;
        s += esp_mail_str_92;
        imapCB(imap, s.c_str(), false, false);
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
        MB_String s = download ? esp_mail_str_90 : esp_mail_str_83;

        s += esp_mail_str_131;
        if (cPart(imap)->filename.length() > 0)
        {
            s += esp_mail_str_136;
            s += cPart(imap)->filename;
            s += esp_mail_str_136;
            s += esp_mail_str_91;
        }
        s += progress;
        s += esp_mail_str_92;
        imapCB(imap, s.c_str(), false, false);
        s.clear();
        imap->_lastProgress = progress;
    }
}

void ESP_Mail_Client::searchReport(IMAPSession *imap, int progress, const char *percent)
{

    if (progress > 100)
        progress = 100;

    if (imap->_lastProgress != progress && (progress == 0 || progress == 100 || imap->_lastProgress + ESP_MAIL_PROGRESS_REPORT_STEP <= progress))
    {
        MB_String s = esp_mail_str_261;
        s += progress;
        s += percent;
        s += esp_mail_str_34;
        esp_mail_debug_print(s.c_str(), false);
        imap->_lastProgress = progress;
    }
}

struct esp_mail_imap_msg_num_t ESP_Mail_Client::cMSG(IMAPSession *imap)
{
    return imap->_imap_msg_num[cIdx(imap)];
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
            chunkIdx++;
            octetCount = 0;
            octetLength = atoi(tmp);
            delP(&tmp);
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
                        esp_mail_debug_print(e.c_str(), true);
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
                esp_mail_debug_print(s1.c_str(), false);
            }
        }
    }

    idle();

    if (octetLength == 0)
        return;

    bool enableDownloads = (imap->_config->download.rfc822 && rfc822_body_subtype) || (!rfc822_body_subtype && ((cPart(imap)->msg_type == esp_mail_msg_type_html && imap->_config->download.html) || ((cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched) && imap->_config->download.text)));

    if (imap->_config->download.rfc822 || imap->_config->download.html || imap->_config->download.text || (rfc822_body_subtype && imap->_config->enable.rfc822) || (!rfc822_body_subtype && ((cPart(imap)->msg_type == esp_mail_msg_type_html && imap->_config->enable.html) || ((cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched) && imap->_config->enable.text))))
    {

        if (cPart(imap)->octetCount + bufLen >= octetLength)
        {
            bufLen = octetLength - cPart(imap)->octetCount;
            cPart(imap)->octetCount += bufLen;

            if (octetLength <= octetCount && cPart(imap)->xencoding != esp_mail_msg_xencoding_base64 && cPart(imap)->xencoding != esp_mail_msg_xencoding_qp)
                bufLen = 0;

            buf[bufLen] = 0;
        }
        else
            cPart(imap)->octetCount = octetCount;

        if (imap->_readCallback)
            fetchReport(imap, 100 * cPart(imap)->octetCount / octetLength, enableDownloads);

        if (cPart(imap)->octetCount <= octetLength)
        {
            bool hrdBrk = cPart(imap)->xencoding == esp_mail_msg_xencoding_qp && cPart(imap)->octetCount < octetLength;

            // remove soft break for QP
            if (bufLen <= QP_ENC_MSG_LEN && buf[bufLen - 1] == '=' && cPart(imap)->xencoding == esp_mail_msg_xencoding_qp)
            {
                hrdBrk = false;
                buf[bufLen - 1] = 0;
                bufLen--;
            }

            size_t olen = 0;
            char *decoded = nullptr;
            MB_String str;
            bool dontDeleteOrModify = false;

            // decode the content based on the transfer decoding
            if (cPart(imap)->xencoding == esp_mail_msg_xencoding_base64)
            {
                decoded = (char *)decodeBase64((const unsigned char *)buf, bufLen, &olen);
            }
            else if (cPart(imap)->xencoding == esp_mail_msg_xencoding_qp)
            {
                decoded = (char *)newP(bufLen + 10);
                decodeQP_UTF8(buf, decoded);
                olen = strlen(decoded);
            }
            else if (cPart(imap)->xencoding == esp_mail_msg_xencoding_7bit)
            {
                decoded = decode7Bit_UTF8(buf);
                olen = strlen(decoded);
            }
            else if (cPart(imap)->xencoding == esp_mail_msg_xencoding_8bit)
            {
                decoded = decode8Bit_UTF8(buf);
                olen = strlen(decoded);
            }
            else
            {
                // binary
                dontDeleteOrModify = true;
                decoded = buf;
                olen = bufLen;
            }

            if (decoded)
            {
                // charset? apply character decoding to the decoded or nonencoded content

                if ((rfc822_body_subtype && imap->_config->enable.rfc822) || (!rfc822_body_subtype && ((cPart(imap)->msg_type == esp_mail_msg_type_html && imap->_config->enable.html) || ((cPart(imap)->msg_type == esp_mail_msg_type_plain || cPart(imap)->msg_type == esp_mail_msg_type_enriched) && imap->_config->enable.text))))
                {

                    if (imap->_charDecCallback)
                    {

                        IMAP_Decoding_Info decoding;

                        decoding.charset = cPart(imap)->charset.c_str();
                        decoding.data = decoded;
                        decoding.type = IMAP_Decoding_Info::message_part_type_text;

                        imap->_charDecCallback(&decoding);

                        if (decoding.decodedString.length() > 0)
                        {
                            char *buf2 = (char *)newP(decoding.decodedString.length() + 1);
                            strcpy(buf2, decoding.decodedString.c_str());

                            if (decoded && !dontDeleteOrModify)
                                delP(&decoded);

                            decoded = buf2;
                            olen = strlen(buf2);
                        }
                    }
                    else
                    {

                        if (getEncodingFromCharset(cPart(imap)->charset.c_str()) == esp_mail_char_decoding_scheme_iso8859_1)
                        {
                            int ilen = olen;
                            int olen2 = (ilen + 1) * 2;
                            unsigned char *tmp = (unsigned char *)newP(olen2);
                            decodeLatin1_UTF8(tmp, &olen2, (unsigned char *)decoded, &ilen);

                            if (decoded && !dontDeleteOrModify)
                                delP(&decoded);

                            olen = olen2;
                            decoded = (char *)tmp;
                        }
                        else if (getEncodingFromCharset(cPart(imap)->charset.c_str()) == esp_mail_char_decoding_scheme_tis620)
                        {
                            char *out = (char *)newP((olen + 1) * 3);
                            decodeTIS620_UTF8(out, decoded, olen);
                            olen = strlen(out);
                            if (decoded && !dontDeleteOrModify)
                                delP(&decoded);
                            decoded = out;
                        }
                    }

                    if (cPart(imap)->text.length() < imap->_config->limit.msg_size)
                    {

                        if (cPart(imap)->text.length() + olen < imap->_config->limit.msg_size)
                        {
                            cPart(imap)->textLen += olen;
                            cPart(imap)->text.append(decoded, olen);
                            if (hrdBrk)
                            {
                                cPart(imap)->text += esp_mail_str_34;
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
                                cPart(imap)->text += esp_mail_str_34;
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

                sendStreamCB(imap, (void *)decoded, olen, chunkIdx, hrdBrk);

                if (decoded && !dontDeleteOrModify)
                    delP(&decoded);
            }
        }
    }

    chunkIdx++;
}

void ESP_Mail_Client::sendStreamCB(IMAPSession *imap, void *buf, size_t len, int chunkIndex, bool hrdBrk)
{
    if (imap->_mimeDataStreamCallback && len > 0)
    {

        MIME_Data_Stream_Info streaminfo;

        streaminfo.uid = cHeader(imap)->message_uid;
        streaminfo.disposition = cPart(imap)->content_disposition.c_str();
        streaminfo.type = cPart(imap)->content_type.c_str();
        streaminfo.charSet = cPart(imap)->charset.c_str();
        streaminfo.transfer_encoding = cPart(imap)->content_transfer_encoding.c_str();
        streaminfo.cid = cPart(imap)->CID.c_str();
        streaminfo.description = cPart(imap)->content_description.c_str();
        streaminfo.date = cPart(imap)->creation_date.c_str();
        streaminfo.filename = cPart(imap)->filename.c_str();
        streaminfo.size = (cPart(imap)->sizeProp) ? cPart(imap)->attach_data_size : cPart(imap)->octetLen;

        streaminfo.name = cPart(imap)->name.c_str();
        streaminfo.octet_size = cPart(imap)->octetLen;
        streaminfo.octet_count = cPart(imap)->octetCount;

        streaminfo.isFirstData = chunkIndex == 1;
        streaminfo.isLastData = !hrdBrk ? cPart(imap)->octetLen == cPart(imap)->octetCount : false;

        streaminfo.data_size = len;
        streaminfo.data = buf;
        streaminfo.flowed = cPart(imap)->plain_flowed;
        streaminfo.delsp = cPart(imap)->plain_delsp;

        imap->_mimeDataStreamCallback(streaminfo);

        if (hrdBrk)
        {
            streaminfo.isFirstData = false;
            streaminfo.isLastData = cPart(imap)->octetLen == cPart(imap)->octetCount;
            streaminfo.data_size = 2;
            streaminfo.data = (void *)MBSTRING_FLASH_MCR("\r\n");
            imap->_mimeDataStreamCallback(streaminfo);
        }
    }
}

void ESP_Mail_Client::prepareFilePath(IMAPSession *imap, MB_String &filePath, bool header)
{
    bool rfc822_body_subtype = cPart(imap)->message_sub_type == esp_mail_imap_message_sub_type_rfc822;
    MB_String fpath = imap->_config->storage.saved_path;
    fpath += esp_mail_str_202;
    fpath += cHeader(imap)->message_uid;

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
        esp_mail_debug_print(e.c_str(), true);
    }
#endif
}

IMAPSession::IMAPSession(Client *client, esp_mail_external_client_type type)
{
    setClient(client, type);
}

IMAPSession::IMAPSession()
{
}

IMAPSession::~IMAPSession()
{
    empty();
#if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040)
    _caCert.reset();
    _caCert = nullptr;
#endif
}

bool IMAPSession::closeSession()
{
    _prev_imap_cmd = esp_mail_imap_cmd_sasl_login;
    _prev_imap_custom_cmd = esp_mail_imap_cmd_custom;

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

bool IMAPSession::connected()
{
    return client.connected();
}

bool IMAPSession::connect(ESP_Mail_Session *session, IMAP_Config *config)
{
    bool ssl = false;

    if (session)
        session->clearPorts();

    this->_customCmdResCallback = NULL;

    if (!handleConnection(session, config, ssl))
        return false;

    return MailClient.imapAuth(this, ssl);
}

bool IMAPSession::mCustomConnect(ESP_Mail_Session *session, imapResponseCallback callback, MB_StringPtr tag)
{
    this->_customCmdResCallback = callback;
    this->_imapStatus.tag = tag;

    bool ssl = false;
    if (!handleConnection(session, NULL, ssl))
        return false;

    return true;
}

bool IMAPSession::handleConnection(ESP_Mail_Session *session, IMAP_Config *config, bool &ssl)
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

#if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040)

    _caCert = nullptr;

    if (strlen(_sesson_cfg->certificate.cert_data) > 0)
        _caCert = std::shared_ptr<const char>(_sesson_cfg->certificate.cert_data);

#endif

    ssl = false;

    if (!connect(ssl))
    {
        MailClient.closeTCPSession(this);
        return false;
    }

    return true;
}

bool IMAPSession::connect(bool &ssl)
{
    ssl = false;
    MB_String buf;
#if defined(ESP32) && defined(ESP32_TCP_CLIENT)
    client.setDebugCallback(NULL);
#elif defined(ESP8266) || defined(PICO_RP2040)

#endif

    client.reset_tlsErr();

    if (_config)
    {
        if (_config->fetch.sequence_set.string.length() > 0 || _config->fetch.uid.length() > 0 || _config->fetch.number.length() > 0)
            _headerOnly = false;
        else
            _headerOnly = true;
    }

    _totalRead = 0;
    _secure = true;
    bool secureMode = true;

#if defined(ESP32) && defined(ESP32_TCP_CLIENT)
    if (_debug && !_customCmdResCallback)
        client.setDebugCallback(esp_mail_debug_print);
#elif (defined(ESP8266) || defined(PICO_RP2040)) && defined(ESP8266_TCP_CLIENT)
    client.txBufDivider = 16; // minimum, tx buffer size for ssl data and request command data
    client.rxBufDivider = 1;
    if (_config)
    {
        if (!_headerOnly && !_config->enable.html && !_config->enable.text && !_config->download.attachment && !_config->download.inlineImg && !_config->download.html && !_config->download.text)
            client.rxBufDivider = 16; // minimum rx buffer size for only message header
    }
#endif

    if (_sesson_cfg->ports_functions.list)
    {
        if (_sesson_cfg->ports_functions.use_internal_list)
        {
            _sesson_cfg->ports_functions.use_internal_list = false;
            delete[] _sesson_cfg->ports_functions.list;
        }
    }

    if (!_sesson_cfg->ports_functions.list)
    {
        _sesson_cfg->ports_functions.use_internal_list = true;

        _sesson_cfg->ports_functions.list = new port_function[2];
        _sesson_cfg->ports_functions.size = 2;

        _sesson_cfg->ports_functions.list[0].port = 143;
        _sesson_cfg->ports_functions.list[0].protocol = esp_mail_protocol_plain_text;

        _sesson_cfg->ports_functions.list[1].port = 993;
        _sesson_cfg->ports_functions.list[1].protocol = esp_mail_protocol_ssl;
    }

    MailClient.getPortFunction(_sesson_cfg->server.port, _sesson_cfg->ports_functions, _secure, secureMode, ssl, _sesson_cfg->secure.startTLS);

    if (_readCallback && !_customCmdResCallback)
        MailClient.imapCB(this, esp_mail_str_50, false, false);

    MB_String s;

    if (_debug && !_customCmdResCallback)
    {
        s = esp_mail_str_314;
        s += ESP_MAIL_VERSION;
        s += client.fwVersion();
        esp_mail_debug_print(s.c_str(), true);

#if defined(BOARD_HAS_PSRAM) && defined(MB_STRING_USE_PSRAM)
        if (ESP.getPsramSize() == 0 && !_customCmdResCallback)
        {
            s = esp_mail_str_353;
            esp_mail_debug_print(s.c_str(), true);
        }
#endif
    }

#if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040) || defined(ARDUINO_ARCH_SAMD) || defined(__AVR_ATmega4809__) || defined(ARDUINO_NANO_RP2040_CONNECT)
    bool validTime = false;
#endif

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)
    validTime = strlen(_sesson_cfg->certificate.cert_file) > 0 || _caCert != nullptr;
#endif

#if defined(ESP32) || defined(ESP8266) || defined(PICO_RP2040) || defined(ARDUINO_ARCH_SAMD) || defined(__AVR_ATmega4809__) || defined(ARDUINO_NANO_RP2040_CONNECT)

    if (!_customCmdResCallback && (_sesson_cfg->time.ntp_server.length() > 0 || validTime))
    {
        s = esp_mail_str_355;

        if (!_customCmdResCallback)
            esp_mail_debug_print(s.c_str(), true);

        MailClient.setTime(_sesson_cfg->time.gmt_offset, _sesson_cfg->time.day_light_offset, _sesson_cfg->time.ntp_server.c_str(), _sesson_cfg->time.timezone_env_string.c_str(), _sesson_cfg->time.timezone_file.c_str(), true);

        if (!MailClient.Time.clockReady())
            MailClient.errorStatusCB(this, MAIL_CLIENT_ERROR_NTP_TIME_SYNC_TIMED_OUT);
    }

#endif

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)
    MailClient.setCACert(client, _sesson_cfg, _caCert);
#endif

    if (_debug && !_customCmdResCallback)
    {
        esp_mail_debug_print(esp_mail_str_225, true);
        s = esp_mail_str_261;
        s += esp_mail_str_211;
        s += _sesson_cfg->server.host_name;
        esp_mail_debug_print(s.c_str(), true);
        s = esp_mail_str_261;
        s += esp_mail_str_201;
        s += _sesson_cfg->server.port;
        esp_mail_debug_print(s.c_str(), true);
    }

    client.begin(_sesson_cfg->server.host_name.c_str(), _sesson_cfg->server.port);

    client.ethDNSWorkAround();

    if (!client.connect(secureMode, _sesson_cfg->certificate.verify))
        return MailClient.handleIMAPError(this, IMAP_STATUS_SERVER_CONNECT_FAILED, false);

    // server connected
    _tcpConnected = true;

    client.setTimeout(TCP_CLIENT_DEFAULT_TCP_TIMEOUT_SEC);

    // wait for greeting
    unsigned long dataMs = millis();
    while (client.connected() && client.available() == 0 && millis() - dataMs < 2000)
    {
        MailClient.idle();
    }

    int chunkBufSize = client.available();

    if (chunkBufSize > 0)
    {
        char *buf = (char *)MailClient.newP(chunkBufSize + 1);
        client.readBytes(buf, chunkBufSize);
        if (_debugLevel > esp_mail_debug_level_basic && !_customCmdResCallback)
            esp_mail_debug_print((const char *)buf, true);

        if (_customCmdResCallback)
        {
            MailClient.imapResponseStatus(this, buf, esp_mail_str_183);
            _imapStatus.text = buf;

            if (_imapStatus.text[_imapStatus.text.length() - 2] == '\r' && _imapStatus.text[_imapStatus.text.length() - 1] == '\n')
                _imapStatus.text[_imapStatus.text.length() - 2] = 0;

            if (_imapStatus.tag.length() == 0)
                this->_imapStatus.tag = esp_mail_str_27;

            _customCmdResCallback(_imapStatus);
        }

        MailClient.delP(&buf);
    }

    if (!_customCmdResCallback)
    {
        if (_readCallback)
            MailClient.imapCB(this, esp_mail_str_54, false, false);

        if (_debug)
            esp_mail_debug_print(esp_mail_str_228, true);
    }

    return true;
}

void IMAPSession::debug(int level)
{
    if (level > esp_mail_debug_level_none)
    {
        _debugLevel = level;
        _debug = true;
        client.setDebugLevel(level);
    }
    else
    {
        _debugLevel = esp_mail_debug_level_none;
        _debug = false;
        client.setDebugLevel(0);
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

    case MAIL_CLIENT_ERROR_NTP_TIME_SYNC_TIMED_OUT:
        ret += esp_mail_str_356;
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

void IMAPSession::setClient(Client *client, esp_mail_external_client_type type)
{
#if (defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) || defined(ESP_MAIL_USE_SDK_SSL_ENGINE)) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
    this->client.setClient(client);
    this->client.setExtClientType(type);
#endif
}

void IMAPSession::connectionRequestCallback(ConnectionRequestCallback connectCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
    ESP_MAIL_PRINTF("> I: The Connection Request Callback is now optional.\n\n");
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

void IMAPSession::networkDisconnectionRequestCallback(NetworkDisconnectionRequestCallback networkDisconnectionCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
    this->client.networkDisconnectionRequestCallback(networkDisconnectionCB);
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
    MailClient.networkStatus = status;
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

        bool ssl = false;

        if (!connect(ssl))
        {
            MailClient.closeTCPSession(this);
            return false;
        }

        // re-authenticate after session closed
        if (!MailClient.imapAuth(this, ssl))
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
                MailClient.imapCB(this, s.c_str(), true, false);
            }

            if (_debug)
                esp_mail_debug_print(esp_mail_str_335, true);
        }

        if (MailClient.imapSend(this, prependTag(esp_mail_str_27, esp_mail_str_331).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return false;

        _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_idle;
        if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
            return false;

        if (!recon)
        {
            if (_readCallback)
                MailClient.imapCB(this, esp_mail_str_338, false, false);

            if (_debug)
            {
                if (MailClient.Time.clockReady() && (_readCallback || _debug))
                {
                    s += esp_mail_str_343;
                    s += MailClient.Time.getDateTimeString();
                }
                esp_mail_debug_print(s.c_str(), true);
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

        return MailClient.parseIdleResponse(this);
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

    if (MailClient.imapSend(this, esp_mail_str_332, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_done;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    if (!recon)
    {
        if (_readCallback)
            MailClient.imapCB(this, esp_mail_str_340, false, false);

        if (_debug)
            esp_mail_debug_print(esp_mail_str_341, true);
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
        _config->fetch.uid = esp_mail_str_183;
}

void IMAPSession::checkPath()
{
    MB_String path = _config->storage.saved_path;
    if (path[0] != '/')
    {
        path = '/';
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

#if defined(MB_MCU_ESP)
        if (MailClient.getFreeHeap() < ESP_MAIL_MIN_MEM)
            continue;
#endif
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

void IMAPSession::characterDecodingCallback(imapCharacterDecodingCallback callback)
{
    _charDecCallback = callback;
}

void IMAPSession::mimeDataStreamCallback(MIMEDataStreamCallback mimeDataStreamCallback)
{
    _mimeDataStreamCallback = mimeDataStreamCallback;
}

void IMAPSession::setSystemTime(time_t ts)
{
    MailClient.Time.setTimestamp(ts);
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
                if (_headers[messageIndex].part_headers[i].attach_type == esp_mail_att_type_attachment || (!_headers[messageIndex].part_headers[i].rfc822_part && _headers[messageIndex].part_headers[i].message_sub_type != esp_mail_imap_message_sub_type_rfc822))
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
                        att.description = _headers[messageIndex].part_headers[i].content_description.c_str();
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
                if (_headers[messageIndex].part_headers[i].message_sub_type == esp_mail_imap_message_sub_type_rfc822 && _headers[messageIndex].part_headers[i].attach_type != esp_mail_att_type_attachment)
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
                                att.description = _headers[messageIndex].part_headers[i].content_description.c_str();
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
        MailClient.imapCB(this, s.c_str(), true, false);
    }

    if (_debug)
        esp_mail_debug_print(esp_mail_str_197, true);

    if (MailClient.imapSend(this, prependTag(esp_mail_str_27, esp_mail_str_195).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
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
        s = esp_mail_str_61;
        s += _currentFolder;
        s += esp_mail_str_96;
        MailClient.imapCB(this, s.c_str(), true, false);
    }

    if (_debug)
        esp_mail_debug_print(esp_mail_str_248, true);

    if (mode == esp_mail_imap_mode_examine)
    {
        s = prependTag(esp_mail_str_27, esp_mail_str_135);
        _imap_cmd = esp_mail_imap_cmd_examine;
    }
    else if (mode == esp_mail_imap_mode_select)
    {
        s = prependTag(esp_mail_str_27, esp_mail_str_247);
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
        MailClient.imapCB(this, esp_mail_str_58, true, false);

    if (_debug)
        esp_mail_debug_print(esp_mail_str_230, true);

    if (MailClient.imapSend(this, prependTag(esp_mail_str_27, esp_mail_str_133).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_list;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_LIST_MAILBOXS_FAILED, false))
        return false;

    folders = _folders;
    return true;
}

bool IMAPSession::mGetSubscribesMailboxes(MB_StringPtr reference, MB_StringPtr mailbox, FoldersCollection &folders)
{
    folders.clear();

    FoldersCollection tmp;

    if (_readCallback)
        MailClient.imapCB(this, esp_mail_str_376, true, false);

    if (_debug)
        esp_mail_debug_print(esp_mail_str_375, true);

    MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_377);
    cmd += esp_mail_str_136;
    cmd += reference;
    cmd += esp_mail_str_136;

    cmd += esp_mail_str_131;

    cmd += esp_mail_str_136;
    cmd += mailbox;
    cmd += esp_mail_str_136;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    for (size_t i = 0; i < this->_folders.size(); i++)
        tmp.add(this->_folders[i]);

    this->_folders.clear();

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_lsub;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_LIST_MAILBOXS_FAILED, false))
        return false;

    for (size_t i = 0; i < this->_folders.size(); i++)
        folders.add(this->_folders[i]);

    this->_folders.clear();

    for (size_t i = 0; i < tmp.size(); i++)
        this->_folders.add(tmp[i]);

    tmp.clear();

    return true;
}

bool IMAPSession::mSubscribe(MB_StringPtr folder)
{

    if (_readCallback)
        MailClient.imapCB(this, esp_mail_str_379, true, false);

    if (_debug)
        esp_mail_debug_print(esp_mail_str_378, true);

    MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_380);
    cmd += esp_mail_str_136;
    cmd += folder;
    cmd += esp_mail_str_136;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_subscribe;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_LIST_MAILBOXS_FAILED, false))
        return false;

    return true;
}

bool IMAPSession::mUnSubscribe(MB_StringPtr folder)
{

    if (_readCallback)
        MailClient.imapCB(this, esp_mail_str_382, true, false);

    if (_debug)
        esp_mail_debug_print(esp_mail_str_381, true);

    MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_383);
    cmd += esp_mail_str_136;
    cmd += folder;
    cmd += esp_mail_str_136;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_unsubscribe;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_LIST_MAILBOXS_FAILED, false))
        return false;

    return true;
}

bool IMAPSession::mFetchSequenceSet()
{

    MB_String cmd;
    if (_config->fetch.sequence_set.UID)
    {
        cmd = prependTag(esp_mail_str_27, esp_mail_str_137);
        cmd += esp_mail_str_143;
    }
    else
        cmd = prependTag(esp_mail_str_27, esp_mail_str_143);

    cmd += _config->fetch.sequence_set.string;
    cmd += esp_mail_str_131;

    cmd += esp_mail_str_198;
    cmd += esp_mail_str_140;
    cmd += esp_mail_str_79;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_msg_num.clear();

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_fetch_sequence_set;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_LIST_MAILBOXS_FAILED, false))
        return false;

    return true;
}

MB_String IMAPSession::prependTag(PGM_P tag, PGM_P cmd)
{
    MB_String s = tag;
    s += esp_mail_str_131;
    s += cmd;
    return s;
}

bool IMAPSession::checkCapabilities()
{
    if (_readCallback)
        MailClient.imapCB(this, esp_mail_str_64, true, false);

    if (_debug)
        esp_mail_debug_print(esp_mail_str_65, true);

    if (MailClient.imapSend(this, prependTag(esp_mail_str_27, esp_mail_str_2).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
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
        esp_mail_debug_print();
        esp_mail_debug_print(esp_mail_str_320, true);
    }

    MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_322);
    cmd += folderName;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_create;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    return true;
}

bool IMAPSession::mRenameFolder(MB_StringPtr currentFolderName, MB_StringPtr newFolderName)
{
    if (_debug)
    {
        esp_mail_debug_print();
        esp_mail_debug_print(esp_mail_str_374, true);
    }

    MB_String o = currentFolderName;
    MB_String n = newFolderName;

    o.trim();
    n.trim();

    if (o == n)
        return true;

    if (o.length() == 0 || n.length() == 0)
        return false;

    MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_373);
    cmd += esp_mail_str_136;
    cmd += o;
    cmd += esp_mail_str_136;

    cmd += esp_mail_str_131;

    cmd += esp_mail_str_136;
    cmd += n;
    cmd += esp_mail_str_136;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_rename;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    if (_currentFolder == o)
        selectFolder(n.c_str(), false);

    return true;
}

int IMAPSession::getUID(int msgNum)
{
    if (_currentFolder.length() == 0)
        return 0;

    MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_143);
    cmd += msgNum;
    cmd += esp_mail_str_138;

    MB_String s;

    if (_readCallback)
        MailClient.imapCB(this, esp_mail_str_156, false, false);

    if (_debug)
        esp_mail_debug_print(esp_mail_str_189, true);

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
            MailClient.imapCB(this, s.c_str(), false, false);
        }

        if (_debug)
        {
            s = esp_mail_str_111;
            s += _uid_tmp;
            esp_mail_debug_print(s.c_str(), true);
        }
    }

    return _uid_tmp;
}

const char *IMAPSession::getFlags(int msgNum)
{
    _flags_tmp.clear();
    if (_currentFolder.length() == 0)
        return _flags_tmp.c_str();

    MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_143);
    cmd += msgNum;
    cmd += esp_mail_str_273;

    MB_String s;

    if (_readCallback)
        MailClient.imapCB(this, esp_mail_str_279, false, false);

    if (_debug)
        esp_mail_debug_print(esp_mail_str_105, true);

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return _flags_tmp.c_str();

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_get_flags;
    MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false);

    return _flags_tmp.c_str();
}

bool IMAPSession::mSendCustomCommand(MB_StringPtr cmd, imapResponseCallback callback, MB_StringPtr tag)
{

    _customCmdResCallback = callback;

    _cmd = cmd;

    _cmd.trim();

    MB_String _tag = cmd;
    _tag.trim();

    MB_String _tag2 = tag;
    _tag2.trim();

    if (_tag2.length() == 0)
    {
        int p = MailClient.strpos(_tag.c_str(), " ", 0);
        if (p > -1)
        {
            _tag.erase(p, _tag.length() - p);
            _tag.trim();
            _imapStatus.tag = _tag;
        }
    }
    else
    {
        _imapStatus.tag = tag;
        _imapStatus.tag.trim();
        if (MailClient.strpos(_cmd.c_str(), _imapStatus.tag.c_str(), 0, false) == -1)
            _cmd = prependTag(_imapStatus.tag.c_str(), _cmd.c_str());
    }

    if (_cmd.find(" IDLE") != MB_String::npos)
        _imap_custom_cmd = esp_mail_imap_cmd_idle;
    else if (_cmd.find(" APPEND ") != MB_String::npos)
        _imap_custom_cmd = esp_mail_imap_cmd_append;
    else
        _imap_custom_cmd = esp_mail_imap_cmd_custom;

    if (_prev_imap_custom_cmd != _imap_custom_cmd || _imap_custom_cmd != esp_mail_imap_cmd_idle)
    {
        if (MailClient.imapSend(this, _cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        {
            _prev_imap_custom_cmd = esp_mail_imap_cmd_custom;
            return false;
        }
    }

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_custom;

    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
    {
        _prev_imap_custom_cmd = esp_mail_imap_cmd_custom;
        return false;
    }

    _prev_imap_custom_cmd = _imap_custom_cmd;

    if (MailClient.strposP(_cmd.c_str(), esp_mail_imap_response_14, 0, false) == 0)
    {
        bool verify = false;

        if (_sesson_cfg)
            verify = _sesson_cfg->certificate.verify;

        if (!client.connectSSL(verify))
            return false;

        // set the secure mode
        if (_sesson_cfg)
            _sesson_cfg->secure.startTLS = false;

        _secure = true;
    }

    return true;
}

bool IMAPSession::mSendData(MB_StringPtr data, bool lastData, esp_mail_imap_command cmd)
{

    MB_String _data = data;

    if (MailClient.imapSend(this, _data.c_str(), lastData) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    if (lastData)
    {
        _imap_cmd = cmd;
        _cmd.clear();

        if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
            return false;
    }

    return true;
}

bool IMAPSession::mSendData(uint8_t *data, size_t size, bool lastData, esp_mail_imap_command cmd)
{

    if (MailClient.imapSend(this, data, size) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    if (lastData)
    {
        if (MailClient.imapSend(this, "\r\n", false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return false;

        _imap_cmd = cmd;
        _cmd.clear();

        if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
            return false;
    }

    return true;
}

bool IMAPSession::mDeleteFolder(MB_StringPtr folderName)
{
    if (_debug)
    {
        esp_mail_debug_print();
        esp_mail_debug_print(esp_mail_str_321, true);
    }

    MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_323);
    cmd += folderName;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_delete;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    return true;
}

bool IMAPSession::mDeleteMessages(MessageList *toDelete, bool expunge)
{
    if (toDelete->_list.size() > 0)
    {

        if (!selectFolder(_currentFolder.c_str(), false))
            return false;

        if (_debug)
        {
            esp_mail_debug_print();
            esp_mail_debug_print(esp_mail_str_316, true);
        }

        MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_249);
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
            if (MailClient.imapSend(this, prependTag(esp_mail_str_27, esp_mail_str_317).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;

            _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_expunge;
            if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
                return false;
        }
    }

    return true;
}

bool IMAPSession::mDeleteMessagesSet(MB_StringPtr sequenceSet, bool UID, bool expunge)
{

    if (!selectFolder(_currentFolder.c_str(), false))
        return false;

    if (_debug)
    {
        esp_mail_debug_print();
        esp_mail_debug_print(esp_mail_str_316, true);
    }

    MB_String cmd;

    if (UID)
    {
        cmd = prependTag(esp_mail_str_27, esp_mail_str_137);
        cmd += esp_mail_str_249;
    }
    else
        cmd = prependTag(esp_mail_str_27, esp_mail_str_249);

    cmd += sequenceSet;

    cmd += esp_mail_str_315;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_store;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    if (expunge)
    {
        if (MailClient.imapSend(this, prependTag(esp_mail_str_27, esp_mail_str_317).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return false;

        _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_expunge;
        if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
            return false;
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
            esp_mail_debug_print(s.c_str(), true);
        }

        MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_137);
        cmd += esp_mail_str_319;
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

bool IMAPSession::mCopyMessagesSet(MB_StringPtr sequenceSet, bool UID, MB_StringPtr dest)
{

    if (!selectFolder(_currentFolder.c_str(), false))
        return false;

    if (_debug)
    {
        MB_String s = esp_mail_str_318;
        s += dest;
        esp_mail_debug_print(s.c_str(), true);
    }

    MB_String cmd;
    if (UID)
    {
        cmd = prependTag(esp_mail_str_27, esp_mail_str_137);
        cmd += esp_mail_str_319;
    }
    else
        cmd = prependTag(esp_mail_str_27, esp_mail_str_319);

    cmd += sequenceSet;
    cmd += esp_mail_str_131;
    cmd += dest;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_store;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    return true;
}

bool IMAPSession::mMoveMessages(MessageList *toMove, MB_StringPtr dest)
{

    if (!_read_capability.move)
    {

        bool ret = mCopyMessages(toMove, dest);

        if (ret)
            ret = mDeleteMessages(toMove, true);

        return ret;
    }

    if (toMove->_list.size() > 0)
    {

        if (!selectFolder(_currentFolder.c_str(), false))
            return false;

        if (_debug)
        {
            MB_String s = esp_mail_str_386;
            s += dest;
            esp_mail_debug_print(s.c_str(), true);
        }

        MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_137);
        cmd += esp_mail_str_387;
        for (size_t i = 0; i < toMove->_list.size(); i++)
        {
            if (i > 0)
                cmd += esp_mail_str_263;
            cmd += toMove->_list[i];
        }
        cmd += esp_mail_str_131;
        cmd += dest;

        if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return false;

        _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_move;
        if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
            return false;
    }

    return true;
}

bool IMAPSession::mMoveMessagesSet(MB_StringPtr sequenceSet, bool UID, MB_StringPtr dest)
{

    if (!_read_capability.move)
    {

        bool ret = mCopyMessagesSet(sequenceSet, UID, dest);

        if (ret)
            ret = mDeleteMessagesSet(sequenceSet, UID, true);

        return ret;
    }

    if (!selectFolder(_currentFolder.c_str(), false))
        return false;

    if (_debug)
    {
        MB_String s = esp_mail_str_386;
        s += dest;
        esp_mail_debug_print(s.c_str(), true);
    }

    MB_String cmd;
    if (UID)
    {
        cmd = prependTag(esp_mail_str_27, esp_mail_str_137);
        cmd += esp_mail_str_387;
    }
    else
        cmd = prependTag(esp_mail_str_27, esp_mail_str_387);

    cmd += sequenceSet;
    cmd += esp_mail_str_131;
    cmd += dest;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_move;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    return true;
}

bool IMAPSession::mGetSetQuota(MB_StringPtr quotaRoot, IMAP_Quota_Root_Info *data, bool getMode)
{
    if (!_read_capability.quota)
        return false;

    MB_String _quotaRoot = quotaRoot;

    if (_readCallback)
    {
        if (getMode)
            MailClient.imapCB(this, esp_mail_str_388, true, false);
        else
            MailClient.imapCB(this, esp_mail_str_389, true, false);
    }

    if (_debug)
    {
        if (getMode)
            esp_mail_debug_print(esp_mail_str_390, true);
        else
            esp_mail_debug_print(esp_mail_str_391, true);
    }

    MB_String cmd;
    if (getMode)
    {
        cmd = prependTag(esp_mail_str_27, esp_mail_str_392);
        cmd += esp_mail_str_131;
        cmd += esp_mail_str_136;
        cmd += _quotaRoot;
        cmd += esp_mail_str_136;
    }
    else
    {
        cmd = prependTag(esp_mail_str_27, esp_mail_str_393);
        cmd += esp_mail_str_131;
        cmd += esp_mail_str_136;
        cmd += _quotaRoot;
        cmd += esp_mail_str_136;
        cmd += esp_mail_str_131;
        cmd += esp_mail_str_198;

        cmd += data->name;
        cmd += esp_mail_str_131;
        cmd += data->limit;

        cmd += esp_mail_str_192;
    }

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _quota_tmp.clear();

    _imap_cmd = (getMode) ? esp_mail_imap_command::esp_mail_imap_cmd_get_quota : esp_mail_imap_command::esp_mail_imap_cmd_set_quota;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    if (getMode)
    {
        mParseQuota(_quota_tmp, data);
    }

    _quota_tmp.clear();

    return true;
}

void IMAPSession::mParseQuota(MB_String &quota, IMAP_Quota_Root_Info *data)
{
    MB_VECTOR<MB_String> tokens;
    MailClient.splitToken(quota, tokens, " ");
    data->quota_root = tokens[0];
    tokens[1].erase(0, 1);
    data->name = tokens[1];
    data->usage = atoi(tokens[2].c_str());
    data->limit = atoi(tokens[3].c_str());
}

bool IMAPSession::mGetQuotaRoots(MB_StringPtr mailbox, IMAP_Quota_Roots_List *quotaRootsList)
{
    if (!_read_capability.quota)
        return false;

    MB_String _mailbox = mailbox;

    if (_readCallback)
        MailClient.imapCB(this, esp_mail_str_395, true, false);

    if (_debug)
        esp_mail_debug_print(esp_mail_str_396, true);

    MB_String cmd;

    cmd = prependTag(esp_mail_str_27, esp_mail_str_392);
    cmd += esp_mail_str_394;
    cmd += esp_mail_str_131;
    cmd += esp_mail_str_136;
    cmd += _mailbox;
    cmd += esp_mail_str_136;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _quota_root_tmp.clear();
    _quota_tmp.clear();

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_get_quota_root;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    MB_VECTOR<MB_String> tokens;
    MailClient.splitToken(_quota_root_tmp, tokens, ",");

    for (size_t i = 0; i < tokens.size(); i++)
    {
        MB_VECTOR<MB_String> tk;
        MailClient.splitToken(tokens[i], tk, ":");
        IMAP_Quota_Root_Info data;
        if (tk.size() > 1)
            mParseQuota(tk[1], &data);
        else
            data.quota_root = tk[0];

        quotaRootsList->add(data);
    }

    _quota_root_tmp.clear();
    _quota_tmp.clear();

    return true;
}

bool IMAPSession::mManageACL(MB_StringPtr mailbox, IMAP_Rights_List *acl_list, IMAP_Rights_Info *acl, MB_StringPtr identifier, esp_mail_imap_command type)
{

    if (!_read_capability.acl)
        return false;

    MB_String _mailbox = mailbox;
    MB_String _identifier = identifier;

    if (_readCallback)
    {
        if (type == esp_mail_imap_cmd_get_acl)
            MailClient.imapCB(this, esp_mail_str_397, true, false);
        else if (type == esp_mail_imap_cmd_set_acl)
            MailClient.imapCB(this, esp_mail_str_399, true, false);
        else if (type == esp_mail_imap_cmd_delete_acl)
            MailClient.imapCB(this, esp_mail_str_401, true, false);
        else if (type == esp_mail_imap_cmd_my_rights)
            MailClient.imapCB(this, esp_mail_str_403, true, false);
    }

    if (_debug)
    {
        if (type == esp_mail_imap_cmd_get_acl)
            esp_mail_debug_print(esp_mail_str_398, true);
        else if (type == esp_mail_imap_cmd_set_acl)
            esp_mail_debug_print(esp_mail_str_400, true);
        else if (type == esp_mail_imap_cmd_delete_acl)
            esp_mail_debug_print(esp_mail_str_402, true);
        else if (type == esp_mail_imap_cmd_my_rights)
            esp_mail_debug_print(esp_mail_str_404, true);
    }

    MB_String cmd;
    if (type == esp_mail_imap_cmd_get_acl)
    {
        cmd = prependTag(esp_mail_str_27, esp_mail_str_405);
        cmd += esp_mail_str_131;
        cmd += esp_mail_str_136;
        cmd += _mailbox;
        cmd += esp_mail_str_136;
    }
    else if (type == esp_mail_imap_cmd_set_acl)
    {
        cmd = prependTag(esp_mail_str_27, esp_mail_str_406);
        cmd += esp_mail_str_131;
        cmd += esp_mail_str_136;
        cmd += mailbox;
        cmd += esp_mail_str_136;
        cmd += esp_mail_str_131;
        cmd += esp_mail_str_136;
        cmd += acl->identifier.c_str();
        cmd += esp_mail_str_136;
        cmd += esp_mail_str_131;
        MB_String s;
        getRights(s, acl);
        cmd += esp_mail_str_136;
        cmd += s.c_str();
        cmd += esp_mail_str_136;
    }
    else if (type == esp_mail_imap_cmd_delete_acl)
    {
        cmd = prependTag(esp_mail_str_27, esp_mail_str_407);
        cmd += esp_mail_str_131;
        cmd += esp_mail_str_136;
        cmd += _mailbox;
        cmd += esp_mail_str_136;

        cmd += esp_mail_str_131;
        cmd += esp_mail_str_136;
        cmd += _identifier;
        cmd += esp_mail_str_136;
    }
    else if (type == esp_mail_imap_cmd_my_rights)
    {
        cmd = prependTag(esp_mail_str_27, esp_mail_str_408);
        cmd += esp_mail_str_131;
        cmd += esp_mail_str_136;
        cmd += _mailbox;
        cmd += esp_mail_str_136;
    }

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _acl_tmp.clear();

    _imap_cmd = type;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    if (type == esp_mail_imap_cmd_get_acl)
        parseACL(_acl_tmp, acl_list);
    else if (type == esp_mail_imap_cmd_my_rights)
        parseRights(_acl_tmp, acl);

    _acl_tmp.clear();

    return true;
}

void IMAPSession::parseACL(MB_String &acl_str, IMAP_Rights_List *right_list)
{
    MB_VECTOR<MB_String> tokens;
    MailClient.splitToken(acl_str, tokens, " ");

    for (size_t i = 0; i < tokens.size(); i += 2)
    {
        IMAP_Rights_Info info;
        info.identifier = tokens[i];
        parseRights(tokens[i + 1], &info);
        right_list->add(info);
    }
}

void IMAPSession::parseRights(MB_String &righs_str, IMAP_Rights_Info *info)
{
    for (size_t i = 0; i < righs_str.length(); i++)
    {
        char c = righs_str[i];

        if (c == 'l')
            info->rights.lookup = true;
        else if (c == 'r')
            info->rights.read = true;
        else if (c == 's')
            info->rights.seen = true;
        else if (c == 'w')
            info->rights.write = true;
        else if (c == 'i')
            info->rights.insert = true;
        else if (c == 'p')
            info->rights.post = true;
        else if (c == 'k')
            info->rights.create = true;
        else if (c == 'c')
            info->rights.create_c = true;
        else if (c == 'x')
            info->rights.delete_mailbox = true;
        else if (c == 't')
            info->rights.delete_messages = true;
        else if (c == 'd')
            info->rights.delete_d = true;
        else if (c == 'e')
            info->rights.expunge = true;
        else if (c == 'a')
            info->rights.administer = true;
    }
}

void IMAPSession::getRights(MB_String &righs_str, IMAP_Rights_Info *info)
{
    if (info->rights.lookup)
        righs_str += (char)'l';
    if (info->rights.read)
        righs_str += (char)'r';
    if (info->rights.seen)
        righs_str += (char)'s';
    if (info->rights.write)
        righs_str += (char)'w';
    if (info->rights.insert)
        righs_str += (char)'i';
    if (info->rights.post)
        righs_str += (char)'p';
    if (info->rights.create)
        righs_str += (char)'k';
    if (info->rights.create_c)
        righs_str += (char)'c';
    if (info->rights.delete_mailbox)
        righs_str += (char)'x';
    if (info->rights.delete_mailbox)
        righs_str += (char)'t';
    if (info->rights.delete_d)
        righs_str += (char)'d';
    if (info->rights.expunge)
        righs_str += (char)'e';
    if (info->rights.administer)
        righs_str += (char)'a';
}

bool IMAPSession::mNamespace(IMAP_Namespaces_List *ns)
{
    if (!_read_capability.name_space)
        return false;

    if (_readCallback)
        MailClient.imapCB(this, esp_mail_str_409, true, false);

    if (_debug)
        esp_mail_debug_print(esp_mail_str_410, true);

    MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_411);

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _ns_tmp.clear();

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_namespace;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    int cnt = 0;
    MB_String personal, other, shared;

    for (size_t i = 0; i < _ns_tmp.length(); i++)
    {
        if (i > 0 && i < _ns_tmp.length() - 1 && _ns_tmp[i] == ' ' && _ns_tmp[i - 1] != '"' && _ns_tmp[i + 1] != '"')
        {
            if (cnt == 0)
                personal = _ns_tmp.substr(0, i);
            else if (cnt == 1)
            {
                other = _ns_tmp.substr(personal.length() + 1, i - personal.length() - 1);
                shared = _ns_tmp.substr(i + 1, _ns_tmp.length() - i - 1);
            }

            cnt++;
        }
    }

    if (personal.length() > 4)
        parseNamespaces(personal, &(ns->personal_namespaces));

    if (other.length() > 4)
        parseNamespaces(other, &(ns->other_users_namespaces));

    if (shared.length() > 4)
        parseNamespaces(shared, &(ns->shared_namespaces));

    return true;
}

bool IMAPSession::mEnable(MB_StringPtr capability)
{
    if (!_read_capability.enable)
        return false;

    MB_String _cap = capability;

    if (_readCallback)
        MailClient.imapCB(this, esp_mail_str_412, true, false);

    if (_debug)
        esp_mail_debug_print(esp_mail_str_413, true);

    MB_String cmd = prependTag(esp_mail_str_27, esp_mail_str_414);
    cmd += esp_mail_str_131;
    cmd += _cap;

    if (MailClient.imapSend(this, cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    _imap_cmd = esp_mail_imap_command::esp_mail_imap_cmd_enable;
    if (!MailClient.handleIMAPResponse(this, IMAP_STATUS_BAD_COMMAND, false))
        return false;

    return true;
}

void IMAPSession::parseNamespaces(MB_String &ns_str, IMAP_Namespaces *ns)
{
    MB_String tmp = ns_str.substr(2, ns_str.length() - 4);
    tmp.replaceAll(")(", " ");
    MB_VECTOR<MB_String> tokens;
    MailClient.splitToken(tmp, tokens, " ");

    for (size_t i = 0; i < tokens.size(); i += 2)
    {
        IMAP_Namespace_Info info;
        info.prefix = tokens[i];
        info.delimiter = tokens[i + 1];
        ns->add(info);
    }
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
    _imap_msg_num.clear();
    _mbif._searchCount = 0;
    _flags_tmp.clear();

#if defined(MB_USE_STD_VECTOR)
    _folders.clear();
    _mbif._flags.clear();
#endif
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

#endif /* ESP_MAIL_IMAP_H */