# ESP-Mail-Client

The complete and secured Mail Client for ESP32 and ESP8266 for sending and reading the E-mail through the SMTP and IMAP servers.

This library is the upgraded version of ESP32 Mail Client library to support ESP8266.

# Features

- Support ESP32 and ESP8266.
- Customizable configurations using the structured data type.
- Support reuse session for continuous Email sending and reading without re-sign in.
- Support normal Email and password authentication and OAuth2.0 access token authentication for SMTP and IMAP.
- Support secured connection SSL and TLS with STARTTLS command.
- Support port 25, 465 and 587 for SMTP and port 143 and 993 for IMAP
- Able to select the specific mailbox folder for Email reading and searching.
- Support UTF8 (international headers), UTF-7 (encoded) and quoted printable (encoded) data decoding for both header and body in Email reading.
- Support ISO-8859-1 (latin1)  and ISO-8859-11 (Thai) decoding for body contents in Email reading.
- Able to add inline image (embedded attachment) for Email sending.
- Support plain or base64 attachments for Email sending.
- Progress report for searching and fetching Email, upload and download attachment operations during Email sending.
- Full debug for SMTP and IMAP.

# Status
This repository is currently in update progress, if you're interested, please stay tune.
