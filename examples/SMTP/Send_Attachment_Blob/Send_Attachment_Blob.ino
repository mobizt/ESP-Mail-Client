

/**
 * This example shows how to send Email with attachments and inline images.
 *
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: suwatchai@outlook.com
 *
 * Github: https://github.com/mobizt/ESP-Mail-Client
 *
 * Copyright (c) 2022 mobizt
 *
 */

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else

// Other Client defined here
// To use custom Client, define ENABLE_CUSTOM_CLIENT in  src/ESP_Mail_FS.h.
// See the example Custom_Client.ino for how to use.

#endif

#include <ESP_Mail_Client.h>

/* This is for attachment data */
#include "blob_data.h"

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

/** For Gmail, to send Email via port 465 (SSL), less secure app option
 * should be enabled in the account settings. https://myaccount.google.com/lesssecureapps?pli=1
 *
 * Some Gmail user still not able to sign in using account password even above option was set up,
 * for this case, use "App Password" to sign in instead.
 * About Gmail "App Password", go to https://support.google.com/accounts/answer/185833?hl=en
 *
 * For Yahoo mail, log in to your yahoo mail in web browser and generate app password by go to
 * https://login.yahoo.com/account/security/app-passwords/add/confirm?src=noSrc
 *
 * To use Gmai and Yahoo's App Password to sign in, define the AUTHOR_PASSWORD with your App Password
 * and AUTHOR_EMAIL with your account email.
 */

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "<host>"

/** The smtp port e.g.
 * 25  or esp_mail_smtp_port_25
 * 465 or esp_mail_smtp_port_465
 * 587 or esp_mail_smtp_port_587
 */
#define SMTP_PORT esp_mail_smtp_port_587

/* The log in credentials */
#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

void setup()
{

  Serial.begin(115200);

#if defined(ARDUINO_ARCH_SAMD)
  while (!Serial)
    ;
  Serial.println();
  Serial.println("**** Custom built WiFiNINA firmware need to be installed.****\nTo install firmware, read the instruction here, https://github.com/mobizt/ESP-Mail-Client#install-custom-built-wifinina-firmware");

#endif

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

  /** Enable the debug via Serial port
   * 0 for no debugging
   * 1 for basic level debugging
   *
   * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
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
  session.login.user_domain = F("mydomain.net");

  /* Set the NTP config time */
  session.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  session.time.gmt_offset = 3;
  session.time.day_light_offset = 0;

  /* Declare the message class */
  SMTP_Message message;

  /* Enable the chunked data transfer with pipelining for large message if server supported */
  message.enable.chunking = true;

  /* Set the message headers */
  message.sender.name = F("ESP Mail");
  message.sender.email = AUTHOR_EMAIL;

  message.subject = F("Test sending Email with attachments and inline images");
  message.addRecipient(F("user1"), F("change_this@your_mail_dot_com"));

  message.html.content = F("<span style=\"color:#ff0000;\">This message contains 3 inline images and 1 attachment file.</span><br/><br/><img src=\"firebase_logo.png\"  width=\"80\" height=\"60\"> <img src=\"tree.gif\" width=\"40\" height=\"60\"> <img src=\"bird.gif\" width=\"116\" height=\"75\">");

  /** The HTML text message character set e.g.
   * us-ascii
   * utf-8
   * utf-7
   * The default value is utf-8
   */
  message.html.charSet = F("utf-8");

  /** The content transfer encoding e.g.
   * enc_7bit or "7bit" (not encoded)
   * enc_qp or "quoted-printable" (encoded)
   * enc_base64 or "base64" (encoded)
   * enc_binary or "binary" (not encoded)
   * enc_8bit or "8bit" (not encoded)
   * The default value is "7bit"
   */
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_qp;

  message.text.content = F("This message contains 3 inline images and 1 attachment file.\r\nThe inline images were not shown in the plain text message.");
  message.text.charSet = F("utf-8");
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /** The message priority
   * esp_mail_smtp_priority_high or 1
   * esp_mail_smtp_priority_normal or 3
   * esp_mail_smtp_priority_low or 5
   * The default value is esp_mail_smtp_priority_low
   */
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_normal;

  /** The Delivery Status Notifications e.g.
   * esp_mail_smtp_notify_never
   * esp_mail_smtp_notify_success
   * esp_mail_smtp_notify_failure
   * esp_mail_smtp_notify_delay
   * The default value is esp_mail_smtp_notify_never
   */
  // message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Set the custom message header */
  message.addHeader(F("Message-ID: <user1@gmail.com>"));

  /* The attachment data item */
  SMTP_Attachment att[4];
  int attIndex = 0;

  /** Set the inline image info e.g.
   * file name, MIME type, BLOB data, BLOB data size,
   * transfer encoding (should be base64 for inline image)
   */
  att[attIndex].descr.filename = F("firebase_logo.png");
  att[attIndex].descr.mime = F("image/png");
  att[attIndex].blob.data = firebase_logo_png;
  att[attIndex].blob.size = sizeof(firebase_logo_png);
  att[attIndex].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add inline image to the message */
  message.addInlineImage(att[attIndex]);

  /** Set the inline image info e.g.
   * file name, MIME type, BLOB data, BLOB data size.
   * The default transfer encoding is base64.
   */
  attIndex++;
  att[attIndex].descr.filename = F("tree.gif");
  att[attIndex].descr.mime = F("image/gif");
  att[attIndex].blob.data = tree_img_gif;
  att[attIndex].blob.size = sizeof(tree_img_gif);
  att[attIndex].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add inline image to the message */
  message.addInlineImage(att[attIndex]);

  /** Set the inline image info e.g.
   * file name, MIME type, BLOB data, BLOB data size.
   * The default transfer encoding is base64.
   */
  attIndex++;
  att[attIndex].descr.filename = F("bird.gif");
  att[attIndex].descr.mime = F("image/gif");
  att[attIndex].blob.data = bird_img_gif;
  att[attIndex].blob.size = sizeof(bird_img_gif);
  att[attIndex].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add inline image to the message */
  message.addInlineImage(att[attIndex]);

  /* Prepare the attachment data (from ram) */
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
  attIndex++;
  att[attIndex].descr.filename = F("test.dat");
  att[attIndex].descr.mime = F("application/octet-stream");
  att[attIndex].blob.data = a;
  att[attIndex].blob.size = 512;
  att[attIndex].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  /* Add attachment to the message */
  message.addAttachment(att[attIndex]);

  /* Connect to server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending the Email and close the session */
  if (!MailClient.sendMail(&smtp, &message, true))
    Serial.println("Error sending Email, " + smtp.errorReason());

  // to clear sending result log
  // smtp.sendingResult.clear();

  ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
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
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}
