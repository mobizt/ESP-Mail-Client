/**
 * This example shows how to search the unread messages and read them.
 *
 * To get the incoming mail notification, see Mailbox_Changes_Notification.ino.
 *
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: suwatchai@outlook.com
 *
 * Github: https://github.com/mobizt/ESP-Mail-Client
 *
 * Copyright (c) 2023 mobizt
 *
 */

/**
 * To use library in silent mode (no debug printing and callback), please define this macro in src/ESP_Mail_FS.h.
 * #define SILENT_MODE
 */

#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#endif

#include <ESP_Mail_Client.h>

#include <extras/SDHelper.h>

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

#define IMAP_HOST "<host>"

#define IMAP_PORT 993

#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

void imapCallback(IMAP_Status status);

void printAllMailboxesInfo(IMAPSession &imap);

void printSelectedMailboxInfo(SelectedFolderInfo sFolder);

void printMessageData();

IMAPSession imap;

Session_Config config;

IMAP_Data imap_data;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

// Max messages in the search result
int max_result = 5;

// Array to store the UID of messages in search result
int msg_uid[5];

void setup()
{

    Serial.begin(115200);

#if defined(ARDUINO_ARCH_SAMD)
    while (!Serial)
        ;
    Serial.println();
    Serial.println("**** Custom built WiFiNINA firmware need to be installed.****\n");
    Serial.println("To install firmware, read the instruction here, https://github.com/mobizt/ESP-Mail-Client#install-custom-build-wifinina-firmware");
#endif

    Serial.println();

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    multi.addAP(WIFI_SSID, WIFI_PASSWORD);
    multi.run();
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

    Serial.print("Connecting to Wi-Fi");

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    unsigned long ms = millis();
#endif

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
        if (millis() - ms > 10000)
            break;
#endif
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    MailClient.networkReconnect(true);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    MailClient.clearAP();
    MailClient.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

    /** Enable the debug via Serial port
     * 0 for no debugging
     * 1 for basic level debugging
     *
     * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
     */
    imap.debug(1);

    /* Set the callback function to get the reading results */
    imap.callback(imapCallback);

    config.server.host_name = IMAP_HOST;
    config.server.port = IMAP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;

    // Clear all these fetch options to perform search
    imap_data.fetch.uid.clear();
    imap_data.fetch.number.clear();
    imap_data.fetch.sequence_set.string.clear();

    imap_data.search.unseen_msg = true;

    // Don't download all to filesystem
    imap_data.download.header = false;
    imap_data.download.text = false;
    imap_data.download.html = false;
    imap_data.download.attachment = false;
    imap_data.download.inlineImg = false;

    // Store html/text message body in IMAPSession object
    imap_data.enable.html = true;
    imap_data.enable.text = true;

    imap_data.enable.recent_sort = true;

    // Max messages in the search result
    imap_data.limit.search = max_result;

    imap_data.limit.msg_size = 128;

    if (!imap.connect(&config, &imap_data))
    {
        ESP_MAIL_PRINTF("Connection error, Error Code: %d, Reason: %s", imap.errorCode(), imap.errorReason().c_str());
        return;
    }

    if (imap.isAuthenticated())
        Serial.println("\nSuccessfully logged in.");
    else
        Serial.println("\nConnected with no Auth.");

    printAllMailboxesInfo(imap);

    if (!imap.selectFolder(F("INBOX")))
    {
        ESP_MAIL_PRINTF("Folder selection error, Error Code: %d, Reason: %s", imap.errorCode(), imap.errorReason().c_str());
        return;
    }

    printSelectedMailboxInfo(imap.selectedFolder());

    // We search the unseen messages first to get its UID and stored in msg_uid.
    imap_data.search.criteria = F("SEARCH UNSEEN");

    MailClient.readMail(&imap, false /* keep session open for fetching message in opened mailbox later */);

    // We already get the search result message, fetch it

    // Fetch the messages using UID stored in msg_uid one by one
    for (int i = 0; i < max_result; i++)
    {
        imap_data.search.criteria.clear();

        // Mark this message as read
        MailClient.addFlag(&imap, msg_uid[i], F("\\Seen"), false /* Close session */, false /* Ignore response */);

        // Now Fech message by UID stored in msg_uid
        imap_data.fetch.uid = msg_uid[i];

        MailClient.readMail(&imap, false /* keep session open for fetching message in opened mailbox later */);
    }

    imap.closeSession();
    imap.empty();
}

void loop()
{
}

void imapCallback(IMAP_Status status)
{
    Serial.println(status.info());

    if (status.success())
    {
        // If this is the search result (imap contains only header info),
        // store the message UID that we can fetch for its body later.
        if (imap.headerOnly())
        {
            max_result = imap.data().msgItems.size();
            for (size_t i = 0; i < imap.data().msgItems.size(); i++)
                msg_uid[i] = imap.data().msgItems[i].UID;
        }
        else
        {
            // This is the fetch result, print the whole message (header + body)
            printMessageData();
        }

        imap.empty();
    }
}

