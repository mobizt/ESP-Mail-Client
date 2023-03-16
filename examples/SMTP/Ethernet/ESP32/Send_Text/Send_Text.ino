

/**
 * This example shows how to send Email using ESP32 and LAN8720 Ethernet module.
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

/**
 * There are many sources for LAN8720 and ESP32 interconnection on the internet which may
 * work for your LAN8720 board.
 *
 * Some methods worked unless no IP is available.
 *
 * This modification and interconnection provided in this example are mostly worked as
 * the 50 MHz clock was created internally in ESP32 which GPIO 17 is set to be output of this clock
 * and feeds to the LAN8720 chip XTAL input.
 *
 * The on-board LAN8720 50 MHz XTAL chip will be disabled by connect its enable pin or pin 1 to GND.
 *
 * Please see the images in the folder "modified_LAN8720_board_images" for how to modify the LAN8720 board.
 *
 * The LAN8720 Ethernet modified board and ESP32 board wiring connection.
 *
 * ESP32                        LAN8720
 *
 * GPIO17 - EMAC_CLK_OUT_180    nINT/REFCLK - LAN8720 XTAL1/CLKIN     4k7 Pulldown
 * GPIO22 - EMAC_TXD1           TX1
 * GPIO19 - EMAC_TXD0           TX0
 * GPIO21 - EMAC_TX_EN          TX_EN
 * GPIO26 - EMAC_RXD1           RX1
 * GPIO25 - EMAC_RXD0           RX0
 * GPIO27 - EMAC_RX_DV          CRS
 * GPIO23 - MDC                 MDC
 * GPIO18 - MDIO                MDIO
 * GND                          GND
 * 3V3                          VCC
 *
 */

#include <WiFi.h>
#include <ESP_Mail_Client.h>

#ifdef ETH_CLK_MODE
#undef ETH_CLK_MODE
#endif
#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT // RMII clock output from GPIO17

// Pin# of the enable signal for the external crystal oscillator (-1 to disable)
#define ETH_POWER_PIN -1

// Type of the Ethernet PHY (LAN8720 or TLK110)
#define ETH_TYPE ETH_PHY_LAN8720

// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
#define ETH_ADDR 1

// Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_MDC_PIN 23

// Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_MDIO_PIN 18

static bool eth_connected = false;

#define SMTP_HOST "<host>"
#define SMTP_PORT 25

#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

SMTPSession smtp;

void smtpCallback(SMTP_Status status);

unsigned long sendMillis = 0;

void WiFiEvent(WiFiEvent_t event)
{
  // Do not run any function here to prevent stack overflow or nested interrupt

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)

  switch (event)
  {
  case ARDUINO_EVENT_ETH_START:
    Serial.println("ETH Started");
    // set eth hostname here
    ETH.setHostname("esp32-ethernet");
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    Serial.println("ETH Connected");
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    Serial.print("ETH MAC: ");
    Serial.print(ETH.macAddress());
    Serial.print(", IPv4: ");
    Serial.print(ETH.localIP());
    if (ETH.fullDuplex())
    {
      Serial.print(", FULL_DUPLEX");
    }
    Serial.print(", ");
    Serial.print(ETH.linkSpeed());
    Serial.println("Mbps");
    eth_connected = true;
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    Serial.println("ETH Disconnected");
    eth_connected = false;
    break;
  case ARDUINO_EVENT_ETH_STOP:
    Serial.println("ETH Stopped");
    eth_connected = false;
    break;
  default:
    break;
  }

#else
  switch (event)
  {
  case SYSTEM_EVENT_ETH_START:
    Serial.println("ETH Started");
    // set eth hostname here
    ETH.setHostname("esp32-ethernet");
    break;
  case SYSTEM_EVENT_ETH_CONNECTED:
    Serial.println("ETH Connected");
    break;
  case SYSTEM_EVENT_ETH_GOT_IP:
    Serial.print("ETH MAC: ");
    Serial.print(ETH.macAddress());
    Serial.print(", IPv4: ");
    Serial.print(ETH.localIP());
    if (ETH.fullDuplex())
    {
      Serial.print(", FULL_DUPLEX");
    }
    Serial.print(", ");
    Serial.print(ETH.linkSpeed());
    Serial.println("Mbps");
    eth_connected = true;
    break;
  case SYSTEM_EVENT_ETH_DISCONNECTED:
    Serial.println("ETH Disconnected");
    eth_connected = false;
    break;
  case SYSTEM_EVENT_ETH_STOP:
    Serial.println("ETH Stopped");
    eth_connected = false;
    break;
  default:
    break;
  }
#endif
}

void sendMail()
{

  smtp.debug(1);

  smtp.callback(smtpCallback);

  Session_Config config;

  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;

  config.login.user_domain = F("mydomain.net");

  config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

  SMTP_Message message;

  message.sender.name = F("ESP Mail");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("Test sending plain text Email");
  message.addRecipient(F("Someone"), F("change_this@your_mail_dot_com"));

  String textMsg = "This is simple plain text message";
  message.text.content = textMsg;

  if (!smtp.connect(&config))
    return;

  if (smtp.isAuthenticated())
    Serial.println("\nSuccessfully logged in.");
  else
    Serial.println("\nConnected with no Auth.");

  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  WiFi.onEvent(WiFiEvent);

  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
}

void loop()
{
  if (eth_connected && (millis() - sendMillis > 300000 || sendMillis == 0))
  {
    sendMillis = millis();
    sendMail();
  }
}

void smtpCallback(SMTP_Status status)
{
  Serial.println(status.info());

  if (status.success())
  {

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {

      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", asctime(localtime(&ts)));
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");
    smtp.sendingResult.clear();
  }
}
