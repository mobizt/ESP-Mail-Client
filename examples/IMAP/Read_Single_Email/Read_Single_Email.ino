/**
 * This example will fetch or read the Email which the known message UID
 * was used for fetching.
 * 
 * Email: suwatchai@outlook.com
 * 
 * Github: https://github.com/mobizt/ESP-Mail-Client
 * 
 * Copyright (c) 2021 mobizt
 *
*/

/** To receive Email using Gmail, IMAP option should be enabled. https://support.google.com/mail/answer/7126229?hl=en
 * and also https://accounts.google.com/b/0/DisplayUnlockCaptcha
 * 
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
#endif
#include <ESP_Mail_Client.h>

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

/* Callback function to get the Email reading status */
void imapCallback(IMAP_Status status);

/* Print the list of mailbox folders */
void printAllMailboxesInfo(IMAPSession &imap);

/* Print the selected folder info */
void printSelectedMailboxInfo(SelectedFolderInfo sFolder);

/* Print all messages from the message list */
void printMessages(std::vector<IMAP_MSG_Item> &msgItems, bool headerOnly);

/* Print all attachments info from the message */
void printAttacements(std::vector<IMAP_Attach_Item> &atts);

/* The IMAP Session object used for Email reading */
IMAPSession imap;


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
     * none debug or 0
     * basic debug or 1
     * 
     * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
    */
    imap.debug(1);

    /* Set the callback function to get the reading results */
    imap.callback(imapCallback);

    /** In case the SD card/adapter was used for the file storagge, the SPI pins can be configure from
     * MailClient.sdBegin function which may be different for ESP32 and ESP8266
     * For ESP32, assign all of SPI pins
     * MailClient.sdBegin(14,2,15,13)
     * Which SCK = 14, MISO = 2, MOSI = 15 and SS = 13
     * And for ESP8266, assign the CS pins of SPI port
     * MailClient.sdBegin(15)
     * Which pin 15 is the CS pin of SD card adapter
    */

    /* Declare the session config data */
    ESP_Mail_Session session;

    /** ########################################################
     * Some properties of IMAP_Config and ESP_Mail_Session data
     * accept the pointer to constant char i.e. const char*. 
     * 
     * You may assign a string literal to that properties like 
     * below example.
     *   
     * config.storage.saved_path = String("/email_data").c_str();
     * 
     * String folder = "INBOX";
     * imap.selectFolder(folder.c_str());
     * 
     * ###########################################################
    */

    /* Set the session config */
    session.server.host_name = IMAP_HOST;
    session.server.port = IMAP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;


    /* Setup the configuration for searching or fetching operation and its result */
    IMAP_Config config;

    /* Set seen flag */
    //config.fetch.set_seen = true;

    /* Search criteria */
    config.search.criteria = "";

    /* Also search the unseen message */
    config.search.unseen_msg = true;

    /* Set the storage to save the downloaded files and attachments */
    config.storage.saved_path = "/email_data";

    /** The file storage type e.g.
     * esp_mail_file_storage_type_none,
     * esp_mail_file_storage_type_flash, and 
     * esp_mail_file_storage_type_sd 
    */
    config.storage.type = esp_mail_file_storage_type_flash;

    /** Set to download heades, text and html messaeges, 
     * attachments and inline images respectively.
    */
    config.download.header = true;
    config.download.text = true;
    config.download.html = true;
    config.download.attachment = true;
    config.download.inlineImg = true;

    /** Set to enable the results i.e. html and text messaeges 
     * which the content stored in the IMAPSession object is limited
     * by the option config.limit.msg_size.
     * The whole message can be download through config.download.text
     * or config.download.html which not depends on these enable options.
    */
    config.enable.html = true;
    config.enable.text = true;

    /* Set to enable the sort the result by message UID in the ascending order */
    config.enable.recent_sort = true;

    /* Set to report the download progress via the default serial port */
    config.enable.download_status = true;

    /* Header fields parsing is case insensitive by default to avoid uppercase header in some server e.g. iCloud
    , to allow case sensitive parse, uncomment below line*/
    //config.enable.header_case_sensitive = true;

    /* Set the limit of number of messages in the search results */
    config.limit.search = 5;

    /** Set the maximum size of message stored in 
     * IMAPSession object in byte
    */
    config.limit.msg_size = 512;

    /** Set the maximum attachments and inline images files size
     * that can be downloaded in byte. 
     * The file which its size is largger than this limit may be saved 
     * as truncated file.
    */
    config.limit.attachment_size = 1024 * 1024 * 5;


    /* Connect to server with the session and config */
    if (!imap.connect(&session, &config))
        return;

    /*  {Optional} */
    printAllMailboxesInfo(imap);

    /* Open or select the mailbox folder to read or search the message */
    if (!imap.selectFolder("INBOX"))
        return;

    /*  {Optional} */
    printSelectedMailboxInfo(imap.selectedFolder());

    /** Message UID to fetch or read e.g. 100. 
     * In this case we will get the UID from the max message number (lastest message) 
    */
    String uid = String(imap.getUID(imap.selectedFolder().msgCount()));
    config.fetch.uid = uid.c_str();

    /* Read or search the Email and close the session */

    //When message was fetched or read, the /Seen flag will not set or message remained in unseen or unread status,
    //as this is the purpose of library (not UI application), user can set the message status as read by set \Seen flag
    //to message, see the Set_Flags.ino example.
    MailClient.readMail(&imap);

    /* Clear all stored data in IMAPSession object */
    imap.empty();
}

