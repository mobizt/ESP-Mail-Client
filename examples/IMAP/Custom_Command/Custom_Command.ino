/**
 * This example shows how to send custom IMAP command and get the response.
 * 
 * Email: suwatchai@outlook.com
 * 
 * Github: https://github.com/mobizt/ESP-Mail-Client
 * 
 * Copyright (c) 2022 mobizt
 *
*/

/** For Gmail, IMAP option should be enabled. https://support.google.com/mail/answer/7126229?hl=en
 * and also https://accounts.google.com/b/0/DisplayUnlockCaptcha
*/

/** For ESP8266, with BearSSL WiFi Client 
 * The memory reserved for completed valid SSL response from IMAP is 16 kbytes which
 * may cause your device out of memory reset in case the memory 
 * allocation error.
*/

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else

//other Client defined here
//To use custom Client, define ENABLE_CUSTOM_CLIENT in  src/ESP_Mail_FS.h.
//See the example Custom_Client.ino for how to use.

#endif

#include <ESP_Mail_Client.h>

//To use only IMAP functions, you can exclude the SMTP from compilation, see ESP_Mail_FS.h.

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

/* The imap host name e.g. imap.gmail.com for GMail or outlook.office365.com for Outlook */
#define IMAP_HOST "<host>"

/** The imap port e.g. 
 * 143  or esp_mail_imap_port_143
 * 993 or esp_mail_imap_port_993
*/
#define IMAP_PORT 993

/* The log in credentials */
#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

/* Print the list of mailbox folders */
void printAllMailboxesInfo(IMAPSession &imap);

/* Print the selected folder info */
void printSelectedMailboxInfo(IMAPSession &imap);

/* The IMAP Session object used for Email reading */
IMAPSession imap;

void customCommandCallback(const char *res)
{
    // The server responses will included tagged and/or untagged data.

    // Tagged data is the status which begins with command identifier (tag) i.e. "A01" in this case.
    // Tagged status responses included OK, NO, BAD, PREAUTH and BYE.

    // Untagged data is the information or result of the request which begins with *

    // When you send multiple commands with different tag simultaneously, 
    // tag will be used as command identifier. 

    Serial.print("< S: ");
    Serial.println(res);
}

void setup()
{

    Serial.begin(115200);

#if defined(ARDUINO_ARCH_SAMD)
    while (!Serial)
        ;
    Serial.println();
    Serial.println("**** Custom built WiFiNINA firmware need to be installed.****\nTo install firmware, read the instruction here, https://github.com/mobizt/ESP-Mail-Client#install-custom-built-wifinina-firmware");

#endif

    Serial.println();

    Serial.print("Connecting to AP");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(200);
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    /** Enable the debug via Serial port
     * esp_mail_debug_level_0 or 0 for no debugging
     * esp_mail_debug_level_1 or 1 for basic level debugging
     * esp_mail_debug_level_2 or 2 for all level debugging
     *
     * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
    */
    imap.debug(esp_mail_debug_level_1);

    /* Declare the session config data */
    ESP_Mail_Session session;

    /* Set the session config */
    session.server.host_name = IMAP_HOST;
    session.server.port = IMAP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;

    /* Setup the configuration for searching or fetching operation and its result */
    IMAP_Config config;

    /* Connect to server with the session and config */
    if (!imap.connect(&session, &config))
        return;

    /*  {Optional} */
    printAllMailboxesInfo(imap);

    /* Open or select the mailbox folder to read or search the message */
    if (!imap.selectFolder(F("INBOX")))
        return;

    /* Send custom command to fetch message no.1 for UID */
    Serial.println("Send custom command to fetch message no.1 for UID");
    Serial.println("---------------------");

    // A01 is command identifier or tag which can be any string.
    // Any command that required the mailbox access,
    // the mailbox should be selected or opened before sending the command.
    // Please read the RFC 3501 and RFC 9051 documents for the details of IMAP protocol.

    imap.sendCustomCommand(F("A01 FETCH 1 UID"), customCommandCallback);
}

void loop()
{
}

void printAllMailboxesInfo(IMAPSession &imap)
{
    /* Declare the folder collection class to get the list of mailbox folders */
    FoldersCollection folders;

    /* Get the mailbox folders */
    if (imap.getFolders(folders))
    {
        for (size_t i = 0; i < folders.size(); i++)
        {
            /* Iterate each folder info using the  folder info item data */
            FolderInfo folderInfo = folders.info(i);
            ESP_MAIL_PRINTF("%s%s%s", i == 0 ? "\nAvailable folders: " : ", ", folderInfo.name, i == folders.size() - 1 ? "\n" : "");
        }
    }
}
