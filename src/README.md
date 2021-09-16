# ESP Mail Client Arduino Library for ESP32, ESP8266, SAMD21


The description of the available functions in the current reease are shown below.


## Global functions


#### Sending Email through the SMTP server.

param **`smtp`** The pointer to SMTP session object which holds the data and the TCP client.

param **`msg`** The pointer to SMTP_Message class which contains the header, body, and attachments.

param **`closeSession`** The option to Close the SMTP session after sent.

return **`boolean`** The boolean value indicates the success of operation.

```cpp
bool sendMail(SMTPSession *smtp, SMTP_Message *msg, bool closeSession = true);
```





#### Reading Email through IMAP server.

param **`imap`** The pointer to IMAP sesssion object which holds the data and the TCP client.

param **`closeSession`** The option to close the IMAP session after fetching or searching the Email.

return **`boolean`** The boolean value indicates the success of operation.

```cpp
bool readMail(IMAPSession *imap, bool closeSession = true);
```





#### Set the argument to the Flags for the specified message.

param **`imap`** The pointer to IMAP session object which holds the data and the TCP client.

param **`msgUID`** The UID of the message.

param **`flags`** The flag list to set.

param **`closeSession`** The option to close the IMAP session after set flag.

return **`boolean`** The boolean value indicates the success of operation.

```cpp
bool setFlag(IMAPSession *imap, int msgUID, const char *flags, bool closeSession);
```





#### Add the argument to the Flags for the specified message.

param **`imap`** The pointer to IMAP session object which holds the data and the TCP client.

param **`msgUID`** The UID of the message.

param **`flags`** The flag list to set.

param **`closeSession`** The option to close the IMAP session after add flag.

return **`boolean`** The boolean value indicates the success of operation.
 
```cpp
bool addFlag(IMAPSession *imap, int msgUID, const char *flags, bool closeSession);
```






#### Remove the argument from the Flags for the specified message.

param **`imap`** The pointer to IMAP session object which holds the data and the TCP client.

param **`msgUID`** The UID of the message that flags to be removed.

param **`flags`** The flag list to remove.

param **`closeSession`** The option to close the IMAP session after remove flag.

return **`boolean`** The boolean value indicates the success of operation.

```cpp
bool removeFlag(IMAPSession *imap, int msgUID, const char *flags, bool closeSession);
```






#### Initialize the SD card with the SPI port.

param **`sck`** The SPI Clock pin (ESP32 only).

param **`miso`** The SPI MISO pin (ESSP32 only).

param **`mosi`** The SPI MOSI pin (ESP32 only).

param **`ss`** The SPI Chip/Slave Select pin (ESP32 and ESP8266).

return **`boolean`** The boolean value indicates the success of operation.

```cpp
bool sdBegin(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t ss);
```






#### Initialize the SD_MMC card (ESP32 only).

param **`mountpoint`** The mounting point.

param **`mode1bit`** Allow 1 bit data line (SPI mode).

param **`format_if_mount_failed`** Format SD_MMC card if mount failed.

return **`Boolean`** type status indicates the success of the operation.

```cpp
bool sdMMCBegin(const char *mountpoint = "/sdcard", bool mode1bit = false, bool format_if_mount_failed = false);
```




#### Get free Heap memory.

return **`int`** Free memory amount in byte.

```cpp
int getFreeHeap();
```





#### Initialize the SD card with the default SPI port.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool sdBegin(void);
```






## IMAPSession class functions


The following functions are available from the IMAP Session class.

This class used for controlling IMAP transports and retrieving the data from the IMAP server.





#### Begin the IMAP server connection.

param **`session`** The pointer to ESP_Mail_Session structured data that keeps the server and log in details.

param **`config`** The pointer to IMAP_Config structured data that keeps the operation options.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool connect(ESP_Mail_Session *session, IMAP_Config *config);
```


#### Close the IMAP session.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool closeSession();
```







#### Set to enable the debug.

param **`level`** The level to enable the debug message

level = 0, no debug

level = 1, basic debug

level = 2, full debug 1

level = 333, full debug 2

```cpp
void debug(int level);
```





#### Get the list of all the mailbox folders since the TCP session was opened and user was authenticated.

param **`folders`** The FoldersCollection class that contains the collection of the 
FolderInfo structured data.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool getFolders(FoldersCollection &folders);
```





