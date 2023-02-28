
#ifndef ESP_MAIL_SMTP_H
#define ESP_MAIL_SMTP_H

/**
 * Mail Client Arduino Library for Espressif's ESP32 and ESP8266, Raspberry Pi RP2040 Pico, and SAMD21 with u-blox NINA-W102 WiFi/Bluetooth module
 *
 * Created February 16, 2023
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

#if defined(ENABLE_SMTP)

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

bool ESP_Mail_Client::smtpAuth(SMTPSession *smtp, bool &ssl)
{
    if (!smtp)
        return false;

non_authenticated:

    // Sending greeting helo response
    if (smtp->_sendCallback)
        smtpCB(smtp, esp_mail_str_122, true, false);

    if (smtp->_debug)
        esp_mail_debug_print(esp_mail_str_239, true);

    // Extended HELLO (EHLO) or HELLO (HELO) was used to identify Client (ourself)

    // If we (client) are able to process service extensions, let the server know by sending
    // the ESMTP (rfc5321) EHLO command to identify ourself first

    // If the EHLO command is not acceptable to the SMTP server, 501, 500,
    // 502, or 550 failure replies MUST be returned as appropriate.
    // It server accept EHLO, it should response with the extensions it supported.

    MB_String s = esp_mail_str_6; /* "EHLO " "*/
    // The EHLO/HELO command parameter should be the primary host name (domain name) of client system.
    // Alternatively client public IP address string (IPv4 or IPv6) can be assign when no host name is available
    // to prevent connection rejection.

    if (smtp->_sesson_cfg->login.user_domain.length() > 0)
        s += smtp->_sesson_cfg->login.user_domain;
    else
        s += esp_mail_str_44;

    if (smtpSend(smtp, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return false;

    if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_greeting, esp_mail_smtp_status_code_250, 0))
    {

        // In case EHLO command is not acceptable,
        // we would fall back and send SMTP (rfc821) HELO command to identify ourself.
        s = esp_mail_str_5;
        if (smtp->_sesson_cfg->login.user_domain.length() > 0)
            s += smtp->_sesson_cfg->login.user_domain;
        else
            s += esp_mail_str_44;

        if (!smtpSend(smtp, s.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
            return false;

        if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_greeting, esp_mail_smtp_status_code_250, SMTP_STATUS_SMTP_GREETING_SEND_ACK_FAILED))
            return false;

        smtp->_send_capability.esmtp = false;
        smtp->_auth_capability.login = true;
    }
    else
        smtp->_send_capability.esmtp = true;

    // start TLS when needed
    // rfc3207
    if ((smtp->_auth_capability.start_tls || smtp->_sesson_cfg->secure.startTLS) && !ssl)
    {
        // send starttls command
        if (smtp->_sendCallback)
            smtpCB(smtp, esp_mail_str_209, true, false);

        if (smtp->_debug)
        {
            s = esp_mail_str_196;
            esp_mail_debug_print(s.c_str(), true);
        }

        // expected status code 250 for complete the request
        // some server returns 220 to restart to initial state
        smtpSend(smtp, esp_mail_str_311, false);
        if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_start_tls, esp_mail_smtp_status_code_250, SMTP_STATUS_SMTP_GREETING_SEND_ACK_FAILED))
            return false;

        if (smtp->_debug)
        {
            esp_mail_debug_print(esp_mail_str_310, true);
        }

        // connect in secure mode
        // do TLS handshake
        if (!smtp->client.connectSSL(smtp->_sesson_cfg->certificate.verify))
            return handleSMTPError(smtp, MAIL_CLIENT_ERROR_SSL_TLS_STRUCTURE_SETUP);

        // set the secure mode
        smtp->_sesson_cfg->secure.startTLS = false;
        ssl = true;
        smtp->_secure = true;

        // return to initial state if the response status is 220.
        if (smtp->_smtpStatus.respCode == esp_mail_smtp_status_code_220)
            goto non_authenticated;
    }

    bool creds = smtp->_sesson_cfg->login.email.length() > 0 && smtp->_sesson_cfg->login.password.length() > 0;
    bool sasl_auth_oauth = smtp->_sesson_cfg->login.accessToken.length() > 0 && smtp->_auth_capability.xoauth2;
    bool sasl_login = smtp->_auth_capability.login && creds;
    bool sasl_auth_plain = smtp->_auth_capability.plain && creds;

    if (sasl_auth_oauth || sasl_login || sasl_auth_plain)
    {
        if (smtp->_sendCallback)
            smtpCB(smtp, esp_mail_str_56, true, false);

        // log in
        if (sasl_auth_oauth)
        {
            if (smtp->_debug)
                esp_mail_debug_print(esp_mail_str_288, true);

            if (!smtp->_auth_capability.xoauth2)
                return handleSMTPError(smtp, SMTP_STATUS_SERVER_OAUTH2_LOGIN_DISABLED, false);

            if (smtpSend(smtp, esp_mail_str_289, false) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;

            if (smtpSend(smtp, getXOAUTH2String(smtp->_sesson_cfg->login.email, smtp->_sesson_cfg->login.accessToken).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;

            if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_auth, esp_mail_smtp_status_code_235, SMTP_STATUS_AUTHEN_FAILED))
                return false;
        }
        else if (sasl_auth_plain)
        {

            if (smtp->_debug)
            {
                esp_mail_debug_print(esp_mail_str_241, true);

                s = esp_mail_str_261;
                s += smtp->_sesson_cfg->login.email;
                esp_mail_debug_print(s.c_str(), true);

                s += esp_mail_str_131;
                for (size_t i = 0; i < smtp->_sesson_cfg->login.password.length(); i++)
                    s += esp_mail_str_183;
                esp_mail_debug_print(s.c_str(), true);
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
        }
        else if (sasl_login)
        {
            if (smtp->_debug)
                esp_mail_debug_print(esp_mail_str_240, true);

            if (smtpSend(smtp, esp_mail_str_4, true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;

            if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_auth, esp_mail_smtp_status_code_334, SMTP_STATUS_AUTHEN_FAILED))
                return false;

            if (smtp->_debug)
            {
                s = esp_mail_str_261;
                s += smtp->_sesson_cfg->login.email;
                esp_mail_debug_print(s.c_str(), true);
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
                esp_mail_debug_print(s.c_str(), true);
            }

            if (smtpSend(smtp, encodeBase64Str((const unsigned char *)smtp->_sesson_cfg->login.password.c_str(), smtp->_sesson_cfg->login.password.length()).c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
                return false;

            if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_login_psw, esp_mail_smtp_status_code_235, SMTP_STATUS_PASSWORD_LOGIN_FAILED))
                return false;
        }

        smtp->_authenticated = true;
    }

    return true;
}

bool ESP_Mail_Client::connected(SMTPSession *smtp)
{
    if (!smtp)
        return false;
    return smtp->client.connected();
}

bool ESP_Mail_Client::addSendingResult(SMTPSession *smtp, SMTP_Message *msg, bool result)
{
    if (!smtp)
        return false;

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
    if (!smtp)
        return false;

    smtp->_customCmdResCallback = NULL;

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
        return addSendingResult(smtp, msg, false);
    }

    for (uint8_t i = 0; i < msg->_rcp.size(); i++)
    {
        if (validEmail(msg->_rcp[i].email.c_str()))
            validRecipient = true;
    }

    if (!validRecipient)
    {
        errorStatusCB(smtp, SMTP_STATUS_NO_VALID_RECIPIENTS_EXISTED);
        return addSendingResult(smtp, msg, false);
    }

    return true;
}