void printAllMailboxesInfo(IMAPSession &imap)
{

    FoldersCollection folders;

    if (imap.getFolders(folders))
    {
        for (size_t i = 0; i < folders.size(); i++)
        {
            FolderInfo folderInfo = folders.info(i);
            ESP_MAIL_PRINTF("%s%s%s", i == 0 ? "\nAvailable folders: " : ", ", folderInfo.name, i == folders.size() - 1 ? "\n" : "");
        }
    }
}

void printSelectedMailboxInfo(SelectedFolderInfo sFolder)
{
    /* Show the mailbox info */
    ESP_MAIL_PRINTF("\nInfo of the selected folder\nTotal Messages: %d\n", sFolder.msgCount());
    ESP_MAIL_PRINTF("UID Validity: %d\n", sFolder.uidValidity());
    ESP_MAIL_PRINTF("Predicted next UID: %d\n", sFolder.nextUID());
    if (sFolder.unseenIndex() > 0)
        ESP_MAIL_PRINTF("First Unseen Message Number: %d\n", sFolder.unseenIndex());
    else
        ESP_MAIL_PRINTF("Unseen Messages: No\n");

    if (sFolder.modSeqSupported())
        ESP_MAIL_PRINTF("Highest Modification Sequence: %llu\n", sFolder.highestModSeq());
    for (size_t i = 0; i < sFolder.flagCount(); i++)
        ESP_MAIL_PRINTF("%s%s%s", i == 0 ? "Flags: " : ", ", sFolder.flag(i).c_str(), i == sFolder.flagCount() - 1 ? "\n" : "");

    if (sFolder.flagCount(true))
    {
        for (size_t i = 0; i < sFolder.flagCount(true); i++)
            ESP_MAIL_PRINTF("%s%s%s", i == 0 ? "Permanent Flags: " : ", ", sFolder.flag(i, true).c_str(), i == sFolder.flagCount(true) - 1 ? "\n" : "");
    }
}

void printMessageData()
{

    IMAP_MSG_Item msg = imap.data().msgItems[0]; // msgItems contains only one message from fetch

    Serial.println("****************************");

    ESP_MAIL_PRINTF("Number: %d\n", msg.msgNo);
    ESP_MAIL_PRINTF("UID: %d\n", msg.UID);

    // The attachment status in search may be true in case the "multipart/mixed"
    // content type header was set with no real attachtment included.
    ESP_MAIL_PRINTF("Attachment: %s\n", msg.hasAttachment ? "yes" : "no");

    ESP_MAIL_PRINTF("Messsage-ID: %s\n", msg.ID);

    if (strlen(msg.flags))
        ESP_MAIL_PRINTF("Flags: %s\n", msg.flags);
    if (strlen(msg.acceptLang))
        ESP_MAIL_PRINTF("Accept Language: %s\n", msg.acceptLang);
    if (strlen(msg.contentLang))
        ESP_MAIL_PRINTF("Content Language: %s\n", msg.contentLang);
    if (strlen(msg.from))
        ESP_MAIL_PRINTF("From: %s\n", msg.from);
    if (strlen(msg.sender))
        ESP_MAIL_PRINTF("Sender: %s\n", msg.sender);
    if (strlen(msg.to))
        ESP_MAIL_PRINTF("To: %s\n", msg.to);
    if (strlen(msg.cc))
        ESP_MAIL_PRINTF("CC: %s\n", msg.cc);
    if (strlen(msg.date))
    {
        ESP_MAIL_PRINTF("Date: %s\n", msg.date);
        ESP_MAIL_PRINTF("Timestamp: %d\n", (int)MailClient.Time.getTimestamp(msg.date));
    }
    if (strlen(msg.subject))
        ESP_MAIL_PRINTF("Subject: %s\n", msg.subject);
    if (strlen(msg.reply_to))
        ESP_MAIL_PRINTF("Reply-To: %s\n", msg.reply_to);
    if (strlen(msg.return_path))
        ESP_MAIL_PRINTF("Return-Path: %s\n", msg.return_path);
    if (strlen(msg.in_reply_to))
        ESP_MAIL_PRINTF("In-Reply-To: %s\n", msg.in_reply_to);
    if (strlen(msg.references))
        ESP_MAIL_PRINTF("References: %s\n", msg.references);
    if (strlen(msg.comments))
        ESP_MAIL_PRINTF("Comments: %s\n", msg.comments);
    if (strlen(msg.keywords))
        ESP_MAIL_PRINTF("Keywords: %s\n", msg.keywords);

    if (strlen(msg.text.content))
        ESP_MAIL_PRINTF("Text Message: %s\n", msg.text.content);
    if (strlen(msg.text.charSet))
        ESP_MAIL_PRINTF("Text Message Charset: %s\n", msg.text.charSet);
    if (strlen(msg.text.transfer_encoding))
        ESP_MAIL_PRINTF("Text Message Transfer Encoding: %s\n", msg.text.transfer_encoding);
    if (strlen(msg.html.content))
        ESP_MAIL_PRINTF("HTML Message: %s\n", msg.html.content);
    if (strlen(msg.html.charSet))
        ESP_MAIL_PRINTF("HTML Message Charset: %s\n", msg.html.charSet);
    if (strlen(msg.html.transfer_encoding))
        ESP_MAIL_PRINTF("HTML Message Transfer Encoding: %s\n\n", msg.html.transfer_encoding);

    Serial.println();
}