#### Select or open the mailbox folder to search or fetch the message inside.

param **`folderName`** The known mailbox folder name. The default name is INBOX.

param **`readOnly`** The option to open the mailbox for read only. Set this option to false when you wish 
to modify the Flags using the setFlag, addFlag and removeFlag functions.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool selectFolder(const char *folderName, bool readOnly = true);
```





#### Open the mailbox folder to read or search the mesages. 

param **`folderName`** The name of known mailbox folder to be opened.

param **`readOnly`** The option to open the mailbox for reading only. Set this option to false when you wish 
to modify the flags using the setFlag, addFlag and removeFlag functions.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool openFolder(const char *folderName, bool readOnly = true);
```





#### Close the mailbox folder that was opened. 

param **`folderName`** The mailbox folder name.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool closeFolder(const char *folderName);
```






#### Create folder. 

param **`folderName`** The name of folder to create.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool createFolder(const char *folderName);
```






#### Delete folder. 

param **`folderName`** The name of folder to delete..

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool deleteFolder(const char *folderName);
```






#### Get UID number in selected or opened mailbox. 

param **`msgNum`** The message number or order in the total message numbers.

return **`boolean`** The boolean value which indicates the success of operation.

Returns 0 when fail to get UID.

```cpp
int getUID(int msgNum);
```







#### Get message flags in selected or opened mailbox. 

param **`msgNum`** The message number or order in the total message numbers.

return **`string`** Message flags in selected or opened mailbox.

empty string when fail to get flags.

```cpp
const char *getFlags(int msgNum);
```






#### Send the custom IMAP command and get the result via callback.

param **`cmd`** The command string.

param **`callback`** The function that accepts the pointer to const char (const char*) as parameter.

return **`boolean`** The boolean value which indicates the success of operation.

imap.connect and imap.selectFolder or imap.openFolder are needed to call once prior to call this function.

```cpp
bool sendCustomCommand(const char *cmd, imapResponseCallback callback);
```








#### Copy the messages to the defined mailbox folder. 

param **`toCopy`** The pointer to the MessageList class that contains the list of messages to copy.

param **`dest`** The destination folder that the messages to copy to.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool copyMessages(MessageList *toCopy, const char *dest);
```





#### Delete the messages in the opened mailbox folder. 

param **`toDelete`** The pointer to the MessageList class that contains the list of messages to delete.

param **`expunge`** The boolean option to expunge all messages.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool deleteMessages(MessageList *toDelete, bool expunge = false);
```







#### Listen for the selected or open mailbox for updates.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool listen();
```








#### Stop listen for the mailbox for updates.

return **`boolean`** The boolean value which indicates the success of operation.

```cpp
bool stopListen();
```







#### Check for the selected or open mailbox updates.

return **`boolean`** The boolean value which indicates the changes status of mailbox.

```cpp
bool folderChanged();
```






#### Assign the callback function that returns the operating status when fetching or reading the Email.

param **`imapCallback`** The function that accepts the imapStatusCallback as parameter.

```cpp
void callback(imapStatusCallback imapCallback);
```





#### Determine if no message body contained in the search result and only the message header is available.

```cpp
bool headerOnly();
```





#### Get the message list from search or fetch the Emails

return **`The IMAP_MSG_List structured`** data which contains the text and html contents, 
attachments, inline images, embedded rfc822 messages details for each message.

```cpp
IMAP_MSG_List data();
```





#### Get the details of the selected or opned mailbox folder

return **`The SelectedFolderInfo class`** instance which contains the info about flags, total messages, next UID,  
earch count and the available messages count.

```cpp
SelectedFolderInfo selectedFolder();
```





#### Get the error details when readingg the Emails

return **`String`** The string of error details.

```cpp
String errorReason();
```





#### Clear all the cache data stored in the IMAP session object.

```cpp
void empty();
```





## IMAPSession class functions


The following functions are available from the SMTP Session class.

This class is similar to the IMAP session class, used for controlling SMTP transports 
and retrieving the data from the SMTP server.






#### Begin the SMTP server connection.

