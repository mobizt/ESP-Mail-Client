

/**
 * This example shows how to send Email using EthernetClient and SSLClient.
 *
 * This example used ESP32 and WIZnet W5500 Ethernet module.
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
 * and upgrade the connection mode (do SSL/TLS handshake) from non-secure connection
 * to secure connection.
 *
 * The purposes of callback functions are following
 *
 * 2.1 networkConnectionRequestCallback function is for resume or reconnect the network.
 *
 * User needs to place the code to reset or re-connect the network here.
 *
 * User can assign the callback via
 *
 * smtp.networkConnectionRequestCallback(<callback function>);
 * imap.networkConnectionRequestCallback(<callback function>);
 *
 * 2.2 networkStatusRequestCallback function is for network status acknowledgement.
 *
 * User needs to place the code to set back the network status via method smtp.setNetworkStatus(<bool>);
 * and imap.setNetworkStatus(<bool>);
 *
 * User can assign the callback via
 *
 * smtp.networkStatusRequestCallback(<callback function>);
 * imap.networkStatusRequestCallback(<callback function>);
 *
 * 2.3 connectionUpgradeRequestCallback function is for SSL/TLS handshake to upgrade the server connection
 * from pain text mode (non-secure) to SSL mode. This callback function will be called when STARTTLS
 * command request was accepted by server.
 *
 * User needs to place the code that tells the external Client to do SSL/TLS handshake.
 *
 * User can assign the callback via
 *
 * smtp.connectionUpgradeRequestCallback(<callback function>);
 * imap.connectionUpgradeRequestCallback(<callback function>);
 *
 * The SSL clients that can connect in both non-secure and secure modes are
 *
 * Mobizt's ESP_SSLClient library https://github.com/mobizt/ESP_SSLClient. This library supports ESP8266, ESP32
 * and Raspberry Pi Pico.
 *
 * OPEnSLab's SSLClient fork version library https://github.com/mobizt/SSLClient. This library supports all microcontrollers
 * that have enough flash memory and ram except for ESP8266 that has stack overfow issue.
 *
 * With above two SSL client libraries, the SSL/TLS hanshake can be done via
 *
 * #if defined(SSLCLIENT_CONNECTION_UPGRADABLE)
 * ssl_client.connectSSL(SMTP_HOST, SMTP_PORT);
 * #endif
 *
 * The mcro "SSLCLIENT_CONNECTION_UPGRADABLE" was defined in the those two SSL client libraries.
 *
 * 2.4 connectionRequestCallback is the function for starting server connection which is currently not required.
 *
 * User can assign the callback via
 *
 * smtp.connectionRequestCallback(<callback function>);
 * imap.connectionRequestCallback(<callback function>);
 *
 * The callback function assigned should accept the const char* for hosname and integer for port ,
 * user needs to place the code using external client to connect to the server as the following.
 *
 * void connectionRequestCallback(const char *host, int port)
 * {
 *  basic_or_ssl_client.connect(host, port);
 * }
 *
 * 3. SSL client can be adssigned to the IMAPSession and SMTPSession objects during class constructor
 *
 * SMTPSession smtp(&basic_client, esp_mail_external_client_type_basic);
 * IMAPSession imap(&ssl_client, esp_mail_external_client_type_ssl)
 *
 * Or can be assign through the method
 *
 * smtp.setClient(&basic_client, esp_mail_external_client_type_basic);
 * imap.setClient(&ssl_client, esp_mail_external_client_type_ssl);
 *
 * The first argument of class constructor and method is the pointer to Arduino client class.
 * The second argument of class constructor and method is the esp_mail_external_client_type enum
 * i.e., esp_mail_external_client_type_basic and esp_mail_external_client_type_ssl
 *
 * If the first argument is basic client or non-secure client which works directly with network module
 * e.g. WiFiClient, EthernetClient and GSMClient, the second argument should be
 * esp_mail_external_client_type_basic.
 *
 * This kind of client can be used for IMAP and SMTP transports via non-secure ports e.g., 25 (SMTP) and 143 (IMAP).
 *
 * If the first argument is ssl client that starting connection in secure mode e.g., WiFiClientSecure and
 * WiFiSSLClient, the second argument should be esp_mail_external_client_type_ssl.
 *
 * This kind of client can be used for IMAP and SMTP transports via secure ports e.g., 465 (SMTP) and 993 (IMAP).
 *
 * If the first argument is ssl client that can starting connection in non-secure mode as Mobizt's ESP_SSLClient and
 * OPEnSLab's SSLClient fork version, the second argument should be esp_mail_external_client_type_basic.
 *
 * This kind of client can be used for IMAP and SMTP transports via Plain/TLS via STARTTLS ports
 * e.g., 25 (SMTP), 25 (587), and 143 (IMAP).
 *
 * With this type of client, the callback function connectionUpgradeRequestCallback is required to perform SSL/TLS handshake.
 *
 *
 * When using ESP8266, ESP32 and Raspberry Pi Pico devices with network modules with network interface module, the external SSL client library is not required
 * because internal SSL engine is used.
 *
 * User can use the basic client of network module directly and choose esp_mail_external_client_type_basic for the second argument of the method.
 *
 *
 * 4. When using external client to do some tasks that required valid time e.g., sending Email and SSL certificate validation, the external
 * UDP client is required for internal NTP time synching.
 *
 * User can assign the UDP client via
 *
 * MailClient.setUDPClient(&udp_client, 0);
 *
 * Which the second argument is the GMT offset. This GMT offset will be used to set the time offset instead of GMT offset set from the session object
 * config.time.gmt_offset.
 *
 * IN ESP8266 and ESP32, device time will be updated after synching fishished and can get via time(nullptr).
 *
 * In Raspberry Pi Pico and other Arduino devices, the device time is not available. The valid time can be obtained from
 *
 * uint32_t timestamp = MailClient.Time.getCurrentTimestamp();
 *
 * If the valid time can be obtaoned from RTC chip or other sources, the library internal time can be changed or set with
 *
 * MailClient.Time.setTimestamp(timestamp);
 *
 * In Raspberry Pi Pico, the device time can not set manually by user except for using SDK's NTP class that works with WiFi only.
 * When use this device with external client, the device time will not be changed and user should call
 * MailClient.Time.getCurrentTimestamp() to get the valid time instead.
 *
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

#include <ESP_Mail_Client.h>

#include <Ethernet.h>
#include <EthernetUdp.h>

// Forked version of SSLClient
// https://github.com/mobizt/SSLClient
#include <SSLClient.h>

// Trus anchors for the server i.e. gmail.com for this case
// https://github.com/mobizt/SSLClient/blob/master/TrustAnchors.md
#include "trust_anchors.h"

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

unsigned long timestamp = 0;

unsigned long sentMillis = 0;

const int analog_pin = 34;

uint8_t Eth_MAC[] = {0x02, 0xF0, 0x0D, 0xBE, 0xEF, 0x01};

EthernetClient basic_client;
EthernetUDP udp_client;

SSLClient ssl_client(basic_client, TAs, (size_t)TAs_NUM, analog_pin);

SMTPSession smtp;

void smtpCallback(SMTP_Status status);

void ResetEthernet()
{
    Serial.println("Resetting WIZnet W5500 Ethernet Board...  ");
    pinMode(WIZNET_RESET_PIN, OUTPUT);
    digitalWrite(WIZNET_RESET_PIN, HIGH);
    delay(200);
    digitalWrite(WIZNET_RESET_PIN, LOW);
    delay(50);
    digitalWrite(WIZNET_RESET_PIN, HIGH);
    delay(200);
}

void networkConnection()
{

    Ethernet.init(WIZNET_CS_PIN);

    ResetEthernet();

    Serial.println("Starting Ethernet connection...");
    Ethernet.begin(Eth_MAC);

    unsigned long to = millis();

    while (Ethernet.linkStatus() == LinkOFF || millis() - to < 2000)
    {
        delay(100);
    }

    if (Ethernet.linkStatus() == LinkON)
    {
        Serial.print("Connected with IP ");
        Serial.println(Ethernet.localIP());
    }
    else
    {
        Serial.println("Can't connect");
    }
}

void networkStatusRequestCallback()
{
    smtp.setNetworkStatus(Ethernet.linkStatus() == LinkON);
}

void connectionUpgradeRequestCallback()
{
    Serial.println("> U: Upgrad the connection...");

#if defined(SSLCLIENT_CONNECTION_UPGRADABLE)
    ssl_client.connectSSL(SMTP_HOST, SMTP_PORT);
#endif
}

void sendEmail()
{

    Session_Config config;

    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;

    config.login.user_domain = F("mydomain.net");

    SMTP_Message message;

    message.sender.name = F("ESP Mail");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Test sending plain text Email");
    message.addRecipient(F("Someone"), RECIPIENT_EMAIL);

    String textMsg = "This is simple plain text message";
    message.text.content = "hiiiiii";

    smtp.setClient(&ssl_client, esp_mail_external_client_type_basic);

    smtp.connectionUpgradeRequestCallback(connectionUpgradeRequestCallback);
    smtp.networkConnectionRequestCallback(networkConnection);
    smtp.networkStatusRequestCallback(networkStatusRequestCallback);

    if (!smtp.connect(&config))
    {
        ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
        return;
    }

    if (smtp.isAuthenticated())
        Serial.println("\nSuccessfully logged in.");
    else
        Serial.println("\nConnected with no Auth.");

    if (!MailClient.sendMail(&smtp, &message))
        ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());

    ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
}

void setup()
{

    Serial.begin(115200);

    Serial.println();

    networkConnection();

    /* For internal NTP client
    For times east of the Prime Meridian use 0-12
    For times west of the Prime Meridian add 12 to the offset.
    Ex. American/Denver GMT would be -6. 6 + 12 = 18
    See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
    */
    MailClient.setUDPClient(&udp_client, 0 /* GMT offset */);

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