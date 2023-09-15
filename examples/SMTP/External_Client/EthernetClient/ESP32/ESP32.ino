/**
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: suwatchai@outlook.com
 *
 * Github: https://github.com/mobizt/ESP-Mail-Client
 *
 * Copyright (c) 2023 mobizt
 */

/**
 * This example shows how to send Email using EthernetClient.
 *
 * This example used ESP32 and WIZnet W5500 Ethernet module.
 *
 * For ESP32 and LAN8720 see examples/SMTP/Ethernet/ESP32/Send_Text.ino.
 *
 * ESP32 Arduino SDK native Ethernet using ETH.h is currently support Ethernet PHY chips included the following
 * LAN8720, TLK101, RTL8201, DP83848, DM9051, KSZ8041 and KSZ8081.
 *
 */

/** Note for library update from v2.x.x to v3.x.x.
 *
 *  Struct data names changed
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
 */

#include <Arduino.h>

#include <ESP_Mail_Client.h>

// https://github.com/arduino-libraries/Ethernet
#include <Ethernet.h>

// For using other Ethernet library that works with other Ethernet module, 
// the following build flags or macros should be assigned in src/ESP_Mail_FS.h or your Custom_ESP_Mail_FS.h.
// ESP_MAIL_ETHERNET_MODULE_LIB and ESP_MAIL_ETHERNET_MODULE_CLASS
// See src/ESP_Mail_FS.h for detail.

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 587

#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"
#define RECIPIENT_EMAIL "<recipient email here>"

#define WIZNET_RESET_PIN 26 // Connect W5500 Reset pin to GPIO 26 of ESP32
#define WIZNET_CS_PIN 5     // Connect W5500 CS pin to GPIO 5 of ESP32
#define WIZNET_MISO_PIN 19  // Connect W5500 MISO pin to GPIO 19 of ESP32
#define WIZNET_MOSI_PIN 23  // Connect W5500 MOSI pin to GPIO 23 of ESP32
#define WIZNET_SCLK_PIN 18  // Connect W5500 SCLK pin to GPIO 18 of ESP32

unsigned long sentMillis = 0;

const int analog_pin = 34;

uint8_t Eth_MAC[] = {0x02, 0xF0, 0x0D, 0xBE, 0xEF, 0x01};

/**
IPAddress localIP(192, 168, 1, 104);
IPAddress subnet(255, 255, 0, 0);
IPAddress gateway(192, 168, 1, 1);
IPAddress dnsServer(8, 8, 8, 8);
bool optional = false; // Use this static IP only no DHCP
ESP_Mail_StaticIP staIP(localIP, subnet, gateway, dnsServer, optional);
*/

SMTPSession smtp;

EthernetClient eth_client;

void smtpCallback(SMTP_Status status);

void sendEmail()
{

    Session_Config config;

    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;

    config.login.user_domain = F("127.0.0.1");

    SMTP_Message message;

    message.sender.name = F("ESP Mail");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Test sending plain text Email");

    message.addRecipient(F("Someone"), RECIPIENT_EMAIL);

    message.text.content = "This is simple plain text message";

    /* Assign the pointer to global defined Ethernet Client object */
    smtp.setEthernetClient(&eth_client, Eth_MAC, WIZNET_CS_PIN, WIZNET_RESET_PIN); // The staIP can be assigned to the fifth param

    if (!smtp.connect(&config))
    {
        MailClient.printf("Connection error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
        return;
    }

    if (!smtp.isLoggedIn())
    {
        Serial.println("Error, Not yet logged in.");
    }
    else
    {
        if (smtp.isAuthenticated())
            Serial.println("Successfully logged in.");
        else
            Serial.println("Connected with no Auth.");
    }

    if (!MailClient.sendMail(&smtp, &message))
        MailClient.printf("Error, Status Code: %d, Error Code: %d, Reason: %s\n", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());

    MailClient.printf("Free Heap: %d\n", MailClient.getFreeHeap());
}

void setup()
{

    Serial.begin(115200);

    Serial.println();

    smtp.debug(1);

    smtp.callback(smtpCallback);
}

void loop()
{
    if (millis() - sentMillis > 120000 || sentMillis == 0)
    {
        sentMillis = millis();
        sendEmail();
    }
}

void smtpCallback(SMTP_Status status)
{
    Serial.println(status.info());

    if (status.success())
    {
        Serial.println("----------------");
        MailClient.printf("Message sent success: %d\n", status.completedCount());
        MailClient.printf("Message sent failed: %d\n", status.failedCount());
        Serial.println("----------------\n");

        for (size_t i = 0; i < smtp.sendingResult.size(); i++)
        {
            SMTP_Result result = smtp.sendingResult.getItem(i);

            MailClient.printf("Message No: %d\n", i + 1);
            MailClient.printf("Status: %s\n", result.completed ? "success" : "failed");
            MailClient.printf("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
            MailClient.printf("Recipient: %s\n", result.recipients.c_str());
            MailClient.printf("Subject: %s\n", result.subject.c_str());
        }
        Serial.println("----------------\n");
        smtp.sendingResult.clear();
    }
}