param **`session`** The pointer to ESP_Mail_Session structured data that keeps the server and log in details.

return **`boolean`** The boolean value indicates the success of operation.

```cpp
bool connect(ESP_Mail_Session *session);
```





### Close the SMTP session.

```cpp
bool closeSession();
```





#### Set to enable the debug.

param **`level`** The level to enable the debug message

level = 0, no debug

level = 1, basic debug

level = 2, full debug 1

level = 333, full debug 2

```cpp
void debug(int level);
```





#### Get the error details when sending the Email

return **`String`** The string of error details.

```cpp
String errorReason();
```





#### Set the Email sending status callback function.

param **`smtpCallback`** The callback function that accept the smtpStatusCallback param.

```cpp
void callback(smtpStatusCallback smtpCallback);
```





## SMTP_Message class functions


The following functions are available from the SMTP Message class.

This class is used for storing the message data including headers, body and attachments
which will be processed with the SMTP session class.




#### To reset the SMTP_Attachment item data

param **`att`** The SMTP_Attachment class that stores the info about attachment

This function was used for clear the internal data of attachment item to be ready for reuse.

```cpp
void resetAttachItem(SMTP_Attachment &att);
```



#### To clear all data in SMTP_Message class included headers, bodies and attachments

```cpp
void clear();
```




#### To clear all the inline images in SMTP_Message class.

```cpp
void clearInlineimages();
```





#### To clear all the attachments.

```cpp
void clearAttachments();
```





#### To clear all rfc822 message attachment.

```cpp
void clearRFC822Messages();
```





#### To clear the primary recipient mailboxes.

```cpp
void clearRecipients();
```





#### To clear the Carbon-copy recipient mailboxes.

```cpp
void clearCc();
```





#### To clear the Blind-carbon-copy recipient mailboxes.

```cpp
void clearBcc();
```


#### To clear the custom message headers.

```cpp
void clearHeader();
```




#### To add attachment to the message.

param **`att`** The SMTP_Attachment data item

```cpp
void addAttachment(SMTP_Attachment &att);
```





#### To add parallel attachment to the message. 

param **`att`** The SMTP_Attachment data item

```cpp
void addParallelAttachment(SMTP_Attachment &att);
```





#### To add inline image to the message.

param **`att`** The SMTP_Attachment data item

```cpp
void addInlineImage(SMTP_Attachment &att);
```





#### To add rfc822 message to the message.

param **`msg`** The RFC822_Message class object

```cpp
void addMessage(SMTP_Message &msg);
```





#### To add the primary recipient mailbox to the message.

param **`name`** The name of primary recipient

param **`email`** The Email address of primary recipient

```cpp
void addRecipient(const char *name, const char *email);
```





#### To add Carbon-copy recipient mailbox.

param **`email`** The Email address of secondary recipient

```cpp
void addCc(const char *email);
```





#### To add Blind-carbon-copy recipient mailbox.

param **`email`** The Email address of the tertiary recipient

```cpp
void addBcc(const char *email);
```





#### To add the custom header to the message.

param **`hdr`** The header name and value

```cpp
void addHeader(const char *hdr);
```




##### [properties] The message author config

This property has the sub properties

###### [const char*] name - The sender name.

###### [const char*] email - The sender Email address.

```cpp
esp_mail_email_info_t sender;
```


##### [properties] The topic of message

```cpp
const char *subject;
```


##### [properties] The message type

```cpp
byte type;
```


##### [properties] The PLAIN text message

This property has the sub properties

###### [esp_mail_smtp_embed_message_body_t] embed - The option to embed this message content as a file.

###### [const char*] content - The PLAIN text content of the message.

###### [esp_mail_blob_message_content_t] blob - The blob that contins PLAIN text content of the message.

###### [esp_mail_file_message_content_t] file - The file that contins PLAIN text content of the message.

###### [const char*] charSet - The character transcoding of the PLAIN text content of the message.

###### [const char*] content_type - The content type of message.

###### [const char*] transfer_encoding - The option to encode the content for data transfer.

###### [boolean] flowed - The option to send the PLAIN text with wrapping.

```cpp
esp_mail_plain_body_t text;
```


##### [properties] The HTML text message

This propery has the sub properties

###### [const char*] content - The HTML content of the message.

