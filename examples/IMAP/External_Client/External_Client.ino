/**
 * This example shows how to read Email using WiFiSSLClient SSL client.
 *
 * This example used SAMD21 device and WiFiNINA as the client.
 *
 * ///////////////////////////////////////////////////////////////
 * Important Information when using the custom or external Client
 * ///////////////////////////////////////////////////////////////
 *
 * 1. The custom or external Client that works with network interface module
 * e.g. WiFiClient, EthernetClient and GSMClient can be used with this library.
 *
 * To let the library knows that external client was used, user needs to define
 * "ENABLE_CUSTOM_CLIENT" macro in file "src/ESP_Mail_FS.h" or
 * "src/Custom_ESP_Mail_FS.h" like the following
 *
 * #define ENABLE_CUSTOM_CLIENT
 *
 * 2. The two or three callback functions required to allow library to
 * check the network status and resume or reconnect the network interface module
 * and upgrade the connection mode (do SSL/TLS handshake) from non-secure connection
 * to secure connection.
 *
 * The purposes of callback functions are following
 *
 * 2.1 networkConnectionRequestCallback function is for resume or reconnect the network.
 *
 * User needs to place the code to reset or re-connect the network here.
 *
 * User can assign the callback via
 *
 * smtp.networkConnectionRequestCallback(<callback function>);
 * imap.networkConnectionRequestCallback(<callback function>);
 *
 * 2.2 networkStatusRequestCallback function is for network status acknowledgement.
 *
 * User needs to place the code to set back the network status via method smtp.setNetworkStatus(<bool>);
 * and imap.setNetworkStatus(<bool>);
 *
 * User can assign the callback via
 *
 * smtp.networkStatusRequestCallback(<callback function>);
 * imap.networkStatusRequestCallback(<callback function>);
 *
 * 2.3 connectionUpgradeRequestCallback function is for SSL/TLS handshake to upgrade the server connection
 * from pain text mode (non-secure) to SSL mode. This callback function will be called when STARTTLS
 * command request was accepted by server.
 *
 * User needs to place the code that tells the external Client to do SSL/TLS handshake.
 *
 * User can assign the callback via
 *
 * smtp.connectionUpgradeRequestCallback(<callback function>);
 * imap.connectionUpgradeRequestCallback(<callback function>);
 *
 * The SSL clients that can connect in both non-secure and secure modes are
 *
 * Mobizt's ESP_SSLClient library https://github.com/mobizt/ESP_SSLClient. This library supports ESP8266, ESP32
 * and Raspberry Pi Pico.
 *
 * OPEnSLab's SSLClient fork version library https://github.com/mobizt/SSLClient. This library supports all microcontrollers
 * that have enough flash memory and ram except for ESP8266 that has stack overfow issue.
 *
 * With above two SSL client libraries, the SSL/TLS hanshake can be done via
 *
 * #if defined(SSLCLIENT_CONNECTION_UPGRADABLE)
 * ssl_client.connectSSL(SMTP_HOST, SMTP_PORT);
 * #endif
 *
 * The mcro "SSLCLIENT_CONNECTION_UPGRADABLE" was defined in the those two SSL client libraries.
 *
 * 2.4 connectionRequestCallback is the function for starting server connection which is currently not required.
 *
 * User can assign the callback via
 *
 * smtp.connectionRequestCallback(<callback function>);
 * imap.connectionRequestCallback(<callback function>);
 *
 * The callback function assigned should accept the const char* for hosname and integer for port ,
 * user needs to place the code using external client to connect to the server as the following.
 *
 * void connectionRequestCallback(const char *host, int port)
 * {
 *  basic_or_ssl_client.connect(host, port);
 * }
 *
 * 3. SSL client can be adssigned to the IMAPSession and SMTPSession objects during class constructor
 *
 * SMTPSession smtp(&basic_client, esp_mail_external_client_type_basic);
 * IMAPSession imap(&ssl_client, esp_mail_external_client_type_ssl)
 *
 * Or can be assign through the method
 *
 * smtp.setClient(&basic_client, esp_mail_external_client_type_basic);
 * imap.setClient(&ssl_client, esp_mail_external_client_type_ssl);
 *
 * The first argument of class constructor and method is the pointer to Arduino client class.
 * The second argument of class constructor and method is the esp_mail_external_client_type enum
 * i.e., esp_mail_external_client_type_basic and esp_mail_external_client_type_ssl
 *
 * If the first argument is basic client or non-secure client which works directly with network module
 * e.g. WiFiClient, EthernetClient and GSMClient, the second argument should be
 * esp_mail_external_client_type_basic.
 *
 * This kind of client can be used for IMAP and SMTP transports via non-secure ports e.g., 25 (SMTP) and 143 (IMAP).
 *
 * If the first argument is ssl client that starting connection in secure mode e.g., WiFiClientSecure and
 * WiFiSSLClient, the second argument should be esp_mail_external_client_type_ssl.
 *
 * This kind of client can be used for IMAP and SMTP transports via secure ports e.g., 465 (SMTP) and 993 (IMAP).
 *
 * If the first argument is ssl client that can starting connection in non-secure mode as Mobizt's ESP_SSLClient and
 * OPEnSLab's SSLClient fork version, the second argument should be esp_mail_external_client_type_basic.
 *
 * This kind of client can be used for IMAP and SMTP transports via Plain/TLS via STARTTLS ports
 * e.g., 25 (SMTP), 25 (587), and 143 (IMAP).
 *
 * With this type of client, the callback function connectionUpgradeRequestCallback is required to perform SSL/TLS handshake.
 *
 *
 * When using ESP8266, ESP32 and Raspberry Pi Pico devices with network modules with network interface module, the external SSL client library is not required
 * because internal SSL engine is used.
 *
 * User can use the basic client of network module directly and choose esp_mail_external_client_type_basic for the second argument of the method.
 *
 *
 * 4. When using external client to do some tasks that required valid time e.g., sending Email and SSL certificate validation, the external
 * UDP client is required for internal NTP time synching.
 *
 * User can assign the UDP client via
 *
 * MailClient.setUDPClient(&udp_client, 0);
 *
 * Which the second argument is the GMT offset. This GMT offset will be used to set the time offset instead of GMT offset set from the session object
 * config.time.gmt_offset.
 *
 * IN ESP8266 and ESP32, device time will be updated after synching fishished and can get via time(nullptr).
 *
 * In Raspberry Pi Pico and other Arduino devices, the device time is not available. The valid time can be obtained from
 *
 * uint32_t timestamp = MailClient.Time.getCurrentTimestamp();
 *
 * If the valid time can be obtaoned from RTC chip or other sources, the library internal time can be changed or set with
 *
 * MailClient.Time.setTimestamp(timestamp);
 *
 * In Raspberry Pi Pico, the device time can not set manually by user except for using SDK's NTP class that works with WiFi only.
 * When use this device with external client, the device time will not be changed and user should call
 * MailClient.Time.getCurrentTimestamp() to get the valid time instead.
 *
 *
 * ///////////////////////////////////////////////////////////////
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

/** ////////////////////////////////////////////////
 *  Struct data names changed from v2.x.x to v3.x.x
 *  ////////////////////////////////////////////////
 *
 * "ESP_Mail_Session" changes to "Session_Config"
 * "IMAP_Config" changes to "IMAP_Data"
 *
 * Changes in the examples
 *
 * ESP_Mail_Session session;
 * to
 * Session_Config config;
 *
 * IMAP_Config config;
 * to
 * IMAP_Data imap_data;
 *
 */

