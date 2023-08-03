

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
 * Mobizt's ESP_SSLClient library https://github.com/mobizt/ESP_SSLClient. This library supports all Arduino devices.
 *
 * OPEnSLab's SSLClient fork version library https://github.com/mobizt/SSLClient. This library supports all Arduino devices except for ESP8266 that has stack overfow issue.
 *
 * With above two SSL client libraries, the SSL/TLS hanshake can be done via
 *
 * #if defined(SSLCLIENT_CONNECTION_UPGRADABLE)
 * ssl_client.connectSSL(); // Require host/ip and port as parameter in SSLClient library
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
 * When using ESP8266 and Raspberry Pi Pico devices with network interface module, the external SSL client library is not required
 * because internal SSL engine is used.
 *
 * User can use the basic client of network module directly and choose esp_mail_external_client_type_basic for the second argument of the method.
 *
 * For ESP32, since the library version 3.3.0, the internal SSL engine (mbedTLS) will not handle SSL handshake for external client any more, 
 * then user needs to use external SSL client (ESP_SSLClient or SSLClient) to do handshake and it required connectionUpgradeRequestCallback setup.  
 *
 * 4. When using external client to do some tasks that required valid time e.g., sending Email and SSL certificate validation, the external
 * UDP client is required for internal NTP time reading.
 *
 * User can assign the UDP client via
 *
 * MailClient.setUDPClient(&udp_client, 0);
 *
 * Which the second argument is the GMT offset. This GMT offset will be used to set the time offset instead of GMT offset set from the session object
 * config.time.gmt_offset.
 *
 * IN ESP8266 and ESP32, device time will be updated after reading fishished and can get via time(nullptr).
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
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>

#endif

#include <ESP_Mail_Client.h>

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

#define SMTP_HOST "<host>"
#define SMTP_PORT 587

SMTPSession smtp;

WiFiClient basic_client;
WiFiUDP udp_client;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

void smtpCallback(SMTP_Status status);

void networkStatusRequestCallback()
{
  smtp.setNetworkStatus(WiFi.status() == WL_CONNECTED);
}

void networkConnectionRequestCallback()
{
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
}

void setup()
{

  Serial.begin(115200);

  networkConnectionRequestCallback();

  /*
  For internal NTP client
  For times east of the Prime Meridian use 0-12
  For times west of the Prime Meridian add 12 to the offset.
  Ex. American/Denver GMT would be -6. 6 + 12 = 18
  See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
  */
  MailClient.setUDPClient(&udp_client, 0 /* GMT offset */);

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

  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");

  smtp.setClient(&basic_client, esp_mail_external_client_type_basic);

  smtp.networkStatusRequestCallback(networkStatusRequestCallback);
  smtp.networkConnectionRequestCallback(networkConnectionRequestCallback);

  smtp.connect(&config);

  if (smtp.isAuthenticated())
    Serial.println("\nSuccessfully logged in.");
  else
    Serial.println("\nConnected with no Auth.");

  smtp.closeSession();

  ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
}

void loop()
{
}

void smtpCallback(SMTP_Status status)
{
  Serial.println(status.info());
}