###### [esp_mail_blob_message_content_t] blob - The blob that contins HTML content of the message.

###### [esp_mail_file_message_content_t] file - The file that contins HTML content of the message.

###### [const char*] charSet - The character transcoding of the HTML content of the message.

###### [const char*] content_type - The content type of message.

###### [const char*] transfer_encoding - The option to encode the content for data transfer.

```cpp
esp_mail_html_body_t html;
```


##### [properties] The response config

This propery has the sub properties

###### [const char*] reply_to - The author Email address to reply.

###### [const char*] return_path - The sender Email address to return the message.

###### [int] notify - The Delivery Status Notifications enumeration e.g. 

esp_mail_smtp_notify_never = 0,

esp_mail_smtp_notify_success = 1,

esp_mail_smtp_notify_failure = 2, and

esp_mail_smtp_notify_delay = 4 

```cpp
esp_mail_smtp_msg_response_t response;
```


##### [properties] The priority of the message

This property has the enumeration values

esp_mail_smtp_priority_high = 1,

esp_mail_smtp_priority_normal = 3,

esp_mail_smtp_priority_low = 5

```cpp
esp_mail_smtp_priority priority;
```


##### [properties] The enable options

This propery has the sub property

###### [boolean] chunking - enable chunk data sending for large message.

```cpp
esp_mail_smtp_enable_option_t enable;
```


##### [properties] The message from config

This property has the sub properties

###### [const char*] name - The messsage author name.

###### [const char*] email - The message author Email address.

```cpp
esp_mail_email_info_t from;
```


##### [properties] The message identifier

```cpp
const char *ID;
```

##### [properties] The keywords or phrases, separated by commas

```cpp
const char *keywords;
```


##### [properties] The comments about message

```cpp
const char *comments;
```


##### [properties] The date of message

```cpp
const char *date;
```


##### [properties] The return recipient of the message

```cpp
const char *return_path;
```



##### [properties] The field that contains the parent's message ID of the message to which this one is a reply

```cpp
const char *in_reply_to;
```



##### [properties] The field that contains the parent's references (if any) and followed by the parent's message ID (if any) of the message to which this one is a reply

```cpp
const char *references;
```


## IMAP_Status class functions


The following functions are available from the IMAP Status class.

This class is used as the callback parameter for retrieving the status while reading the Email.




#### Provide the information of each process in the reading operation.

return **`string`** The info for each process

```cpp
const char *info();
```




#### Provide the status of completion.

return **`boolean`** The bool value indicates that all reading processes are finished

```cpp
bool success();
```





#### To clear all data store in this class.

```cpp
void empty();
```






## SMTP_Status class functions


The following functions are available from the SMTP Status class.

This class is used as the callback parameter for retrieving the status while sending the Email.




#### Provide the information of each process in the sending operation.

return **`string`** The info for each process

```cpp
const char *info();
```




#### Provide the status of completion.

return **`boolean`** The bool value indicates that all sending processes are finished

```cpp
bool success();
```





#### To clear all data store in this class.

```cpp
void empty();
```




#### Provide the number of complete sending message.

return **`number`** The number of message that was sent

```cpp
size_t completedCount();
```





#### Provide the number of failed sending message.

return **`number`** The number of message that was not sent

```cpp
size_t failedCount();
```




## SendingResult class functions


The following functions are available from the SendingResult class.

This class is used for retrieving the info about the result of sending the messages.




#### Provide the information of a message sending status.

param **`index`** The index number of a message sending status

return **`SMTP_Result`** The SMTP_Result type data that provides these properties

##### [bool] completed - The status of the message

#### [const char *] recipients - The primary recipient mailbox of the message

#### [const char *] subject - The topic of the message

#### [time_t] timesstamp - The timestamp of the message

```cpp
SMTP_Result getItem(size_t index);
```





#### Provide the amount of the result data.

return **`number`** The number of result item

```cpp
size_t size();
```




## FoldersCollection class functions


The following functions are available from the FoldersCollection class.

This class is used for retrieving the info about the mailbox folders which available to read or serach
in the user Email mailbox.




#### Provide the information of a folder in a folder collection.

param **`index`** The index number of folders

return **`esp_mail_folder_info_item_t`** The esp_mail_folder_info_item_t structured data that provides these properties

