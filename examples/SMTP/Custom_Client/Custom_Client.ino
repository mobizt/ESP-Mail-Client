

/**
 * This example shows how to send Email using custom Clients.
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


#if defined(ARDUINO_ARCH_SAMD)
#include <WiFiNINA.h>
#endif

#include <ESP_Mail_Client.h>

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
#define SMTP_HOST "smtp.gmail.com"

/** The smtp port e.g. 
 * 25  or esp_mail_smtp_port_25
 * 465 or esp_mail_smtp_port_465
 * 587 or esp_mail_smtp_port_587
*/
#define SMTP_PORT esp_mail_smtp_port_465 //port 465 is not available for Outlook.com

/* The log in credentials */
#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

/* Define the Client object */
WiFiSSLClient client;

/* The SMTP Session object used for Email sending */
SMTPSession smtp(&client); // or assign the Client later with smtp.setClient(&client);

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

void networkConnection()
{
    // Reset the network connection
    WiFi.disconnect();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
        if (millis() - ms >= 5000)
        {
            Serial.println(" failed!");
            return;
        }
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

// Define the callback function to handle server status acknowledgement
void networkStatusRequestCallback()
{
    // Set the network status
    smtp.setNetworkStatus(WiFi.status() == WL_CONNECTED);
}

//Define the callback function to handle server connection
void connectionRequestCallback(const char *host, int port)
{
    // You may need to set the system timestamp in case of custom client
    // time is used to set the date header while sending email.
    smtp.setSystemTime(WiFi.getTime());

    Serial.print("> U: Connecting to server via custom Client... ");
    if (!client.connect(host, port))
    {
        Serial.println("failed.");
        return;
    }
    Serial.println("success.");
}

//Define the callback function to handle server connection upgrade.
//This required when connect to port 587 which requires TLS
void connectionUpgradeRequestCallback()
{
    Serial.println("> U: Upgrad the connection...");

    //Required for SMTP on port 587.

    //Connection upgrade code here...

    //The most client library does not allow user to upgrade the existing connection to secure mode since it
    //was connected to server in non-secure mode.

    //You may need to edit the clients sources to make this.
}

void setup()
{

    Serial.begin(115200);

#if defined(ARDUINO_ARCH_SAMD)
    while (!Serial)
        ;
#endif

    Serial.println();

    networkConnection();

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

    
    /* Declare the message class */
    SMTP_Message message;

    /* Set the message headers */
    message.sender.name = F("ESP Mail");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Test sending plain text Email");
    message.addRecipient(F("Someone"), F("change_this@your_mail_dot_com"));

    String textMsg = "This is simple plain text message";
    message.text.content = "hiiiiii";

    /** The Plain text message character set e.g.
   * us-ascii
   * utf-8
   * utf-7
   * The default value is utf-8
  */
    message.text.charSet = F("us-ascii");

    /** The content transfer encoding e.g.
   * enc_7bit or "7bit" (not encoded)
   * enc_qp or "quoted-printable" (encoded)
   * enc_base64 or "base64" (encoded)
   * enc_binary or "binary" (not encoded)
   * enc_8bit or "8bit" (not encoded)
   * The default value is "7bit"
  */
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    //If this is a reply message
    //message.in_reply_to = "<parent message id>";
    //message.references = "<parent references> <parent message id>";

    /** The message priority
   * esp_mail_smtp_priority_high or 1
   * esp_mail_smtp_priority_normal or 3
   * esp_mail_smtp_priority_low or 5
   * The default value is esp_mail_smtp_priority_low
  */
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;

    //message.response.reply_to = "someone@somemail.com";
    //message.response.return_path = "someone@somemail.com";

    /** The Delivery Status Notifications e.g.
   * esp_mail_smtp_notify_never
   * esp_mail_smtp_notify_success
   * esp_mail_smtp_notify_failure
   * esp_mail_smtp_notify_delay
   * The default value is esp_mail_smtp_notify_never
  */
    //message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

    /* Set the custom message header */
    message.addHeader(F("Message-ID: <abcde.fghij@gmail.com>"));

    //For Root CA certificate verification (ESP8266 and ESP32 only)
    //session.certificate.cert_data = rootCACert;
    //or
    //session.certificate.cert_file = "/path/to/der/file";
    //session.certificate.cert_file_storage_type = esp_mail_file_storage_type_flash; // esp_mail_file_storage_type_sd
    //session.certificate.verify = true;

    //The WiFiNINA firmware the Root CA certification can be added via the option in Firmware update tool in Arduino IDE

    //Set the callback functions to hadle the required tasks.
    smtp.connectionRequestCallback(connectionRequestCallback);

    smtp.connectionUpgradeRequestCallback(connectionUpgradeRequestCallback);

    smtp.networkConnectionRequestCallback(networkConnection);

    smtp.networkStatusRequestCallback(networkStatusRequestCallback);

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
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
        }
        Serial.println("----------------\n");

        //You need to clear sending result as the memory usage will grow up.
        smtp.sendingResult.clear();
    }
}