#include <Arduino.h>

#if defined(ARDUINO_ARCH_SAMD)
#include <WiFiNINA.h>
#endif

#include <ESP_Mail_Client.h>

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

#define IMAP_HOST "<host>"
#define IMAP_PORT 993

#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

void imapCallback(IMAP_Status status);

void printAllMailboxesInfo(IMAPSession &imap);

void printSelectedMailboxInfo(SelectedFolderInfo sFolder);

void printMessages(MB_VECTOR<IMAP_MSG_Item> &msgItems, bool headerOnly);

void printAttacements(MB_VECTOR<IMAP_Attach_Item> &atts);

WiFiSSLClient ssl_client;
WiFiUDP udp_client;

IMAPSession imap(&ssl_client, esp_mail_external_client_type_ssl);

void networkConnection()
{
    // Reset the network connection
    WiFi.disconnect();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
        if (millis() - ms >= 5000)
        {
            Serial.println(" failed!");
            return;
        }
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

void networkStatusRequestCallback()
{
    imap.setNetworkStatus(WiFi.status() == WL_CONNECTED);
}

void setup()
{

    Serial.begin(115200);

#if defined(ARDUINO_ARCH_SAMD)
    while (!Serial)
        ;
#endif

    Serial.println();

    MailClient.networkReconnect(true);

    // The WiFi credentials are required for SAMD21
    // due to it does not have reconnect feature.
    MailClient.clearAP();
    MailClient.addAP(WIFI_SSID, WIFI_PASSWORD);

    networkConnection();

    imap.debug(1);

    /*
    For internal NTP client
    For times east of the Prime Meridian use 0-12
    For times west of the Prime Meridian add 12 to the offset.
    Ex. American/Denver GMT would be -6. 6 + 12 = 18
    See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
    */
    MailClient.setUDPClient(&udp_client, 0 /* GMT offset */);

    MailClient.networkReconnect(true);

    imap.callback(imapCallback);

    Session_Config config;

    config.server.host_name = IMAP_HOST;
    config.server.port = IMAP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;

    IMAP_Data imap_data;

    imap_data.search.criteria.clear();

    imap_data.search.unseen_msg = true;

    imap_data.storage.saved_path = F("/email_data");

    imap_data.storage.type = esp_mail_file_storage_type_flash;

    imap_data.download.header = true;
    imap_data.download.text = true;
    imap_data.download.html = true;
    imap_data.download.attachment = true;
    imap_data.download.inlineImg = true;

    imap_data.enable.html = true;
    imap_data.enable.text = true;

    imap_data.enable.recent_sort = true;

    imap_data.enable.download_status = true;

    imap_data.limit.search = 5;

    imap_data.limit.msg_size = 512;

    imap_data.limit.attachment_size = 1024 * 1024 * 5;

    imap.networkConnectionRequestCallback(networkConnection);

    imap.networkStatusRequestCallback(networkStatusRequestCallback);

    if (!imap.connect(&config, &imap_data))
        return;

    if (imap.isAuthenticated())
        Serial.println("\nSuccessfully logged in.");
    else
        Serial.println("\nConnected with no Auth.");

    printAllMailboxesInfo(imap);

    if (!imap.selectFolder(F("INBOX")))
        return;

    printSelectedMailboxInfo(imap.selectedFolder());

    imap_data.fetch.uid = imap.getUID(imap.selectedFolder().msgCount());

    MailClient.readMail(&imap);

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
        IMAP_MSG_List msgList = imap.data();
        printMessages(msgList.msgItems, imap.headerOnly());
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
    ESP_MAIL_PRINTF("Unseen Message Index: %d\n", sFolder.unseenIndex());
    if (sFolder.modSeqSupported())
        ESP_MAIL_PRINTF("Highest Modification Sequence: %d\n", sFolder.highestModSeq());
    for (size_t i = 0; i < sFolder.flagCount(); i++)
        ESP_MAIL_PRINTF("%s%s%s", i == 0 ? "Flags: " : ", ", sFolder.flag(i).c_str(), i == sFolder.flagCount() - 1 ? "\n" : "");

    if (sFolder.flagCount(true))
    {
        for (size_t i = 0; i < sFolder.flagCount(true); i++)
            ESP_MAIL_PRINTF("%s%s%s", i == 0 ? "Permanent Flags: " : ", ", sFolder.flag(i, true).c_str(), i == sFolder.flagCount(true) - 1 ? "\n" : "");
    }
}

void printAttacements(MB_VECTOR<IMAP_Attach_Item> &atts)
{
    ESP_MAIL_PRINTF("Attachment: %d file(s)\n****************************\n", atts.size());
    for (size_t j = 0; j < atts.size(); j++)
    {
        IMAP_Attach_Item att = atts[j];
        ESP_MAIL_PRINTF("%d. Filename: %s, Name: %s, Size: %d, MIME: %s, Type: %s, Description: %s, Creation Date: %s\n", j + 1, att.filename, att.name, att.size, att.mime, att.type == esp_mail_att_type_attachment ? "attachment" : "inline", att.description, att.creationDate);
    }
    Serial.println();
}

void printMessages(MB_VECTOR<IMAP_MSG_Item> &msgItems, bool headerOnly)
{

    for (size_t i = 0; i < msgItems.size(); i++)
    {
        IMAP_MSG_Item msg = msgItems[i];

        Serial.println("****************************");
        ESP_MAIL_PRINTF("Number: %d\n", msg.msgNo);
        ESP_MAIL_PRINTF("UID: %d\n", msg.UID);
        ESP_MAIL_PRINTF("Messsage-ID: %s\n", msg.ID);

        ESP_MAIL_PRINTF("Flags: %s\n", msg.flags);

        ESP_MAIL_PRINTF("Attachment: %s\n", msg.hasAttachment ? "yes" : "no");

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

        if (!headerOnly)
        {
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

            if (msg.rfc822.size() > 0)
            {
                ESP_MAIL_PRINTF("\r\nRFC822 Messages: %d message(s)\n****************************\n", msg.rfc822.size());
                printMessages(msg.rfc822, headerOnly);
            }

            if (msg.attachments.size() > 0)
                printAttacements(msg.attachments);
        }

        Serial.println();
    }
}