#### [const char *] name - The name of folder

#### [const char *] attributes - The attributes of folder

#### [const char *] delimiter - The delimeter of folder

```cpp
esp_mail_folder_info_item_t info(size_t index);
```





#### Provide the number of folders in the collection.

return **`number`** The number of folder in the collection

```cpp
size_t size();
```






## SelectedFolderInfo class functions


The following functions are available from the SelectedFolderInfo class.

This class is used for retrieving the info about the sselected or opened mailbox folder.




#### Provide the numbers of flags in the user Email mailbox.

return **`number`** The numbers of flags

```cpp
size_t flagCount();
```





#### Provide the numbers of messages in this mailbox.

return **`number`** The numbers of messages in the selected mailbox folder

```cpp
size_t msgCount();
```






#### Get the numbers of messages in this mailbox that recent flag was set.

return **`number`** The numbers of messages in the selected mailbox folder that recent flag was set

```cpp
size_t recentCount();
```






#### Get the order of message in number of message in this mailbox that reoved.

return **`IMAP_Polling_Status`** The data that holds the polling status.

The IMAP_Polling_Status has the properties e.g. type, messageNum, and argument.

The type property is the type of status e.g.imap_polling_status_type_undefined, imap_polling_status_type_new_message,
imap_polling_status_type_remove_message, and imap_polling_status_type_fetch_message.

The messageNum property is message number or order from the total number of message that added, fetched or deleted.

The argument property is the argument of commands e.g. FETCH

```cpp
struct IMAP_Polling_Status pollingStatus();
```








#### Provide the predicted next message UID in the sselected folder.

return **`number`** The number represents the next message UID number

```cpp
size_t nextUID();
```







#### Provide the numbers of messages from search result based on the search criteria.

return **`number`** The total number of messsages from search

```cpp
size_t searchCount();
```





#### Provide the numbers of messages to be stored in the ressult.

return **`number`** The number of messsage stored from search

```cpp
size_t availableMessages();
```





#### Provide the flag argument at the specified index.

return **`index`** The index of flag in the flags list

return **`String`** The argument of selected flag

```cpp
String flag(size_t index);
```




## ESP_Mail_Session type data


The following properties are available from the ESP_Mail_Session data type.

This data type is used for storing the session info about the server and login credentials.


#### [Properties] The server config

This property has the sub properties

##### [const char *] host_name - The hostName of the server.

##### [uint16_t] port - The port on the server to connect to.

```cpp
esp_mail_sesson_sever_config_t server;
```


#### [Properties] The log in config

This property has the sub properties

##### [const char *] email - The user Email address to log in.

##### [consst char *] password - The user password to log in.

##### [consst char *] accessToken - The OAuth2.0 access token to log in.

##### [consst char *] user_domain - The user domain or ip of client.

```cpp
esp_mail_sesson_login_config_t login;
```


#### [Properties] The secure config

This property has the sub properties

##### [bool] startTLS - The option to send the command to start the TLS connection.

```cpp
esp_mail_sesson_secure_config_t secure;
```



#### [Properties] The certificate config

##### [const char *] cert_data - The certificate data (base64 data).

##### [consst char *] cert_file - The certificate file (DER format).

##### [esp_mail_file_storage_type] cert_file_storage_type - The storage type.

##### [bool] verify - The cerificate verification option.

```cpp
esp_mail_sesson_cert_config_t certificate;
```




## IMAP_Config type data


The following properties are available from the IMAP_Config data type.

This data type is used for storing the IMAP transport and operating options to 
control and store the operation result e.g. the messahe contents from search and fetch.




#### [Properties] The config for fetching

This property has the sub property

##### [const char *] uid - The UID of message to fetch.

```cpp
esp_mail_imap_fetch_config_t fetch;
```


#### [Properties] The config for search

This property has the sub properties

##### [const char *] criteria - The search criteria.

##### [boolean] unseen_msg - The option to search the unseen message.

```cpp
esp_mail_imap_search_config_t search;
```


#### [Properties] The config about the limits

This property has the sub properties

##### [size_t] search - The maximum messages from the search result.

##### [size_t] msg_size - The maximum size of the memory buffer to store the message content.

