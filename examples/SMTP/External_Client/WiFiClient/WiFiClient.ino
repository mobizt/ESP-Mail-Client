

/**
 * This example shows how to send Email using WiFiClient.
 *
 * This example used ESP32 and external WiFiClient.
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
#elif __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#elif __has_include(<WiFiS3.h>)
#include <WiFiS3.h>
#endif

#include <ESP_Mail_Client.h>

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

#define SMTP_HOST "<host>"
#define SMTP_PORT 587

SMTPSession smtp;

WiFiClient wifi_client;

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

  smtp.setClient(&wifi_client);

  smtp.networkStatusRequestCallback(networkStatusRequestCallback);
  
  smtp.networkConnectionRequestCallback(networkConnectionRequestCallback);

  smtp.connect(&config);

  if (smtp.isAuthenticated())
    Serial.println("\nSuccessfully logged in.");
  else
    Serial.println("\nConnected with no Auth.");

  smtp.closeSession();

  MailClient.printf("Free Heap: %d\n", MailClient.getFreeHeap());
}

void loop()
{
}

void smtpCallback(SMTP_Status status)
{
  Serial.println(status.info());
}
