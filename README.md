# Mail Client Arduino Library v1.5.3


[![Join the chat at https://gitter.im/mobizt/ESP_Mail_Client](https://badges.gitter.im/mobizt/ESP_Mail_Client.svg)](https://gitter.im/mobizt/ESP_Mail_Client?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)


The first, complete and secure Arduino EMail Client library for Espressif's ESP32 and ESP8266, Atmel's SAMD21 devices with u-blox NINA-W102 WiFi/Bluetooth module to send and read Email through the SMTP and IMAP servers.

This library allows sending and reading Email with various attachments supported and provides more reliable and flexibilities of usages.

The library was tested and work well with ESP32s, ESP8266s, SAMD21s based modules.

This library was developed to replace the deprecated ESP32 Mail Client library with more options and features, better reliability and also conforms to the RFC standards.


This library has built-in WiFi client and aim to be complete Email client that can send and read Email with no restrictions and no indirect Email proxy (Email sending server) services needed.

Other serial Mobile network modem (GSM/3G/4G) and SPI/I2C Ethernet board which GSM or Ethernet Plain/SSL TCP client libraries are available, are not compattible to integrate to use with this library because network upgradable was not supported in those libraries. 

This library supports sending and receiving Email via Ethernet in ESP32 and ESP8266 devices using Ethernet module through secure (SSL/TLS) and non-secure ports.


![ESP Mail](/media/images/esp-mail-client.svg)

Copyright (c) 2021 K. Suwatchai (Mobizt).




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
* Support full debuging.
* Support flash memory (ESP32 and ESP8266), SD and SD_MMC (ESP32) for file storages which can be changed in [**ESP_Mail_FS.h**](/src/ESP_Mail_FS.h).
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

 Note: Arduino UNO WiFi Rev.2 (AVR Platform) is not supported.





## Tested Devices

This following devices were tested.

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





## Prerequisites

### ESP32 and ESP8266

For Espressif's ESP32 and ESP8266 based boards, this library requires Arduino's ESP32 or ESP8266 Core SDK to be installed.

The latest Core SDK is recommended. For ESP8266, the Core SDK version 3.x.x or later is recommended.

The ESP8266 Core SDK version 2.5.x and earlier are not supported.

### SAMD21

For Atmel's SAMD21 based boards, [custom built WiFiNINA firmware](https://github.com/mobizt/nina-fw) is needed to be installed instead of official Arduino WiFiNINA firmware.

#### Comparison between custom built and official WiFiNINA firmwares.

| Options | Custom Built Firmware | Arduino Official Firmware |
| ------------- | ------------- | ------------- |
| Plain connection via port 25, 143  | Yes  | Yes  |
| Secure (SSL) connection via port 465, 993  | Yes  | Yes  |
| Upgradable (STARTTLS) via port 25, 587, 143  | Yes  | No  |
| Require Email Server Root Certificate installation  | Optional, not required by default  | *Yes  |
| Require WiFiNINA library installation  | No (already built-in)  | Yes  |

*Require root certificate of Email server which is available in Arduino IDE's WiFi101 /WiFiNINA Firmware Updater tool.


### Install Custom Built WiFiNINA Firmware


To install custom built WiFiNINA firmware, please follow the following instructions.

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


If the custom built WiFiNINA firmware was installed, the debug message will show the library version with WiFiNINA firmware version which followed by built number (+21060).

```
> C: ESP Mail Client v1.5.3, Fw v1.4.5+21060
```

## Library Instalation


Click on **Clone or download** dropdown at the top of repository, select **Download ZIP** and save file on your computer.

From Arduino IDE, goto menu **Sketch** -> **Include Library** -> **Add .ZIP Library...** and choose **ESP-Mail-Client-master.zip** that previously downloaded.

Go to menu **Files** -> **Examples** -> **ESP-Mail-Client-master** and choose one from examples






## IDE Configuaration for ESP8266 MMU - Adjust the Ratio of ICACHE to IRAM

### Arduino IDE

When you update the ESP8266 Arduino Core SDK to v3.0.0, the memory can be configurable from Arduino IDE board settings.

By default MMU **option 1** was selected, the free Heap can be low and may not suitable for the SSL client usage in this library.

To increase the Heap, choose the MMU **option 3**, 16KB cache + 48KB IRAM and 2nd Heap (shared).

![Arduino IDE config](/media/images/ArduinoIDE.png)


More about MMU settings.
https://arduino-esp8266.readthedocs.io/en/latest/mmu.html

### PlatformIO IDE

When Core SDK v3.0.0 becomes available in PlatformIO,

By default the balanced ratio (32KB cache + 32KB IRAM) configuration is used.

To increase the heap, **PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48_SECHEAP_SHARED** build flag should be assigned in platformio.ini.

At the time of writing, to update SDK to v3.0.0 you can follow these steps.

1. In platformio.ini, edit the config as the following

```ini
[env:d1_mini]
platform = https://github.com/platformio/platform-espressif8266.git
build_flags = -D PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48_SECHEAP_SHARED
board = d1_mini
framework = arduino
monitor_speed = 115200
```

2. Delete this folder **C:\Users\UserName\\.platformio\platforms\espressif8266@src-?????????????**
3. Delete .pio and .vscode folders in your project.
4. Clean and Compile the project.



The supportedd MMU build flags in PlatformIO.

- **PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48**

   16KB cache + 48KB IRAM (IRAM)

- **PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM48_SECHEAP_SHARED**

   16KB cache + 48KB IRAM and 2nd Heap (shared)

- **PIO_FRAMEWORK_ARDUINO_MMU_CACHE16_IRAM32_SECHEAP_NOTSHARED**

   16KB cache + 32KB IRAM + 16KB 2nd Heap (not shared)

- **PIO_FRAMEWORK_ARDUINO_MMU_EXTERNAL_128K**

   128K External 23LC1024

- **PIO_FRAMEWORK_ARDUINO_MMU_EXTERNAL_1024K**

   1M External 64 MBit PSRAM

- **PIO_FRAMEWORK_ARDUINO_MMU_CUSTOM**

   Disables default configuration and expects user-specified flags

   
### Test code for MMU

```cpp

#include <Arduino.h>
#include <umm_malloc/umm_heap_select.h>

void setup() 
{
  Serial.begin(74880);
  HeapSelectIram ephemeral;
  Serial.printf("IRAM free: %6d bytes\r\n", ESP.getFreeHeap());
  {
    HeapSelectDram ephemeral;
    Serial.printf("DRAM free: %6d bytes\r\n", ESP.getFreeHeap());
  }
}

void loop() {
  // put your main code here, to run repeatedly:
}

```





## Usage


See [Full Examples](/examples) for complete usages.

See [Function Description](/src/README.md) for all available functions.


The following examples showed the minimum usage which many options are not configured.

The examples in the examples folder provide the full options usages.

## Notes

The string in the function's parameters or properties of structured data is the pointer to constant char or char array.

You need to assign the string literal or char array or pointer to constant char to it.

#### Ex.

```cpp
message.sender.name = "My Mail";
message.sender.email = "sender or your Email address";
```

Or using String class

```cpp
String name = "John";
String email = "john@mail.com";

message.sender.name = name.c_str();
message.sender.email = email.c_str();
```




### Send the Email


```C++

// Include ESP Mail Client library (this library)
#include <ESP_Mail_Client.h>


// Define the SMTP Session object which used for SMTP transsport
SMTPSession smtp;

// Define the session config data which used to store the TCP session configuration
ESP_Mail_Session session;

// Set the session config
session.server.host_name = "smtp.office365.com"; //for outlook.com
session.server.port = 587;
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

//Base64 data of image
const char *greenImg = "iVBORw0KGgoAAAANSUhEUgAAAGQAAABkCAYAAABw4pVUAAAAoUlEQVR42u3RAQ0AMAgAoJviyWxtAtNYwzmoQGT/eqwRQoQgRAhChCBECEKECBGCECEIEYIQIQgRghCECEGIEIQIQYgQhCBECEKEIEQIQoQgBCFCECIEIUIQIgQhCBGCECEIEYIQIQhBiBCECEGIEIQIQQhChCBECEKEIEQIQhAiBCFCECIEIUIQghAhCBGCECEIEYIQIUKEIEQIQoQg5LoBBaDPbQYiMoMAAAAASUVORK5CYII=";

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


```


### Read the Email


```C++

// Include ESP Mail Client library (this library)
#include <ESP_Mail_Client.h>


// Define the IMAP Session object which used for IMAP transsport
IMAP_Config config;


// Define the session config data which used to store the TCP session configuration
ESP_Mail_Session session;

// Set the session config
session.server.host_name = "outlook.office365.com"; //for outlook.com
session.server.port = 993;
session.login.email = "your Email address";
session.login.password = "your Email password";

// Define the config class variable for searching or fetching operation and store the messsagess data
IMAP_Config config;

// Define the message UID which required to fetch or read the message
config.fetch.uid = "100";

// Define the empty search criteria to disable the messsage search
config.search.criteria = "";

// Set to enable the message content which will be stored in the IMAP_Config data
config.enable.html = true;
config.enable.text = true;


// Connect to the server with the defined session and options
imap.connect(&session, &config);

// Open or select the mailbox folder to read the message
imap.selectFolder("INBOX");


// Read the Email and close the session
MailClient.readMail(&imap);


// Get the message(s) list
IMAP_MSG_List msgList = imap.data();

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

```



## License

The MIT License (MIT)

Copyright (c) 2021 K. Suwatchai (Mobizt)


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
