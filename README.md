![ESP Mail](https://raw.githubusercontent.com/mobizt/ESP-Mail-Client/master/media/images/esp-mail-client.svg)


![Compile](https://github.com/mobizt/ESP-Mail-Client/actions/workflows/compile_library.yml/badge.svg) ![Examples](https://github.com/mobizt/ESP-Mail-Client/actions/workflows/compile_examples.yml/badge.svg) [![Github Stars](https://img.shields.io/github/stars/mobizt/ESP-Mail-Client?logo=github)](https://github.com/mobizt/ESP-Mail-Client/stargazers) ![Github Issues](https://img.shields.io/github/issues/mobizt/ESP-Mail-Client?logo=github)

![arduino-library-badge](https://www.ardu-badge.com/badge/ESP%20Mail%20Client.svg) ![PlatformIO](https://badges.registry.platformio.org/packages/mobizt/library/ESP%20Mail%20Client.svg)


The comprehensive Arduino Email Client Library to send and read Email for Arduino devices. 

This library was tested and works well with ESP32s, ESP8266s, SAMD21s and RP2040 Pico based modules.

The library supported external networking devices hat work with Arduino Clients e.g. WiFiClient, EthernetClient, and GSMClient.

Minimum 128k flash space device is recommended for library installation and user code.

The minimum ram requirement is based on the applications (SMTP and IMAP). IMAP application may required up to 20k memory while SMTP application required much less memory.


## Contents


[1. Features](#features)

[2. Supported Devices](#supported-devices)

[3. Prerequisites](#prerequisites)
- [Gmail SMTP and IMAP required App Passwords to sign in](#gmail-smtp-and-imap-required-app-passwords-to-sign-in)

- [PlatformIO IDE Compile Options](#platformio-ide-compile-options)

- [Third party SD library must be removed](#third-party-sd-library-must-be-removed)

- [SdFat conflicts in ESP8266 and must be removed](#sdfat-conflicts-in-esp8266-and-must-be-removed)

- [ESP32 and ESP8266 SDKs](#esp32-and-esp8266-sdks)

- [RP2040 Arduino SDK](#rp2040-arduino-sdk)

[4. Library Instalation](#library-instalation)

- [Using Library Manager](#using-library-manager)

- [Manual installation](#manual-installation)

[5. Memory Options](#memory-options)

- [Memory Options for ESP8266](#memory-options-for-esp8266)

  - [Arduino IDE](#arduino-ide)

  - [PlatformIO IDE](#platformio-ide)

  - [ESP8266 and SRAM/PSRAM Chip connection](#esp8266-and-srampsram-chip-connection)

- [Memory Options for ESP32](#memory-options-for-esp32)

  - [Arduino IDE](#arduino-ide-1)

  - [PlatformIO IDE](#platformio-ide-1)


[6. Exclude unused classes to save program space](#exclude-unused-classes-to-save-program-space)


[7. Usage](#usage)

- [Send Email](#send-email)

- [Read Email](#read-email)

- [Get Incoming Message Notification and Reading](#get-incoming-message-notification-and-reading)

- [Sending Custom IMAP commands](#sending-custom-imap-commands)

- [Using TCP session KeepAlive in ESP8266 and ESP32](#using-tcp-session-keepalive-in-esp8266-and-esp32)

- [Use external Arduino Clients interfaces](#use-external-arduino-clients-interfaces)

  - [TTGO T-A7670 LTE with TinyGSM](#ttgo-t-a7670-lte-with-tinygsm)

  - [ESP32 and W5500](#esp32-and-w5500)

[8. License](#license)



## Features

* Supports sending Email with attachments.
* Supports reading the message and listening the mailbox changes.
* Supports custom SMTP and IMAP commands.
* Supports PLAIN, LOGIN and XOAUTH2 authentication mechanisms.
* Supports standard ports and user defined ports.
* Supports STARTTLS for both SMTP and IMAP.
* Supports TCP session reusage.
* Supports the content encodings e.g. quoted-printable and base64.
* Supports the content decodings e.g. base64, UTF-8, UTF-7, quoted-printable, ISO-8859-1 (latin1) and ISO-8859-11 (Thai).
* Supports embedded contents e.g. inline images, attachments, parallel media attachments and RFC822 message.
* Supports IMAP MIME data stream callback for external reader.
* supports IMAP custom character decoding callback based on the character set.
* Support full debuging.
* Support on-board or native networking (WiFi and Ethernet) and external networking (WiFi, Ethernet and GSM) via external basic WiFiClient, EthernetClient and GSMClient.
* Supports TinyGSM library integration.




## Supported Devices

This following devices are supported.

 * ESP8266 MCUs based boards
 * ESP32 MCUs based boards
 * Arduino MKR WiFi 1010
 * Arduino MKR 1000 WIFI
 * Arduino Nano 33 IoT
 * Arduino MKR Vidor 4000
 * Raspberry Pi Pico (RP2040)
 * Arduino UNO R4 WiFi (Renesas).
 * LAN8720 Ethernet PHY
 * TLK110 Ethernet PHY
 * IP101 Ethernet PHY
 * ENC28J60 SPI Ethernet module
 * W5100 SPI Ethernet module
 * W5500 SPI Ethernet module
 * SIMCom Modules with TinyGSMClient



## Prerequisites


### Gmail SMTP and IMAP required App Passwords to sign in

From May 30, 2022, Google no longer supports the use of third-party apps or devices which ask you to sign in to your  GoogleAccount using only your username and password.

This means the Gmail account credentials i.e. account Email and account password can't be used to sign in with Google SMTP and IMAP servers. This prevents the third party apps hack to Gmail user account.

To use Gmail with this library, you need to use App Passwords instead.

For setting up the App Passwords, please read [here](https://support.google.com/accounts/answer/185833).

After you created App Password, you can use Gmail Email address and App Password created to sign in as the following.

```cpp
config.login.email = "<your email>";
config.login.password = "<your app password>";
```
 

### PlatformIO IDE Compile Options

For Arduino Nano RP2040 Connect board, using PlatformIO IDE, to prevent the compile error due to wrong headers compilation, please set the lib_ldf_mode in platformio.ini as this.

```ini
lib_ldf_mode = chain+
```

### Third party SD library must be removed

In Arduino IDE, all third party SD libraries installed in libraries folder must be reboved.

The Core SD library was used instead of third party SD libraries.

### SdFat conflicts in ESP8266 and must be removed

The [SdFat](https://github.com/greiman/SdFat) is already implemented as wrapper class in ESP8266 core library.

For Arduino IDE, the SdFat library should be removed from libraries folder when you compile this library for ESP8266 because of conclicts with core library SDFS.h.


### ESP32 and ESP8266 SDKs

For Espressif's ESP32 and ESP8266 based boards, this library requires Arduino's ESP32 or ESP8266 Core SDK to be installed.

The latest Core SDK is recommended. For ESP8266, the Core SDK version 3.x.x or later is recommended.

The ESP8266 Core SDK version 2.5.x and earlier are not supported.

For ESP32, the Core SDK version 2.0.4 or later is recommended.

The ESP32 Core SDK version 1.0.4 and earlier are not supported.


### RP2040 Arduino SDK

For Arduino IDE, the Arduino-Pico SDK can be installed from Boards Manager by searching pico and choose Raspberry Pi Pico/RP2040 to install.

For PlatformIO, the Arduino-Pico SDK can be installed via platformio.ini

```ini
[env:rpipicow]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = rpipicow
framework = arduino
board_build.core = earlephilhower
monitor_speed = 115200
board_build.filesystem_size = 1m
```

See this Arduino-Pico SDK [documentation](https://arduino-pico.readthedocs.io/en/latest/) for more information.


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



## Memory Options


### Memory Options for ESP8266

This section is optional for memory settings in IDE.

When you update the ESP8266 Arduino Core SDK to v3.0.0, the memory can be configurable from IDE.

You can choose the Heap memory between internal and external memory chip from IDE e.g. Arduino IDE and PlatformIO on VSCode or Atom IDE.

#### Arduino IDE


For ESP8266 devices that don't have external SRAM/PSRAM chip installed, choose the MMU **option 3**, 16KB cache + 48KB IRAM and 2nd Heap (shared).

![Arduino IDE config](/media/images/ArduinoIDE.png)

For ESP8266 devices that have external 23LC1024 SRAM chip installed, choose the MMU **option 5**, 128K External 23LC1024.

![MMU VM 128K](/media/images/ESP8266_VM.png)

For ESP8266 devices that have external ESP-PSRAM64 chip installed, choose the MMU **option 6**, 1M External 64 MBit PSRAM.


#### PlatformIO IDE

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


#### ESP8266 and SRAM/PSRAM Chip connection

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



### Memory Options for ESP32

This section is optional for memory settings in IDE.

In ESP32 module that has PSRAM installed, you can enable it and set the library to use this external memory instead.

#### Arduino IDE

To enable PSRAM in ESP32 module.

![Enable PSRAM in ESP32](/media/images/ESP32-PSRAM.png)


#### PlatformIO IDE


In PlatformIO on VSCode or Atom IDE, add the following build_flags in your project's platformio.ini file.

```ini
build_flags = -DBOARD_HAS_PSRAM -mfix-esp32-psram-cache-issue
```

As in ESP8266, once the external Heap memory was enabled in IDE, to allow the library to use the external memory, you can set it in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h) by define this macro.

```cpp
#define ESP_MAIL_USE_PSRAM
```



## Exclude unused classes to save program space 

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

In case you want to set your device/library time manually, you can exclude the internal NTP time reading by comment this macro that defined in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h).

```cpp
#define ENABLE_NTP_TIME
```

To set the time manually, please see [**examples/SMTP/Set_Time/Set_Time.ino**](examples/SMTP/Set_Time/Set_Time.ino)


In case you don't want to print any debug and error in debug port and completely exclude all flash string that used for debug and error, please define SILENT_MODE macro in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h).

```cpp
#define SILENT_MODE
```

In case you only want to exclude the error flash string from library, please comment this macro that defined in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h).


```cpp
#define ENABLE_ERROR_STRING
```

Since this library is not a single header library, the macro defined before the library inclusion in user sketch file will not be used or seen by all library codes in all source files due to different File Scope.

Then your modified version of [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h) need to be back up before update the library because it will be overwritten when update the library.

Alternatively, by leaving config in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h) as its default setting and creat your own config file
 Custom_ESP_Mail_FS.h in the same folder as [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h) and undefine the macro that already defined in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h).

When the library updated, the config file [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h) will be overwritten or replaced unless your Custom_ESP_Mail_FS.h will not be replaced and stayed there. 


The following is the example of Custom_ESP_Mail_FS.h

```cpp
#ifndef ESP_MAIL_CUSTOM_FS_H_
#define ESP_MAIL_CUSTOM_FS_H_

// Not use SD
#undef ESP_MAIL_DEFAULT_SD_FS 

// Not use Flash
#undef ESP_MAIL_DEFAULT_FLASH_FS

// Not use NTP time (required device time should set by user)
#undef ENABLE_NTP_TIME

// Not show error string (show only error code)
#undef ENABLE_ERROR_STRING

// Not use SMTP
#undef ENABLE_SMTP 

// Not print all debug and callback
#define SILENT_MODE

#endif
```



## Usage


See [examples folder](/examples) for all usage examples.

See [src/README.md](/src/README.md) for the functions descriptions.

The usefull blogs that described how to send and read E-mail in detail can be found here.

[ESP32 Send Emails using an SMTP Server: HTML, Text, and Attachments (Arduino IDE) by Rui and Sara from randomnerdtutorials.com](https://randomnerdtutorials.com/esp32-send-email-smtp-server-arduino-ide/)

[Receiving Emails with ESP32 using IMAP Server by Alina Mybeth from theengineeringprojects.com](https://www.theengineeringprojects.com/2022/01/receiving-emails-with-esp32-using-imap-server.html)


The following code snippet showed the minimum usage of the library.



### Send Email message

The following code will send email with image attachment.

```C++
// Include WiFi library
#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif  __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#endif

// Include ESP Mail Client library (this library)
#include <ESP_Mail_Client.h>


// Declare the global used SMTPSession object for SMTP transport
SMTPSession smtp;

// Declare the global used Session_Config for user defined session credentials
Session_Config config;

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
  config.server.host_name = "smtp.office365.com"; // for outlook.com
  config.server.port = 587; // for TLS with STARTTLS or 25 (Plain/TLS with STARTTLS) or 465 (SSL)
  config.login.email = "your Email address"; // set to empty for no SMTP Authentication
  config.login.password = "your Email password"; // set to empty for no SMTP Authentication
  
  // For client identity, assign invalid string can cause server rejection
  config.login.user_domain = "client domain or public ip only e.g. mydomain.com";  

  /*
   Set the NTP config time
   For times east of the Prime Meridian use 0-12
   For times west of the Prime Meridian add 12 to the offset.
   Ex. American/Denver GMT would be -6. 6 + 12 = 18
   See https://en.wikipedia.org/wiki/Time_zone for a list of the GMT/UTC timezone offsets
   */
  config.time.ntp_server = "pool.ntp.org,time.nist.gov";
  config.time.gmt_offset = 3;
  config.time.day_light_offset = 0;

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

  //Base64 data of image
  const char *greenImg = "iVBORw0KGgoAAAANSUhEUgAAAGQAAABkCAYAAABw4pVUAAAAoUlEQVR42u"
                         "3RAQ0AMAgAoJviyWxtAtNYwzmoQGT/eqwRQoQgRAhChCBECEKECBGCECEI"
                         "EYIQIQgRghCECEGIEIQIQYgQhCBECEKEIEQIQoQgBCFCECIEIUIQIgQhCB"
                         "GCECEIEYIQIQhBiBCECEGIEIQIQQhChCBECEKEIEQIQhAiBCFCECIEIUIQ"
                         "ghAhCBGCECEIEYIQIUKEIEQIQoQg5LoBBaDPbQYiMoMAAAAASUVORK5CYII=";

  // Declare the attachment data
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

  // Connect to the server
  smtp.connect(&config);

  // Start sending Email and close the session
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());

}


```


### Read Email message

The following code will read the latest email message in the "INBOX" mailbox.

```C++
// Include WiFi library
#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#elif  __has_include(<WiFiNINA.h>)
#include <WiFiNINA.h>
#elif __has_include(<WiFi101.h>)
#include <WiFi101.h>
#endif

// Include ESP Mail Client library (this library)
#include <ESP_Mail_Client.h>

// Declare the global used IMAPSession object for IMAP transport
IMAPSession imap;

// Declare the global used Session_Config for user defined session credentials
Session_Config config;


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
  config.server.host_name = "outlook.office365.com"; //for outlook.com
  config.server.port = 993; // for SSL or 143 for Plain or TLS with STARTTLS
  config.login.email = "your Email address";
  config.login.password = "your Email password";

  // Declare the IMAP_Data object used for user defined IMAP operating options 
  // and contains the IMAP operating result
  IMAP_Data imap_data;

  
  // Set to enable the message content which will be stored in the IMAP_Data data
  imap_data.enable.html = true;
  imap_data.enable.text = true;


  // Connect to the server
  imap.connect(&config, &imap_data);

  // Open or select the mailbox folder to read the message
  imap.selectFolder("INBOX");


  // Define the message UID (number) which required to fetch or read the message
  // In this case we will get the UID from the max message number (lastest message)
  // then imap.getUID and imap.selectedFolder().msgCount() should be called after 
  // calling select or open the folder (mailbox).
  imap_data.fetch.uid = imap.getUID(imap.selectedFolder().msgCount());

  // Empty search criteria to disable the messsage search
  imap_data.search.criteria.clear();


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


### Get Mailbox Changes Notification

See [Mailbox_Changes_Notification.ino](/examples/IMAP/Mailbox_Changes_Notification/Mailbox_Changes_Notification.ino) for the example.


### Sending Custom IMAP commands

Can't find what you want from exising IMAP functions, sending custom command was supported.

Please read the RFC 3501 and RFC 9051 documents for the details of IMAP protocol commands.

See [Custom_Command examples](/examples/IMAP/Custom_Command) for how to use.


### Using TCP session KeepAlive in ESP8266 and ESP32

The server connection will be probed at some intervals to maintain connection.

The TCP session KeepAlive can be enabled from executing `<SMTPSession>.keepAlive` or `<IMAPSession>.keepAlive` with providing TCP options as arguments, i.e.,

`tcpKeepIdleSeconds`, `tcpKeepIntervalSeconds` and `tcpKeepCount`.

Ex.

```cpp
smtp.keepAlive(5 /* tcp KeepAlive idle 5 seconds */, 5 /* tcp KeeAalive interval 5 seconds */, 1 /* tcp KeepAlive count 1 */);

imap.keepAlive(5 /* tcp KeepAlive idle 5 seconds */, 5 /* tcp KeeAalive interval 5 seconds */, 1 /* tcp KeepAlive count 1 */);

// If one of three arguments is zero, the KeepAlive will be disabled.
```

To check the KeepAlive status, use `<SMTPSession>.isKeepAlive` or `<IMAPSession>.isKeepAlive`.


For the TCP (KeepAlive) options, see [here](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/lwip.html#tcp-options).

You can check the server connecting status, by executing `<SMTPSession>.connected()` or `<IMAPSession>.connected()` which will return true when connection to the server is still alive. 


The TCP KeepAlive was currently available in ESP32 unless in ESP8266, [this ESP8266 PR #8940](https://github.com/esp8266/Arduino/pull/8940) should be merged in the [ESP8266 Arduino Core SDK](https://github.com/esp8266/Arduino/releases), i.e., it will be supported in the ESP8266 core version newer than v3.1.2.


In ESP8266 core v3.1.2 and older, the error can be occurred when executing `<SMTPSession>.keepAlive` or `<IMAPSession>.isKeepAlive` because of object slicing.


The Arduino Pico is currently not support TCP KeepAlive until it's implemented in WiFiClientSecure library as in ESP8266.

 
For External Client, this TCP KeepAlive option is not appliable and should be managed by external Client library.




### Use External Arduino Clients for External Networking Devices

This library supports external netwoking devices e.g. WiFi modules, Ethernet modules and GSM modules that connected to the Arduino device via communication ports e.g. SPI and Serial, through the Arduino Clients (driver) for those networking devices e.g. WiFiClient, EthernetClient and GSMClient.

Since v3.4.0, the Arduino Clients can be used with this library without additional external SSL Client required.

No additional setup needed, only pass the Arduino Client to the function `setClient` or pass the TinyGSMClient and TinyGSM modem to the function `setGSMClient`.

Two callback functions required (except for using `setGSMClient`) for network connection (with disconnection) and sending connecting status back to the Mail Client.

If device has on-board WiFi and supports native (SDK) Ethernet, these two native networks will be auto detectd and used.

If you don't want to let Mail Client library to use the native networking and use external networking devices using Arduino Clients instead, the following macro should be defined in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h).

```cpp
#define ESP_MAIL_DISABLE_ONBOARD_WIFI


#define ESP_MAIL_DISABLE_NATIVE_ETHERNET
```

See [External Client Examples](/examples/SMTP/External_Client) for more external Client usage.


#### TTGO T-A7670 LTE with TinyGSM

The following example showed how to use TTGO T-A7670 with `GSMClient` to connect to SMTP server.

To allow TinyGSM library integration, the following macro should be defined in [**ESP_Mail_FS.h**](src/ESP_Mail_FS.h).

```cpp
#define TINY_GSM_MODEM_SIM7600
```

See the TinyGSM documentation and example for other SIMCom modules definition.


```cpp

// For TTGO T-A7670
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


```

#### ESP32 and W5500

The below example will use ESP32 and W5500 and Ethernet client library to connect to SMTP server


```cpp

#include <Ethernet.h>

#include <ESP_Mail_Client.h>

#define WIZNET_RESET_PIN 26 // Connect W5500 Reset pin to GPIO 26 of ESP32
#define WIZNET_CS_PIN 5     // Connect W5500 CS pin to GPIO 5 of ESP32
#define WIZNET_MISO_PIN 19  // Connect W5500 MISO pin to GPIO 19 of ESP32
#define WIZNET_MOSI_PIN 23  // Connect W5500 MOSI pin to GPIO 23 of ESP32
#define WIZNET_SCLK_PIN 18  // Connect W5500 SCLK pin to GPIO 18 of ESP32


EthernetClient eth_client;

uint8_t Eth_MAC[] = {0x02, 0xF0, 0x0D, 0xBE, 0xEF, 0x01};

SMTPSession smtp; 

Session_Config config;

void ResetEthernet()
{
    Serial.println("Resetting WIZnet W5500 Ethernet Board...  ");
    pinMode(WIZNET_RESET_PIN, OUTPUT);
    digitalWrite(WIZNET_RESET_PIN, HIGH);
    delay(200);
    digitalWrite(WIZNET_RESET_PIN, LOW);
    delay(50);
    digitalWrite(WIZNET_RESET_PIN, HIGH);
    delay(200);
}

void networkConnection()
{

    Ethernet.init(WIZNET_CS_PIN);

    ResetEthernet();

    Serial.println("Starting Ethernet connection...");
    Ethernet.begin(Eth_MAC);

    unsigned long to = millis();

    while (Ethernet.linkStatus() == LinkOFF || millis() - to < 2000)
    {
        delay(100);
    }

    if (Ethernet.linkStatus() == LinkON)
    {
        Serial.print("Connected with IP ");
        Serial.println(Ethernet.localIP());
    }
    else
    {
        Serial.println("Can't connect");
    }
}

void networkStatusRequestCallback()
{

    smtp.setNetworkStatus(Ethernet.linkStatus() == LinkON);
}

void serup()
{
    Serial.begin(115200);

    networkConnection();

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

     // Set the callback function for connection upgrade
    smtp.networkStatusRequestCallback(networkStatusRequestCallback);

    smtp.networkConnectionRequestCallback(networkConnection);

    smtp.setClient(&eth_client);

    // Connect to the server with the defined session and options
    smtp.connect(&config);

    // Start sending Email and close the session
    if (!MailClient.sendMail(&smtp, &message))
      Serial.println("Error sending Email, " + smtp.errorReason());
  
}


```


## License

The MIT License (MIT)

Copyright (c) 2023 K. Suwatchai (Mobizt)


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