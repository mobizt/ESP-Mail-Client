

/**
 * This example showed how to send a reply message when specific email was received.
 * 
 * Created by K. Suwatchai (Mobizt)
 * 
 * Email: suwatchai@outlook.com
 * 
 * Github: https://github.com/mobizt/ESP-Mail-Client
 * 
 * Copyright (c) 2021 mobizt
 *
*/

//The account 2 will send Hello message to account 1.

//The account 1 will poll the mailbox for incoming message, when new message received with matched subject 
//and sent from account 1, the account 1 will send a reply messsage to account 2.


//To use send Email for Gmail to port 465 (SSL), less secure app option should be enabled. https://myaccount.google.com/lesssecureapps?pli=1

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
#define IMAP_HOST "<imap host for account 1>"
#define IMAP_PORT 993

#define IMAP_AUTHOR_EMAIL "<email for account 1>"
#define IMAP_AUTHOR_PASSWORD "<password for account 1>"

#define REPLY_SMTP_AUTHOR_EMAIL "<email for account 1>" 
#define REPLY_SMTP_AUTHOR_PASSWORD "<password for account 1>"

/** The smtp port e.g. 
 * 25  or esp_mail_smtp_port_25
 * 465 or esp_mail_smtp_port_465
 * 587 or esp_mail_smtp_port_587
*/
#define REPLY_SMTP_PORT 587
#define REPLY_SMTP_HOST "<smtp host for account 1>"




#define HELLO_SMTP_AUTHOR_EMAIL "<email for account 2>"
#define HELLO_SMTP_AUTHOR_PASSWORD "<password for account 2>"

#define HELLO_SMTP_PORT 587
#define HELLO_SMTP_HOST "<smtp host for account 2>"

void setupIMAP();

bool setupHelloSMTP();

bool setupReplySMTP();

void sendHelloMessage();

void sendReplyMessage(const char *subject, const char *reply_email, const char *msgID, const char *references);

/* Print the selected folder update info */
void printPollingStatus(IMAPSession &imap);

/* Callback function to get the Email reading status */
void imapCallback(IMAP_Status status);

/* Callback function to get the Email sending status */
void helloSMTPCallback(SMTP_Status status);

void replySMTPCallback(SMTP_Status status);

/* The IMAP Session object used for Email reading */
IMAPSession imap;

/* Declare the imap mail session config data */
ESP_Mail_Session imap_mail_app_session;

/* Setup the configuration for searching or fetching operation and its result */
IMAP_Config imap_config;

/* The SMTP Session object used for Email sending */
SMTPSession hello_smtp;
SMTPSession reply_smtp;

/* Declare the smtp mail session config data */
ESP_Mail_Session hello_smtp_mail_app_session;
ESP_Mail_Session reply_smtp_mail_app_session;

bool imapSetupOk = false;

unsigned long helloSendingMillis = 0;

String sendingSubject = "ESP Mail Hello Test!";

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

    Serial.print("Setup and connect to IMAP server... ");

    setupIMAP();

    if (!imapSetupOk)
        return;
}

void loop()
{

    /* imap.connect and imap.selectFolder or imap.openFolder nedded to be called once prior to listen */

    //Listen for mailbox changes
    if (!imap.listen())
        return;

    //Check the changes
    if (imap.folderChanged())
        printPollingStatus(imap);

    //To stop listen, use imap.stopListen(); and to listen again, call imap.listen()

    if (millis() - helloSendingMillis > 5 * 60 * 1000 || helloSendingMillis == 0)
    {
        helloSendingMillis = millis();
        Serial.print("Sending Hello message... ");
        sendHelloMessage();
    }
}

void setupIMAP()
{
    imap.debug(1);

    /* Set the callback function to get the reading results */
    imap.callback(imapCallback);

    /* Set the imap app config */
    imap_mail_app_session.server.host_name = IMAP_HOST;
    imap_mail_app_session.server.port = IMAP_PORT;
    imap_mail_app_session.login.email = IMAP_AUTHOR_EMAIL;
    imap_mail_app_session.login.password = IMAP_AUTHOR_PASSWORD;

    /* Connect to server with the session and config */
    if (!imap.connect(&imap_mail_app_session, &imap_config))
        return;

    /* Open or select the mailbox folder to read or search the message */
    if (!imap.selectFolder("INBOX"))
        return;

    imapSetupOk = true;
}

bool setupHelloSMTP()
{
    hello_smtp.debug(1);

    /* Set the callback function to get the sending results */
    hello_smtp.callback(helloSMTPCallback);

    /* Set the session config */
    hello_smtp_mail_app_session.server.host_name = HELLO_SMTP_HOST;
    hello_smtp_mail_app_session.server.port = HELLO_SMTP_PORT;
    hello_smtp_mail_app_session.login.email = HELLO_SMTP_AUTHOR_EMAIL;
    hello_smtp_mail_app_session.login.password = HELLO_SMTP_AUTHOR_PASSWORD;
    hello_smtp_mail_app_session.login.user_domain = "mydomain.net";

    /* Connect to server with the session config */
    if (!hello_smtp.connect(&hello_smtp_mail_app_session))
        return false;

    return true;
}

