

/**
 * This example shows how to send Email with inline images from OV2640 camera.
 *
 * The html and text version messages will be sent.
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

// To use send Email for Gmail to port 465 (SSL), less secure app option should be enabled. https://myaccount.google.com/lesssecureapps?pli=1

// The file systems for flash and sd memory can be changed in ESP_Mail_FS.h.

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#endif

#include <ESP_Mail_Client.h>

// The OV2640 library
#if defined(ESP32)
#include "cam/OV2640.h"
#endif

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

/* Define the OV2640 object */
#if defined(ESP32)
OV2640 cam;
#endif

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

void initCam();

void setup()
{
    Serial.begin(115200);

#if defined(ESP32)

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

    /*  Set the network reconnection option */
    MailClient.networkReconnect(true);

    initCam();

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

    message.subject = F("Test sending camera image");
    message.addRecipient(F("user1"), F("change_this@your_mail_dot_com"));

    message.html.content = F("<span style=\"color:#ff0000;\">The camera image.</span><br/><br/><img src=\"cid:image-001\" alt=\"esp32 cam image\"  width=\"800\" height=\"600\">");

    /** The content transfer encoding e.g.
     * enc_7bit or "7bit" (not encoded)
     * enc_qp or "quoted-printable" (encoded) <- not supported for message from blob and file
     * enc_base64 or "base64" (encoded)
     * enc_binary or "binary" (not encoded)
     * enc_8bit or "8bit" (not encoded)
     * The default value is "7bit"
     */
    message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    /** The HTML text message character set e.g.
     * us-ascii
     * utf-8
     * utf-7
     * The default value is utf-8
     */
    message.html.charSet = F("utf-8");

    SMTP_Attachment att;

    /** Set the inline image info e.g.
     * file name, MIME type, file path, file storage type,
     * transfer encoding and content encoding
     */
    att.descr.filename = F("camera.jpg");
    att.descr.mime = F("image/jpg");

    att.blob.data = cam.getfb();
    att.blob.size = cam.getSize();

    att.descr.content_id = F("image-001"); // The content id (cid) of camera.jpg image in the src tag

    /* Need to be base64 transfer encoding for inline image */
    att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

    /* Add inline image to the message */
    message.addInlineImage(att);

    /* Connect to server with the session config */
    if (!smtp.connect(&session))
        return;

    /* Start sending the Email and close the session */
    if (!MailClient.sendMail(&smtp, &message, true))
        Serial.println("Error sending Email, " + smtp.errorReason());

    // to clear sending result log
    // smtp.sendingResult.clear();

    ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());

#endif
}

void loop()
{
}

void initCam()
{

#if defined(ESP32)

    camera_config_t camera_config;

    /** For M5Stack M5Cam - ESP32 Camera (OV2640)
     * Change to match your pin configuration between OV2640 Camera and ESP32 connection
     */
    camera_config.ledc_channel = LEDC_CHANNEL_0;
    camera_config.ledc_timer = LEDC_TIMER_0;
    camera_config.pin_d0 = 17;
    camera_config.pin_d1 = 35;
    camera_config.pin_d2 = 34;
    camera_config.pin_d3 = 5;
    camera_config.pin_d4 = 39;
    camera_config.pin_d5 = 18;
    camera_config.pin_d6 = 36;
    camera_config.pin_d7 = 19;
    camera_config.pin_xclk = 27;
    camera_config.pin_pclk = 21;
    camera_config.pin_vsync = 22;
    camera_config.pin_href = 26;
    camera_config.pin_sscb_sda = 25;
    camera_config.pin_sscb_scl = 23;
    camera_config.pin_reset = 15;
    camera_config.xclk_freq_hz = 20000000;

    camera_config.pixel_format = CAMERA_PF_JPEG;
    camera_config.frame_size = CAMERA_FS_SVGA;

    cam.init(camera_config);

    delay(100);

    cam.run();
#endif
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