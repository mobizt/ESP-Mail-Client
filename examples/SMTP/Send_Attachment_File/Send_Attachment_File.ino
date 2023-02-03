

/**
 * This example shows how to send Email with attachments and  inline images stored in SD card.
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

/** Assign SD card type and FS used in src/ESP_Mail_FS.h and
 * change the config for that card interfaces in src/extras/SDHelper.h
 */

#include <Arduino.h>
#if defined(ESP32) || defined(PICO_RP2040)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else

// Other Client defined here
// To use custom Client, define ENABLE_CUSTOM_CLIENT in src/ESP_Mail_FS.h.
// See the example Custom_Client.ino for how to use.

#endif

#include <ESP_Mail_Client.h>

// Provide the SD card interfaces setting and mounting
#include <extras/SDHelper.h>

// To use only SMTP functions, you can exclude the IMAP from compilation, see ESP_Mail_FS.h.

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

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

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

void setup()
{

  Serial.begin(115200);

#if defined(ARDUINO_ARCH_SAMD)
  while (!Serial)
    ;
  Serial.println();
  Serial.println("**** Custom built WiFiNINA firmware need to be installed.****\n");
  Serial.println("To install firmware, read the instruction here, https://github.com/mobizt/ESP-Mail-Client#install-custom-build-wifinina-firmware");
#endif

  Serial.println();

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
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

#if defined(ESP_MAIL_DEFAULT_SD_FS) // defined in src/ESP_Mail_FS.h

  // Mount SD card.
  SD_Card_Mounting(); // See src/extras/SDHelper.h

  Serial.println("Preparing SD file attachments...");

  const char *orangeImg = "iVBORw0KGgoAAAANSUhEUgAAAGQAAABkCAYAAABw4pVUAAAAoUlEQVR42u3RMQ0AMAgAsCFgftHLiQpsENJaaFT+fqwRQoQgRAhChCBECEKECBGCECEIEYIQIQgRghCECEGIEIQIQYgQhCBECEKEIEQIQoQgBCFCECIEIUIQIgQhCBGCECEIEYIQIQhBiBCECEGIEIQIQQhChCBECEKEIEQIQhAiBCFCECIEIUIQghAhCBGCECEIEYIQIUKEIEQIQoQg5LoBGi/oCaOpTXoAAAAASUVORK5CYII=";

  // Write demo data to file

  static uint8_t buf[512];

// SDFat?
#if defined(ESP_MAIL_USE_SDFAT) // ESP_MAIL_USE_SDFAT is auto defined when you set to use SdFat in src/ESP_Mail_FS.h
  SdFile file;
  file.open("/orange.png", O_RDWR | O_CREAT);
  file.print(orangeImg);
  file.close();

  file.open("/bin1.dat", O_RDWR | O_CREAT);
  buf[0] = 'H';
  buf[1] = 'E';
  buf[2] = 'A';
  buf[3] = 'D';
  file.write(buf, 4);

  size_t i;

  for (i = 0; i < 4; i++)
  {
    memset(buf, i + 1, 512);
    file.write(buf, 512);
  }

  buf[0] = 'T';
  buf[1] = 'A';
  buf[2] = 'I';
  buf[3] = 'L';
  file.write(buf, 4);
  file.close();

#else

#if defined(ESP32)
  File file = ESP_MAIL_DEFAULT_SD_FS.open("/orange.png", FILE_WRITE);
#else
  File file = ESP_MAIL_DEFAULT_SD_FS.open("/orange.png", "w");
#endif
  file.print(orangeImg);
  file.close();

#if defined(ESP32)
  file = ESP_MAIL_DEFAULT_SD_FS.open("/bin1.dat", FILE_WRITE);
#else
  file = ESP_MAIL_DEFAULT_SD_FS.open("/bin1.dat", "w");
#endif

  buf[0] = 'H';
  buf[1] = 'E';
  buf[2] = 'A';
  buf[3] = 'D';
  file.write(buf, 4);

  size_t i;

  for (i = 0; i < 4; i++)
  {
    memset(buf, i + 1, 512);
    file.write(buf, 512);
  }

  buf[0] = 'T';
  buf[1] = 'A';
  buf[2] = 'I';
  buf[3] = 'L';
  file.write(buf, 4);
  file.close();
#endif

#endif

  /*  Set the network reconnection option */
  MailClient.networkReconnect(true);

  // The WiFi credentials are required for Pico W
  // due to it does not have reconnect feature.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
  MailClient.clearAP();
  MailClient.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

  /** Enable the debug via Serial port
   * 0 for no debugging
   * 1 for basic level debugging
   *
   * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
   */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the ESP_Mail_Session for user defined session credentials */
  ESP_Mail_Session session;

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;

  /** Assign your host name or you public IPv4 or IPv6 only
   * as this is the part of EHLO/HELO command to identify the client system
   * to prevent connection rejection.
   * If host name or public IP is not available, ignore this or
   * use generic host "mydomain.net".
   *
   * Assign any text to this option may cause the connection rejection.
   */
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

  message.subject = F("Test sending Email with attachments and inline images from SD card and Flash");

  message.addRecipient(F("user1"), F("change_this@your_mail_dot_com"));

  /** Two alternative content versions are sending in this example e.g. plain text and html */
  String htmlMsg = "<span style=\"color:#ff0000;\">This message contains 1 inline image and 1 attachment file.</span><br/><br/><img src=\"orange.png\" width=\"100\" height=\"100\">";

  message.html.content = htmlMsg;

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

  message.text.content = F("This message contains 1 inline image and 1 attachment file.\r\nThe inline image will not show in the plain text message.");

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
  SMTP_Attachment att[2];
  int attIndex = 0;

  /** Set the inline image info e.g.
   * file name, MIME type, file path, file storage type,
   * transfer encoding and content encoding
   */
  att[attIndex].descr.filename = F("orange.png");
  att[attIndex].descr.mime = F("image/png");
  att[attIndex].file.path = F("/orange.png");

  /** The file storage type e.g.
   * esp_mail_file_storage_type_none,
   * esp_mail_file_storage_type_flash, and
   * esp_mail_file_storage_type_sd
   */
  att[attIndex].file.storage_type = esp_mail_file_storage_type_sd;

  /* Need to be base64 transfer encoding for inline image */
  att[attIndex].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /** The orange.png file is already base64 encoded file.
   * Then set the content encoding to match the transfer encoding
   * which no encoding was taken place prior to sending.
   */
  att[attIndex].descr.content_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add inline image to the message */
  message.addInlineImage(att[attIndex]);

  /** Set the attachment info e.g.
   * file name, MIME type, file path, file storage type,
   * transfer encoding and content encoding
   */

  attIndex++;
  att[attIndex].descr.filename = F("bin1.dat");
  att[attIndex].descr.mime = F("application/octet-stream"); // binary data
  att[attIndex].descr.description = F("This is binary data");
  att[attIndex].file.path = F("/bin1.dat");
  att[attIndex].file.storage_type = esp_mail_file_storage_type_sd;
  att[attIndex].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add attachment to the message */
  message.addAttachment(att[attIndex]);

  /* Connect to the server */
  if (!smtp.connect(&session /* session credentials */))
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
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

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