This is only limit for data to be stored in the IMAPSession. 

##### [size_t] attachment_size - The maximum size of each attachment to download.

The IMAP idle (polling) timeout in ms.

##### [size_t] imap_idle_timeout - The maximum size of each attachment to download.

```cpp
esp_mail_imap_limit_config_t limit;
```



#### [Properties] The config to enable the features

This property has the sub properties

##### [boolean] text - To store the PLAIN text of the message in the IMAPSession.

##### [boolean] html - To store the HTML of the message in the IMAPSession.

##### [boolean] rfc822 - To store the rfc822 messages in the IMAPSession.

##### [boolean] download_status - To enable the download status via the serial port.

##### [boolean] recent_sort - To sort the message UID of the search result in descending order.

##### [boolean] header_case_sesitive - To allow case sesitive in header parsing.

```cpp
esp_mail_imap_enable_config_t enable;
```



#### [Properties] The config about downloads

This property has the sub properties

##### [boolean] text - To download the PLAIN text content of the message.

##### [boolean] html - To download the HTML content of the message.

##### [boolean] attachment - To download the attachments of the message.

##### [boolean] inlineImg - To download the inline image of the message.

##### [boolean] rfc822 - To download the rfc822 mesages in the message.

##### [boolean] header - To download the message header.

```cpp
esp_mail_imap_download_config_t download;
```



#### [Properties] The config about the storage and path to save the downloaded file.

This property has the sub properties

##### [const char*] saved_path - The path to save the downloaded file.

##### [esp_mail_file_storage_type] type - The type of file storages enumeration e.g.

esp_mail_file_storage_type_none = 0,

esp_mail_file_storage_type_flash = 1, and

esp_mail_file_storage_type_sd = 2

```cpp
esp_mail_imap_storage_config_t storage;
```





## esp_mail_smtp_embed_message_body_t structured data


The following properties are available from the IMAP_Config data type.

This data type is used for storing the IMAP transport and operating options to 
control and store the operation result e.g. the messahe contents from search and fetch.




##### [Properties] Enable to send this message body as file

```cpp
bool enable;
```


##### [Properties] The name of embedded file

```cpp
const char* enable;
```


##### [Properties] The embedded type enumeration

esp_mail_smtp_embed_message_type_attachment = 0

sp_mail_smtp_embed_message_type_inline = 1

```cpp
esp_mail_smtp_embed_message_type type;
```






## esp_mail_blob_message_content_t structured data


The following properties are available from the esp_mail_blob_message_content_t data type.

This data type is used for storing the blob info of message body.




##### [Properties] The array of content in flash memory.

```cpp
const uint8_t * data;
```


##### [Properties] The array size in bytes.

```cpp
size_t size;
```





## esp_mail_file_message_content_t structured data


The following properties are available from the esp_mail_file_message_content_t data type.

This data type is used for storing the file info of message body.




##### [Properties] The file path include its name.

```cpp
const char *name;
```


##### [Properties] The type of file storages.

```cpp
esp_mail_file_storage_type type;
```






## IMAP_MSG_Item type data


The following properties are available from the IMAP_MSG_Item data type.

This data type is used for message item info and its contents from search and fetch.




#### [Properties] The message number

```cpp
int msgNo;
```


#### [Properties] The message UID

```cpp
int UID;
```


#### [Properties] The message identifier

```cpp
const char *ID;
```



#### [Properties] The language(s) for auto-responses

```cpp
const char *acceptLang;
```



#### [Properties] The language of message content

```cpp
const char *contentLang;
```



#### [Properties] The mailbox of message author

```cpp
const char *from;
```


#### [Properties] The charset of the mailbox of message author (deprecate)

```cpp
const char *fromCharset;
```


#### [Properties] The primary recipient mailbox

```cpp
const char *to;
```


#### [Properties] The charset of the primary recipient mailbox (deprecate)

```cpp
const char *toCharset;
```


#### [Properties] The Carbon-copy recipient mailboxes

```cpp
const char *cc;
```


#### [Properties] The charset of the Carbon-copy recipient mailbox header (deprecate)

```cpp
const char *ccCharset;
```

#### [Properties] The message date and time

```cpp
const char *date;
```

#### [Properties] The topic of message