bool ESP_Mail_Client::mSendMail(SMTPSession *smtp, SMTP_Message *msg, bool closeSession)
{
    if (!smtp)
        return false;

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
            return addSendingResult(smtp, msg, false);
        }

        if (!smtpAuth(smtp, ssl))
        {
            closeTCPSession(smtp);
            return addSendingResult(smtp, msg, false);
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
            if (smtp->_sentSuccessCount || smtp->_sentFailedCount)
                smtpCB(smtp, esp_mail_str_267, true, false);
            else
                smtpCB(smtp, esp_mail_str_208, true, false);
        }

        if (smtp->_debug)
        {
            if (smtp->_sentSuccessCount || smtp->_sentFailedCount)
                esp_mail_debug_print(esp_mail_str_268, true);
            else
                esp_mail_debug_print(esp_mail_str_207, true);
        }
    }

    if (smtp->_sendCallback)
        smtpCB(smtp, esp_mail_str_125, true, false);

    if (smtp->_debug)
        esp_mail_debug_print(esp_mail_str_242, true);

    imap = nullptr;
    calDataLen = false;
    dataLen = 0;

    return sendContent(smtp, msg, closeSession, rfc822MSG);
}

bool ESP_Mail_Client::sendContent(SMTPSession *smtp, SMTP_Message *msg, bool closeSession, bool rfc822MSG)
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

    if (!imap && smtp)
    {
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

        if (!altSendData(buf, true, smtp, msg, true, true, esp_mail_smtp_cmd_send_header_sender, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_SENDER_FAILED))
            return false;
    }

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

        if (!imap && smtp)
        {

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

            if (!altSendData(buf, true, smtp, msg, true, true, esp_mail_smtp_cmd_send_header_recipient, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_RECIPIENT_FAILED))
                return false;
        }
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

        if (!imap)
        {

            buf.clear();

            // only address
            buf += esp_mail_str_9; // 'RCP TO' command
            buf += esp_mail_str_14;
            buf += msg->_cc[i].email; // cc recipient Email
            buf += esp_mail_str_15;

            if (!altSendData(buf, true, smtp, msg, true, true, esp_mail_smtp_cmd_send_header_recipient, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_RECIPIENT_FAILED))
                return false;
        }
    }

    if (!imap && smtp)
    {
        for (uint8_t i = 0; i < msg->_bcc.size(); i++)
        {
            // only address
            buf = esp_mail_str_9; // 'RCP TO' command
            buf += esp_mail_str_14;
            buf += msg->_bcc[i].email; // bcc recipient Email
            buf += esp_mail_str_15;

            if (!altSendData(buf, true, smtp, msg, true, true, esp_mail_smtp_cmd_send_header_recipient, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_HEADER_RECIPIENT_FAILED))
                return false;
        }

        altSendCallback(smtp, esp_mail_str_126, esp_mail_str_243, true, false);

        if (smtp->_send_capability.chunking && msg->enable.chunking)
        {
            smtp->_chunkedEnable = true;
            if (!sendBDAT(smtp, msg, buf2.length(), false))
                return false;
        }
        else
        {
            MB_String sdata = esp_mail_str_16;
            if (!altSendData(sdata, true, smtp, msg, true, true, esp_mail_smtp_cmd_send_body, esp_mail_smtp_status_code_354, SMTP_STATUS_SEND_BODY_FAILED))
                return false;
        }
    }

#if defined(ENABLE_IMAP)
    if (imap)
        altSendCallback(smtp, esp_mail_str_361, esp_mail_str_362, true, false);
