
/**
 * This example is for SMTP server connection testing using multiples netwok types and clients.
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
 * session.time.gmt_offset.
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

// Change the folowing WiFi credentials, TEST_MODE and SSLCLIENT_LIB
// #################################################################
#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "password"

// Test mode
// 0 - WiFi test
// 1 - Ethernet test (W5500)
#define TEST_MODE 0

// SSLClient library used for test
// 0 - OPEnSLab's fork SSLClient // https://github.com/mobizt/SSLClient
// 1 - Mobizt's MB_SSLClient // https://github.com/mobizt/ESP_SSLClient
// 2 - Built-in SSL Engine
#define SSLCLIENT_LIB 0
// #################################################################

#include <Arduino.h>
#include <ESP_Mail_Client.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#if SSLCLIENT_LIB == 0
#include <SSLClient.h>
#include "trust_anchors.h"
const int analog_pin = 34;
#elif SSLCLIENT_LIB == 1
#include <ESP_SSLClient.h>
#endif

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

#if TEST_MODE == 0
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif
WiFiUDP udpClient;
#elif TEST_MODE == 1

#if defined(ESP32)
#define WIZNET_RESET_PIN 26 // Connect W5500 Reset pin to GPIO 26 of ESP32
#define WIZNET_CS_PIN 5     // Connect W5500 CS pin to GPIO 5 of ESP32
#define WIZNET_MISO_PIN 19  // Connect W5500 MISO pin to GPIO 19 of ESP32
#define WIZNET_MOSI_PIN 23  // Connect W5500 MOSI pin to GPIO 23 of ESP32
#define WIZNET_SCLK_PIN 18  // Connect W5500 SCLK pin to GPIO 18 of ESP32
#elif defined(ESP8266)
#define WIZNET_RESET_PIN 5 // Connect W5500 Reset pin to GPIO 5 (D1) of ESP8266
#define WIZNET_CS_PIN 16   // Connect W5500 CS pin to GPIO 16 (D0) of ESP8266
#define WIZNET_MISO_PIN 12 // Connect W5500 MISO pin to GPIO 12 (D6) of ESP8266
#define WIZNET_MOSI_PIN 13 // Connect W5500 MOSI pin to GPIO 13 (D7) of ESP8266
#define WIZNET_SCLK_PIN 14 // Connect W5500 SCLK pin to GPIO 14 (D5) of ESP8266
#elif defined(PICO_RP2040)
#define WIZNET_RESET_PIN 20 // Connect W5500 Reset pin to GPIO 20 of Raspberry Pi Pico
#define WIZNET_CS_PIN 17    // Connect W5500 CS pin to GPIO 17 of Raspberry Pi Pico
#define WIZNET_MISO_PIN 16  // Connect W5500 MISO pin to GPIO 16 of Raspberry Pi Pico
#define WIZNET_MOSI_PIN 19  // Connect W5500 MOSI pin to GPIO 19 of Raspberry Pi Pico
#define WIZNET_SCLK_PIN 18  // Connect W5500 SCLK pin to GPIO 18 of Raspberry Pi Pico
#endif

uint8_t Eth_MAC[] = {0x02, 0xF0, 0x0D, 0xBE, 0xEF, 0x01};
IPAddress Eth_IP(192, 168, 1, 104);

EthernetUDP udpClient;
#endif

#if TEST_MODE == 0
WiFiClient basic_client;
#elif TEST_MODE == 1
EthernetClient basic_client;
#endif

#if SSLCLIENT_LIB == 0
SSLClient ssl_client(basic_client, TAs, (size_t)TAs_NUM, analog_pin);
#elif SSLCLIENT_LIB == 1
ESP_SSLClient ssl_client;
#endif

#if SSLCLIENT_LIB < 2
SMTPSession smtp(&ssl_client, esp_mail_external_client_type_ssl);
#define WORK_CLIENT ssl_client
#else
SMTPSession smtp(&basic_client, esp_mail_external_client_type_basic);
#define WORK_CLIENT basic_client
#endif

void smtpCallback(SMTP_Status status);

void networkConnection()
{

#if TEST_MODE == 0

    Serial.print("> I: Connecting to AP");

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
    Serial.print("> I: Connected with IP ");
    Serial.println(WiFi.localIP());
    Serial.println();

#elif TEST_MODE == 1

    Ethernet.init(WIZNET_CS_PIN);
    Serial.println("> I: Resetting Ethernet...  ");
    pinMode(WIZNET_RESET_PIN, OUTPUT);
    digitalWrite(WIZNET_RESET_PIN, HIGH);
    delay(200);
    digitalWrite(WIZNET_RESET_PIN, LOW);
    delay(50);
    digitalWrite(WIZNET_RESET_PIN, HIGH);
    delay(200);

    Serial.println("> I: Starting Ethernet connection...");
    Ethernet.begin(Eth_MAC, Eth_IP);

    unsigned long to = millis();

    while (Ethernet.linkStatus() == LinkOFF || millis() - to < 2000)
    {
        delay(100);
    }

    if (Ethernet.linkStatus() == LinkON)
    {
        Serial.print("> I: Connected with IP ");
        Serial.println(Ethernet.localIP());
    }

#endif
}

void networkStatusRequestCallback()
{
    smtp.setNetworkStatus(WiFi.status() == WL_CONNECTED || Ethernet.linkStatus() == LinkON);
}

void connectionRequestCallback(const char *host, int port)
{
    Serial.print("> U: Connecting to server via custom Client... ");
    if (!WORK_CLIENT.connect(host, port))
    {
        Serial.println("failed.");
        return;
    }
    Serial.println("success.");
}

void connectionUpgradeRequestCallback()
{
    Serial.println("> U: Upgrad the connection...");
#if defined(SSLCLIENT_CONNECTION_UPGRADABLE)
    WORK_CLIENT.connectSSL(SMTP_HOST, SMTP_PORT);
#endif
}

void setup()
{

    ESP_Mail_Session session;

    Serial.begin(115200);

    delay(5000);

    Serial.println();

#if SSLCLIENT_LIB == 0
    Serial.print("> I: Test started using OPEnSLab's SSL Client and ");
#elif SSLCLIENT_LIB == 1
    Serial.print("> I: Test started using Mobizt's ESP_SSLClient and ");
#elif SSLCLIENT_LIB == 2
    Serial.print("> I: Test started using Built-in SSL Engine and ");
#endif

#if TEST_MODE == 0
    Serial.println("WiFiClient\n");
#elif TEST_MODE == 1
    Serial.println("EthernetClient\n");
#endif

#if !defined(ENABLE_CUSTOM_CLIENT)
    Serial.println("> E: ENABLE_CUSTOM_CLIENT Macro is required.\n> I: Please add #define ENABLE_CUSTOM_CLIENT in src/ESP_Mail_FS.h");
    goto exit;
#endif

#if defined(ESP8266) && SSLCLIENT_LIB == 0
    Serial.println("> E: OPEnSLab's SSL Client cannot use with ESP8266");
    goto exit;
#endif

    networkConnection();

    if (WiFi.status() != WL_CONNECTED && Ethernet.linkStatus() != LinkON)
    {
        Serial.println("> E: network connection failed.");
        goto exit;
    }

#if SSLCLIENT_LIB == 1
    WORK_CLIENT.setClient(&basic_client);
    WORK_CLIENT.setInsecure();
#endif

    MailClient.networkReconnect(true);

    MailClient.setUDPClient(&udpClient, 0);

    smtp.debug(1);

    smtp.callback(smtpCallback);

    session.server.host_name = SMTP_HOST;
    session.server.port = SMTP_PORT;
    session.time.ntp_server = F("pool.ntp.org,time.nist.gov");

    smtp.connectionRequestCallback(connectionRequestCallback);

    smtp.networkConnectionRequestCallback(networkConnection);

    smtp.networkStatusRequestCallback(networkStatusRequestCallback);

#if defined(SSLCLIENT_CONNECTION_UPGRADABLE)
    smtp.connectionUpgradeRequestCallback(connectionUpgradeRequestCallback);
#endif

    smtp.connect(&session);

    smtp.closeSession();

    WORK_CLIENT.stop();

exit:

    Serial.println("\n> I: Test done!");
}

void loop()
{
}

void smtpCallback(SMTP_Status status)
{
    Serial.println(status.info());
}