

/**
 * This example shows how to send Email using ESP8266 and ENC28J60 Ethernet module.
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

// This example requires ESP8266 Arduino Core SDK v3.x.x

/**
 *
 * The ENC28J60 Ethernet module and ESP8266 board, SPI port wiring connection.
 *
 * ESP8266 (Wemos D1 Mini or NodeMCU)        ENC28J60
 *
 * GPIO12 (D6) - MISO                        SO
 * GPIO13 (D7) - MOSI                        SI
 * GPIO14 (D5) - SCK                         SCK
 * GPIO16 (D0) - CS                          CS
 * GND                                       GND
 * 3V3                                       VCC
 *
 */

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <ENC28J60lwIP.h>
// #include <W5100lwIP.h>
// #include <W5500lwIP.h>

/**
 * For ENC28J60 ethernet module, uncomment this line in ESP_Mail_FS.h
  #define ENABLE_ESP8266_ENC28J60_ETH

 * For W5500 ethernet module, uncomment this line in ESP_Mail_FS.h
  #define ENABLE_ESP8266_W5500_ETH

 * For W5100 ethernet module, uncomment this line in ESP_Mail_FS.h
  #define ENABLE_ESP8266_W5100_ETH
*/

#include <ESP_Mail_Client.h>

#define SMTP_HOST "<host>"
#define SMTP_PORT 25

#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

SMTPSession smtp;

void smtpCallback(SMTP_Status status);

unsigned long sendMillis = 0;

#ifdef ESP8266_CORE_SDK_V3_X_X

#define ETH_CS_PIN 16 // D0
ENC28J60lwIP eth(ETH_CS_PIN);
// Wiznet5100lwIP eth(ETH_CS_PIN);
// Wiznet5500lwIP eth(ETH_CS_PIN);

#endif

void sendMail()
{

  smtp.debug(1);

  smtp.callback(smtpCallback);

  ESP_Mail_Session session;

  /* Assign the pointer to Ethernet module lwip interface */
#ifdef ESP8266_CORE_SDK_V3_X_X
  session.spi_ethernet_module.enc28j60 = &eth;
  // session.spi_ethernet_module.w5100 = &eth;
  // session.spi_ethernet_module.w5500 = &eth;
#endif

  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;

  session.login.user_domain = F("mydomain.net");

  session.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  session.time.gmt_offset = 3;
  session.time.day_light_offset = 0;

  SMTP_Message message;

  message.sender.name = F("ESP Mail");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("Test sending plain text Email");
  message.addRecipient(F("Someone"), F("change_this@your_mail_dot_com"));

  String textMsg = "This is simple plain text message";
  message.text.content = textMsg;

  if (!smtp.connect(&session))
    return;

  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

#ifdef ESP8266_CORE_SDK_V3_X_X

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV4); // 4 MHz?
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  eth.setDefault(); // use ethernet for default route
  if (!eth.begin())
  {
    Serial.println("ethernet hardware not found ... sleeping");
    while (1)
    {
      delay(1000);
    }
  }
  else
  {
    Serial.print("connecting ethernet");
    while (!eth.connected())
    {
      Serial.print(".");
      delay(1000);
    }
  }
  Serial.println();
  Serial.print("ethernet IP address: ");
  Serial.println(eth.localIP());

#else
  Serial.println("This example requires ESP8266 Arduino Core SDK v3.x.x, please update.");
#endif
}

void loop()
{
#ifdef ESP8266_CORE_SDK_V3_X_X
  if (millis() - sendMillis > 300000 || sendMillis == 0)
  {
    sendMillis = millis();
    sendMail();
  }
#endif
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