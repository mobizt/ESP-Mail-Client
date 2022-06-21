![ESP Mail](https://raw.githubusercontent.com/mobizt/ESP-Mail-Client/master/media/images/esp-mail-client.svg)


![Compile](https://github.com/mobizt/ESP-Mail-Client/actions/workflows/compile_library.yml/badge.svg) ![Examples](https://github.com/mobizt/ESP-Mail-Client/actions/workflows/compile_examples.yml/badge.svg) [![Github Stars](https://img.shields.io/github/stars/mobizt/ESP-Mail-Client?logo=github)](https://github.com/mobizt/ESP-Mail-Client/stargazers) ![Github Issues](https://img.shields.io/github/issues/mobizt/ESP-Mail-Client?logo=github)

![arduino-library-badge](https://www.ardu-badge.com/badge/ESP%20Mail%20Client.svg) ![PlatformIO](https://badges.registry.platformio.org/packages/mobizt/library/ESP%20Mail%20Client.svg)


Arduino E-Mail Client Library to send, read and get incoming email notification for ESP32, ESP8266 and SAMD21 devices. The library also supported other Arduino devices using Clients interfaces e.g. WiFiClient, EthernetClient, and GSMClient.

This library allows sending and reading Email with various attachments supported and provides more reliable and flexibilities of usages.

The library was tested and works well with ESP32s, ESP8266s, SAMD21s based modules.


This library has built-in WiFi client and aim to be full functionality Email client that can send, read and get Email notification without other indirect Email proxy services needed. 


External Arduino Client can be used which this allows other devices (with minimum 80k flash space) to work with this library.



# Features

* Support Espressif's ESP32 and ESP8266, Atmel's SAMD21 devices with u-blox NINA-W102 WiFi/Bluetooth module.
* Support TCP session reusage.
* Support PLAIN, LOGIN and XOAUTH2 authentication mechanisms.
* Support secured (with SSL and TLS or upgrade via STARTTLS) and non-secure ports.
* Support mailbox selection for Email reading and searching (IMAP).
* Support mailbox changes notification (IMAP).
* Support the content encodings e.g. quoted-printable and base64.
* Support the content decodings e.g. base64, UTF-8, UTF-7, quoted-printable, ISO-8859-1 (latin1) and ISO-8859-11 (Thai).
* Support embedded contents e.g. inline images, attachments, parallel media attachments and RFC822 message.
* Support IMAP MIME data stream callback for external reader.
* support IMAP custom character decoding callback based on the character set.
* Support custom commands for both IMAP and SMTP.
* Support full debuging.
* Support flash memory (ESP32 and ESP8266), SD, SdFat and SD_MMC (ESP32) for file storages which can be changed in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h).
* Support Ethernet (ESP32 using LAN8720, TLK110 and IP101 Ethernet modules, and ESP8266 (Arduino Core SDK v3.x.x and later) using ENC28J60, W5100 and W5500 Ethernet modules).
* Customizable configurations (see the examples for the usages)





## Supported Devices

This following devices are supported.

 * ESP8266 MCUs based boards
 * ESP32 MCUs based boards
 * Arduino MKR WiFi 1010
 * Arduino Nano 33 IoT
 * Arduino MKR Vidor 4000
 * LAN8720 Ethernet PHY
 * TLK110 Ethernet PHY
 * IP101 Ethernet PHY
 * ENC28J60 SPI Ethernet module
 * W5100 SPI Ethernet module
 * W5500 SPI Ethernet module


## Support other Arduino devices using external Clients.

Since version 2.0.0, library allows you to use custom (external) Arduino Clients interfaces e.g. WiFiClient, EthernetClient and GSMClient, the devices that support C++ and have enough flash (> 80k) and RAM can now use this library.

With external Clients, you needed to set the callback functions to hanle the the server connection and connection upgrade tasks.

Some ports e.g. 587 for SMTP and 143 for IMAP require the TLS then the external Clients should be able to upgrade the connection from existing plain (non-secure) to secure connection and some SMTP port e.g. 25 may require upgrade too.

This connection upgrade process is generally not available from general Arduino Clients and you need to make your custom client version that supports connection upgrade.

See [Use external Arduino Clients interfaces](#use-external-arduino-clients-interfaces) section for how to use external Clients.


## Tested Devices

### This following devices were tested.

 * Sparkfun ESP32 Thing
 * NodeMCU-32
 * WEMOS LOLIN32
 * TTGO T8 V1.8
 * M5Stack ESP32
 * NodeMCU ESP8266
 * Wemos D1 Mini (ESP8266)
 * Arduino MKR WiFi 1010
 * LAN8720 Ethernet PHY
 * ENC28J60 SPI Ethernet module

### Supposted Devices with flash size > 80k, using custom Clients.

 * ESP32
 * ESP8266
 * Arduino SAMD
 * Arduino STM32
 * Arduino AVR
 * Teensy 3.1 to 4.1
 * Arduino Nano RP2040 Connect
 * Raspberry Pi Pico 

 ### Gmail SMTP and IMAP required App Passwords to sign in

From May 30, 2022, Google no longer supports the use of third-party apps or devices which ask you to sign in to your  GoogleAccount using only your username and password.

This means the Gmail account credentials i.e. account Email and account password can't be used to sign in with Google SMTP andIMAP servers. This prevents the third party apps hack to Gmail user account.

To use Gmail with this library, you need to use App Passwords instead.

For setting up the App Passwords, please read [here](https://support.google.com/accounts/answer/185833).

After you created App Password, you can use Gmail Email address and App Password created to sign in as the following.

```cpp
session.login.email = "<your email>";
session.login.password = "<your app password>";
```
 




## Prerequisites

This section is for the built-in Client to update the Core SDK or install the firmware for full functionality supports.

### Third party SD library must be removed

In Arduino IDE, all third party SD libraries installed in libraries folder **MUST BE UNINSTALLED**.

The Core SD library was used instead of third party SD libraries.

### ESP32 and ESP8266

For Espressif's ESP32 and ESP8266 based boards, this library requires Arduino's ESP32 or ESP8266 Core SDK to be installed.

The latest Core SDK is recommended. For ESP8266, the Core SDK version 3.x.x or later is recommended.

The ESP8266 Core SDK version 2.5.x and earlier are not supported.

### SAMD21

For Atmel's SAMD21 based boards, [custom build WiFiNINA firmware](https://github.com/mobizt/nina-fw) is needed to be installed instead of official Arduino WiFiNINA firmware.

This requirement is optional and has more advantages over the standard Arduino WiFiNINA firmware.

#### Comparison between custom build and official WiFiNINA firmwares.

| Options | Custom Build Firmware | Arduino Official Firmware |
| ------------- | ------------- | ------------- |
| Plain connection via port 25, 143  | Yes  | Yes  |
| Secure (SSL) connection via port 465, 993  | Yes  | Yes  |
| Upgradable (STARTTLS) via port 25, 587, 143  | Yes  | No  |
| Require Email Server Root Certificate installation  | Optional, not required by default  | *Yes  |
| Require WiFiNINA library installation  | No (already built-in)  | Yes  |

*Require root certificate of Email server which is available in Arduino IDE's WiFi101 /WiFiNINA Firmware Updater tool.


### Install Custom Build WiFiNINA Firmware


To install custom build WiFiNINA firmware, please follow the following instructions.

1. Install flash tool, esptool.py from [here](https://github.com/espressif/esptool). To instal esptool python script, you will need either [Python 2.7 or Python 3.4 or newer](https://www.python.org/downloads/) installed on your system.


2. Download [nina-fw.bin](/firmwares/samd21) in [firmwares/samd21](/firmwares/samd21) and [SerialNINAPassthrough.ino](/firmwares/SerialNINAPassthrough/SerialNINAPassthrough.ino).


3. Compile and upload SerialNINAPassthrough.ino to the device.

![SerialNINAPassthrough.ino](/media/images/SerialNINAPassthrough.png)


4. Open the terminal program (Linux and macOS) or command prompt (Windows), and type this command.


```
esptool.py --port <com-port> --baud 115200 --before default_reset --after hard_reset write_flash 0x0 <path/to/file>/nina-fw.bin
```


From command above, replace ```<com-port>``` with your comport e.g. COM5 (Windows) or /dev/ttyUSB0 (Linux and macOS) and also replace ```<path/to/file>``` with your path to the nina-fw.bin that has been extracted.

The flash (upload) result shows in the command prompt window will look similar to below image.

![esptool command](/media/images/esptool.png)


If the custom build WiFiNINA firmware was installed, the debug message will show the library version with WiFiNINA firmware version which followed by build number (+21120).

```
> C: ESP Mail Client v2.x.x, Fw v1.4.8+21120
```

## Library Instalation


### Using Library Manager

At Arduino IDE, go to menu **Sketch** -> **Include Library** -> **Manage Libraries...**

In Library Manager Window, search **"ESP Mail Client"** in the search form then select **"ESP Mail Client"**. 

Click **"Install"** button.



For PlatformIO IDE.

Go to **PIO Home** -> **Libraries** -> **Registry** then search **ESP Mail Client**.


If you ever installed this library in Global storage in PlatformIO version prior to v2.0.0 and you have updated the PlatformIO to v2.0.0 and later, the global library installation was not available, the sources files of old library version still be able to search by the library dependency finder (LDF), you needed to remove the library from folder **C:\Users\\<UserName\>\\.platformio\lib** to prevent unexpected behavior when compile and run.



### Manual installation


Click on **Code** dropdown at the top of repository, select **Download ZIP** and save file on your computer.

From Arduino IDE, goto menu **Sketch** -> **Include Library** -> **Add .ZIP Library...** and choose **ESP-Mail-Client-master.zip** that previously downloaded.

Rename **ESP-Mail-Client-master** folder to **ESP_Mail_Client**.

Go to menu **Files** -> **Examples** -> **ESP Mail Client** and choose one from examples







## Memory Options for ESP8266

This section is optional for memory settings in IDE.

When you update the ESP8266 Arduino Core SDK to v3.0.0, the memory can be configurable from IDE.

You can choose the Heap memory between internal and external memory chip from IDE e.g. Arduino IDE and PlatformIO on VSCode or Atom IDE.

### Arduino IDE


For ESP8266 devices that don't have external SRAM/PSRAM chip installed, choose the MMU **option 3**, 16KB cache + 48KB IRAM and 2nd Heap (shared).

![Arduino IDE config](/media/images/ArduinoIDE.png)

For ESP8266 devices that have external 23LC1024 SRAM chip installed, choose the MMU **option 5**, 128K External 23LC1024.

![MMU VM 128K](/media/images/ESP8266_VM.png)

For ESP8266 devices that have external ESP-PSRAM64 chip installed, choose the MMU **option 6**, 1M External 64 MBit PSRAM.


### PlatformIO IDE

The MMU options can be selected from build_flags in your project's platformio.ini file

For ESP8266 devices that don't not have external SRAM/PSRAM chip installed, add build flag as below.

```ini
[env:d1_mini]
platform = espressif8266
build_flags = -D PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48_SECHEAP_SHARED
board = d1_mini
framework = arduino
monitor_speed = 115200
```


For ESP8266 devices that have external 23LC1024 SRAM chip installed, add build flag as below.

```ini
[env:d1_mini]
platform = espressif8266
;128K External 23LC1024
build_flags = -D PIO_FRAMEWORK_ARDUINO_MMU_EXTERNAL_128K
board = d1_mini
framework = arduino
monitor_speed = 115200
```


For ESP8266 devices that have external ESP-PSRAM64 chip installed, add build flag as below.

```ini
[env:d1_mini]
platform = espressif8266
;1M External 64 MBit PSRAM
build_flags = -D PIO_FRAMEWORK_ARDUINO_MMU_EXTERNAL_1024K
board = d1_mini
framework = arduino
monitor_speed = 115200
```


### ESP8266 and SRAM/PSRAM Chip connection

Most ESP8266 modules don't have the built-in SRAM/PSRAM on board. External memory chip connection can be done via SPI port as below.

```
23LC1024/ESP-PSRAM64                ESP8266

CS (Pin 1)                          GPIO15
SCK (Pin 6)                         GPIO14
MOSI (Pin 5)                        GPIO13
MISO (Pin 2)                        GPIO12
/HOLD (Pin 7 on 23LC1024 only)      3V3
Vcc (Pin 8)                         3V3
Vcc (Pin 4)                         GND
```

Once the external Heap memory was selected in IDE, to allow the library to use the external memory, you can set it in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h) by define this macro.


```cpp
#define ESP_MAIL_USE_PSRAM
```

This macro was defined by default when you installed or update the library.



## Memory Options for ESP32

This section is optional for memory settings in IDE.

In ESP32 module that has PSRAM installed, you can enable it and set the library to use this external memory instead.

### Arduino IDE

To enable PSRAM in ESP32 module.

![Enable PSRAM in ESP32](/media/images/ESP32-PSRAM.png)


### PlatformIO IDE


In PlatformIO on VSCode or Atom IDE, add the following build_flags in your project's platformio.ini file.

```ini
build_flags = -DBOARD_HAS_PSRAM -mfix-esp32-psram-cache-issue
```

As in ESP8266, once the external Heap memory was enabled in IDE, to allow the library to use the external memory, you can set it in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h) by define this macro.

```cpp
#define ESP_MAIL_USE_PSRAM
```







## Exclude unused classes to save memory 

Now you can compile the library only for seclected classes.

In [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h), the IMAP and SMTP class can be enabled with the macros.

```cpp
#define ENABLE_IMAP

#define ENABLE_SMTP
```


In ESP8266 and ESP32, when no attachments require for uploading and downloading, the storage file systems libraries e.g. SD or SD_MMC (ESP32), SPIFFS and LittleFS will no longer use and can be excluded when compiling the code to reduce program flash size, by comment the following macros to exclude them.

```cpp
#define ESP_MAIL_DEFAULT_SD_FS SD

#define ESP_Mail_DEFAULT_FLASH_FS SPIFFS
```





## Usage


See [examples folder](/examples) for all usage examples.

See [src/README.md](/src/README.md) for the functions descriptions.

The usefull blogs that described how to send and read E-mail in detail can be found here.

[ESP32 Send Emails using an SMTP Server: HTML, Text, and Attachments (Arduino IDE) by Rui and Sara from randomnerdtutorials.com](https://randomnerdtutorials.com/esp32-send-email-smtp-server-arduino-ide/)

[Receiving Emails with ESP32 using IMAP Server by Alina Mybeth from theengineeringprojects.com](https://www.theengineeringprojects.com/2022/01/receiving-emails-with-esp32-using-imap-server.html)


The following code snippet showed the minimum usage of the library.



### Send Email

The following code will send email with image attachment.

```C++

// Include ESP Mail Client library (this library)
#include <ESP_Mail_Client.h>


// Define the global used SMTP Session object which used for SMTP transsport
SMTPSession smtp;

// Define the global used session config data which used to store the TCP session configuration
ESP_Mail_Session session;

void setup()
{

  Serial.begin(115200);

  WiFi.begin("<ssid>", "<password>");
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Set the session config
  session.server.host_name = "smtp.office365.com"; // for outlook.com
  session.server.port = 587;
  session.login.email = "your Email address"; // set to empty for no SMTP Authentication
  session.login.password = "your Email password"; // set to empty for no SMTP Authentication
  session.login.user_domain = "client domain or ip e.g. mydomain.com";

  // Set the NTP config time
  session.time.ntp_server = "pool.ntp.org,time.nist.gov";
  session.time.gmt_offset = 3;
  session.time.day_light_offset = 0;

  // Define the SMTP_Message class variable to handle to message being transport
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

  //Base64 data of image
  const char *greenImg = "iVBORw0KGgoAAAANSUhEUgAAAGQAAABkCAYAAABw4pVUAAAAoUlEQVR42u"
                         "3RAQ0AMAgAoJviyWxtAtNYwzmoQGT/eqwRQoQgRAhChCBECEKECBGCECEI"
                         "EYIQIQgRghCECEGIEIQIQYgQhCBECEKEIEQIQoQgBCFCECIEIUIQIgQhCB"
                         "GCECEIEYIQIQhBiBCECEGIEIQIQQhChCBECEKEIEQIQhAiBCFCECIEIUIQ"
                         "ghAhCBGCECEIEYIQIUKEIEQIQoQg5LoBBaDPbQYiMoMAAAAASUVORK5CYII=";

  // Define the attachment data
  SMTP_Attachment att;

  // Set the attatchment info
  att.descr.filename = "green.png";
  att.descr.mime = "image/png";
  att.blob.data = (uint8_t *)greenImg;
  att.blob.size = strlen(greenImg);
  // Set the transfer encoding to base64
  att.descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  // We set the content encoding to match the above greenImage data
  att.descr.content_encoding = Content_Transfer_Encoding::enc_base64;

  // Add attachment to the message
  message.addAttachment(att);

  // Connect to server with the session config
  smtp.connect(&session);

  // Start sending Email and close the session
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());

}


```


### Read Email

The following code will read the latest email.

```C++

// Include ESP Mail Client library (this library)
#include <ESP_Mail_Client.h>


// Define the global used IMAP Session object which used for IMAP transsport
IMAP_Config config;


// Define the global used session config data which used to store the TCP session configuration
ESP_Mail_Session session;


void setup()
{

  Serial.begin(115200);

  WiFi.begin("<ssid>", "<password>");
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  // Set the session config
  session.server.host_name = "outlook.office365.com"; //for outlook.com
  session.server.port = 993;
  session.login.email = "your Email address";
  session.login.password = "your Email password";

  // Define the config class variable for searching or fetching operation and store the messsagess data
  IMAP_Config config;

  
  // Set to enable the message content which will be stored in the IMAP_Config data
  config.enable.html = true;
  config.enable.text = true;


  // Connect to the server with the defined session and options
  imap.connect(&session, &config);

  // Open or select the mailbox folder to read the message
  imap.selectFolder("INBOX");


  // Define the message UID (number) which required to fetch or read the message
  // In this case we will get the UID from the max message number (lastest message)
  // then imap.getUID and imap.selectedFolder().msgCount() should be called after 
  // calling select or open the folder (mailbox).
  config.fetch.uid = imap.getUID(imap.selectedFolder().msgCount());

  // Define the empty search criteria to disable the messsage search
  config.search.criteria.clear();


  // Read the Email and close the session
  MailClient.readMail(&imap);


  // Get the message(s) list
  IMAP_MSG_List msgList = imap.data();

  // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
  // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
  // In ESP32 and ESP32, you can use Serial.printf directly.

  for (size_t i = 0; i < msgList.msgItems.size(); i++)
  {
    // Iterate to get each message data through the message item data
    IMAP_MSG_Item msg = msgList.msgItems[i];

    Serial.println("################################");
    ESP_MAIL_PRINTF("Messsage Number: %s\n", msg.msgNo);
    ESP_MAIL_PRINTF("Messsage UID: %s\n", msg.UID);
    ESP_MAIL_PRINTF("Messsage ID: %s\n", msg.ID);
    ESP_MAIL_PRINTF("Accept Language: %s\n", msg.acceptLang);
    ESP_MAIL_PRINTF("Content Language: %s\n", msg.contentLang);
    ESP_MAIL_PRINTF("From: %s\n", msg.from);
    ESP_MAIL_PRINTF("From Charset: %s\n", msg.fromCharset);
    ESP_MAIL_PRINTF("To: %s\n", msg.to);
    ESP_MAIL_PRINTF("To Charset: %s\n", msg.toCharset);
    ESP_MAIL_PRINTF("CC: %s\n", msg.cc);
    ESP_MAIL_PRINTF("CC Charset: %s\n", msg.ccCharset);
    ESP_MAIL_PRINTF("Date: %s\n", msg.date);
    ESP_MAIL_PRINTF("Subject: %s\n", msg.subject);
    ESP_MAIL_PRINTF("Subject Charset: %s\n", msg.subjectCharset);

    // If the message body is available
    if (!imap.headerOnly())
    {
      ESP_MAIL_PRINTF("Text Message: %s\n", msg.text.content);
      ESP_MAIL_PRINTF("Text Message Charset: %s\n", msg.text.charSet);
      ESP_MAIL_PRINTF("Text Message Transfer Encoding: %s\n", msg.text.transfer_encoding);
      ESP_MAIL_PRINTF("HTML Message: %s\n", msg.html.content);
      ESP_MAIL_PRINTF("HTML Message Charset: %s\n", msg.html.charSet);
      ESP_MAIL_PRINTF("HTML Message Transfer Encoding: %s\n\n", msg.html.transfer_encoding);
    }
  }

}

```


### Get Incoming Message Notification and Reading

See [Mailbox_Changes_Notification.ino](/examples/IMAP/Mailbox_Changes_Notification/Mailbox_Changes_Notification.ino) for the example.


### Sending Custom IMAP commands

Can't find what you want from exising IMAP functions, sending custom command was supported.

Please read the RFC 3501 and RFC 9051 documents for the details of IMAP protocol commands.

See [Custom_Command.ino](/examples/IMAP/Custom_Command/Custom_Command.ino) for the example.


### Use external Arduino Clients interfaces

The Arduino Clients for network interfaces (WiFiClient, EthernetClient and GSMClient) which support non-secure network connection can be used as external client.


By default, the built-in Clients will be used when you compile the library for devices e.g. ESP32, ESP8266 and SAMD21 with built-in U-blox NINA-W102 module and custom (external) Client will be used for other Arduino compatible devices 

You can change from built-in Clients to external Clients in case of ESP32, ESP8266 and SAMD21 with NINA-W102 by define the following macro in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h).

```cpp
#define ENABLE_CUSTOM_CLIENT
```

Arduino Clients e.g. WiFiClient, EthernetClient and GSMClient provided the basic connection to server without SSL.  You can use these clients as external client in this library unless they work only for SMTP port 25 and IMAP port 143.

Some Email servers require the connection upgrade before authentication.


#### Arduino Clients via TLS

The `STARTTLS` command will be sent when the device connected to the **SMTP Server via port 587** and the TLS is required before log in.

This required the Client to upgrade from non-secure connection to secure connection after STARTTLS command sent.

You can use the [upgradable SSLClient](https://github.com/mobizt/SSLClient) that can handle this upgrade process.

When the connection upgrades needed, the callback function assigned to function `connectionUpgradeRequestCallback` will be called which you can place the connection upgrade code inside this callback function.

This is the example code show how to use `GSMClient` and [SSLClient](https://github.com/mobizt/SSLClient) to connect to SMTP server via port 587 which required connection upgrade.


```cpp

GSMClient client; // basic non-secure client

/** The parameters passed to the SSLClient constructor
 * TAs is Trust anchors used in the verification of the SSL server certificate. 
 * Check out https://github.com/mobizt/SSLClient/blob/master/TrustAnchors.md for more info.
 * TAs_NUM is the number of objects in the trust_anchors array.
 * rand_pin is an analog pin to pull random bytes from, used in seeding the RNG.
 */
// https://github.com/mobizt/SSLClient
SSLClient ssl_client(client, TAs, (size_t)TAs_NUM, rand_pin); 

SMTPSession smtp(&ssl_client); 

void connectionUpgradeRequestCallback()
{

    //To make sure that upgradable SSLClient https://github.com/mobizt/SSLClient was installed instead of
    // the original version
#if defined(SSLCLIENT_CONNECTION_UPGRADABLE)

    // Upgrade the connection
    // The host and port parameters will be ignored for this case and can be any
    ssl_client.connectSSL("" /* host */, 0 /* port */);
    
#endif
}

void connectionRequestCallback(const char *host, int port)
{
    client.connect(host, port)
}

void serup()
{

    session.server.host_name = "smtp.gmail.com"; //for gmail.com
    session.server.port = 587; // requires connection upgrade via STARTTLS
    session.login.email = "your Email address"; //set to empty for no SMTP Authentication
    session.login.password = "your Email password"; //set to empty for no SMTP Authentication
    session.login.user_domain = "client domain or ip e.g. mydomain.com";

    /**
     * Other setup codes
     * 
     */

    // Set the callback function for server connection.
    smtp.connectionRequestCallback(connectionRequestCallback);
    
    // Set the callback function for connection upgrade
    smtp.connectionUpgradeRequestCallback(connectionUpgradeRequestCallback);

}


```



For Arduino Nano RP2040 Connect board, using PlatformIO IDE, to prevent the compile error due to wrong WiFi compilation, please set the lib_ldf_mode in platformio.ini as this.

```ini
lib_ldf_mode = chain+
```


The below example will use Arduino MKR 1000 and set WiFi101 for SSLClient.


The example will send message using Gmail via port 465 which is already using SSL (no connection upgrade required), then you need to add Gmail server cetificate to the board using Arduino IDE's WiFi101/WiFiNINA Firmware Updater tool.


For SSL ports, e.g. SMTP port 465 and IMAP port 993, you can use SSL ready client e.g. WiFiClientSecure and SSLClient to connect to these ports normally wich no connection upgrade code required unless port 587 on SMTP server.


See [Custom_Client.ino](/examples/SMTP/Custom_Client/Custom_Client.ino) for complete Client example.


```cpp

#include <WiFi101.h>

// Define the global used Client object
WiFiSSLClient ssl_client; // secured client

// Define the global used smtp object 
SMTPSession smtp(&ssl_client); // or assign the Client later with smtp.setClient(&ssl_client);

// Since we use WiFiSSLClient that supported SSL and no basic non-secure client can be pass to its constructor,
// only SMTP port 465 works in the following code.

// Define the global used session config data which used to store the TCP session configuration
ESP_Mail_Session session;

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

// Define the callback function to handle server connection
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

void setup()
{

  Serial.begin(115200);

  networkConnection();

  // Set the session config
  session.server.host_name = "smtp.gmail.com"; //for gmail.com
  session.server.port = 465; // SSL 
  session.login.email = "your Email address"; //set to empty for no SMTP Authentication
  session.login.password = "your Email password"; //set to empty for no SMTP Authentication
  session.login.user_domain = "client domain or ip e.g. mydomain.com";


  // Define the SMTP_Message class variable to handle to message being transport
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
  
  // Set the callback functions to hadle the required tasks.
  smtp.connectionRequestCallback(connectionRequestCallback);

  smtp.networkConnectionRequestCallback(networkConnection);

  smtp.networkStatusRequestCallback(networkStatusRequestCallback);


  // Connect to the server with the defined session and options
  imap.connect(&session, &config);


  // Start sending Email and close the session
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());


}


```

## License

The MIT License (MIT)

Copyright (c) 2022 K. Suwatchai (Mobizt)


Permission is hereby granted, free of charge, to any person returning a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.