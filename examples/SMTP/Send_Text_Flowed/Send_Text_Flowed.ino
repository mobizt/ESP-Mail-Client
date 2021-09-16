

/**
 * This example will send the Email in plain text version
 * with the quoted text and long line text.
 * 
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

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <ESP_Mail_Client.h>

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

    /** ########################################################
     * Some properties of SMTPSession data and parameters pass to 
     * SMTP_Message class accept the pointer to constant char
     * i.e. const char*. 
     * 
     * You may assign a string literal to that properties or function 
     * like below example.
     *   
     * session.login.user_domain = "mydomain.net";
     * session.login.user_domain = String("mydomain.net").c_str();
     * 
     * or
     * 
     * String doman = "mydomain.net";
     * session.login.user_domain = domain.c_str();
     * 
     * And
     * 
     * String name = "Jack " + String("dawson");
     * String email = "jack_dawson" + String(123) + "@mail.com";
     * 
     * message.addRecipient(name.c_str(), email.c_str());
     * 
     * message.addHeader(String("Message-ID: <abcde.fghij@gmail.com>").c_str());
     * 
     * or
     * 
     * String header = "Message-ID: <abcde.fghij@gmail.com>";
     * message.addHeader(header.c_str());
     * 
     * ###########################################################
    */

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
    message.subject = "Test sending flowed plain text Email";
    message.addRecipient("Someone", "####@#####_dot_com");

    /** The option to add soft line break to to the message for
     * the long text message > 78 characters (rfc 3676)
     * Some Servers may not compliant with the standard.
    */
    message.text.flowed = true;

    /** if the option message.text.flowed is true,
     * the following plain text message will be wrapped.
    */
    message.text.content = "The text below is the long quoted text which breaks into several lines.\r\n\r\n>> Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.\r\n\r\nThis is the normal short text.\r\n\r\nAnother long text, abcdefg hijklmnop qrstuv wxyz abcdefg hijklmnop qrstuv wxyz abcdefg hijklmnop qrstuv wxyz.";

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

    /** The Delivery Status Notifications e.g.
     * esp_mail_smtp_notify_never
     * esp_mail_smtp_notify_success
     * esp_mail_smtp_notify_failure
     * esp_mail_smtp_notify_delay
     * The default value is esp_mail_smtp_notify_never
    */
    //message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

    /* Set the custom message header */
    message.addHeader("Message-ID: <abcde.fghij@gmail.com>");

    /* Connect to server with the session config */
    if (!smtp.connect(&session))
        return;

    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
        Serial.println("Error sending Email, " + smtp.errorReason());

    //to clear sending result log
    //smtp.sendingResult.clear();

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
