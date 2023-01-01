/**
 * This example shows how to send custom IMAP command and get the response.
 *
 * Email: suwatchai@outlook.com
 *
 * Github: https://github.com/mobizt/ESP-Mail-Client
 *
 * Copyright (c) 2023 mobizt
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
#else

// Other Client defined here
// To use custom Client, define ENABLE_CUSTOM_CLIENT in  src/ESP_Mail_FS.h.
// See the example Custom_Client.ino for how to use.

#endif

#include <ESP_Mail_Client.h>

// To use only IMAP functions, you can exclude the SMTP from compilation, see ESP_Mail_FS.h.

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

void customCommandCallback(IMAP_Response res)
{
    // The server responses will included tagged and/or untagged data.

    // Tagged data is the status which begins with command identifier (tag) i.e. "A01" in this case.
    // Tagged status responses included OK, NO, BAD, PREAUTH and BYE.

    // Untagged data is the information or result of the request which begins with *

    // When you send multiple commands with different tag simultaneously,
    // tag will be used as command identifier.

    Serial.print("> C: TAG ");
    Serial.println(res.tag.c_str());
    Serial.print("< S: ");
    Serial.println(res.text.c_str());

    if (res.completed)
    {
        Serial.print("> C: Response finished with status ");
        Serial.println(res.status.c_str());
        Serial.println();
    }
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

    /*  Set the network reconnection option */
    MailClient.networkReconnect(true);

    /* Declare the ESP_Mail_Session for user defined session credentials */
    ESP_Mail_Session session;

    /* Set the session config */
    session.server.host_name = IMAP_HOST;
    session.server.port = IMAP_PORT;

    /* Connect to the server */
    if (!imap.customConnect(&session /* session credentials */, customCommandCallback, F("A01") /* tag */))
        return;

    String cmd = F("LOGIN ");
    cmd += AUTHOR_EMAIL;
    cmd += F(" ");
    cmd += AUTHOR_PASSWORD;

    // You can also assign tag to the begining of the command e.g. "A01 FETCH 1 UID"
    // Do not assign tag to command when you assign tag to the last parameter of function.

    imap.sendCustomCommand(cmd, customCommandCallback, F("A02") /* tag */);

    imap.sendCustomCommand(F("SELECT \"INBOX\""), customCommandCallback, F("A03") /* tag */);

    imap.sendCustomCommand(F("LIST \"\" *"), customCommandCallback, F("A04") /* tag */);

    imap.sendCustomCommand(F("FETCH 1 UID"), customCommandCallback, F("A05") /* tag */);
}

void loop()
{
}

