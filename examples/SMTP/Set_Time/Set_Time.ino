

/**
 * This example showes how to set the library and/or device time manually.
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
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else

#endif

#include <ESP_Mail_Client.h>

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

#define SMTP_HOST "<host>"
#define SMTP_PORT esp_mail_smtp_port_587

#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"
#define RECIPIENT_EMAIL "<recipient email here>"

SMTPSession smtp;

void smtpCallback(SMTP_Status status);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

void setup()
{

    Serial.begin(115200);

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

    // Method 1, set time using core function e.g. configTime or configTzTime before connecting to mail server
    // This method requires internet connection.
    // This method suites for ESP8266, ESP32, Raspberry Pi Pico (W)

    Serial.print("Waiting for NTP server time reading");

    configTime(3, 0, "pool.ntp.org", "time.nist.gov");

    ms = millis();

    while (millis() - ms < 10000 && time(nullptr) < ESP_TIME_DEFAULT_TS)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.println();

    // Method 2, set timestamp directly via smtp.setSystemTime or imap.setSystemTime
    // This method doen not require internet connection.
    // This method suites for non-system time (device local time) devices e.g. ARM, SAMD and AVR devices.
    // The timestamp is the seconds since Midnight Jan 1, 1970 and can be taken from RTC chip
    // and other providers.

    // If use this method with ESP8266 and ESP32, the device system time will be set automatically.

    // time_t ts = 1577836800;

    /*
    Set the NTP config time
    For times east of the Prime Meridian use 0-12
    For times west of the Prime Meridian add 12 to the offset.
    Ex. American/Denver GMT would be -6. 6 + 12 = 18
    See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
    */
    // float gmtOffset = 3.0; // GMT offset in hour

    // smtp.setSystemTime(ts, gmtOffset);

    // Note:
    // If time setting faild when using method 1 and method 2, the library internal NTP time reading will
    // be started when valid time is required during server connection.

    // To disable library internal NTP time reading, please comment or remove the following macro defined in src/ESP_Mail_FS.h
    // #define ENABLE_NTP_TIME

    MailClient.networkReconnect(true);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    MailClient.clearAP();
    MailClient.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

    smtp.debug(1);

    smtp.callback(smtpCallback);

    Session_Config config;

    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;

    config.login.user_domain = F("mydomain.net");

    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = F("ESP Mail");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Test sending plain text Email");
    message.addRecipient(F("Someone"), RECIPIENT_EMAIL);

    String textMsg = "This is simple plain text message";
    message.text.content = textMsg;

    message.text.charSet = F("us-ascii");

    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    if (!smtp.connect(&config))
    {
        ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
        return;
    }

    if (!smtp.isLoggedIn())
    {
        Serial.println("\nNot yet logged in.");
    }
    else
    {
        if (smtp.isAuthenticated())
            Serial.println("\nSuccessfully logged in.");
        else
            Serial.println("\nConnected with no Auth.");
    }

    if (!MailClient.sendMail(&smtp, &message))
        ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
}

void loop()
{
}

void smtpCallback(SMTP_Status status)
{
    Serial.println(status.info());

    if (status.success())
    {

        Serial.println("----------------");
        ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
        ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
        Serial.println("----------------\n");

        for (size_t i = 0; i < smtp.sendingResult.size(); i++)
        {

            SMTP_Result result = smtp.sendingResult.getItem(i);

            ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
            ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
            ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
        }
        Serial.println("----------------\n");

        smtp.sendingResult.clear();
    }
}
