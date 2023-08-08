/**
 * This example shows how to get, set the access control list.
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

/** For ESP8266, with BearSSL WiFi Client
 * The memory reserved for completed valid SSL response from IMAP is 16 kbytes which
 * may cause your device out of memory reset in case the memory
 * allocation error.
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

// To use only IMAP functions, you can exclude the SMTP from compilation, see ESP_Mail_FS.h.

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

/** For Gmail, IMAP option should be enabled. https://support.google.com/mail/answer/7126229?hl=en
 * and also https://accounts.google.com/b/0/DisplayUnlockCaptcha
 *
 * Some Gmail user still not able to sign in using account password even above options were set up,
 * for this case, use "App Password" to sign in instead.
 * About Gmail "App Password", go to https://support.google.com/accounts/answer/185833?hl=en
 *
 * For Yahoo mail, log in to your yahoo mail in web browser and generate app password by go to
 * https://login.yahoo.com/account/security/app-passwords/add/confirm?src=noSrc
 *
 * To use Gmai and Yahoo's App Password to sign in, define the AUTHOR_PASSWORD with your App Password
 * and AUTHOR_EMAIL with your account email.
 */

/* The imap host name e.g. imap.gmail.com for GMail or outlook.office365.com for Outlook */
#define IMAP_HOST "<host>"

/** The imap port e.g.
 * 143  or esp_mail_imap_port_143
 * 993 or esp_mail_imap_port_993
 */
#define IMAP_PORT 993

/* The log in credentials */
#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

/* Declare the global used IMAPSession object for IMAP transport */
IMAPSession imap;

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
    imap.debug(1);

    /* Declare the Session_Config for user defined session credentials */
    Session_Config config;

    /* Set the session config */
    config.server.host_name = IMAP_HOST;
    config.server.port = IMAP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;

    /* Define the IMAP_Data object used for user defined IMAP operating options. */
    IMAP_Data imap_data;

    /* Connect to the server */
    if (!imap.connect(&config, &imap_data))
        return;

    if (imap.isAuthenticated())
        Serial.println("\nSuccessfully logged in.");
    else
        Serial.println("\nConnected with no Auth.");

    IMAP_Rights_List acl_list;

    Serial.println("\nGet ACLs...");

    if (!imap.getACL("INBOX", &acl_list))
    {
        Serial.println("Get ACLs failed");
    }
    else
    {
        Serial.println("ACL...");

        for (size_t i = 0; i < acl_list.size(); i++)
        {
            ESP_MAIL_PRINTF("Identifier: %s, Rights: ", acl_list[i].identifier.c_str());
            if (acl_list[i].rights.lookup)
                Serial.print("l");
            if (acl_list[i].rights.read)
                Serial.print("r");
            if (acl_list[i].rights.seen)
                Serial.print("s");
            if (acl_list[i].rights.write)
                Serial.print("w");
            if (acl_list[i].rights.insert)
                Serial.print("i");
            if (acl_list[i].rights.post)
                Serial.print("p");
            if (acl_list[i].rights.create)
                Serial.print("k");
            if (acl_list[i].rights.create_c)
                Serial.print("c");
            if (acl_list[i].rights.delete_mailbox)
                Serial.print("x");
            if (acl_list[i].rights.delete_messages)
                Serial.print("t");
            if (acl_list[i].rights.delete_d)
                Serial.print("d");
            if (acl_list[i].rights.expunge)
                Serial.print("e");
            if (acl_list[i].rights.administer)
                Serial.print("a");
            Serial.println();
        }
    }

    IMAP_Rights_Info acl;
    acl.identifier = "Steve";
    acl.rights.administer = true;
    acl.rights.create = true;
    acl.rights.create_c = true;
    acl.rights.delete_messages = true;
    acl.rights.delete_d = true;
    acl.rights.delete_mailbox = true;
    acl.rights.expunge = true;
    acl.rights.lookup = true;
    acl.rights.insert = true;
    acl.rights.post = true;
    acl.rights.read = true;
    acl.rights.write = true;
    acl.rights.seen = true;

    Serial.println("\nSet ACLs...");

    if (!imap.setACL("INBOX", &acl))
    {
        Serial.println("Set ACLs failed");
    }
    else
    {
        Serial.println("Set ACLs success");
    }

    Serial.println("\nGet my rights...");

    if (!imap.myRights("INBOX", &acl))
    {
        Serial.println("Set my rights failed");
    }
    else
    {
        Serial.print("Rights: ");
        if (acl.rights.lookup)
            Serial.print("l");
        if (acl.rights.read)
            Serial.print("r");
        if (acl.rights.seen)
            Serial.print("s");
        if (acl.rights.write)
            Serial.print("w");
        if (acl.rights.insert)
            Serial.print("i");
        if (acl.rights.post)
            Serial.print("p");
        if (acl.rights.create)
            Serial.print("k");
        if (acl.rights.create_c)
            Serial.print("c");
        if (acl.rights.delete_mailbox)
            Serial.print("x");
        if (acl.rights.delete_messages)
            Serial.print("t");
        if (acl.rights.delete_d)
            Serial.print("d");
        if (acl.rights.expunge)
            Serial.print("e");
        if (acl.rights.administer)
            Serial.print("a");
        Serial.println();
    }

    Serial.println("\nDelete ACLs...");

    if (!imap.deleteACL("INBOX", "Steve"))
    {
        Serial.println("Set ACLs failed");
    }
    else
    {
        Serial.println("Delete ACLs success");
    }

    ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
}

void loop()
{
}