```cpp
const char *subject;
```

#### [Properties] The topic of message charset (deprecate)

```cpp
const char *subjectCharset;
```

#### [Properties] The PLAIN text content of the message

```cpp
esp_mail_plain_body_t text;
```

#### [Properties] The HTML content of the message

```cpp
esp_mail_html_body_t html;
```

#### [Properties] The sender Email

```cpp
const char *sender;
```


#### [Properties] The keywords or phrases, separated by commas

```cpp
const char *keyword;
```

#### [Properties] The comments about message

```cpp
const char *comments;
```


#### [Properties] The return recipient of the message

```cpp
const char *return_path;
```


#### [Properties] The Email address to reply

```cpp
const char *reply_to;
```


#### [Properties] The field that contains the parent's message ID of the message to which this one is a reply

```cpp
const char *in_reply_to;
```


#### [Properties] The field that contains the parent's references (if any) and followed by the parent's message ID (if any) of the message to which this one is a reply

```cpp
const char *references;
```


#### [Properties] The Blind carbon-copy recipients

```cpp
const char *bcc;
```


#### [Properties] The error description from fetching the message

```cpp
const char *fetchError;
```


#### [Properties] The info about the attachments in the message

```cpp
std::vector<IMAP_Attach_Item> attachments;
```

#### [Properties] The info about the rfc822 messages included in the message

```cpp
std::vector<IMAP_MSG_Item> rfc822;
```






## Search Criteria

Search crieria is used for searching the mailbox for messages that match
the given searching criteria.  

Searching criteria consist of one or more search keys. When multiple keys are 
specified, the result is the intersection (AND function) of all the messages 
that match those keys.  

Example:

 **`DELETED FROM "SMITH" SINCE 1-Feb-1994`** refers 
to all deleted messages from Smith that were placed in the mailbox since 
February 1, 1994.  

A search key can also be a parenthesized list of one or more search keys 
(e.g., for use with the OR and NOT keys).

**`SINCE 10-Feb-2019`**  will search all messages that received since 10 Feb 2019

**`UID SEARCH ALL`**  will seach all message which will return the message UID 
that can be use later for fetch one or more messages.

 
The following keywords can be used for the search criteria.


**ALL** - All messages in the mailbox; the default initial key for	ANDing.

**ANSWERED** - Messages with the \Answered flag set.

**BCC** - Messages that contain the specified string in the envelope structure's BCC field.

**BEFORE** - Messages whose internal date (disregarding time and timezone) is earlier than the specified date.

**BODY** - Messages that contain the specified string in the body of the message.

**CC** - Messages that contain the specified string in the envelope structure's CC field.

**DELETED** - Messages with the \Deleted flag set.

**DRAFT** - Messages with the \Draft flag set.

**FLAGGED** - Messages with the \Flagged flag set.

**FROM** - Messages that contain the specified string in the envelope	structure's FROM field.

**HEADER** - Messages that have a header with the specified field-name (as defined in [RFC-2822])

and that contains the specified string	in the text of the header (what comes after the colon).

If the string to search is zero-length, this matches all messages that have a header line with 

the specified field-name regardless of	the contents.

**KEYWORD** - Messages with the specified keyword flag set.

**LARGER** - Messages with an (RFC-2822) size larger than the specified number of octets.

**NEW** -  Messages that have the \Recent flag set but not the \Seen flag.

This is functionally equivalent to **"(RECENT UNSEEN)"**.

**NOT** - Messages that do not match the specified search key.

**OLD** - Messages that do not have the \Recent flag set.  This is	functionally equivalent to

**"NOT RECENT"** (as opposed to **"NOT NEW"**).

**ON** - Messages whose internal date (disregarding time and timezone) is within the specified date.

**OR** - Messages that match either search key.

**RECENT** - Messages that have the \Recent flag set.

**SEEN** - Messages that have the \Seen flag set.

**SENTBEFORE** - Messages whose (RFC-2822) Date: header (disregarding time and	timezone) is earlier than the specified date.

**SENTON** - Messages whose (RFC-2822) Date: header (disregarding time and timezone) is within the specified date.

**SENTSINCE** - Messages whose (RFC-2822) Date: header (disregarding time and timezone) is within or later than the specified date.

