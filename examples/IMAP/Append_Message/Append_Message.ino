/**
 * This example shows how to append message to mailbox.
 *
 * Email: suwatchai@outlook.com
 *
 * Github: https://github.com/mobizt/ESP-Mail-Client
 *
 * Copyright (c) 2022 mobizt
 *
 */

/** Assign SD card type and FS used in src/ESP_Mail_FS.h and
 * change the config for that card interfaces in src/extras/SDHelper.h
 */

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else

// Other Client defined here
// To use custom Client, define ENABLE_CUSTOM_CLIENT in  src/ESP_Mail_FS.h.
// See the example Custom_Client.ino for how to use.

#endif

#include <data.h>

#include <ESP_Mail_Client.h>

// required IMAP and SMTP
#if defined(ENABLE_IMAP) && defined(ENABLE_SMTP)

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

/** For Gmail, IMAP option should be enabled. https://support.google.com/mail/answer/7126229?hl=en
 * and also https://accounts.google.com/b/0/DisplayUnlockCaptcha
 *
 * Some Gmail user still not able to sign in using account password even above options were set up,
 * for this case, use "App Password" to sign in instead.
 * About Gmail "App Password", go to https://support.google.com/accounts/answer/185833?hl=en
 *
 * For Yahoo mail, log in to your yahoo mail in web browser and generate app password by go to
 * https://login.yahoo.com/account/security/app-passwords/add/confirm?src=noSrc
 *
 * To use Gmai and Yahoo's App Password to sign in, define the AUTHOR_PASSWORD with your App Password
 * and AUTHOR_EMAIL with your account email.
 */

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

/* Declare the global used IMAPSession object for IMAP transport */
IMAPSession imap;

/* Callback function to get the Email reading status */
void imapCallback(IMAP_Status status)
{
    /* Print the current status */
    Serial.println(status.info());
}

#endif

void setup()
{

    Serial.begin(115200);

// required IMAP and SMTP
#if defined(ENABLE_IMAP) && defined(ENABLE_SMTP)

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

    /*  Set the network reconnection option */
    MailClient.networkReconnect(true);

    /** Enable the debug via Serial port
     * 0 for no debugging
     * 1 for basic level debugging
     *
     * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
     */
    imap.debug(1);

    /* Set the callback function to get the reading results */
    imap.callback(imapCallback);

    /* Declare the ESP_Mail_Session for user defined session credentials */
    ESP_Mail_Session session;

    /* Set the session config */
    session.server.host_name = IMAP_HOST;
    session.server.port = IMAP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;

    /** Declare the IMAP_Config object used for user defined IMAP operating options
     * and contains the IMAP operating result
     */
    IMAP_Config config;

    ESP_Mail_Message message[2]; // The same usage as SMTP_Message

    message[0].sender.name = "Fred Foobar";
    message[0].sender.email = "foobar@Blurdybloop.example.COM";
    message[0].subject = "afternoon meeting";
    message[0].addRecipient("Joe Mooch", "mooch@owatagu.example.net");
    message[0].text.content = "Hello Joe, do you think we can meet at 3:30 tomorrow?";
    message[0].text.charSet = "us-ascii";
    message[0].text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    ESP_Mail_Attachment att[2]; // The same usage as SMTP_Attachment

    att[0].descr.filename = "shaun.png";
    att[0].descr.mime = "image/png";
    att[0].blob.data = shaun_png;
    att[0].blob.size = sizeof(shaun_png);
    att[0].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
    message[0].addAttachment(att[0]);

    message[1].sender.name = "Joe Mooch";
    message[1].sender.email = "mooch@OWaTaGu.example.net";
    message[1].subject = "Re: afternoon meeting";
    message[1].addRecipient("Fred Foobar", "foobar@blurdybloop.example.com");
    message[1].text.content = "3:30 is fine with me.";
    message[1].text.charSet = "us-ascii";
    message[1].text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    att[1].descr.filename = "mu_law.wav";
    att[1].descr.mime = "audio/basic";
    att[1].blob.data = mu_law_wave;
    att[1].blob.size = sizeof(mu_law_wave);
    att[1].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

    message[1].addAttachment(att[1]);

    /* Connect to the server */
    if (!imap.connect(&session /* session credentials */, &config /* operating options and its result */))
        return;

    if (!imap.selectFolder(F("INBOX")))
        return;

    // If MULTIAPPEND extension is supported, the multiple messages will send by a single APPEND command.
    // If not, one message can append for a APPEND command.
    // Outlook.com does not accept flag and date/time arguments in APPEND command
    if (!MailClient.appendMessage(&imap, &message[0], false /* if not last message to append */, "\\Flagged" /* flags or empty string for Outlook.com */, "Thu, 16 Jun 2022 12:30:25 -0800 (PST)" /* date/time or empty string for Outlook.com */))
        Serial.println("Error appending message, " + imap.errorReason());

    if (!MailClient.appendMessage(&imap, &message[1], true /* last message to append */))
        Serial.println("Error appending message, " + imap.errorReason());

    ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());

#endif
}

void loop()
{
}
