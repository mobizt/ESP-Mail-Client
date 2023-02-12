

/**
 * This example shows how to send Email using EthernetClient.
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
 * and upgrade the connection mode from non-secure to secure connection or perform
 * SSL/TLS handshake.
 * 
 * The purposes of callback functions are following
 * 2.1 networkConnectionRequestCallback is for resume or reconnect the network.
 * User needs to place the code to reset or disconnect and re-connect the network here.
 * The methods that used to assign the callback are
 * 
 * smtp.networkConnectionRequestCallback(<callback function>);
 * imap.networkConnectionRequestCallback(<callback function>);
 * 
 * 2.2 networkStatusRequestCallback is for library to check the network status.
 * User needs to place the code to set back the network status via method smtp.setNetworkStatus(<bool>);
 * and imap.setNetworkStatus(<bool>);
 * 
 * The methods that used to assign the callback are
 * 
 * smtp.networkStatusRequestCallback(<callback function>);
 * imap.networkStatusRequestCallback(<callback function>);
 * 
 * 2.3 connectionUpgradeRequestCallback is for upgrade the server connection from pain text mode (non-secure) 
 * to SSL mode. This callback function should perform SSL/TLS handshake which will be required when the STARTTLS
 * command request was accepted.
 * User needs to place the code that tells the external Client to perform SSL/TLS handshake.
 * 
 * The methods that used to assign the callback are
 * 
 * smtp.connectionUpgradeRequestCallback(<callback function>);
 * imap.connectionUpgradeRequestCallback(<callback function>);
 * 
 * The external Client used for this purpose should be able to work as basic Arduino client that starting connection
 * to server in non-secure mode and able to perform SSL/TLS handshake to upgrade the connection when required.
 * 
 * The current SSL clients that have this ability are
 * 
 * Mobizt's ESP_SSLClient library https://github.com/mobizt/ESP_SSLClient. This library supports ESP8266, ESP32 
 * and Raspberry Pi Pico.
 * 
 * OPEnSLab's SSLClient fork version library https://github.com/mobizt/SSLClient. This library support all microcontrollers
 * that have enough flash memory and ram except for ESP8266 that has stack overfow issue.
 * 
 * With the above two SSL client libraries, the SSL/TLS hanshake can be done with the following code
 * 
 * #if defined(SSLCLIENT_CONNECTION_UPGRADABLE)
 * ssl_client.connectSSL(SMTP_HOST, SMTP_PORT);
 * #endif
 * 
 * The mcro "SSLCLIENT_CONNECTION_UPGRADABLE" was defined in the those two SSL client libraries.
 * 
 * 2.4 connectionRequestCallback is the function for server connection which is set as optional and is not required.
 * When it assigned, user need to place the code to connect to the server with host name and port as following code.
 * 
 * basic_client.connect(host, port); or ssl_client.connect(host, port);
 * 
 * The methods that used to assign the callback are
 * 
 * smtp.connectionRequestCallback(<callback function>);
 * imap.connectionRequestCallback(<callback function>);
 * 
 * 3. SSL client can be adssign to the IMAPSession or SMTPSession object during class constructor like following
 * 
 * SMTPSession smtp(&basic_client, esp_mail_external_client_type_basic);
 * IMAPSession imap(&ssl_client, esp_mail_external_client_type_ssl)
 * 
 * Or can be assign through the method
 * 
 * smtp.setClient(&basic_client, esp_mail_external_client_type_basic);
 * imap.setClient(&ssl_client, esp_mail_external_client_type_ssl);
 * 
 * The first argument of class constructor and method is the Arduino client derived class.
 * The second argument of class constructor and method is the esp_mail_external_client_type enum
 * i.e., esp_mail_external_client_type_basic and esp_mail_external_client_type_ssl
 * 
 * If the firstargument is basic client or non-secure client that works directly with network module 
 * e.g. WiFiClient, EthernetClient and GSMClient, the second argument should be 
 * esp_mail_external_client_type_basic.
 * 
 * This kind of client can use for IMAP and SMTP transports via non-secure ports e.g., 25 (SMTP), 143 (IMAP).
 * 
 * If the firstargument is ssl client that cannot connect in non-secure mode (works as a wrapper class of basic client)
 * e.g., WiFiClientSecure and WiFiSSLClient, the second argument should be esp_mail_external_client_type_ssl.
 * 
 * This kind of client can use for IMAP and SMTP transports via secure ports e.g., 465 (SMTP), 993 (IMAP).
 * 
 * If the firstargument is ssl client that can connect in non-secure mode as Mobizt's ESP_SSLClient and 
 * OPEnSLab's SSLClient fork version, the second argument should be esp_mail_external_client_type_basic.
 * 
 * This kind of client can use for IMAP and SMTP transports via Plain/TLS via STARTTLS ports 
 * e.g., 25 (SMTP), 25 (587), 143 (IMAP).
 * Which the callback function connectionUpgradeRequestCallback in the topic 2.3 is required.
 * 
 * 
 * When using ESP8266, ESP32 and Raspberry Pi Pico with network modules, user does not need to assign the SSL client
 * to the SMTPSession and IMAPSession object or external SSL client library is not required.
 * If ENABLE_CUSTOM_CLIENT mocro was defined in in file "src/ESP_Mail_FS.h" or "src/Custom_ESP_Mail_FS.h", user can assign
 * the basic client of network module as first argument and esp_mail_external_client_type_basic for the second argument.
 * 
 * In above case, library will use the internal SDK SSL engine for SSL/TLS handshake process when working with SSL ports e.g., 465 (SMTP),
 * 993 (IMAP) and Plain/TLS via STARTTLS ports e.g., 25 (SMTP), 25 (587), 143 (IMAP).
 * 
 * 4. Some tasks required valid time e.g., sending Email and SSL certificate validation.
 * Normally when using the WiFi or Ethernet that supported natively on ESP8266, ESP32 and Raspberry Pi Pico (WiFi), 
 * the system (device) time was setup internally by acquiring the NTP server response for synching which is not need any
 * time or UDP client.
 * 
 * For external client usage, the external UDP client is required when valid time is needed because library does not know how and
 * where to get the valid time from.
 * 
 * The UDP clients e.g. WiFiUDP and EthernetUDP required for internal time synching with NTP server which only GMT offset can be assigned.
 * 
 * The external UDP client can be assign to library from
 * 
 * MailClient.setUDPClient(&udp_client, 0);
 * 
 * Which the second argument is the GMT offset. This GMT offset will be used to set the time instead of GMT offset from the session object
 * session.time.gmt_offset.
 * 
 * With external Client usage, the device time will be updated in ESP8266 and ESP32 which user can get it from time(nullptr).
 * While Raspberry Pi Pico and other Arduino devices, the device time is not available. If user need to get the valid time
 * from library, the internal time will be provided from
 * 
 * uint32_t timestamp = MailClient.Time.getCurrentTimestamp();
 * 
 * If user can get the valid time from RTC or any source, the internal time of library (also ESP8266 and ESP32's system time)
 * can be set from
 * 
 * MailClient.Time.setTimestamp(timestamp);
 * 
 * In Raspberry Pi Pico, the device time that can get from time(nullptr) can not set manually except for WiFi using SDK's NTP class.
 * Then when external client was used, the device time will not update by library's internal time synching and when calling 
 * MailClient.Time.setTimestamp. User should call MailClient.Time.getCurrentTimestamp to get the valid time instead.
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

#include <Arduino.h>

#include <ESP_Mail_Client.h>

#include <Ethernet.h>
#include <EthernetUdp.h>

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 587


#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

#define WIZNET_RESET_PIN 26 // Connect W5500 Reset pin to GPIO 26 of ESP32
#define WIZNET_CS_PIN 5     // Connect W5500 CS pin to GPIO 5 of ESP32
#define WIZNET_MISO_PIN 19  // Connect W5500 MISO pin to GPIO 19 of ESP32
#define WIZNET_MOSI_PIN 23  // Connect W5500 MOSI pin to GPIO 23 of ESP32
#define WIZNET_SCLK_PIN 18  // Connect W5500 SCLK pin to GPIO 18 of ESP32

unsigned long sentMillis = 0;

const int analog_pin = 34;

uint8_t Eth_MAC[] = {0x02, 0xF0, 0x0D, 0xBE, 0xEF, 0x01};

SMTPSession smtp;

EthernetClient basic_client;
EthernetUDP udp_client;

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

void sendEmail()
{

    ESP_Mail_Session session;

    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;

    session.login.user_domain = F("mydomain.net");

    SMTP_Message message;

    message.sender.name = F("ESP Mail");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Test sending plain text Email");

    message.addRecipient(F("Someone"), F("change_this@your_mail_dot_com"));

    String textMsg = "This is simple plain text message";
    message.text.content = "hiiiiii";

    smtp.setClient(&basic_client, esp_mail_external_client_type_basic);

    smtp.networkConnectionRequestCallback(networkConnection);
    smtp.networkStatusRequestCallback(networkStatusRequestCallback);

    if (!smtp.connect(&session))
        return;

    if (!MailClient.sendMail(&smtp, &message))
        Serial.println("Error sending Email, " + smtp.errorReason());

    ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
}

void setup()
{

    Serial.begin(115200);

    Serial.println();

    networkConnection();

    // For internal NTP client
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

            time_t ts = (time_t)result.timestamp;

            ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
            ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
            ESP_MAIL_PRINTF("Date/Time: %s\n", asctime(localtime(&ts)));
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
        }
        Serial.println("----------------\n");
        smtp.sendingResult.clear();
    }
}