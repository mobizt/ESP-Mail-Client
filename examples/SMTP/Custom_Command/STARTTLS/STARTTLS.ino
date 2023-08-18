

/**
 * This example showes how to send Email using custom commands.
 *
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: suwatchai@outlook.com
 *
 * Github: https://github.com/mobizt/ESP-Mail-Client
 *
 * Copyright (c) 2022 mobizt
 *
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

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

/** For Gmail, the app password will be used for log in
 *  Check out https://github.com/mobizt/ESP-Mail-Client#gmail-smtp-and-imap-required-app-passwords-to-sign-in
 *
 * For Yahoo mail, log in to your yahoo mail in web browser and generate app password by go to
 * https://login.yahoo.com/account/security/app-passwords/add/confirm?src=noSrc
 *
 * To use Gmai and Yahoo's App Password to sign in, define the AUTHOR_PASSWORD with your App Password
 * and AUTHOR_EMAIL with your account email.
 */

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "<host>"

/** The smtp port e.g.
 * 25  or esp_mail_smtp_port_25
 * 465 or esp_mail_smtp_port_465 // port 465 is not available for Outlook.com
 * 587 or esp_mail_smtp_port_587
 */
#define SMTP_PORT esp_mail_smtp_port_587 // for STARTTLS

/* The log in credentials */
#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;

const char rootCACert[] PROGMEM = "-----BEGIN CERTIFICATE-----\n"
                                  "-----END CERTIFICATE-----\n";

void customCommandCallback(SMTP_Response res)
{

    // The res.id is the command identifier number that use to identify the source of command.

    // The command identifier number can be set via the last parameter of customConnect and sendCustomCommand functions.

    // If command identifier number was not set in those functions, the res.id received will be auto increased and begins with 0

    Serial.print("> C: Command ID ");
    Serial.println(res.id);

    Serial.print("< S: ");
    Serial.println(res.text.c_str());

    if (res.respCode > 0)
    {
        Serial.print("> C: Response finished with code ");
        Serial.println(res.respCode);
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
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;

    bool tls = false;

    /* Connect to the server */
    if (smtp.customConnect(&session /* session credentials */, customCommandCallback) != 220)
    {
        Serial.println("> E: Unable to connect to server");
        return;
    }

init:

    if (smtp.sendCustomCommand(F("EHLO mydomain.net"), customCommandCallback) != 250)
    {
        smtp.closeSession();
        return;
    }

    // Only for SMTP port 587 in supported server that accepts STARTTLS

    if (!tls)
    {
        if (smtp.sendCustomCommand(F("STARTTLS"), customCommandCallback) != 220)
        {
            smtp.closeSession();
            return;
        }

        tls = true;

        // Send greeting again
        goto init;
    }

    if (smtp.sendCustomCommand(F("AUTH LOGIN"), customCommandCallback) != 334)
    {
        smtp.closeSession();
        return;
    }

    if (smtp.sendCustomCommand(MailClient.toBase64(session.login.email), customCommandCallback) != 334)
    {
        smtp.closeSession();
        return;
    }

    if (smtp.sendCustomCommand(MailClient.toBase64(session.login.password), customCommandCallback) != 235)
    {
        smtp.closeSession();
        return;
    }

    // Please don't forget to change sender@xxxxxx.com to your email
    if (smtp.sendCustomCommand(F("MAIL FROM:<sender@xxxxxx.com>"), customCommandCallback) != 250)
    {
        smtp.closeSession();
        return;
    }

    // Please don't forget to change recipient@xxxxx.com with your recipient email
    if (smtp.sendCustomCommand(F("RCPT TO:<recipient@xxxxx.com>"), customCommandCallback) != 250)
    {
        smtp.closeSession();
        return;
    }

    if (smtp.sendCustomCommand(F("DATA"), customCommandCallback) != 354)
    {
        smtp.closeSession();
        return;
    }

    if (!smtp.sendCustomData(F("Subject: Test sending Email\r\n")))
    {
        smtp.closeSession();
        return;
    }

    if (!smtp.sendCustomData(F("Hello World!\r\n")))
    {
        smtp.closeSession();
        return;
    }

    if (smtp.sendCustomCommand(F("."), customCommandCallback) != 250)
    {
        smtp.closeSession();
        return;
    }

    // Do not use this command in ESP8266 due to memory leaks in ESP8266 core BearSSL.
    // smtp.sendCustomCommand(F("QUIT"), customCommandCallback);

    smtp.closeSession();

    // to clear sending result log
    // smtp.sendingResult.clear();

    ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
}

void loop()
{
}
