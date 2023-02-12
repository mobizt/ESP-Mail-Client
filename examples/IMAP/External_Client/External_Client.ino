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
 * and upgrade the connection mode from non-secure to secure connection or perform
 * SSL/TLS handshake.
 * 
 * The purposes of callback functions are following
 * 2.1 networkConnectionRequestCallback is for resume or reconnect the network.
 * User needs to place the code to reset or disconnect and re-connect the network here.
 * The methods that used to assign the callback are
 * 
 * smtp.networkConnectionRequestCallback(<callback function>);
 * imap.networkConnectionRequestCallback(<callback function>);
 * 
 * 2.2 networkStatusRequestCallback is for library to check the network status.
 * User needs to place the code to set back the network status via method smtp.setNetworkStatus(<bool>);
 * and imap.setNetworkStatus(<bool>);
 * 
 * The methods that used to assign the callback are
 * 
 * smtp.networkStatusRequestCallback(<callback function>);
 * imap.networkStatusRequestCallback(<callback function>);
 * 
 * 2.3 connectionUpgradeRequestCallback is for upgrade the server connection from pain text mode (non-secure) 
 * to SSL mode. This callback function should perform SSL/TLS handshake which will be required when the STARTTLS
 * command request was accepted.
 * User needs to place the code that tells the external Client to perform SSL/TLS handshake.
 * 
 * The methods that used to assign the callback are
 * 
 * smtp.connectionUpgradeRequestCallback(<callback function>);
 * imap.connectionUpgradeRequestCallback(<callback function>);
 * 
 * The external Client used for this purpose should be able to work as basic Arduino client that starting connection
 * to server in non-secure mode and able to perform SSL/TLS handshake to upgrade the connection when required.
 * 
 * The current SSL clients that have this ability are
 * 
 * Mobizt's ESP_SSLClient library https://github.com/mobizt/ESP_SSLClient. This library supports ESP8266, ESP32 
 * and Raspberry Pi Pico.
 * 
 * OPEnSLab's SSLClient fork version library https://github.com/mobizt/SSLClient. This library support all microcontrollers
 * that have enough flash memory and ram except for ESP8266 that has stack overfow issue.
 * 
 * With the above two SSL client libraries, the SSL/TLS hanshake can be done with the following code
 * 
 * #if defined(SSLCLIENT_CONNECTION_UPGRADABLE)
 * ssl_client.connectSSL(SMTP_HOST, SMTP_PORT);
 * #endif
 * 
 * The mcro "SSLCLIENT_CONNECTION_UPGRADABLE" was defined in the those two SSL client libraries.
 * 
 * 2.4 connectionRequestCallback is the function for server connection which is set as optional and is not required.
 * When it assigned, user need to place the code to connect to the server with host name and port as following code.
 * 
 * basic_client.connect(host, port); or ssl_client.connect(host, port);
 * 
 * The methods that used to assign the callback are
 * 
 * smtp.connectionRequestCallback(<callback function>);
 * imap.connectionRequestCallback(<callback function>);
 * 
 * 3. SSL client can be adssign to the IMAPSession or SMTPSession object during class constructor like following
 * 
 * SMTPSession smtp(&basic_client, esp_mail_external_client_type_basic);
 * IMAPSession imap(&ssl_client, esp_mail_external_client_type_ssl)
 * 
 * Or can be assign through the method
 * 
 * smtp.setClient(&basic_client, esp_mail_external_client_type_basic);
 * imap.setClient(&ssl_client, esp_mail_external_client_type_ssl);
 * 
 * The first argument of class constructor and method is the Arduino client derived class.
 * The second argument of class constructor and method is the esp_mail_external_client_type enum
 * i.e., esp_mail_external_client_type_basic and esp_mail_external_client_type_ssl
 * 
 * If the firstargument is basic client or non-secure client that works directly with network module 
 * e.g. WiFiClient, EthernetClient and GSMClient, the second argument should be 
 * esp_mail_external_client_type_basic.
 * 
 * This kind of client can use for IMAP and SMTP transports via non-secure ports e.g., 25 (SMTP), 143 (IMAP).
 * 
 * If the firstargument is ssl client that cannot connect in non-secure mode (works as a wrapper class of basic client)
 * e.g., WiFiClientSecure and WiFiSSLClient, the second argument should be esp_mail_external_client_type_ssl.
 * 
 * This kind of client can use for IMAP and SMTP transports via secure ports e.g., 465 (SMTP), 993 (IMAP).
 * 
 * If the firstargument is ssl client that can connect in non-secure mode as Mobizt's ESP_SSLClient and 
 * OPEnSLab's SSLClient fork version, the second argument should be esp_mail_external_client_type_basic.
 * 
 * This kind of client can use for IMAP and SMTP transports via Plain/TLS via STARTTLS ports 
 * e.g., 25 (SMTP), 25 (587), 143 (IMAP).
 * Which the callback function connectionUpgradeRequestCallback in the topic 2.3 is required.
 * 
 * 
 * When using ESP8266, ESP32 and Raspberry Pi Pico with network modules, user does not need to assign the SSL client
 * to the SMTPSession and IMAPSession object or external SSL client library is not required.
 * If ENABLE_CUSTOM_CLIENT mocro was defined in in file "src/ESP_Mail_FS.h" or "src/Custom_ESP_Mail_FS.h", user can assign
 * the basic client of network module as first argument and esp_mail_external_client_type_basic for the second argument.
 * 
 * In above case, library will use the internal SDK SSL engine for SSL/TLS handshake process when working with SSL ports e.g., 465 (SMTP),
 * 993 (IMAP) and Plain/TLS via STARTTLS ports e.g., 25 (SMTP), 25 (587), 143 (IMAP).
 * 
 * 4. Some tasks required valid time e.g., sending Email and SSL certificate validation.
 * Normally when using the WiFi or Ethernet that supported natively on ESP8266, ESP32 and Raspberry Pi Pico (WiFi), 
 * the system (device) time was setup internally by acquiring the NTP server response for synching which is not need any
 * time or UDP client.
 * 
 * For external client usage, the external UDP client is required when valid time is needed because library does not know how and
 * where to get the valid time from.
 * 
 * The UDP clients e.g. WiFiUDP and EthernetUDP required for internal time synching with NTP server which only GMT offset can be assigned.
 * 
 * The external UDP client can be assign to library from
 * 
 * MailClient.setUDPClient(&udp_client, 0);
 * 
 * Which the second argument is the GMT offset. This GMT offset will be used to set the time instead of GMT offset from the session object
 * session.time.gmt_offset.
 * 
 * With external Client usage, the device time will be updated in ESP8266 and ESP32 which user can get it from time(nullptr).
 * While Raspberry Pi Pico and other Arduino devices, the device time is not available. If user need to get the valid time
 * from library, the internal time will be provided from
 * 
 * uint32_t timestamp = MailClient.Time.getCurrentTimestamp();
 * 
 * If user can get the valid time from RTC or any source, the internal time of library (also ESP8266 and ESP32's system time)
 * can be set from
 * 
 * MailClient.Time.setTimestamp(timestamp);
 * 
 * In Raspberry Pi Pico, the device time that can get from time(nullptr) can not set manually except for WiFi using SDK's NTP class.
 * Then when external client was used, the device time will not update by library's internal time synching and when calling 
 * MailClient.Time.setTimestamp. User should call MailClient.Time.getCurrentTimestamp to get the valid time instead.
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

    // For internal NTP client
    MailClient.setUDPClient(&udp_client, 0 /* GMT offset */);

    MailClient.networkReconnect(true);

    imap.callback(imapCallback);

    ESP_Mail_Session session;

    session.server.host_name = IMAP_HOST;
    session.server.port = IMAP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;

    IMAP_Config config;

    config.search.criteria.clear();

    config.search.unseen_msg = true;

    config.storage.saved_path = F("/email_data");

    config.storage.type = esp_mail_file_storage_type_flash;

    config.download.header = true;
    config.download.text = true;
    config.download.html = true;
    config.download.attachment = true;
    config.download.inlineImg = true;

    config.enable.html = true;
    config.enable.text = true;

    config.enable.recent_sort = true;

    config.enable.download_status = true;

    config.limit.search = 5;

    config.limit.msg_size = 512;

    config.limit.attachment_size = 1024 * 1024 * 5;

    imap.networkConnectionRequestCallback(networkConnection);

    imap.networkStatusRequestCallback(networkStatusRequestCallback);

    if (!imap.connect(&session, &config))
        return;

    printAllMailboxesInfo(imap);

    if (!imap.selectFolder(F("INBOX")))
        return;

    printSelectedMailboxInfo(imap.selectedFolder());

    config.fetch.uid = imap.getUID(imap.selectedFolder().msgCount());

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
    ESP_MAIL_PRINTF("\nInfo of the selected folder\nTotal Messages: %d\n", sFolder.msgCount());
    ESP_MAIL_PRINTF("Predicted next UID: %d\n", sFolder.nextUID());
    ESP_MAIL_PRINTF("Unseen Message Index: %d\n", sFolder.unseenIndex());
    for (size_t i = 0; i < sFolder.flagCount(); i++)
        ESP_MAIL_PRINTF("%s%s%s", i == 0 ? "Flags: " : ", ", sFolder.flag(i).c_str(), i == sFolder.flagCount() - 1 ? "\n" : "");
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
