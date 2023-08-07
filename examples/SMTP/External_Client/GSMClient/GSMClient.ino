

/**
 * This example shows how to send Email using TinyGSMClient.
 *
 * This example used TTGO T-A7670 (ESP32 with SIMCom SIMA7670) and TinyGSMClient.
 *
 * ///////////////////////////////////////////////////////////////
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

//To allow TinyGSM library integration, the following macro should be defined in src/ESP_Mail_FS.h.
// #define TINY_GSM_MODEM_SIM7600


#define TINY_GSM_MODEM_SIM7600 // SIMA7670 Compatible with SIM7600 AT instructions

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

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

#define UART_BAUD 115200
#define PIN_DTR 25
#define PIN_TX 26
#define PIN_RX 27
#define PWR_PIN 4
#define BAT_ADC 35
#define BAT_EN 12
#define PIN_RI 33
#define PIN_DTR 25
#define RESET 5

#define SD_MISO 2
#define SD_MOSI 15
#define SD_SCLK 14
#define SD_CS 13

#include <ESP_Mail_Client.h>

#include <TinyGsmClient.h>


// Set serial for debug console
#define SerialMon Serial

// Set serial for AT commands (to the module)
#define SerialAT Serial1

TinyGsm modem(SerialAT);

TinyGsmClient gsm_client(modem); // basic non-secure client


SMTPSession smtp; 


void initModem()
{

    if(modem.isGprsConnected())
    {
      modem.gprsDisconnect();
      SerialMon.println(F("GPRS disconnected"));
    }

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
        return;
    }

}

void setup()
{

    SerialMon.begin(115200);

    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

    String name = modem.getModemName();
    DBG("Modem Name:", name);

    String modemInfo = modem.getModemInfo();
    DBG("Modem Info:", modemInfo);

    initModem();

    config.server.host_name = "smtp.gmail.com"; //for gmail.com
    config.server.port = 587; // requires connection upgrade via STARTTLS
    config.login.email = "your Email address"; //set to empty for no SMTP Authentication
    config.login.password = "your Email password"; //set to empty for no SMTP Authentication
    config.login.user_domain = "client domain or ip e.g. mydomain.com";

    // Declare the SMTP_Message class variable to handle to message being transport
    SMTP_Message message;

    // Set the message headers
    message.sender.name = "My Mail";
    message.sender.email = "sender or your Email address";
    message.subject = "Test sending Email";
    message.addRecipient("name1", "email1");
    message.addRecipient("name2", "email2");

    message.addCc("email3");
    message.addBcc("email4");

    // Set the message content
    message.text.content = "This is simple plain text message";


    smtp.setGSMClient(&gsm_client, &modem, GSM_PIN, apn, gprsUser, gprsPass);
  
    // Connect to the server with the defined session and options
    smtp.connect(&config);

    // Start sending Email and close the session
    if (!MailClient.sendMail(&smtp, &message))
      Serial.println("Error sending Email, " + smtp.errorReason());

}