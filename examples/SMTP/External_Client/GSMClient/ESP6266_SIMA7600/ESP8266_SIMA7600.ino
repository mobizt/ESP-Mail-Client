
/**
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: suwatchai@outlook.com
 *
 * Github: https://github.com/mobizt/ESP-Mail-Client
 *
 * Copyright (c) 2023 mobizt
 */

// This example used SIM7600x, ESP8266 and TinyGSMClient.

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

// To allow TinyGSM library integration, the following macro should be defined in src/ESP_Mail_FS.h or
// your custom config file src/Custom_ESP_Mail_FS.h.
//  #define TINY_GSM_MODEM_SIM7600

#include <SoftwareSerial.h>

#define ESP8266_RX_PIN 14 // ESP8266 GPIO 14 connected to SIM7600 Pin 71 (TX)
#define ESP8266_TX_PIN 12 // ESP8266 GPIO 12 connected to SIM7600 Pin 68 (RX)
#define ESP8266_PWR_PIN 5 // ESP8266 GPIO 5 connected to SIM7600 Pin 3 (PWRKEY)
#define ESP8266_RESET 4   // ESP8266 GPIO 4 connected to SIM7600 Pin 4 (RESET)
#define UART_BAUD 115200

SoftwareSerial softSerial;

#define TINY_GSM_MODEM_SIM7600

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
#define SerialAT softSerial

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "YourAPN";
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <ESP_Mail_Client.h>
#include <TinyGsmClient.h>

TinyGsm modem(SerialAT);

TinyGsmClient gsm_client(modem);

#define SMTP_HOST "<host>"
#define SMTP_PORT esp_mail_smtp_port_587
#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"
#define RECIPIENT_EMAIL "<recipient email here>"

SMTPSession smtp;

void smtpCallback(SMTP_Status status);

void setup()
{

  SerialMon.begin(115200);

  smtp.debug(1);

  smtp.callback(smtpCallback);

  delay(10);
  pinMode(BAT_EN, OUTPUT);
  digitalWrite(BAT_EN, HIGH);

  // A7670 Reset
  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, LOW);
  delay(100);
  digitalWrite(RESET, HIGH);
  delay(3000);
  digitalWrite(RESET, LOW);

  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, LOW);
  delay(100);
  digitalWrite(PWR_PIN, HIGH);
  delay(1000);
  digitalWrite(PWR_PIN, LOW);

  DBG("Wait...");

  delay(3000);

  SerialAT.begin(UART_BAUD, SERIAL_8N1, ESP8266_RX_PIN, ESP8266_TX_PIN);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  DBG("Initializing modem...");
  if (!modem.init())
  {
    DBG("Failed to restart modem, delaying 10s and retrying");
    return;
  }

  /*
  2 Automatic
  13 GSM Only
  14 WCDMA Only
  38 LTE Only
  */
  modem.setNetworkMode(38);
  if (modem.waitResponse(10000L) != 1)
  {
    DBG(" setNetworkMode faill");
  }

  String name = modem.getModemName();
  DBG("Modem Name:", name);

  String modemInfo = modem.getModemInfo();
  DBG("Modem Info:", modemInfo);

  Session_Config config;

  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = F("127.0.0.1");

  SMTP_Message message;

  message.sender.name = F("ESP Mail");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("Test sending plain text Email using GSM module");
  message.addRecipient(F("Someone"), RECIPIENT_EMAIL);

  message.text.content = "This is simple plain text message";

  smtp.setGSMClient(&gsm_client, &modem, GSM_PIN, apn, gprsUser, gprsPass);

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