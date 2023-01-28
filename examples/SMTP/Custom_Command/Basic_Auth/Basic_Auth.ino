

/**
 * This example showes how to send Email using custom commands.
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
#if defined(ESP32) || defined(PICO_RP2040)
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
 * 465 or esp_mail_smtp_port_465
 * 587 or esp_mail_smtp_port_587 // See STARTTLS.ino example
 */
#define SMTP_PORT esp_mail_smtp_port_465 // port 465 is not available for Outlook.com

/* The log in credentials */
#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;

const char rootCACert[] PROGMEM = "-----BEGIN CERTIFICATE-----\n"
                                  "-----END CERTIFICATE-----\n";

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

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

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    multi.addAP(WIFI_SSID, WIFI_PASSWORD);
    multi.run();
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
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

    /*  Set the network reconnection option */
    MailClient.networkReconnect(true);

    // The WiFi credentials are required for Pico W
    // due to it does not have reconnect feature.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    MailClient.clearAP();
    MailClient.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

    /* Declare the ESP_Mail_Session for user defined session credentials */
    ESP_Mail_Session session;

    /* Set the session config */
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;

    /** Assign your host name or you public IPv4 or IPv6 only
     * as this is the part of EHLO/HELO command to identify the client system
     * to prevent connection rejection.
     * If host name or public IP is not available, ignore this or
     * use generic host "mydomain.net".
     *
     * Assign any text to this option may cause the connection rejection.
     */
    session.login.user_domain = F("mydomain.net");

    /* Connect to the server */
    if (!smtp.connect(&session /* session credentials */))
        return;

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

    // Send data with command which terminated with dot '.'
    if (smtp.sendCustomCommand(F("Subject: Test sending Email\r\nHello World!\r\n."), customCommandCallback) != 250)
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
