

/**
 * This example will send the Email in plain text version
 * with rfc822 message attachment which the rfc822 message
 * also contains its attachement.
 * 
 * Created by K. Suwatchai (Mobizt)
 * 
 * Email: suwatchai@outlook.com
 * 
 * Github: https://github.com/mobizt/ESP-Mail-Client
 * 
 * Copyright (c) 2020 mobizt
 *
*/

//To use send Email for Gmail to port 465 (SSL), less secure app option should be enabled. https://myaccount.google.com/lesssecureapps?pli=1

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <ESP_Mail_Client.h>

/* This is for attachment data */
#include "image.h"

#define WIFI_SSID "################"
#define WIFI_PASSWORD "################"

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com
 * For yahoo mail, log in to your yahoo mail in web browser and generate app password by go to
 * https://login.yahoo.com/account/security/app-passwords/add/confirm?src=noSrc
 * and use the app password as password with your yahoo mail account to login.
 * The google app password signin is also available https://support.google.com/mail/answer/185833?hl=en
*/
#define SMTP_HOST "################"

/** The smtp port e.g. 
 * 25  or esp_mail_smtp_port_25
 * 465 or esp_mail_smtp_port_465
 * 587 or esp_mail_smtp_port_587
*/
#define SMTP_PORT 25

/* The log in credentials */
#define AUTHOR_EMAIL "################"
#define AUTHOR_PASSWORD "################"

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

void setup()
{

  Serial.begin(115200);
  Serial.println();

  Serial.print("Connecting to AP");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.println("Connect to NTP server and set the device time\r\nPlease wait...\r\n");
  float timeZone = 3;//GMT+3
  float daylightOffset = 0;
  
  /* Set the device time */
  MailClient.Time.setClock(timeZone, daylightOffset);

  /** Enable the debug via Serial port
   * none debug or 0
   * basic debug or 1
  */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "mydomain.net";

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = "ESP Mail";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "Test sending Email with rfc822 attachment";
  message.addRecipient("Someone", "####@#####_dot_com");

  message.text.content = "This is simple plain text message with rfc822 attachment";

  /** The Plain text message character set e.g.
   * us-ascii
   * utf-8
   * utf-7
   * The default value is utf-8
  */
  message.text.charSet = "us-ascii";

  /** The content transfer encoding e.g.
   * enc_7bit or "7bit" (not encoded)
   * enc_qp or "quoted-printable" (encoded)
   * enc_base64 or "base64" (encoded)
   * enc_binary or "binary" (not encoded)
   * enc_8bit or "8bit" (not encoded)
   * The default value is "7bit"
  */
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  /** The message priority
   * esp_mail_smtp_priority_high or 1
   * esp_mail_smtp_priority_normal or 3
   * esp_mail_smtp_priority_low or 5
   * The default value is esp_mail_smtp_priority_low
  */
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;

  /* Set the custom message header */
  message.addHeader("Message-ID: <abcde.fghij@gmail.com>");

  SMTP_Message rfc822;
  rfc822.messageID = "1234@local.machine.example";
  rfc822.from.name = "rob";
  rfc822.from.email = "rob@example.com";
  rfc822.sender.name = "steve";
  rfc822.sender.email = "steve@example.com";
  String dt = MailClient.Time.getDateTimeString();
  rfc822.date = dt.c_str();
  rfc822.subject = "Test rfc822 message";
  rfc822.comment = "This is comment";
  rfc822.addRecipient("joe", "joe@example.com");
  rfc822.response.reply_to = "rob@example.com";
  rfc822.text.charSet = "utf-8";
  rfc822.text.content = "This is rfc822 text message";
  rfc822.text.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  rfc822.html.charSet = "utf-8";
  rfc822.html.content = "This is rfc822 html message";
  rfc822.html.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* The attachment data item */
  SMTP_Attachment att;

  /** Set the attachment info e.g. 
   * file name, MIME type, BLOB data, BLOB data size,
   * and transfer encoding
  */
  att.descr.filename = "firebase_logo.png";
  att.descr.mime = "image/png";
  att.blob.data = firebase_png;
  att.blob.size = sizeof(firebase_png);
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add the attachment to the rfc822 message */
  rfc822.addAttachment(att);

  /* Prepare other attachment data  */
  uint8_t *a = new uint8_t[512];
  int j = 0;

  for (int i = 0; i < 512; i++)
  {
    a[i] = j;
    j++;
    if (j > 255)
      j = 0;
  }

  /** Set the attachment info e.g. 
   * file name, MIME type, BLOB data, BLOB data size.
   * The default transfer encoding is base64.
  */
  message.resetAttachItem(att); //Clear the attach item data to reuse
  att.descr.filename = "test.dat";
  att.descr.mime = "application/octet-stream";
  att.blob.data = a;
  att.blob.size = 512;

  /* Add this attachment to the message */
  message.addAttachment(att);

  /* Add rfc822 message in the message */
  message.addMessage(rfc822);

  /* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

void loop()
{
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {
    Serial.println("----------------");
    Serial.printf("Message sent success: %d\n", status.completedCount());
    Serial.printf("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      localtime_r(&result.timesstamp, &dt);

      Serial.printf("Message No: %d\n", i + 1);
      Serial.printf("Status: %s\n", result.completed ? "success" : "failed");
      Serial.printf("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      Serial.printf("Recipient: %s\n", result.recipients);
      Serial.printf("Subject: %s\n", result.subject);
    }
    Serial.println("----------------\n");
  }
}