bool setupReplySMTP()
{
    reply_smtp.debug(1);

    /* Set the callback function to get the sending results */
    reply_smtp.callback(replySMTPCallback);

    /* Set the session config */
    reply_smtp_mail_app_session.server.host_name = REPLY_SMTP_HOST;
    reply_smtp_mail_app_session.server.port = REPLY_SMTP_PORT;
    reply_smtp_mail_app_session.login.email = REPLY_SMTP_AUTHOR_EMAIL;
    reply_smtp_mail_app_session.login.password = REPLY_SMTP_AUTHOR_PASSWORD;
    reply_smtp_mail_app_session.login.user_domain = "mydomain.net";

    /* Connect to server with the session config */
    if (!reply_smtp.connect(&reply_smtp_mail_app_session))
        return false;

    return true;
}

void sendHelloMessage()
{

    if (!setupHelloSMTP())
        return;

    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = "ESP Mail";
    message.sender.email = HELLO_SMTP_AUTHOR_EMAIL;
    message.subject = sendingSubject.c_str();
    message.addRecipient("Me", IMAP_AUTHOR_EMAIL);
    message.response.reply_to = HELLO_SMTP_AUTHOR_EMAIL; //only email address, excluded < and >
    message.text.content = "Hello Me!";

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&hello_smtp, &message))
        Serial.println("Error sending Email, " + hello_smtp.errorReason());
}

void sendReplyMessage(const char *subject, const char *reply_email, const char *msgID, const char *references)
{

    if (!setupReplySMTP())
        return;

    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = "ESP Mail";
    message.sender.email = REPLY_SMTP_AUTHOR_EMAIL;
    String reSubject = "RE: ";
    reSubject += subject;
    message.subject = reSubject.c_str();
    message.addRecipient("Me", reply_email);

    message.in_reply_to = msgID;

    String ref = references;
    if (strlen(references) > 0)
        ref += " ";
    ref += msgID;

    message.references = ref.c_str();
    message.text.content = "Yeah!, it works.";

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&reply_smtp, &message))
        Serial.println("Error sending Email, " + reply_smtp.errorReason());
}

void printPollingStatus(IMAPSession &imap)
{
    /* Declare the selected folder info class to get the info of selected mailbox folder */
    SelectedFolderInfo sFolder = imap.selectedFolder();

    if (sFolder.pollingStatus().type == imap_polling_status_type_new_message)
    {
        /* Show the mailbox info */
        ESP_MAIL_PRINTF("\nMailbox status changed\n----------------------\nTotal Messages: %d\n", sFolder.msgCount());

        ESP_MAIL_PRINTF("New message %d, has been addedd, reading message...\n", (int)sFolder.pollingStatus().messageNum);

        //we need to stop polling before do anything
        imap.stopListen();

        //Get the UID of new message and fetch
        String uid = String(imap.getUID(sFolder.pollingStatus().messageNum));
        imap_config.fetch.uid = uid.c_str();

        //When message was fetched or read, the /Seen flag will not set or message remained in unseen or unread status,
        //as this is the purpose of library (not UI application), user can set the message status as read by set \Seen flag
        //to message, see the Set_Flags.ino example.
        MailClient.readMail(&imap, false);
    }
}

/* Callback function to get the Email reading status */
void imapCallback(IMAP_Status status)
{
    /* Print the current status */
    Serial.println(status.info());

    /* Show the result when reading finished */
    if (status.success())
    {

        /* Get the message list from the message list data */
        IMAP_MSG_List msgList = imap.data();

        if (strcmp(msgList.msgItems[0].subject, sendingSubject.c_str()) == 0)
        {
            Serial.print("Sending Reply message... ");
            std::string replyEmail = msgList.msgItems[0].reply_to;

            //remove < at the beginning and > at the end.
            replyEmail.erase(0, 1);
            replyEmail.pop_back();

            sendReplyMessage(msgList.msgItems[0].subject, replyEmail.c_str(), msgList.msgItems[0].ID, msgList.msgItems[0].references);
        }

        /* Clear all stored data in IMAPSession object */
        imap.empty();
    }
}

/* Callback function to get the Email sending status */
void helloSMTPCallback(SMTP_Status status)
{
    /* Print the current status */
    Serial.println(status.info());

    /* Print the sending result */
    if (status.success())
    {
        Serial.println("----------------");
        ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
        ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
        Serial.println("----------------\n");
        struct tm dt;

        for (size_t i = 0; i < hello_smtp.sendingResult.size(); i++)
        {
            /* Get the result item */
            SMTP_Result result = hello_smtp.sendingResult.getItem(i);
            time_t ts = (time_t)result.timestamp;
            localtime_r(&ts, &dt);

            ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
            ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
            ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
        }
        Serial.println("----------------\n");

        //You need to clear sending result as the memory usage will grow up as it keeps the status, timstamp and
        //pointer to const char of recipients and subject that user assigned to the SMTP_Message object.

        //Because of pointer to const char that stores instead of dynamic string, the subject and recipients value can be
        //a garbage string (pointer points to undefind location) as SMTP_Message was declared as local variable or the value changed.

        //hello_smtp.sendingResult.clear();
    }
}

void replySMTPCallback(SMTP_Status status)
{
    /* Print the current status */
    Serial.println(status.info());

    /* Print the sending result */
    if (status.success())
    {
        Serial.println("----------------");
        ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
        ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
        Serial.println("----------------\n");
        struct tm dt;

        for (size_t i = 0; i < reply_smtp.sendingResult.size(); i++)
        {
            /* Get the result item */
            SMTP_Result result = reply_smtp.sendingResult.getItem(i);
            time_t ts = (time_t)result.timestamp;
            localtime_r(&ts, &dt);

            ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
            ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
            ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
        }
        Serial.println("----------------\n");

        //reply_smtp.sendingResult.clear();
    }
}
