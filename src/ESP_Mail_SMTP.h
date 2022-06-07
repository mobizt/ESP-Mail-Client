
#ifndef ESP_MAIL_SMTP_H
#define ESP_MAIL_SMTP_H

/**
 * Mail Client Arduino Library for Espressif's ESP32 and ESP8266 and SAMD21 with u-blox NINA-W102 WiFi/Bluetooth module
 *
 * Created June 7, 2022
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

#include "ESP_Mail_Client_Version.h"
#include "ESP_Mail_Client.h"

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
            mime = mimeinfo[i].mimeType;
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

bool ESP_Mail_Client::smtpAuth(SMTPSession *smtp, bool &ssl)
{

init:

    // Sending greeting helo response
    if (smtp->_sendCallback)
    {
        smtpCB(smtp, "");
        smtpCBP(smtp, esp_mail_str_122);
    }

    if (smtp->_debug)
        debugInfoP(esp_mail_str_239);

    // ESMTP (rfc2821) support? send EHLO first
    MB_String s = esp_mail_str_6;
    if (smtp->_sesson_cfg->login.user_domain.length() > 0)
        s += smtp->_sesson_cfg->login.user_domain;
    else
        s += esp_mail_str_44;

    if (smtpSendP(smtp, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_greeting, esp_mail_smtp_status_code_250, 0))
    {
        // just SMTP (rfc821), send HELO
        s = esp_mail_str_5;
        if (smtp->_sesson_cfg->login.user_domain.length() > 0)
            s += smtp->_sesson_cfg->login.user_domain;
        else
            s += esp_mail_str_44;

        if (smtpSend(smtp, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return false;

        if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_greeting, esp_mail_smtp_status_code_250, SMTP_STATUS_SMTP_GREETING_SEND_ACK_FAILED))
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
        smtpSendP(smtp, esp_mail_str_311, false);
        if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_start_tls, esp_mail_smtp_status_code_250, SMTP_STATUS_SMTP_GREETING_SEND_ACK_FAILED))
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

            if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_auth, esp_mail_smtp_status_code_235, SMTP_STATUS_AUTHEN_FAILED))
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

            if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_auth_plain, esp_mail_smtp_status_code_235, SMTP_STATUS_USER_LOGIN_FAILED))
                return false;

            return true;
        }
        else if (login_auth)
        {
            if (smtp->_debug)
                debugInfoP(esp_mail_str_240);

            if (smtpSendP(smtp, esp_mail_str_4, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;

            if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_auth, esp_mail_smtp_status_code_334, SMTP_STATUS_AUTHEN_FAILED))
                return false;

            if (smtp->_debug)
            {
                s = esp_mail_str_261;
                s += smtp->_sesson_cfg->login.email;
                esp_mail_debug(s.c_str());
            }

            if (smtpSend(smtp, encodeBase64Str((const unsigned char *)smtp->_sesson_cfg->login.email.c_str(), smtp->_sesson_cfg->login.email.length()).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;

            if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_login_user, esp_mail_smtp_status_code_334, SMTP_STATUS_USER_LOGIN_FAILED))
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

            if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_login_psw, esp_mail_smtp_status_code_235, SMTP_STATUS_PASSWORD_LOGIN_FAILED))
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
        status.timestamp = smtp->ts;
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
    if (smtp)
        smtp->_customCmdResCallback = NULL;

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
        bool ssl = false;

        if (!smtp->connect(ssl))
        {
            closeTCPSession(smtp);
            return setSendingResult(smtp, msg, false);
        }

        if (!smtpAuth(smtp, ssl))
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
    checkUnencodedData(smtp, msg);

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

    // Construct 'From' header field.

    buf2 += esp_mail_str_10; // 'From' header.
    buf2 += esp_mail_str_131;

    if (msg->sender.name.length() > 0)
    {
        buf2 += esp_mail_str_136;
        buf2 += msg->sender.name; // sender name
        buf2 += esp_mail_str_136;
    }

    buf2 += esp_mail_str_14;
    buf2 += msg->sender.email; // sender Email
    buf2 += esp_mail_str_15;
    buf2 += esp_mail_str_34;

    buf = esp_mail_str_8; // 'MAIL FROM' command
    buf += esp_mail_str_14;
    buf += msg->sender.email; // sender Email
    buf += esp_mail_str_15;

    if (msg->text._int.xencoding == esp_mail_msg_xencoding_binary || msg->html._int.xencoding == esp_mail_msg_xencoding_binary)
    {
        if (smtp->_send_capability.binaryMIME || (smtp->_send_capability.chunking && msg->enable.chunking))
            buf += esp_mail_str_104;
    }
    else if (msg->text._int.xencoding == esp_mail_msg_xencoding_8bit || msg->html._int.xencoding == esp_mail_msg_xencoding_8bit)
    {
        if (smtp->_send_capability._8bitMIME)
            buf += esp_mail_str_359;
    }

    if (smtpSend(smtp, buf.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return setSendingResult(smtp, msg, false);

    if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_send_header_sender, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_SENDER_FAILED))
        return setSendingResult(smtp, msg, false);

    // Construct 'To' header fields.

    for (uint8_t i = 0; i < msg->_rcp.size(); i++)
    {
        if (i == 0)
        {
            buf2 += esp_mail_str_11; // 'To' header
            buf2 += esp_mail_str_131;
            if (msg->_rcp[i].name.length() > 0)
            {
                buf2 += esp_mail_str_136;
                buf2 += msg->_rcp[i].name; // recipient name
                buf2 += esp_mail_str_136;
            }

            buf2 += esp_mail_str_14;
            buf2 += msg->_rcp[i].email; // recipient Email
            buf2 += esp_mail_str_15;
        }
        else
        {
            if (msg->_rcp[i].name.length() > 0)
            {
                buf2 += esp_mail_str_263;
                buf2 += esp_mail_str_136;
                buf2 += msg->_rcp[i].name; // recipient name
                buf2 += esp_mail_str_136;
                buf2 += esp_mail_str_14;
            }
            else
                buf2 += esp_mail_str_13;
            buf2 += msg->_rcp[i].email; // recipient Email
            buf2 += esp_mail_str_15;
        }

        if (i == msg->_rcp.size() - 1)
            buf2 += esp_mail_str_34;

        buf.clear();

        // only address
        buf += esp_mail_str_9; // 'RCP TO' command
        buf += esp_mail_str_14;
        buf += msg->_rcp[i].email; // recipient Email
        buf += esp_mail_str_15;

        // rfc3461, rfc3464
        if (smtp->_send_capability.dsn)
        {
            if (msg->response.notify != esp_mail_smtp_notify_never)
            {

                buf += esp_mail_str_262;
                int opcnt = 0;

                if ((msg->response.notify & esp_mail_smtp_notify_success) == esp_mail_smtp_notify_success)
                {
                    if (opcnt > 0)
                        buf += esp_mail_str_263;
                    buf += esp_mail_str_264;
                    opcnt++;
                }

                if ((msg->response.notify & esp_mail_smtp_notify_failure) == esp_mail_smtp_notify_failure)
                {
                    if (opcnt > 0)
                        buf += esp_mail_str_263;
                    buf += esp_mail_str_265;
                    opcnt++;
                }

                if ((msg->response.notify & esp_mail_smtp_notify_delay) == esp_mail_smtp_notify_delay)
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

        if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_send_header_recipient, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_RECIPIENT_FAILED))
            return setSendingResult(smtp, msg, false);
    }

    // Construct 'Cc' header field.
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

        // only address
        buf += esp_mail_str_9; // 'RCP TO' command
        buf += esp_mail_str_14;
        buf += msg->_cc[i].email; // cc recipient Email
        buf += esp_mail_str_15;

        if (smtpSend(smtp, buf.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return setSendingResult(smtp, msg, false);

        if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_send_header_recipient, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_RECIPIENT_FAILED))
            return setSendingResult(smtp, msg, false);
    }

    for (uint8_t i = 0; i < msg->_bcc.size(); i++)
    {
        // only address
        buf = esp_mail_str_9; // 'RCP TO' command
        buf += esp_mail_str_14;
        buf += msg->_bcc[i].email; // bcc recipient Email
        buf += esp_mail_str_15;

        if (smtpSend(smtp, buf.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return setSendingResult(smtp, msg, false);

        if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_send_header_recipient, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_RECIPIENT_FAILED))
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

        if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_send_body, esp_mail_smtp_status_code_354, SMTP_STATUS_SEND_BODY_FAILED))
            return setSendingResult(smtp, msg, false);
    }

    if (smtpSend(smtp, buf2.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return setSendingResult(smtp, msg, false);

    MB_String s = esp_mail_str_24;
    s += esp_mail_str_131;
    s += msg->subject;
    s += esp_mail_str_34;

    // Construct the 'Date' header field.
    // The 'Date' header field should be valid and should be included in the message headers to
    // prevent the 'spam' or 'junk' message considered by mail server.

    bool dateHdr = false;
    MB_String dt;
    smtp->ts = 0;

    // Check if valid 'Date' field assigned from custom headers.
    if (msg->_hdr.size() > 0)
    {
        for (uint8_t k = 0; k < msg->_hdr.size(); k++)
        {
            s += msg->_hdr[k];
            s += esp_mail_str_34;
            if (getHeader(msg->_hdr[k].c_str(), esp_mail_str_99, dt, false))
            {
                smtp->ts = Time.getTimestamp(dt.c_str(), true);
                dateHdr = smtp->ts > ESP_MAIL_CLIENT_VALID_TS;
            }
        }
    }

    // Check if valid 'Date' field assigned from SMTP_Message's date property.
    if (!dateHdr && msg->date.length() > 0)
    {
        dt = msg->date;
        smtp->ts = Time.getTimestamp(msg->date.c_str(), true);
        dateHdr = smtp->ts > ESP_MAIL_CLIENT_VALID_TS;
    }

    if (dateHdr)
    {
        // 'Date' header field assigned.

        s += esp_mail_str_99;
        s += esp_mail_str_131;
        s += dt;
        s += esp_mail_str_34;
    }
    else
    {
        // If there is no 'Date' field assigned, get time from system and construct 'Date' header field.

        smtp->client.setSystemTime(Time.getCurrentTimestamp());
        smtp->ts = smtp->client.getTime();
        if (smtp->ts > ESP_MAIL_CLIENT_VALID_TS)
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

            if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_chunk_termination, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_BODY_FAILED))
                return false;
        }
        else
        {
            if (smtpSendP(smtp, esp_mail_str_37, false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return setSendingResult(smtp, msg, false);

            if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_send_body, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_BODY_FAILED))
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
        if (now > ESP_MAIL_CLIENT_VALID_TS)
        {
            buf += esp_mail_str_99;
            buf += esp_mail_str_131;
            buf += Time.getDateTimeString();
            buf += esp_mail_str_34;
        }
    }

    // Construct 'From' header field.

    if (msg->from.email.length() > 0)
    {
        buf += esp_mail_str_10; // 'From' header
        buf += esp_mail_str_131;

        if (msg->from.name.length() > 0)
        {
            buf += esp_mail_str_136;
            buf += msg->from.name;
            buf += esp_mail_str_136;
        }

        buf += esp_mail_str_14;
        buf += msg->from.email;
        buf += esp_mail_str_15;
        buf += esp_mail_str_34;
    }

    // Construct 'Sender' header field.
    if (msg->sender.email.length() > 0)
    {
        buf += esp_mail_str_150;
        buf += esp_mail_str_131;

        if (msg->sender.name.length() > 0)
        {
            buf += esp_mail_str_136;
            buf += msg->sender.name;
            buf += esp_mail_str_136;
        }

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

    // Construct 'To' header field.

    for (uint8_t i = 0; i < msg->_rcp.size(); i++)
    {
        if (i == 0)
        {
            buf += esp_mail_str_11; // 'To' header
            buf += esp_mail_str_131;
            if (msg->_rcp[i].name.length() > 0)
            {
                buf += esp_mail_str_136;
                buf += msg->_rcp[i].name;
                buf += esp_mail_str_136;
            }

            buf += esp_mail_str_14;
            buf += msg->_rcp[i].email;
            buf += esp_mail_str_15;
        }
        else
        {
            if (msg->_rcp[i].name.length() > 0)
            {
                buf += esp_mail_str_263;
                buf += esp_mail_str_136;
                buf += msg->_rcp[i].name;
                buf += esp_mail_str_136;
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
        if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_send_body, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_BODY_FAILED))
            return setSendingResult(smtp, msg, false);
        smtp->_chunkCount = 0;
    }
    return true;
}

void ESP_Mail_Client::checkUnencodedData(SMTPSession *smtp, SMTP_Message *msg)
{
    if (msg->type & esp_mail_msg_type_plain || msg->type == esp_mail_msg_type_enriched || msg->type & esp_mail_msg_type_html)
    {
        if ((msg->type & esp_mail_msg_type_plain || msg->type == esp_mail_msg_type_enriched) > 0)
        {
            if (msg->text.transfer_encoding.length() > 0)
            {
                if (strcmp(msg->text.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_binary) == 0)
                {
                    msg->text._int.xencoding = esp_mail_msg_xencoding_binary;
                }
                else if (strcmp(msg->text.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_8bit) == 0)
                {
                    msg->text._int.xencoding = esp_mail_msg_xencoding_8bit;
                }
                else if (strcmp(msg->text.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_7bit) == 0)
                {
                    msg->text._int.xencoding = esp_mail_msg_xencoding_7bit;
                }
            }
        }

        if ((msg->type & esp_mail_msg_type_html) > 0)
        {
            if (msg->html.transfer_encoding.length() > 0)
            {
                if (strcmp(msg->html.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_binary) == 0)
                {
                    msg->html._int.xencoding = esp_mail_msg_xencoding_binary;
                }
                else if (strcmp(msg->html.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_8bit) == 0)
                {
                    msg->html._int.xencoding = esp_mail_msg_xencoding_8bit;
                }
                else if (strcmp(msg->html.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_7bit) == 0)
                {
                    msg->html._int.xencoding = esp_mail_msg_xencoding_7bit;
                }
            }
        }
    }

    for (size_t i = 0; i < msg->_att.size(); i++)
    {
        if (strcmpP(msg->_att[i].descr.transfer_encoding.c_str(), 0, esp_mail_str_166))
        {
            msg->_att[i]._int.xencoding = esp_mail_msg_xencoding_binary;
        }
        else if (strcmpP(msg->_att[i].descr.transfer_encoding.c_str(), 0, esp_mail_str_358))
        {
            msg->_att[i]._int.xencoding = esp_mail_msg_xencoding_8bit;
        }
        else if (strcmpP(msg->_att[i].descr.transfer_encoding.c_str(), 0, esp_mail_str_29))
        {
            msg->_att[i]._int.xencoding = esp_mail_msg_xencoding_7bit;
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

    if (smtp->_sendCallback && !smtp->_customCmdResCallback)
    {
        s += esp_mail_str_53;
        s += smtp->errorReason().c_str();
        smtpCB(smtp, s.c_str(), false);
    }

    if (smtp->_debug && !smtp->_customCmdResCallback)
    {
        s = esp_mail_str_185;
        s += smtp->errorReason().c_str();
        esp_mail_debug(s.c_str());
    }
}

size_t ESP_Mail_Client::smtpSendP(SMTPSession *smtp, PGM_P v, bool newline)
{
    int sent = 0;

    if (!reconnect(smtp))
    {
        closeTCPSession(smtp);
        return sent;
    }

    if (!connected(smtp))
    {
        errorStatusCB(smtp, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);
        return sent;
    }

    if (!smtp->_tcpConnected)
    {
        errorStatusCB(smtp, MAIL_CLIENT_ERROR_SERVER_CONNECTION_FAILED);
        return sent;
    }

    MB_String s = v;

    int toSend = newline ? s.length() + 2 : s.length();

    if (!smtp->_customCmdResCallback && smtp->_debugLevel > esp_mail_debug_level_maintener)
        esp_mail_debug_line(s.c_str(), newline);

    sent = newline ? smtp->client.println(s.c_str()) : smtp->client.print(s.c_str());

    if (sent != toSend)
    {
        errorStatusCB(smtp, sent);
        sent = 0;
    }

    return sent;
}

size_t ESP_Mail_Client::smtpSend(SMTPSession *smtp, const char *data, bool newline)
{
    return smtpSendP(smtp, data, newline);
}

size_t ESP_Mail_Client::smtpSend(SMTPSession *smtp, int data, bool newline)
{
    MB_String s = data;
    return smtpSendP(smtp, s.c_str(), newline);
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

    size_t sent = smtp->client.write(data, size);

    if (sent != size)
    {
        errorStatusCB(smtp, sent);
        sent = 0;
    }

    return sent;
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
                header += esp_mail_str_177;
                header += esp_mail_str_136;
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
                header += esp_mail_str_177;
                header += esp_mail_str_136;
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

    if (inlineAttach->descr.description.length() > 0)
    {
        header += esp_mail_str_182;
        header += inlineAttach->descr.description;
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

    if (attach->descr.description.length() > 0)
    {
        header += esp_mail_str_182;
        header += attach->descr.description;
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
    if (smtp->_sendCallback)
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

bool ESP_Mail_Client::handleSMTPResponse(SMTPSession *smtp, esp_mail_smtp_command cmd, esp_mail_smtp_status_code respCode, int errCode)
{
    smtp->_smtp_cmd = cmd;

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

    status.id = smtp->_commandID;

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

                chunkBufSize = ESP_MAIL_CLIENT_RESPONSE_BUFFER_SIZE;
                response = (char *)newP(chunkBufSize + 1);

            read_line:

                readLen = readLine(smtp, response, chunkBufSize, false, count);

                if (readLen)
                {
                    if (smtp->_smtp_cmd != esp_mail_smtp_command::esp_mail_smtp_cmd_initial_state)
                    {
                        // sometimes server sent multiple lines response
                        // sometimes rx buffer is not ready for a while

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

                            if (!smtp->_customCmdResCallback && smtp->_debugLevel > esp_mail_debug_level_basic)
                                esp_mail_debug((const char *)response);
                        }

                        if (smtp->_smtp_cmd == esp_mail_smtp_command::esp_mail_smtp_cmd_greeting)
                            handleAuth(smtp, response);
                    }

                    getResponseStatus(response, respCode, 0, status);

                    // No response code from greeting?
                    // Assumed multi-line greeting responses.

                    if (status.respCode == 0 && smtp->_smtp_cmd == esp_mail_smtp_command::esp_mail_smtp_cmd_initial_state)
                    {
                        s = esp_mail_str_260;
                        s += response;

                        if (smtp->_debug && !smtp->_customCmdResCallback)
                            esp_mail_debug(s.c_str());

                        memset(response, 0, chunkBufSize + 1);

                        // read again until we get the response code
                        goto read_line;
                    }

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
                            size_t olen = 0;
                            char *decoded = (char *)decodeBase64((const unsigned char *)status.text.c_str(), status.text.length(), &olen);
                            if (decoded && olen > 0)
                            {
                                s.append(decoded, olen);
                                delP(&decoded);
                            }
                        }
                        if (!smtp->_customCmdResCallback)
                            esp_mail_debug(s.c_str());
                        r.clear();
                    }

                    if (smtp->_customCmdResCallback && (smtp->_smtp_cmd == esp_mail_smtp_command::esp_mail_smtp_cmd_initial_state || smtp->_smtp_cmd == esp_mail_smtp_command::esp_mail_smtp_cmd_custom))
                    {
                        struct esp_mail_smtp_response_status_t res = status;
                        res.text = response;
                        smtp->_customCmdResCallback(res);
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

        if (!ret && !smtp->_customCmdResCallback)
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
        s = (int)respCode;
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
    bool ssl = false;

    this->_customCmdResCallback = NULL;

    if (!handleConnection(config, ssl))
        return false;

    return MailClient.smtpAuth(this, ssl);
}

int SMTPSession::customConnect(ESP_Mail_Session *config, smtpResponseCallback callback, int commandID)
{
    this->_customCmdResCallback = callback;

    if (commandID > -1)
        this->_commandID = commandID;
    else
        this->_commandID++;

    bool ssl = false;
    if (!handleConnection(config, ssl))
        return -1;

    return this->_smtpStatus.respCode;
}

bool SMTPSession::handleConnection(ESP_Mail_Session *config, bool &ssl)
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

    _sesson_cfg = config;

#if defined(ESP32) || defined(ESP8266)
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

bool SMTPSession::connect(bool &ssl)
{
    if (!MailClient.reconnect(this))
        return false;

    ssl = false;
    _secure = true;
    bool secureMode = true;

    MB_String s;

#if defined(ESP32) && defined(ESP32_TCP_CLIENT)
    client.setDebugCallback(NULL);
#elif defined(ESP8266) && defined(ESP8266_TCP_CLIENT)
    client.rxBufDivider = 16; // minimum rx buffer for smtp status response
    client.txBufDivider = 8;  // medium tx buffer for faster attachment/inline data transfer
#endif

    if (_sesson_cfg->server.port == esp_mail_smtp_port_25)
    {
        _secure = false;
        secureMode = false;
    }
    else
    {
        if (_sesson_cfg->server.port == esp_mail_smtp_port_587)
            _sesson_cfg->secure.startTLS = true;

        secureMode = !_sesson_cfg->secure.startTLS;

        // to prevent to send the connection upgrade command when some server promotes
        // the starttls capability even the current connection was already secured.
        if (_sesson_cfg->server.port == esp_mail_smtp_port_465)
            ssl = true;
    }

    // Server connection attempt: no status code
    if (_sendCallback && !_customCmdResCallback)
        MailClient.smtpCBP(this, esp_mail_str_120);

    if (_debug && !_customCmdResCallback)
    {
        s = esp_mail_str_314;
        s += ESP_MAIL_VERSION;
        s += client.fwVersion();
        esp_mail_debug(s.c_str());

#if defined(BOARD_HAS_PSRAM) && defined(MB_STRING_USE_PSRAM)
        if (ESP.getPsramSize() == 0)
        {
            s = esp_mail_str_353;
            esp_mail_debug(s.c_str());
        }
#endif
    }

    bool validTime = false;

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)
    validTime = true; // strlen(_sesson_cfg->certificate.cert_file) > 0 || _caCert != nullptr;
#endif

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_SAMD) || defined(__AVR_ATmega4809__) || defined(ARDUINO_NANO_RP2040_CONNECT)

    if (!_customCmdResCallback && (_sesson_cfg->time.ntp_server.length() > 0 || validTime))
    {
        s = esp_mail_str_355;
        if (!_customCmdResCallback)
            esp_mail_debug(s.c_str());
        MailClient.setTime(_sesson_cfg->time.gmt_offset, _sesson_cfg->time.day_light_offset, _sesson_cfg->time.ntp_server.c_str(), _sesson_cfg->time.timezone_env_string.c_str(), _sesson_cfg->time.timezone_file.c_str(), true);

        if (!MailClient.Time.clockReady())
            MailClient.errorStatusCB(this, MAIL_CLIENT_ERROR_NTP_TIME_SYNC_TIMED_OUT);
    }

#endif

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)
    MailClient.setSecure(client, _sesson_cfg, _caCert);
#endif

    if (_debug && !_customCmdResCallback)
    {
        MailClient.debugInfoP(esp_mail_str_236);
        s = esp_mail_str_261;
        s += esp_mail_str_211;
        s += _sesson_cfg->server.host_name;
        esp_mail_debug(s.c_str());
        s = esp_mail_str_261;
        s += esp_mail_str_201;
        s += _sesson_cfg->server.port;
        esp_mail_debug(s.c_str());
    }

#if defined(ESP32) && defined(ESP32_TCP_CLIENT)
    if (_debug && !_customCmdResCallback)
        client.setDebugCallback(esp_mail_debug);
#endif

    client.begin(_sesson_cfg->server.host_name.c_str(), _sesson_cfg->server.port);

    client.ethDNSWorkAround();

    if (!client.connect(secureMode, _sesson_cfg->certificate.verify))
        return MailClient.handleSMTPError(this, SMTP_STATUS_SERVER_CONNECT_FAILED);

    // server connected
    _tcpConnected = true;

    if (_debug && !_customCmdResCallback)
        MailClient.debugInfoP(esp_mail_str_238);

    if (_sendCallback && !_customCmdResCallback)
    {
        MailClient.smtpCB(this, "");
        MailClient.smtpCBP(this, esp_mail_str_121);
    }

    client.setTimeout(TCP_CLIENT_DEFAULT_TCP_TIMEOUT_SEC);

    // expected status code 220 for ready to service
    if (!MailClient.handleSMTPResponse(this, esp_mail_smtp_cmd_initial_state, esp_mail_smtp_status_code_220, SMTP_STATUS_SMTP_GREETING_GET_RESPONSE_FAILED))
        return false;

    return true;
}

int SMTPSession::mSendCustomCommand(MB_StringPtr cmd, smtpResponseCallback callback, int commandID)
{
    _customCmdResCallback = callback;

    if (commandID > -1)
        _commandID = commandID;
    else
        _commandID++;

    MB_String _cmd = cmd;

    if (MailClient.smtpSend(this, _cmd.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return -1;

    if (!MailClient.handleSMTPResponse(this, esp_mail_smtp_cmd_custom, esp_mail_smtp_status_code_0, SMTP_STATUS_SEND_CUSTOM_COMMAND_FAILED))
        return -1;

    if (MailClient.strposP(_cmd.c_str(), esp_mail_smtp_response_5, 0, false) == 0)
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

    return this->_smtpStatus.respCode;
}

bool SMTPSession::mSendData(MB_StringPtr data)
{

    MB_String _data = data;

    if (MailClient.smtpSend(this, _data.c_str(), false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    return true;
}

bool SMTPSession::mSendData(uint8_t *data, size_t size)
{

    if (MailClient.smtpSend(this, data, size) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    return true;
}

void SMTPSession::debug(int level)
{
    if (level > esp_mail_debug_level_none)
    {
        _debugLevel = level;
        _debug = true;
    }
    else
    {
        _debugLevel = esp_mail_debug_level_none;
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
    case TCP_CLIENT_ERROR_NOT_CONNECTED:
        ret += esp_mail_str_357;
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

    if (_debug && !_customCmdResCallback)
        MailClient.debugInfoP(esp_mail_str_245);

    bool ret = true;

/* Sign out */
#if !defined(ESP8266)
    /**
     * The strange behavior in ESP8266 SSL client, BearSSLWiFiClientSecure
     * The client disposed without memory released after the server close
     * the connection due to QUIT command, which caused the memory leaks.
     */
    MailClient.smtpSendP(this, esp_mail_str_7, true);
    ret = MailClient.handleSMTPResponse(this, esp_mail_smtp_cmd_logout, esp_mail_smtp_status_code_221, SMTP_STATUS_SEND_BODY_FAILED);
#endif

    if (ret)
    {

        if (_sendCallback)
        {
            MailClient.smtpCB(this, "");
            MailClient.smtpCBP(this, esp_mail_str_129, false);
        }

        if (_debug && !_customCmdResCallback)
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

#endif /* ESP_MAIL_SMTP_H */