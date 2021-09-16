

/**
 * This example will send the Email with inline images stored in flash memory.
 * The message content stores as HTML data in flash memory
 * 
 * The html and text version messages will be sent.
 * 
 * Created by K. Suwatchai (Mobizt)
 * 
 * Email: suwatchai@outlook.com
 * 
 * Github: https://github.com/mobizt/ESP-Mail-Client
 * 
 * Copyright (c) 2021 mobizt
 *
*/

//To use send Email for Gmail to port 465 (SSL), less secure app option should be enabled. https://myaccount.google.com/lesssecureapps?pli=1

//The file systems for flash and sd memory can be changed in ESP_Mail_FS.h.

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#endif
#include <ESP_Mail_Client.h>

//The OV2640 library
#if defined(ESP32)
#include "cam/OV2640.h"
#endif

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com
 * For yahoo mail, log in to your yahoo mail in web browser and generate app password by go to
 * https://login.yahoo.com/account/security/app-passwords/add/confirm?src=noSrc
 * and use the app password as password with your yahoo mail account to login.
 * The google app password signin is also available https://support.google.com/mail/answer/185833?hl=en
*/
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

    initCam();

    /** Enable the debug via Serial port
     * none debug or 0
     * basic debug or 1
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
    session.login.user_domain = "mydomain.net";

    /* Declare the message class */
    SMTP_Message message;

    /* Enable the chunked data transfer with pipelining for large message if server supported */
    message.enable.chunking = true;

    /* Set the message headers */
    message.sender.name = "ESP Mail";
    message.sender.email = AUTHOR_EMAIL;

    message.subject = "Test sending camera image";
    message.addRecipient("user1", "####@#####_dot_com");

    message.html.content = "<span style=\"color:#ff0000;\">The camera image.</span><br/><br/><img src=\"cid:image-001\" alt=\"esp32 cam image\"  width=\"800\" height=\"600\">";

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
    message.html.charSet = "utf-8";

    SMTP_Attachment att;

    /** Set the inline image info e.g.
     * file name, MIME type, file path, file storage type,
     * transfer encoding and content encoding
    */
    att.descr.filename = "camera.jpg";
    att.descr.mime = "image/jpg";

    att.blob.data = cam.getfb();
    att.blob.size = cam.getSize();

    att.descr.content_id = "image-001"; //The content id (cid) of camera.jpg image in the src tag

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

    //to clear sending result log
    //smtp.sendingResult.clear();

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
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients);
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject);
        }
        Serial.println("----------------\n");
        
        //You need to clear sending result as the memory usage will grow up as it keeps the status, timstamp and
        //pointer to const char of recipients and subject that user assigned to the SMTP_Message object.

        //Because of pointer to const char that stores instead of dynamic string, the subject and recipients value can be
        //a garbage string (pointer points to undefind location) as SMTP_Message was declared as local variable or the value changed.

        //smtp.sendingResult.clear();
    }
}
