

/**
 * This example shows how to send Email using EthernetClient and SSLClient.
 *
 * This example used ESP32 and WIZnet W5500 Ethernet module.
 *
 * Normally SSLClient is not required in ESP32 and ESP8266 devices as seen from Ethernet_BasicClient.ino.
 *
 * This example used SSLClient to show how to use it and can adapt with other Arduino devices other than ESP8266 and ESP32.
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

#include <ESP_Mail_Client.h>

// https://github.com/OPEnSLab-OSU/EthernetLarge
// #include <EthernetLarge.h>

#include <Ethernet.h>

// For NTP client
#include <EthernetUdp.h>
#include <extras/MB_NTP.h>

// Forked version of SSLClient
// https://github.com/mobizt/SSLClient
#include <SSLClient.h>

// Trus anchors for the server i.e. gmail.com for this case
// https://github.com/mobizt/SSLClient/blob/master/TrustAnchors.md
#include "trust_anchors.h"

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
#define SMTP_HOST "smtp.gmail.com"

/** The smtp port e.g.
 * 25  or esp_mail_smtp_port_25
 * 465 or esp_mail_smtp_port_465
 * 587 or esp_mail_smtp_port_587
 */
#define SMTP_PORT esp_mail_smtp_port_587

/* The log in credentials */
#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

#define WIZNET_RESET_PIN 26 // Connect W5500 Reset pin to GPIO 26 of ESP32
#define WIZNET_CS_PIN 5     // Connect W5500 CS pin to GPIO 5 of ESP32
#define WIZNET_MISO_PIN 19  // Connect W5500 MISO pin to GPIO 19 of ESP32
#define WIZNET_MOSI_PIN 23  // Connect W5500 MOSI pin to GPIO 23 of ESP32
#define WIZNET_SCLK_PIN 18  // Connect W5500 SCLK pin to GPIO 18 of ESP32

// For NTP client
EthernetUDP udpClient;

MB_NTP ntpClient(&udpClient, "pool.ntp.org" /* NTP host */, 123 /* NTP port */, 0 /* timezone offset in seconds */);

unsigned long timestamp = 0;

unsigned long sentMillis = 0;

const int analog_pin = 34; // ESP32 GPIO 34 (Analog pin)

uint8_t Eth_MAC[] = {0x02, 0xF0, 0x0D, 0xBE, 0xEF, 0x01};

IPAddress Eth_IP(192, 168, 1, 104);

EthernetClient client;

SSLClient ssl_client(client, TAs, (size_t)TAs_NUM, analog_pin);

/* Declare the global used SMTPSession object for SMTP transport */

// Client type should be basic because port 587 required
// non-secure connection during greeting stage
// and later upgrade to TLS with STARTTLS command.
SMTPSession smtp(&ssl_client, esp_mail_external_client_type_basic);

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

void sendNTPpacket(const char *address);

void getTime();

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
    Ethernet.begin(Eth_MAC, Eth_IP);

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
        Serial.println("Can't connected");
    }
}

// Define the callback function to handle server status acknowledgement
void networkStatusRequestCallback()
{
    // Set the network status
    smtp.setNetworkStatus(Ethernet.linkStatus() == LinkON);
}

// Define the callback function to handle server connection
void connectionRequestCallback(const char *host, int port)
{

    Serial.print("> U: Connecting to server via custom Client... ");
    if (!client.connect(host, port))
    {
        Serial.println("failed.");
        return;
    }
    Serial.println("success.");
}

// Define the callback function to handle server connection upgrade (TLS handshake).
void connectionUpgradeRequestCallback()
{
    Serial.println("> U: Upgrad the connection...");

#if defined(SSLCLIENT_CONNECTION_UPGRADABLE)
    // The host and port parameters will be ignored and can be any for this case.
    ssl_client.connectSSL(SMTP_HOST, SMTP_PORT); // or ssl_client.connectSSL("", 0);
#endif
}

void sendEmail()
{
    // Get time from NTP server
    if (timestamp == 0)
    {
        timestamp = ntpClient.getTime(2000 /* wait 2000 ms */);

        if (timestamp > 0)
            smtp.setSystemTime(timestamp);
    }

    /* Declare the ESP_Mail_Session for user defined session credentials */
    ESP_Mail_Session session;

    /* Set the session config */
    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;
    session.login.user_domain = F("mydomain.net");

    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = F("ESP Mail");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Test sending plain text Email");

    // Please don't forget to change the recipient email address.
    message.addRecipient(F("Someone"), F("change_this@your_mail_dot_com"));

    String textMsg = "This is simple plain text message";
    message.text.content = "hiiiiii";

    /** The Plain text message character set e.g.
     * us-ascii
     * utf-8
     * utf-7
     * The default value is utf-8
     */
    message.text.charSet = F("us-ascii");

    /** The content transfer encoding e.g.
     * enc_7bit or "7bit" (not encoded)
     * enc_qp or "quoted-printable" (encoded)
     * enc_base64 or "base64" (encoded)
     * enc_binary or "binary" (not encoded)
     * enc_8bit or "8bit" (not encoded)
     * The default value is "7bit"
     */
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    // If this is a reply message
    // message.in_reply_to = "<parent message id>";
    // message.references = "<parent references> <parent message id>";

    /** The message priority
     * esp_mail_smtp_priority_high or 1
     * esp_mail_smtp_priority_normal or 3
     * esp_mail_smtp_priority_low or 5
     * The default value is esp_mail_smtp_priority_low
     */
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;

    /* Set the custom message header */
    message.addHeader(F("Message-ID: <abcde.fghij@gmail.com>"));

    // Set the callback functions to hadle the required tasks.
    smtp.connectionRequestCallback(connectionRequestCallback);

    smtp.connectionUpgradeRequestCallback(connectionUpgradeRequestCallback);

    smtp.networkConnectionRequestCallback(networkConnection);

    smtp.networkStatusRequestCallback(networkStatusRequestCallback);

    /* Connect to the server */
    if (!smtp.connect(&session /* session credentials */))
        return;

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
        Serial.println("Error sending Email, " + smtp.errorReason());

    // to clear sending result log
    // smtp.sendingResult.clear();

    ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
}

void setup()
{

    Serial.begin(115200);

    Serial.println();

    networkConnection();

    /** Enable the debug via Serial port
     * 0 for no debugging
     * 1 for basic level debugging
     *
     * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
     */
    smtp.debug(1);

    /* Set the callback function to get the sending results */
    smtp.callback(smtpCallback);

    // Begin NTP client
    ntpClient.begin();
}

void loop()
{
    if (millis() - sentMillis > 120000 || sentMillis == 0)
    {
        sentMillis = millis();
        sendEmail();
    }
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
    /* Print the current status */
    Serial.println(status.info());

    /* Print the sending result */
    if (status.success())
    {
        // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
        // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
        // In ESP32 and ESP32, you can use Serial.printf directly.

        Serial.println("----------------");
        ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
        ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
        Serial.println("----------------\n");

        for (size_t i = 0; i < smtp.sendingResult.size(); i++)
        {
            /* Get the result item */
            SMTP_Result result = smtp.sendingResult.getItem(i);

            // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
            // your device time was synched with NTP server.
            // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
            // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)
            time_t ts = (time_t)result.timestamp;

            ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
            ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
            ESP_MAIL_PRINTF("Date/Time: %s\n", asctime(localtime(&ts)));
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
        }
        Serial.println("----------------\n");

        // You need to clear sending result as the memory usage will grow up.
        smtp.sendingResult.clear();
    }
}