void loop()
{

}

/* Callback function to get the Email reading status */
void imapCallback(IMAP_Status status)
{
    /* Print the current status */
    Serial.println(status.info());

    /* Show the result when reading finished */
    if (status.success())
    {
        /* Print the result */
        /* Get the message list from the message list data */
        IMAP_MSG_List msgList = imap.data();
        printMessages(msgList.msgItems, imap.headerOnly());

        /* Clear all stored data in IMAPSession object */
        imap.empty();
    }
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

void printSelectedMailboxInfo(SelectedFolderInfo sFolder)
{
    /* Show the mailbox info */
    ESP_MAIL_PRINTF("\nInfo of the selected folder\nTotal Messages: %d\n", sFolder.msgCount());
    ESP_MAIL_PRINTF("Predicted next UID: %d\n", sFolder.nextUID());
    for (size_t i = 0; i < sFolder.flagCount(); i++)
        ESP_MAIL_PRINTF("%s%s%s", i == 0 ? "Flags: " : ", ", sFolder.flag(i).c_str(), i == sFolder.flagCount() - 1 ? "\n" : "");
}

void printAttacements(std::vector<IMAP_Attach_Item> &atts)
{
    ESP_MAIL_PRINTF("Attachment: %d file(s)\n****************************\n", atts.size());
    for (size_t j = 0; j < atts.size(); j++)
    {
        IMAP_Attach_Item att = atts[j];
        /** att.type can be
         * esp_mail_att_type_none or 0
         * esp_mail_att_type_attachment or 1
         * esp_mail_att_type_inline or 2
        */
        ESP_MAIL_PRINTF("%d. Filename: %s, Name: %s, Size: %d, MIME: %s, Type: %s, Creation Date: %s\n", j + 1, att.filename, att.name, att.size, att.mime, att.type == esp_mail_att_type_attachment ? "attachment" : "inline", att.creationDate);
    }
    Serial.println();
}

void printMessages(std::vector<IMAP_MSG_Item> &msgItems, bool headerOnly)
{

    for (size_t i = 0; i < msgItems.size(); i++)
    {

        /* Iterate to get each message data through the message item data */
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
            ESP_MAIL_PRINTF("Date: %s\n", msg.date);
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

        /* If the result contains the message info (Fetch mode) */
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
                ESP_MAIL_PRINTF("RFC822 Messages: %d message(s)\n****************************\n", msg.rfc822.size());
                printMessages(msg.rfc822, headerOnly);
            }
        }

        Serial.println();
    }
}