**SINCE** - Messages whose internal date (disregarding time and timezone) is within or later than the specified date.

**SMALLER** - Messages with an (RFC-2822) size smaller than the specified number of octets.

**SUBJECT** - Messages that contain the specified string in the envelope structure's SUBJECT field.

**TEXT** - Messages that contain the specified string in the header or body of the message.

**TO** - Messages that contain the specified string in the envelope structure's TO field.

**UID** - Messages with unique identifiers corresponding to the specified unique identifier set.  

Sequence set ranges are permitted.

**UNANSWERED** - Messages that do not have the \Answered flag set.

**UNDELETED** - Messages that do not have the \Deleted flag set.

**UNDRAFT** - Messages that do not have the \Draft flag set.

**UNFLAGGED** - Messages that do not have the \Flagged flag set.

**UNKEYWORD** - Messages that do not have the specified keyword flag set.

**UNSEEN** - Messages that do not have the \Seen flag set.









## MailClient.Time functions


The helper function to set and get the system time.





#### Set the system time from the NTP server

param **`gmtOffset`** The GMT time offset in hour.

param **`daylightOffset`** The Daylight time offset in hour.

return **`boolean`** The status indicates the success of operation.

This requires internet connection

```cpp
bool setClock(float gmtOffset, float daylightOffset);
```






#### Provide the Unix time

return **`uint32_t`** The value of current Unix time.

```cpp
uint32_t getUnixTime();
```






#### Provide the timestamp from the year, month, date, hour, minute, and second provided

param **`year`** The year.

param **`mon`** The months from 1 to 12.

param **`date`** The dates.

param **`hour`** The hours.

param **`mins`** The minutes.

param **`sec`** The seconds.

return **`time_t`** The value of timestamp.

```cpp
time_t getTimestamp(int year, int mon, int date, int hour, int mins, int sec);
```






#### Provide the current year.

return **`int`** The value of current year.

```cpp
int getYear();
```






#### Provide the current month.

return **`int`** The value of current month.

```cpp
int getMonth();
```





#### Provide the current date.

return **`int`** The value of current date.

```cpp
int getDay();
```






#### Provide the current day of week.

return **`int`** The value of day of week.

1 for sunday and 7 for saturday

```cpp
int getDayOfWeek();
```





#### Provide the current day of week in String.

return **`String`** The value of day of week.

Returns sunday, monday, tuesday, wednesday, thurseday, friday and saturday.

```cpp
String getDayOfWeekString();
```






#### Provide the current hour.

return **`int`** The value of current hour (0 to 23).

```cpp
int getHour();
```






#### Provide the current minute.

return **`int`** The value of current minute (0 to 59).

```cpp
int getMin();
```






#### Provide the current second.

return **`int`** The value of current second (0 to 59).

```cpp
int getSecond();
```







#### Provide the total days of current year.

return **`int`** The value of total days of current year.

```cpp
int getNumberOfDayThisYear();
```






#### Provide the total days of from January 1, 1970 to specific date.

param **`year`** The years from 1970.

param **`mon`** The months from 1 to 12.

param **`date`** The dates.

return **`int`** The value of total days.

```cpp
int getTotalDays(int year, int month, int day);
```





#### Provide the day of week from specific date.

param **`year`** The years.

param **`month`** The months from 1 to 12.

param **`day`** The dates.

return **`int`** The value of day of week.

1 for sunday and 7 for saturday

```cpp
int dayofWeek(int year, int month, int day);
```






#### Provide the second of current hour.

return **`int`** The value of current second.

```cpp
int getCurrentSecond();
```





#### Provide the current timestamp.

return **`uint64_t`** The value of current timestamp.

```cpp
uint64_t getCurrentTimestamp();
```






#### Provide the date and time from second counted from January 1, 1970.

param **`sec`** The seconds from January 1, 1970 00.00.

return **`tm`** The tm structured data.

The returned structured data tm has the members e.g.

tm_year (from 1900), tm_mon (from 0 to 11), tm_mday, tm_hour, tm_min and tm_sec.

```cpp
struct tm getTimeFromSec(int secCount);
```






#### Provide the current date time string that valid for Email

return **`String`** The current date time string.

```cpp
String getDateTimeString();
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