#endif

    if (!altSendData(buf2, false, smtp, msg, true, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
        return false;

    MB_String s = esp_mail_str_24;
    s += esp_mail_str_131;
    s += msg->subject;
    s += esp_mail_str_34;

    // Construct the 'Date' header field.
    // The 'Date' header field should be valid and should be included in the message headers to
    // prevent the 'spam' or 'junk' message considered by mail server.

    bool dateHdr = false;
    MB_String dt;

    uint32_t ts = 0;

    if (smtp)
        smtp->ts = ts;

    // Check if valid 'Date' field assigned from custom headers.
    if (msg->_hdr.size() > 0)
    {
        for (uint8_t k = 0; k < msg->_hdr.size(); k++)
        {
            s += msg->_hdr[k];
            s += esp_mail_str_34;
            if (getHeader(msg->_hdr[k].c_str(), esp_mail_str_99, dt, false))
            {
                ts = Time.getTimestamp(dt.c_str(), true);
                dateHdr = ts > ESP_MAIL_CLIENT_VALID_TS;
                if (smtp)
                    smtp->ts = ts;
            }
        }
    }

    // Check if valid 'Date' field assigned from SMTP_Message's date property.
    if (!dateHdr && msg->date.length() > 0)
    {
        dt = msg->date;
        ts = Time.getTimestamp(msg->date.c_str(), true);
        dateHdr = ts > ESP_MAIL_CLIENT_VALID_TS;
        if (smtp)
            smtp->ts = ts;
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
        if (smtp)
        {
            ts = MailClient.Time.getCurrentTimestamp();
            smtp->ts = ts;
        }
        else if (imap)
        {
#if defined(ENABLE_IMAP)
            if (calDataLen)
            {
                ts = MailClient.Time.getCurrentTimestamp();
                imap_ts = ts;
            }
            else
                ts = imap_ts;
#endif
        }

        if (ts > ESP_MAIL_CLIENT_VALID_TS)
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

    if (!sendBDAT(smtp, msg, s.length(), false))
        return false;

    if (!altSendData(s, false, smtp, msg, true, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
        return false;

    return sendMSGData(smtp, msg, closeSession, rfc822MSG);
}

void ESP_Mail_Client::altSendCallback(SMTPSession *smtp, PGM_P s1, PGM_P s2, bool newline1, bool newline2)
{
    if (smtp)
    {

        if (smtp->_sendCallback)
            smtpCB(smtp, s1, newline1, false);

        if (smtp->_debug)
            esp_mail_debug_print(s2, true);

        if (smtp->_sendCallback && newline2)
            esp_mail_debug_print();
    }
    else if (imap && !calDataLen)
    {
#if defined(ENABLE_IMAP)

        if (imap->_readCallback)
            imapCB(imap, s1, newline1, false);

        if (imap->_debug)
            esp_mail_debug_print(s2, true);

        if (imap->_readCallback && newline2)
            esp_mail_debug_print();
#endif
    }
}

bool ESP_Mail_Client::sendMSGData(SMTPSession *smtp, SMTP_Message *msg, bool closeSession, bool rfc822MSG)
{
    MB_String s;
    MB_String mixed = getMIMEBoundary(15);
    MB_String alt = getMIMEBoundary(15);

    if (numAtt(smtp, esp_mail_att_type_attachment, msg) == 0 && msg->_parallel.size() == 0 && msg->_rfc822.size() == 0)
    {
        if (msg->type == (esp_mail_msg_type_plain | esp_mail_msg_type_html | esp_mail_msg_type_enriched) || numAtt(smtp, esp_mail_att_type_inline, msg) > 0)
        {
            if (!sendMSG(smtp, msg, alt))
                return addSendingResult(smtp, msg, false);
        }
        else if (msg->type != esp_mail_msg_type_none)
        {
            if (!sendPartText(smtp, msg, msg->type, ""))
                return addSendingResult(smtp, msg, false);
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

        if (!sendBDAT(smtp, msg, s.length(), false))
            return false;

        if (!altSendData(s, false, smtp, msg, true, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
            return false;

        if (!sendMSG(smtp, msg, alt))
            return addSendingResult(smtp, msg, false);

        if (!sendBDAT(smtp, msg, 2, false))
            return false;

        MB_String str = esp_mail_str_34;
        if (!altSendData(str, false, smtp, msg, true, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
            return false;

        altSendCallback(smtp, esp_mail_str_127, esp_mail_str_244, true, numAtt(smtp, esp_mail_att_type_attachment, msg) > 0);

        if (!sendAttachments(smtp, msg, mixed))
            return addSendingResult(smtp, msg, false);

        if (!sendParallelAttachments(smtp, msg, mixed))
            return addSendingResult(smtp, msg, false);

        if (!sendRFC822Msg(smtp, msg, mixed, closeSession, msg->_rfc822.size() > 0))
            return addSendingResult(smtp, msg, false);

        s = esp_mail_str_33;
        s += mixed;
        s += esp_mail_str_33;

        if (!sendBDAT(smtp, msg, s.length(), false))
            return false;

        if (!altSendData(s, false, smtp, msg, true, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
            return false;
    }

    if (!rfc822MSG && !imap && smtp)
    {

        altSendCallback(smtp, esp_mail_str_303, esp_mail_str_304, true, false);

        if (smtp->_chunkedEnable)
        {

            if (!sendBDAT(smtp, msg, 0, true))
                return false;

            if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_chunk_termination, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_BODY_FAILED))
                return false;
        }
        else
        {
            MB_String str = esp_mail_str_37;
            if (!altSendData(str, false, smtp, msg, true, true, esp_mail_smtp_cmd_send_body, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_BODY_FAILED))
                return false;
        }

        addSendingResult(smtp, msg, true);

        if (closeSession && smtp)
        {
            if (!smtp->closeSession())
                return false;
        }
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

        if (!sendBDAT(smtp, msg, buf.length(), false))
            return false;

        if (!altSendData(buf, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
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
        time_t now = 0;
        if (smtp)
            now = MailClient.Time.getCurrentTimestamp();
        else if (imap)
        {
#if defined(ENABLE_IMAP)
            if (calDataLen)
            {
                now = MailClient.Time.getCurrentTimestamp();
                imap_ts = now;
            }
            else
                now = imap_ts;
#endif
        }

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

bool ESP_Mail_Client::sendBDAT(SMTPSession *smtp, SMTP_Message *msg, int len, bool last)
{
    if (!smtp)
        return true;

    if (!smtp->_chunkedEnable || !msg->enable.chunking)
        return true;

    smtp->_chunkCount++;

    MB_String bdat = esp_mail_str_106;
    bdat += len;
    if (last)
        bdat += esp_mail_str_173;

    if (smtpSend(smtp, bdat.c_str(), true) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        return addSendingResult(smtp, msg, false);

    if (!smtp->_send_capability.pipelining)
    {
        if (!handleSMTPResponse(smtp, esp_mail_smtp_cmd_send_body, esp_mail_smtp_status_code_250, SMTP_STATUS_SEND_BODY_FAILED))
            return addSendingResult(smtp, msg, false);
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

bool ESP_Mail_Client::altIsCB(SMTPSession *smtp)
{
    bool cb = false;
    if (smtp)
        cb = smtp->_sendCallback != NULL;
    else if (imap && !calDataLen)
    {
#if defined(ENABLE_IMAP)
        cb = imap->_readCallback != NULL;
#endif
    }

    return cb;
}

bool ESP_Mail_Client::altIsDebug(SMTPSession *smtp)
{
    bool dbg = false;
    if (smtp)
        dbg = smtp->_debug;
    else if (imap && !calDataLen)
    {
#if defined(ENABLE_IMAP)
        dbg = imap->_debug;
#endif
    }

    return dbg;
}

bool ESP_Mail_Client::sendBlobAttachment(SMTPSession *smtp, SMTP_Message *msg, SMTP_Attachment *att)
{
    bool cb = altIsCB(smtp);
    uint32_t addr = altProgressPtr(smtp);

    if (strcmp(att->descr.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0 && strcmp(att->descr.transfer_encoding.c_str(), att->descr.content_encoding.c_str()) != 0)
    {
        esp_mail_smtp_send_base64_data_info_t data_info;

        data_info.rawPtr = att->blob.data;
        data_info.size = att->blob.size;
        data_info.flashMem = att->_int.flash_blob;
        data_info.filename = att->descr.filename.c_str();

        if (!sendBase64(smtp, msg, data_info, true, cb))
            return false;

        return true;
    }
    else
    {
        if (att->blob.size > 0)
        {
            if (strcmp(att->descr.content_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0)
            {
                esp_mail_smtp_send_base64_data_info_t data_info;

                data_info.rawPtr = att->blob.data;
                data_info.size = att->blob.size;
                data_info.flashMem = att->_int.flash_blob;
                data_info.filename = att->descr.filename.c_str();

                if (!sendBase64(smtp, msg, data_info, false, cb))
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

                    if (!sendBDAT(smtp, msg, chunkSize, false))
                        break;
                    memcpy_P(buf, att->blob.data, chunkSize);

                    if (!altSendData(buf, chunkSize, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                        break;

                    if (cb)
                        uploadReport(att->descr.filename.c_str(), addr, 100 * writeLen / att->blob.size);

                    writeLen += chunkSize;
                }
                delP(&buf);

                if (cb)
                    uploadReport(att->descr.filename.c_str(), addr, 100);

                return writeLen >= att->blob.size;
            }
        }
    }
    return false;
}

bool ESP_Mail_Client::sendFile(SMTPSession *smtp, SMTP_Message *msg, SMTP_Attachment *att)
{
    bool cb = altIsCB(smtp);
    uint32_t addr = altProgressPtr(smtp);

    if (strcmp(att->descr.transfer_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0 && strcmp(att->descr.transfer_encoding.c_str(), att->descr.content_encoding.c_str()) != 0)
    {
        esp_mail_smtp_send_base64_data_info_t data_info;

        data_info.filename = att->descr.filename.c_str();
        data_info.storageType = att->file.storage_type;

        if (!sendBase64(smtp, msg, data_info, true, cb))
            return false;

        return true;
    }
    else
    {
        if (mbfs->size(mbfs_type att->file.storage_type) > 0)
        {
            if (strcmp(att->descr.content_encoding.c_str(), Content_Transfer_Encoding::enc_base64) == 0)
            {
                esp_mail_smtp_send_base64_data_info_t data_info;

                data_info.filename = att->descr.filename.c_str();
                data_info.storageType = att->file.storage_type;

                if (!sendBase64(smtp, msg, data_info, false, cb))
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

                    if (!sendBDAT(smtp, msg, chunkSize, false))
                        break;

                    if (!altSendData(buf, chunkSize, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                        break;

                    if (cb)
                        uploadReport(att->descr.filename.c_str(), addr, 100 * writeLen / fileSize);

                    writeLen += chunkSize;
                }
                delP(&buf);

                if (cb)
                    uploadReport(att->descr.filename.c_str(), addr, 100);

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

    MB_String parallel = getMIMEBoundary(15);
    MB_String buf = esp_mail_str_33;
    buf += boundary;
    buf += esp_mail_str_34;

    buf += esp_mail_str_28;
    buf += parallel;
    buf += esp_mail_str_35;

    if (!sendBDAT(smtp, msg, buf.length(), false))
        return false;

    if (!altSendData(buf, false, smtp, msg, true, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
        return false;

    if (!sendAttachments(smtp, msg, parallel, true))
        return addSendingResult(smtp, msg, false);

    buf = esp_mail_str_33;
    buf += parallel;
    buf += esp_mail_str_33;

    if (!sendBDAT(smtp, msg, buf.length(), false))
        return false;

    if (!altSendData(buf, false, smtp, msg, true, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
        return false;

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

            altSendCallback(smtp, att->descr.filename.c_str(), s.c_str(), cnt > 0, false);

            cnt++;

            if (att->file.storage_type == esp_mail_file_storage_type_none)
            {
                if (!att->blob.data)
                    continue;

                if (att->blob.size == 0)
                    continue;

                altSendCallback(smtp, att->descr.filename.c_str(), s.c_str(), false, false);

                buf.clear();
                getAttachHeader(buf, boundary, att, att->blob.size);

                if (!sendBDAT(smtp, msg, buf.length(), false))
                    return false;

                if (!altSendData(buf, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                    return false;

                if (!sendBlobAttachment(smtp, msg, att))
                    return false;

                if (!sendBDAT(smtp, msg, 2, false))
                    return false;

                MB_String str = esp_mail_str_34;

                if (!altSendData(str, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                    return false;
            }
            else
            {
                if (att->file.storage_type == esp_mail_file_storage_type_sd && !smtp->_sdStorageChecked && !smtp->_sdStorageReady)
                {
                    smtp->_sdStorageChecked = true;
                    smtp->_sdStorageReady = mbfs->sdReady();
                }
                else if (att->file.storage_type == esp_mail_file_storage_type_flash && !smtp->_flashStorageChecked && !smtp->_flashStorageReady)
                {
                    smtp->_flashStorageChecked = true;
                    smtp->_flashStorageReady = mbfs->flashReady();
                }

                if (!smtp->_flashStorageReady && !smtp->_sdStorageReady)
                {
                    sendStorageNotReadyError(smtp, att->file.storage_type);
                    continue;
                }

                if (openFileRead(smtp, msg, att, buf, boundary, false))
                {

                    if (!sendFile(smtp, msg, att))
                        return false;

                    if (!sendBDAT(smtp, msg, 2, false))
                        return false;

                    MB_String str = esp_mail_str_34;

                    if (!altSendData(str, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                        return false;
                }
            }
        }
    }
    return true;
}

void ESP_Mail_Client::altSendStorageErrorCB(SMTPSession *smtp, int err)
{
    if (smtp)
    {
        if (smtp->_sendCallback)
            esp_mail_debug_print(esp_mail_str_158, true);

        if (smtp->_debug)
        {
            smtp->_smtpStatus.statusCode = err;
            smtp->_smtpStatus.text.clear();

            MB_String e = esp_mail_str_185;
            e += smtp->errorReason().c_str();
            esp_mail_debug_print(e.c_str(), true);
        }
    }
    else if (imap && !calDataLen)
    {
#if defined(ENABLE_IMAP)
        if (imap->_readCallback)
            esp_mail_debug_print(esp_mail_str_158, true);

        if (imap->_debug)
        {
            imap->_imapStatus.statusCode = err;
            imap->_imapStatus.text.clear();

            MB_String e = esp_mail_str_185;
            e += imap->errorReason().c_str();
            esp_mail_debug_print(e.c_str(), true);
        }
#endif
    }
}

bool ESP_Mail_Client::openFileRead(SMTPSession *smtp, SMTP_Message *msg, SMTP_Attachment *att, MB_String &buf, const MB_String &boundary, bool inlined)
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
        altSendStorageErrorCB(smtp, sz);
    }
    else
    {

        buf.clear();

        if (inlined)
            getInlineHeader(buf, boundary, att, sz);
        else
            getAttachHeader(buf, boundary, att, sz);

        if (!sendBDAT(smtp, msg, buf.length(), false))
            return false;

        if (!altSendData(buf, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
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
        altSendStorageErrorCB(smtp, sz);
        return false;
    }

    return true;
}

void ESP_Mail_Client::sendStorageNotReadyError(SMTPSession *smtp, esp_mail_file_storage_type storageType)
{

#if defined(MBFS_FLASH_FS) || defined(MBFS_SD_FS)

    if (altIsCB(smtp))
    {
        if (storageType == esp_mail_file_storage_type_flash)
        {
            esp_mail_debug_print(esp_mail_str_348, true);
#if defined(ARDUINO_ARCH_RP2040)
            esp_mail_debug_print(esp_mail_str_415, true);
#endif
        }
        else if (storageType == esp_mail_file_storage_type_sd)
            esp_mail_debug_print(esp_mail_str_349, true);
    }

    if (altIsDebug(smtp))
    {
        MB_String e = esp_mail_str_185;
        if (storageType == esp_mail_file_storage_type_flash)
        {
            e += esp_mail_str_348;
            e += esp_mail_str_34;
#if defined(ARDUINO_ARCH_RP2040)
            e += esp_mail_str_185;
            e += esp_mail_str_415;
#endif
        }
        else if (storageType == esp_mail_file_storage_type_sd)
            e += esp_mail_str_349;
        esp_mail_debug_print(e.c_str(), true);
    }

#endif
}

bool ESP_Mail_Client::sendInline(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary, byte type)
{
    size_t num = numAtt(smtp, esp_mail_att_type_inline, msg) > 0;

    if (num > 0)
    {
        altSendCallback(smtp, esp_mail_str_167, esp_mail_str_271, true, false);
    }

    MB_String buf;
    MB_String related = getMIMEBoundary(15);
    int cnt = 0;
    SMTP_Attachment *att = nullptr;

    MB_String s = esp_mail_str_33;
    s += boundary;
    s += esp_mail_str_34;

    s += esp_mail_str_298;
    s += related;
    s += esp_mail_str_35;

    if (!sendBDAT(smtp, msg, s.length(), false))
        return false;

    if (!altSendData(s, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
        return false;

    if (!sendPartText(smtp, msg, type, related.c_str()))
        return false;

    bool cb = altIsCB(smtp);

    if (cb && numAtt(smtp, esp_mail_att_type_inline, msg) > 0)
        esp_mail_debug_print();

    if (num > 0)
    {
        for (uint8_t i = 0; i < msg->_att.size(); i++)
        {
            att = &msg->_att[i];
            if (att->_int.att_type == esp_mail_att_type_inline)
            {
                s = esp_mail_str_261;
                s += att->descr.filename;

                altSendCallback(smtp, att->descr.filename.c_str(), s.c_str(), cnt > 0, false);

                cnt++;

                if (att->file.storage_type == esp_mail_file_storage_type_none)
                {
                    if (!att->blob.data)
                        continue;

                    if (att->blob.size == 0)
                        continue;

                    altSendCallback(smtp, att->descr.filename.c_str(), s.c_str(), false, false);

                    buf.clear();
                    getInlineHeader(buf, related, att, att->blob.size);

                    if (!sendBDAT(smtp, msg, buf.length(), false))
                        return false;

                    if (!altSendData(buf, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                        return false;

                    if (!sendBlobAttachment(smtp, msg, att))
                        return false;

                    if (!sendBDAT(smtp, msg, 2, false))
                        return false;

                    MB_String str = esp_mail_str_34;

                    if (!altSendData(str, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                        return false;
                }
                else
                {

                    if (att->file.storage_type == esp_mail_file_storage_type_sd && !smtp->_sdStorageChecked && !smtp->_sdStorageReady)
                    {
                        smtp->_sdStorageChecked = true;
                        smtp->_sdStorageReady = mbfs->sdReady();
                    }
                    else if (att->file.storage_type == esp_mail_file_storage_type_flash && !smtp->_flashStorageChecked && !smtp->_flashStorageReady)
                    {
                        smtp->_flashStorageChecked = true;
                        smtp->_flashStorageReady = mbfs->flashReady();
                    }

                    if (!smtp->_flashStorageReady && !smtp->_sdStorageReady)
                    {
                        sendStorageNotReadyError(smtp, att->file.storage_type);
                        continue;
                    }

                    if (openFileRead(smtp, msg, att, buf, related, true))
                    {
                        if (!sendFile(smtp, msg, att))
                            return false;

                        if (!sendBDAT(smtp, msg, 2, false))
                            return false;

                        MB_String str = esp_mail_str_34;

                        if (!altSendData(str, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
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

    if (!sendBDAT(smtp, msg, s.length(), false))
        return false;

    if (!altSendData(s, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
        return false;

    return true;
}

void ESP_Mail_Client::errorStatusCB(SMTPSession *smtp, int error)
{
    if (smtp)
        smtp->_smtpStatus.statusCode = error;
    else if (imap && !calDataLen)
    {
#if defined(ENABLE_IMAP)
        imap->_imapStatus.statusCode = error;
#endif
    }

    MB_String s;

    if (smtp)
    {
        if (smtp->_sendCallback && !smtp->_customCmdResCallback)
        {
            s += esp_mail_str_53;
            s += smtp->errorReason().c_str();
            smtpCB(smtp, s.c_str(), false, false);
        }

        if (smtp->_debug && !smtp->_customCmdResCallback)
        {
            s = esp_mail_str_185;
            s += smtp->errorReason().c_str();
            esp_mail_debug_print(s.c_str(), true);
        }
    }
    else if (imap && !calDataLen)
    {
#if defined(ENABLE_IMAP)
        if (imap->_readCallback && !imap->_customCmdResCallback)
        {
            s += esp_mail_str_53;
            s += imap->errorReason().c_str();
            imapCB(imap, s.c_str(), false);
        }

        if (imap->_debug && !imap->_customCmdResCallback)
        {
            s = esp_mail_str_185;
            s += imap->errorReason().c_str();
            esp_mail_debug_print(s.c_str(), true);
        }
#endif
    }
}

size_t ESP_Mail_Client::smtpSend(SMTPSession *smtp, PGM_P data, bool newline)
{
    if (!smtp)
        return 0;

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

    MB_String s = data;

    int toSend = newline ? s.length() + 2 : s.length();

    if (!smtp->_customCmdResCallback && smtp->_debugLevel > esp_mail_debug_level_maintener)
        esp_mail_debug_print(s.c_str(), newline);

    sent = newline ? smtp->client.println(s.c_str()) : smtp->client.print(s.c_str());

    if (sent != toSend)
    {
        errorStatusCB(smtp, sent);
        sent = 0;
    }

    return sent;
}

size_t ESP_Mail_Client::smtpSend(SMTPSession *smtp, int data, bool newline)
{
    MB_String s = data;
    return smtpSend(smtp, s.c_str(), newline);
}

size_t ESP_Mail_Client::smtpSend(SMTPSession *smtp, uint8_t *data, size_t size)
{
    if (!smtp)
        return 0;

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

    if (smtp)
    {
        if (smtp->_tcpConnected)
            closeTCPSession(smtp);
    }
    else if (imap && !calDataLen)
    {
#if defined(ENABLE_IMAP)
        if (imap->_tcpConnected)
            closeTCPSession(imap);
#endif
    }

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
        if (!sendBDAT(smtp, msg, header.length(), false))
            return false;

        if (!altSendData(header, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
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

    if (!sendBDAT(smtp, msg, header.length(), false))
        return false;

    if (!altSendData(header, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
        return false;

    return true;
}

bool ESP_Mail_Client::sendBlobBody(SMTPSession *smtp, SMTP_Message *msg, uint8_t type)
{

    bool cb = altIsCB(smtp);
    uint32_t addr = altProgressPtr(smtp);

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

        esp_mail_smtp_send_base64_data_info_t data_info;

        data_info.flashMem = true;
        data_info.filename = s1.c_str();
        data_info.rawPtr = raw;
        data_info.size = len;

        return sendBase64(smtp, msg, data_info, true, cb);
    }

    int available = len;
    int sz = len;
    uint8_t *buf = (uint8_t *)newP(bufLen + 1);
    while (available)
    {
        if (available > bufLen)
            available = bufLen;

        memcpy_P(buf, raw + pos, available);

        if (!sendBDAT(smtp, msg, available, false))
        {
            ret = false;
            break;
        }

        if (!altSendData(buf, available, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
        {
            ret = false;
            break;
        }

        pos += available;
        len -= available;
        available = len;

        if (cb)
        {
            MB_String s1 = esp_mail_str_325;
            uploadReport(s1.c_str(), addr, 100 * pos / sz);
        }
    }
    delP(&buf);

    return ret;
}

bool ESP_Mail_Client::sendFileBody(SMTPSession *smtp, SMTP_Message *msg, uint8_t type)
{
    bool cb = altIsCB(smtp);
    uint32_t addr = altProgressPtr(smtp);

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

                esp_mail_smtp_send_base64_data_info_t data_info;

                data_info.filename = s1.c_str();
                data_info.storageType = msg->text.file.type;

                return sendBase64(smtp, msg, data_info, true, cb);
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

                if (!sendBDAT(smtp, msg, chunkSize, false))
                {
                    ret = false;
                    break;
                }

                if (!altSendData(buf, chunkSize, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                {
                    ret = false;
                    break;
                }

                if (cb)
                {
                    MB_String s1 = esp_mail_str_326;
                    uploadReport(s1.c_str(), addr, 100 * writeLen / fileSize);
                }

                writeLen += chunkSize;
            }
            delP(&buf);

            if (cb)
            {
                MB_String s1 = esp_mail_str_326;
                uploadReport(s1.c_str(), addr, 100);
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

                esp_mail_smtp_send_base64_data_info_t data_info;

                data_info.filename = s1.c_str();
                data_info.storageType = msg->html.file.type;

                return sendBase64(smtp, msg, data_info, true, cb);
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

                if (!sendBDAT(smtp, msg, chunkSize, false))
                {
                    ret = false;
                    break;
                }

                if (!altSendData(buf, chunkSize, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                {
                    ret = false;
                    break;
                }

                if (cb)
                {
                    MB_String s1 = esp_mail_str_326;
                    uploadReport(s1.c_str(), addr, 100 * writeLen / fileSize);
                }

                writeLen += chunkSize;
            }

            delP(&buf);
            if (cb)
            {
                MB_String s1 = esp_mail_str_326;
                uploadReport(s1.c_str(), addr, 100);
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

                s.replaceAll(fnd, rep);
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
            out[pos++] = *buf;
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
            out[pos++] = *buf;
            n++;
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
    splitToken(content, tokens, stk);
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
    splitToken(content, tokens, stk);
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

bool ESP_Mail_Client::altSendData(MB_String &s, bool newLine, SMTPSession *smtp, SMTP_Message *msg, bool addSendResult, bool getResponse, esp_mail_smtp_command cmd, esp_mail_smtp_status_code respCode, int errCode)
{
    if (!imap && smtp)
    {
        if (smtpSend(smtp, s.c_str(), newLine) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        {
            if (addSendResult)
                return addSendingResult(smtp, msg, false);
            else
                return false;
        }

        if (getResponse)
        {
            if (!handleSMTPResponse(smtp, cmd, respCode, errCode))
            {
                if (addSendResult)
                    return addSendingResult(smtp, msg, false);
                else
                    return false;
            }
        }
    }
    else if (imap)
    {
#if defined(ENABLE_IMAP)
        if (newLine)
            s += esp_mail_str_34;
        MB_StringPtr data = toStringPtr(s);

        if (calDataLen)
            dataLen += s.length();
        else
            return imap->mSendData(data, false, esp_mail_imap_cmd_append);
#endif
    }

    return true;
}

bool ESP_Mail_Client::altSendData(uint8_t *data, size_t size, SMTPSession *smtp, SMTP_Message *msg, bool addSendResult, bool getResponse, esp_mail_smtp_command cmd, esp_mail_smtp_status_code respCode, int errCode)
{
    if (!imap && smtp)
    {
        if (smtpSend(smtp, data, size) == ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED)
        {
            if (addSendResult)
                return addSendingResult(smtp, msg, false);
            else
                return false;
        }

        if (getResponse)
        {
            if (!handleSMTPResponse(smtp, cmd, respCode, errCode))
            {
                if (addSendResult)
                    return addSendingResult(smtp, msg, false);
                else
                    return false;
            }
        }
    }
    else if (imap)
    {
#if defined(ENABLE_IMAP)

        if (calDataLen)
            dataLen += size;
        else
            return imap->mSendData(data, size, false, esp_mail_imap_cmd_append);
#endif
    }

    return true;
}

bool ESP_Mail_Client::sendMSG(SMTPSession *smtp, SMTP_Message *msg, const MB_String &boundary)
{
    MB_String s;

    if (numAtt(smtp, esp_mail_att_type_inline, msg) > 0)
    {
        s += esp_mail_str_297;
        s += boundary;
        s += esp_mail_str_35;

        if (!sendBDAT(smtp, msg, s.length(), false))
            return false;

        if (!altSendData(s, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
            return false;

        if (msg->type == esp_mail_msg_type_plain || msg->type == esp_mail_msg_type_enriched || msg->type == esp_mail_msg_type_html)
        {
            if (!sendInline(smtp, msg, boundary, msg->type))
                return false;
        }
        else if (msg->type == (esp_mail_msg_type_html | esp_mail_msg_type_enriched | esp_mail_msg_type_plain))
        {
            if (!sendPartText(smtp, msg, esp_mail_msg_type_plain, boundary.c_str()))
                return false;
            if (!sendInline(smtp, msg, boundary, esp_mail_msg_type_html))
                return false;
        }

        s = esp_mail_str_33;
        s += boundary;
        s += esp_mail_str_33;
        s += esp_mail_str_34;

        if (!sendBDAT(smtp, msg, s.length(), false))
            return false;

        if (!altSendData(s, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
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

            s += esp_mail_str_297;
            s += boundary;
            s += esp_mail_str_35;

            if (!sendBDAT(smtp, msg, s.length(), false))
                return false;

            if (!altSendData(s, false, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                return false;

            if (!sendPartText(smtp, msg, esp_mail_msg_type_plain, boundary.c_str()))
                return false;

            if (!sendPartText(smtp, msg, esp_mail_msg_type_html, boundary.c_str()))
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

void ESP_Mail_Client::smtpCB(SMTPSession *smtp, PGM_P info, bool prependCRLF, bool success)
{
    if (smtp)
    {
        smtp->_cbData._info.clear();

        if (prependCRLF)
            smtp->_cbData._info = esp_mail_str_34;

        smtp->_cbData._info += info;
        smtp->_cbData._success = success;
        if (smtp->_sendCallback)
            smtp->_sendCallback(smtp->_cbData);
    }
}

uint32_t ESP_Mail_Client::altProgressPtr(SMTPSession *smtp)
{
    uint32_t addr = 0;
    if (smtp)
    {
        smtp->_lastProgress = -1;
        addr = toAddr(smtp->_lastProgress);
    }
    else if (imap && !calDataLen)
    {
#if defined(ENABLE_IMAP)
        imap->_lastProgress = -1;
        addr = toAddr(imap->_lastProgress);
#endif
    }

    return addr;
}

void ESP_Mail_Client::parseAuthCapability(SMTPSession *smtp, char *buf)
{
    if (!smtp)
        return;

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

// Check if response from basic client is actually TLS alert
// This response may return after basic Client sent plain text packet over SSL/TLS
void ESP_Mail_Client::checkTLSAlert(SMTPSession *smtp, const char *response)
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
    if (!smtp->client.tlsErr() && data0 == 0x15)
    {
        smtp->client.set_tlsErrr(true);

        int proto = smtp->client.getProtocol(smtp->_sesson_cfg->server.port);

        if (proto == (int)esp_mail_protocol_ssl || proto == (int)esp_mail_protocol_tls)
        {
            if (smtp->_debug)
                esp_mail_debug_print(esp_mail_str_204, true);
        }
    }
}

bool ESP_Mail_Client::handleSMTPResponse(SMTPSession *smtp, esp_mail_smtp_command cmd, esp_mail_smtp_status_code respCode, int errCode)
{
    if (!smtp)
        return false;

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
            if (cmd != esp_mail_smtp_cmd_logout)
                errorStatusCB(smtp, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);

            return false;
        }
        chunkBufSize = smtp->client.available();
        idle();
    }

    dataTime = millis();

    if (chunkBufSize > 1)
    {
        while (!completedResponse)
        {
            idle();

            if (!reconnect(smtp, dataTime))
                return false;

            if (!connected(smtp))
            {
                if (cmd != esp_mail_smtp_cmd_logout)
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

                readLen = readLine(&(smtp->client), response, chunkBufSize, false, count);

                if (readLen)
                {

                    checkTLSAlert(smtp, response);

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
                                idle();
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

                            if (!smtp->client.tlsErr() && !smtp->_customCmdResCallback && smtp->_debugLevel > esp_mail_debug_level_basic)
                                esp_mail_debug_print((const char *)response, true);
                        }

                        if (smtp->_smtp_cmd == esp_mail_smtp_command::esp_mail_smtp_cmd_greeting)
                            parseAuthCapability(smtp, response);
                    }

                    getResponseStatus(response, respCode, 0, status);

                    // No response code from greeting?
                    // Assumed multi-line greeting responses.

                    if (status.respCode == 0 && smtp->_smtp_cmd == esp_mail_smtp_command::esp_mail_smtp_cmd_initial_state)
                    {
                        s = esp_mail_str_260;
                        s += response;

                        if (!smtp->client.tlsErr() && smtp->_debug && !smtp->_customCmdResCallback)
                            esp_mail_debug_print(s.c_str(), true);

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
                        if (!smtp->client.tlsErr() && !smtp->_customCmdResCallback)
                            esp_mail_debug_print(s.c_str(), true);
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
    if (!smtp)
        return;

    if (smtp->_tcpConnected)
    {
        smtp->client.stop();
        _lastReconnectMillis = millis();
    }

    smtp->_tcpConnected = false;
    smtp->_auth_capability.clear();
    smtp->_send_capability.clear();
    smtp->_authenticated = false;
}

bool ESP_Mail_Client::reconnect(SMTPSession *smtp, unsigned long dataTime)
{
    if (!smtp)
        return false;

    smtp->client.setSession(smtp->_sesson_cfg);

    networkStatus = smtp->client.networkReady();

    if (dataTime > 0)
    {
        if (millis() - dataTime > (unsigned long)smtp->client.tcpTimeout())
        {
            closeTCPSession(smtp);
            errorStatusCB(smtp, MAIL_CLIENT_ERROR_READ_TIMEOUT);
            return false;
        }
    }

    if (!networkStatus)
    {
        if (smtp->_tcpConnected)
            closeTCPSession(smtp);

        errorStatusCB(smtp, MAIL_CLIENT_ERROR_CONNECTION_CLOSED);

        if (millis() - _lastReconnectMillis > _reconnectTimeout && !smtp->_tcpConnected)
        {
            if (smtp->_sesson_cfg->network_connection_handler)
            {
                // dummy
                smtp->client.disconnect();
                smtp->_sesson_cfg->network_connection_handler();
            }
            else
            {
                if (MailClient.networkAutoReconnect)
                    MailClient.resumeNetwork(&(smtp->client));
            }

            _lastReconnectMillis = millis();
        }

        networkStatus = smtp->client.networkReady();
    }

    return networkStatus;
}

void ESP_Mail_Client::uploadReport(const char *filename, uint32_t pgAddr, int progress)
{
    if (pgAddr == 0)
        return;

    int *lastProgress = addrTo<int *>(pgAddr);

    if (progress > 100)
        progress = 100;

    if (*lastProgress != progress && (progress == 0 || progress == 100 || *lastProgress + ESP_MAIL_PROGRESS_REPORT_STEP <= progress))
    {
        MB_String s = esp_mail_str_160;
        s += esp_mail_str_136;
        s += filename;
        s += esp_mail_str_136;
        s += esp_mail_str_91;
        s += progress;
        s += esp_mail_str_92;
        s += esp_mail_str_34;
        esp_mail_debug_print(s.c_str(), false);
        *lastProgress = progress;
    }
}

MB_String ESP_Mail_Client::getMIMEBoundary(size_t len)
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

int ESP_Mail_Client::chunkAvailable(SMTPSession *smtp, esp_mail_smtp_send_base64_data_info_t &data_info)
{
    if (!data_info.rawPtr)
    {
        int fileSize = mbfs->size(mbfs_type data_info.storageType);
        if (!fileSize)
        {
            errorStatusCB(smtp, MB_FS_ERROR_FILE_IO_ERROR);
            return -1;
        }

        return mbfs->available(mbfs_type data_info.storageType);
    }

    return data_info.size - data_info.dataIndex;
}

int ESP_Mail_Client::getChunk(SMTPSession *smtp, esp_mail_smtp_send_base64_data_info_t &data_info, unsigned char *rawChunk, bool base64)
{
    int available = chunkAvailable(smtp, data_info);

    if (available <= 0)
        return available;

    size_t size = base64 ? 3 : 4;

    if (data_info.dataIndex + size > data_info.size)
        size = data_info.size - data_info.dataIndex;

    if (!data_info.rawPtr)
    {

        int readLen = mbfs->read(mbfs_type data_info.storageType, rawChunk, size);

        if (readLen >= 0)
            data_info.dataIndex += readLen;

        return readLen;
    }

    if (data_info.flashMem)
        memcpy_P(rawChunk, data_info.rawPtr + data_info.dataIndex, size);
    else
        memcpy(rawChunk, data_info.rawPtr + data_info.dataIndex, size);

    data_info.dataIndex += size;

    return size;
}

void ESP_Mail_Client::closeChunk(esp_mail_smtp_send_base64_data_info_t &data_info)
{
    if (!data_info.rawPtr)
    {
        mbfs->close(mbfs_type data_info.storageType);
    }
}

bool ESP_Mail_Client::sendBase64(SMTPSession *smtp, SMTP_Message *msg, esp_mail_smtp_send_base64_data_info_t &data_info, bool base64, bool report)
{
    int size = chunkAvailable(smtp, data_info);

    if (size <= 0)
        return false;

    data_info.size = size;

    bool ret = false;

    uint32_t addr = altProgressPtr(smtp);

    size_t chunkSize = (BASE64_CHUNKED_LEN * UPLOAD_CHUNKS_NUM) + (2 * UPLOAD_CHUNKS_NUM);
    int bufIndex = 0;
    bool dataReady = false;
    int encodedCount = 0;
    int read = 0;

    if (!base64)
    {
        if (data_info.size < chunkSize)
            chunkSize = data_info.size;
    }

    uint8_t *buf = (uint8_t *)newP(chunkSize);
    memset(buf, 0, chunkSize);

    uint8_t *rawChunk = (uint8_t *)newP(base64 ? 3 : 4);

    if (report)
        uploadReport(data_info.filename, addr, data_info.dataIndex / data_info.size);

    int min = base64 ? 3 : 1;

    while (chunkAvailable(smtp, data_info))
    {

        if (chunkAvailable(smtp, data_info) >= min)
        {

            read = getChunk(smtp, data_info, rawChunk, base64);

            if (!read)
                goto ex;

            getBuffer(base64, buf, rawChunk, encodedCount, bufIndex, dataReady, read, chunkSize);

            if (dataReady)
            {

                if (!sendBDAT(smtp, msg, base64 ? bufIndex : bufIndex + 1, false))
                    goto ex;

                if (!altSendData(buf, base64 ? bufIndex : bufIndex + 1, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                    goto ex;

                memset(buf, 0, chunkSize);
                bufIndex = 0;
            }

            if (report)
                uploadReport(data_info.filename, addr, 100 * data_info.dataIndex / data_info.size);
        }
        else if (base64)
        {
            read = getChunk(smtp, data_info, rawChunk, base64);
            if (!read)
                goto ex;
        }
    }

    closeChunk(data_info);

    if (base64)
    {
        if (bufIndex > 0)
        {
            if (!sendBDAT(smtp, msg, bufIndex, false))
                goto ex;

            if (!altSendData(buf, bufIndex, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                goto ex;
        }

        if (read)
        {
            memset(buf, 0, chunkSize);
            bufIndex = 0;
            buf[bufIndex++] = b64_index_table[rawChunk[0] >> 2];
            if (read == 1)
            {
                buf[bufIndex++] = b64_index_table[(rawChunk[0] & 0x03) << 4];
                buf[bufIndex++] = '=';
            }
            else
            {
                buf[bufIndex++] = b64_index_table[((rawChunk[0] & 0x03) << 4) | (rawChunk[1] >> 4)];
                buf[bufIndex++] = b64_index_table[(rawChunk[1] & 0x0f) << 2];
            }
            buf[bufIndex++] = '=';

            if (!sendBDAT(smtp, msg, bufIndex, false))
                goto ex;

            if (!altSendData(buf, bufIndex, smtp, msg, false, false, esp_mail_smtp_cmd_undefined, esp_mail_smtp_status_code_0, SMTP_STATUS_UNDEFINED))
                goto ex;
        }
    }

    ret = true;

    if (report)
        uploadReport(data_info.filename, addr, 100);

ex:
    delP(&buf);
    delP(&rawChunk);

    if (!ret)
        closeChunk(data_info);

    return ret;
}

void ESP_Mail_Client::getBuffer(bool base64, uint8_t *out, uint8_t *in, int &encodedCount, int &bufIndex, bool &dataReady, int &size, size_t chunkSize)
{
    if (base64)
    {
        size = 0;
        out[bufIndex++] = b64_index_table[in[0] >> 2];
        out[bufIndex++] = b64_index_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        out[bufIndex++] = b64_index_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
        out[bufIndex++] = b64_index_table[in[2] & 0x3f];

        encodedCount += 4;

        if (encodedCount == BASE64_CHUNKED_LEN)
        {
            if (bufIndex + 1 < (int)chunkSize)
            {
                out[bufIndex++] = 0x0d;
                out[bufIndex++] = 0x0a;
            }
            encodedCount = 0;
        }

        dataReady = bufIndex >= (int)chunkSize - 4;
    }
    else
    {
        memcpy(out + bufIndex, in, size);
        bufIndex += size;

        if (bufIndex + 1 == BASE64_CHUNKED_LEN)
        {
            if (bufIndex + 2 < (int)chunkSize)
            {
                out[bufIndex++] = 0x0d;
                out[bufIndex++] = 0x0a;
            }
        }

        dataReady = bufIndex + 1 >= (int)chunkSize - size;
    }
}

MB_FS *ESP_Mail_Client::getMBFS()
{
    return mbfs;
}

SMTPSession::SMTPSession(Client *client, esp_mail_external_client_type type)
{
    setClient(client, type);
}

SMTPSession::SMTPSession()
{
}

SMTPSession::~SMTPSession()
{
    closeSession();
#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_RP2040)
    _caCert.reset();
    _caCert = nullptr;
#endif
}

bool SMTPSession::connect(ESP_Mail_Session *config)
{
    bool ssl = false;

    if (config)
        config->clearPorts();

    this->_customCmdResCallback = NULL;

    if (!handleConnection(config, ssl))
        return false;

    return MailClient.smtpAuth(this, ssl);
}

bool SMTPSession::isAuthenticated()
{
    return _authenticated;
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

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_RP2040)

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

    client.reset_tlsErr();

    MB_String s;

#if defined(ESP32) && defined(ESP32_TCP_CLIENT)
    client.setDebugCallback(NULL);
#elif defined(ESP8266) && defined(ESP8266_TCP_CLIENT)
    client.rxBufDivider = 16; // minimum rx buffer for smtp status response
    client.txBufDivider = 8;  // medium tx buffer for faster attachment/inline data transfer
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

        _sesson_cfg->ports_functions.list = new port_function[3];
        _sesson_cfg->ports_functions.size = 3;

        _sesson_cfg->ports_functions.list[0].port = 25;
        _sesson_cfg->ports_functions.list[0].protocol = esp_mail_protocol_plain_text;

        _sesson_cfg->ports_functions.list[1].port = 465;
        _sesson_cfg->ports_functions.list[1].protocol = esp_mail_protocol_ssl;

        _sesson_cfg->ports_functions.list[2].port = 587;
        _sesson_cfg->ports_functions.list[2].protocol = esp_mail_protocol_tls;
    }

    MailClient.getPortFunction(_sesson_cfg->server.port, _sesson_cfg->ports_functions, _secure, secureMode, ssl, _sesson_cfg->secure.startTLS);

    // Server connection attempt: no status code
    if (_sendCallback && !_customCmdResCallback)
        MailClient.smtpCB(this, esp_mail_str_120, false, false);

    if (_debug && !_customCmdResCallback)
    {
        s = esp_mail_str_314;
        s += ESP_MAIL_VERSION;
        s += client.fwVersion();
        esp_mail_debug_print(s.c_str(), true);

#if defined(BOARD_HAS_PSRAM) && defined(MB_STRING_USE_PSRAM)
        if (ESP.getPsramSize() == 0)
        {
            s = esp_mail_str_353;
            esp_mail_debug_print(s.c_str(), true);
        }
#endif
    }

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_SAMD) || defined(__AVR_ATmega4809__) || defined(ARDUINO_NANO_RP2040_CONNECT)
    bool validTime = false;
#endif

#if defined(ESP32_TCP_CLIENT) || defined(ESP8266_TCP_CLIENT)
    validTime = true; // strlen(_sesson_cfg->certificate.cert_file) > 0 || _caCert != nullptr;
#endif

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_SAMD) || defined(__AVR_ATmega4809__) || defined(ARDUINO_NANO_RP2040_CONNECT)

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
        esp_mail_debug_print(esp_mail_str_236, true);
        s = esp_mail_str_261;
        s += esp_mail_str_211;
        s += _sesson_cfg->server.host_name;
        esp_mail_debug_print(s.c_str(), true);
        s = esp_mail_str_261;
        s += esp_mail_str_201;
        s += _sesson_cfg->server.port;
        esp_mail_debug_print(s.c_str(), true);
    }

#if defined(ESP32) && defined(ESP32_TCP_CLIENT)
    if (_debug && !_customCmdResCallback)
        client.setDebugCallback(esp_mail_debug_print);
#endif

    client.begin(_sesson_cfg->server.host_name.c_str(), _sesson_cfg->server.port);

    client.ethDNSWorkAround();

    if (!client.connect(secureMode, _sesson_cfg->certificate.verify))
        return MailClient.handleSMTPError(this, SMTP_STATUS_SERVER_CONNECT_FAILED);

    // server connected
    _tcpConnected = true;

    if (_debug && !_customCmdResCallback)
        esp_mail_debug_print(esp_mail_str_238, true);

    if (_sendCallback && !_customCmdResCallback)
        MailClient.smtpCB(this, esp_mail_str_121, true, false);

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
        client.setDebugLevel(level);
    }
    else
    {
        _debugLevel = esp_mail_debug_level_none;
        _debug = false;
        client.setDebugLevel(0);
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
        MailClient.smtpCB(this, esp_mail_str_128, true, false);
    }

    if (_debug && !_customCmdResCallback)
        esp_mail_debug_print(esp_mail_str_245, true);

    bool ret = true;

    if (_authenticated)
    {

/* Sign out */
#if !defined(ESP8266)

        // QUIT command asks SMTP server to close the TCP session.
        // The connection may drop immediately.

        // There is memory leaks bug in ESP8266 BearSSLWiFiClientSecure class when the remote server
        // drops the connection.

        ret = MailClient.smtpSend(this, esp_mail_str_7, true) > 0;

        // This may return false due to connection drops before get any response.
        MailClient.handleSMTPResponse(this, esp_mail_smtp_cmd_logout, esp_mail_smtp_status_code_221, SMTP_STATUS_SEND_BODY_FAILED);
#endif

        if (ret)
        {
            if (_sendCallback && _sentSuccessCount > 0)
                MailClient.smtpCB(this, esp_mail_str_129, true, false);

            if (_debug && !_customCmdResCallback && _sentSuccessCount > 0)
                esp_mail_debug_print(esp_mail_str_246, true);

            if (_sendCallback)
                MailClient.smtpCB(this, "", false, true);
        }
    }

    return MailClient.handleSMTPError(this, 0, ret);
}

bool SMTPSession::connected()
{
    return client.connected();
}

void SMTPSession::callback(smtpStatusCallback smtpCallback)
{
    _sendCallback = smtpCallback;
}

void SMTPSession::setSystemTime(time_t ts)
{
    MailClient.Time.setTimestamp(ts);
}

void SMTPSession::setClient(Client *client, esp_mail_external_client_type type)
{
#if (defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) || defined(ESP_MAIL_USE_SDK_SSL_ENGINE)) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))

    this->client.setClient(client);
    this->client.setExtClientType(type);

#endif
}

void SMTPSession::connectionRequestCallback(ConnectionRequestCallback connectCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
    ESP_MAIL_PRINTF("> I: The Connection Request Callback is now optional.\n\n");
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

void SMTPSession::networkDisconnectionRequestCallback(NetworkDisconnectionRequestCallback networkDisconnectionCB)
{
#if defined(ESP_MAIL_ENABLE_CUSTOM_CLIENT) && (defined(ENABLE_IMAP) || defined(ENABLE_SMTP))
    this->client.networkDisconnectionRequestCallback(networkDisconnectionCB);
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
    MailClient.networkStatus = status;
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