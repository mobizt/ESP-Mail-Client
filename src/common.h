#ifndef ESP_MAIL_CLIENT_COMMON_H
#define ESP_MAIL_CLIENT_COMMON_H

#include "ESP_Mail_FS.h"
#include "ESP_Mail_Error.h"
#include "extras/MB_FS.h"
#include "extras/RFC2047.h"
#include <time.h>
#include <ctype.h>

#if !defined(__AVR__)
#include <vector>
#include <algorithm>
#endif

#include "extras/MB_List.h"

#if defined(ESP8266)
#include "extras/SDK_Version_Common.h"
#endif

#if defined(ENABLE_SMTP) || defined(ENABLE_IMAP)

#define MAX_EMAIL_SEARCH_LIMIT 1000
#define BASE64_CHUNKED_LEN 76
#define FLOWED_TEXT_LEN 78
#define QP_ENC_MSG_LEN 76
#define ESP_MAIL_NETWORK_RECONNECT_TIMEOUT 10000
#define ESP_MAIL_PROGRESS_REPORT_STEP 5
#define ESP_MAIL_CLIENT_TRANSFER_DATA_FAILED 0
#define ESP_MAIL_CLIENT_STREAM_CHUNK_SIZE 256
#define ESP_MAIL_CLIENT_VALID_TS 1577836800

#endif

enum esp_mail_file_storage_type
{
    esp_mail_file_storage_type_none,
    esp_mail_file_storage_type_flash,
    esp_mail_file_storage_type_sd
};

using namespace mb_string;

#if defined(ENABLE_SMTP) || defined(ENABLE_IMAP)

typedef void (*NetworkConnectionHandler)(void);

enum esp_mail_message_type
{
    esp_mail_msg_type_none = 0,
    esp_mail_msg_type_plain = 1,
    esp_mail_msg_type_html = 2,
    esp_mail_msg_type_enriched = 1
};

enum esp_mail_smtp_embed_message_type
{
    esp_mail_smtp_embed_message_type_attachment = 0,
    esp_mail_smtp_embed_message_type_inline
};

enum esp_mail_attach_type
{
    esp_mail_att_type_none,
    esp_mail_att_type_attachment,
    esp_mail_att_type_inline
};

enum esp_mail_auth_type
{
    esp_mail_auth_type_psw,
    esp_mail_auth_type_oath2,
    esp_mail_auth_type_token
};

enum esp_mail_debug_level
{
    esp_mail_debug_level_0 = 0,
    esp_mail_debug_level_1,
    esp_mail_debug_level_2 = 222,
    esp_mail_debug_level_3 = 333
};

enum esp_mail_msg_part_xencoding
{
    esp_mail_msg_part_xencoding_none,
    esp_mail_msg_part_xencoding_7bit,
    esp_mail_msg_part_xencoding_qp,
    esp_mail_msg_part_xencoding_base64,
    esp_mail_msg_part_xencoding_8bit,
    esp_mail_msg_part_xencoding_binary
};

struct esp_mail_internal_use_t
{
    bool binary = false;
    MB_String cid;
};

struct esp_mail_content_transfer_encoding_t
{
    /* The default 7-bit transfer encoding for US-ACII characters*/
    static constexpr const char *enc_7bit = "7bit";

    /* The quoted printable transfer encoding for non-US-ASCII characters*/
    static constexpr const char *enc_qp = "quoted-printable";

    /* The base64 encoded transfer encoding */
    static constexpr const char *enc_base64 = "base64";

    /* The 8-bit transfer encoding for extended-US-ASCII characters*/
    static constexpr const char *enc_8bit = "8bit";

    /* The binary transfer encoding for extended-US-ASCII characters with no line
   * length limit*/
    static constexpr const char *enc_binary = "binary";
};

struct esp_mail_file_message_content_t
{
    /* The file path include its name */
    MB_String name;

    /** The type of file storages e.g.
   * esp_mail_file_storage_type_none,
   * esp_mail_file_storage_type_flash, and
   * esp_mail_file_storage_type_sd
  */
    esp_mail_file_storage_type type = esp_mail_file_storage_type_flash;
};

struct esp_mail_blob_message_content_t
{
    /* The array of content in flash memory */
    const uint8_t *data = nullptr;

    /* The array size in bytes */
    size_t size = 0;
};

/* The option to embed this message content as a file */
struct esp_mail_smtp_embed_message_body_t
{
    /* Enable to send this message body as file */
    bool enable = false;

    /* The name of embedded file */
    MB_String filename;

    /** The embedded type
   * esp_mail_smtp_embed_message_type_attachment or 0
   * esp_mail_smtp_embed_message_type_inline or 1
  */
    esp_mail_smtp_embed_message_type type = esp_mail_smtp_embed_message_type_attachment;
};

/* The PLAIN text body details of the message */
struct esp_mail_plain_body_t
{
    /* The option to embed this message content as a file */
    struct esp_mail_smtp_embed_message_body_t embed;

    /* The PLAIN text content of the message */
    MB_String content;

    const char *nonCopyContent = "";

    /* The blob that contins PLAIN text content of the message */
    struct esp_mail_blob_message_content_t blob;

    /* The file that contins PLAIN text content of the message */
    struct esp_mail_file_message_content_t file;

    /* The charset of the PLAIN text content of the message */
    MB_String charSet = "UTF-8";

    /* The content type of message */
    MB_String content_type = "text/plain";

    /* The option to encode the content for data transfer */
    MB_String transfer_encoding = "7bit";

    /* The option to send the PLAIN text with wrapping */
    bool flowed = false;

    /* The internal usage data */
    struct esp_mail_internal_use_t _int;
};

struct esp_mail_html_body_t
{
    /* The option to embedded the content as a file */
    struct esp_mail_smtp_embed_message_body_t embed;

    /* The HTML content of the message */
    MB_String content;

    const char *nonCopyContent = "";

    /* The blob that contins HTML content of the message */
    struct esp_mail_blob_message_content_t blob;

    /* The file that contins HTML content of the message */
    struct esp_mail_file_message_content_t file;

    /* The charset of the HTML content of the message */
    MB_String charSet = "UTF-8";

    /* The content type of message */
    MB_String content_type = "text/html";

    /* The option to encode the content for data transfer */
    MB_String transfer_encoding = "7bit";

    /* The internal usage data */
    struct esp_mail_internal_use_t _int;
};

/* The PLAIN text body details of the message */
struct esp_mail_imap_plain_body_t
{
    /* The option to embed this message content as a file */
    struct esp_mail_smtp_embed_message_body_t embed;

    /* The PLAIN text content of the message */
    const char *content = "";

    /* The blob that contins PLAIN text content of the message */
    struct esp_mail_blob_message_content_t blob;

    /* The file that contins PLAIN text content of the message */
    struct esp_mail_file_message_content_t file;

    /* The charset of the PLAIN text content of the message */
    const char *charSet = "UTF-8";

    /* The content type of message */
    const char *content_type = "text/plain";

    /* The option to encode the content for data transfer */
    const char *transfer_encoding = "7bit";

    /* The option to send the PLAIN text with wrapping */
    bool flowed = false;

    /* The internal usage data */
    struct esp_mail_internal_use_t _int;
};

struct esp_mail_imap_html_body_t
{
    /* The option to embedded the content as a file */
    struct esp_mail_smtp_embed_message_body_t embed;

    /* The HTML content of the message */
    const char *content = "";

    /* The blob that contins HTML content of the message */
    struct esp_mail_blob_message_content_t blob;

    /* The file that contins HTML content of the message */
    struct esp_mail_file_message_content_t file;

    /* The charset of the HTML content of the message */
    const char *charSet = "UTF-8";

    /* The content type of message */
    const char *content_type = "text/html";

    /* The option to encode the content for data transfer */
    const char *transfer_encoding = "7bit";

    /* The internal usage data */
    struct esp_mail_internal_use_t _int;
};

struct esp_mail_attachment_info_t
{
    const char *filename = "";
    const char *name = "";
    const char *creationDate = "";
    const char *mime = "";
    esp_mail_attach_type type = esp_mail_att_type_none;
    size_t size;
};

#endif

#if defined(ENABLE_SMTP)

enum esp_mail_smtp_notify
{
    esp_mail_smtp_notify_never = 0,
    esp_mail_smtp_notify_success = 1,
    esp_mail_smtp_notify_failure = 2,
    esp_mail_smtp_notify_delay = 4
};

enum esp_mail_smtp_status_code
{
    esp_mail_smtp_status_code_0, // default

    /* Positive Completion */
    esp_mail_smtp_status_code_211 = 221, // System status, or system help reply
    esp_mail_smtp_status_code_214 = 214, // Help message(A response to the HELP command)
    esp_mail_smtp_status_code_220 = 220, //<domain> Service ready
    esp_mail_smtp_status_code_221 = 221, //<domain> Service closing transmission channel [RFC 2034]
    esp_mail_smtp_status_code_235 = 235, // 2.7.0 Authentication succeeded[RFC 4954]
    esp_mail_smtp_status_code_250 = 250, // Requested mail action okay, completed
    esp_mail_smtp_status_code_251 = 251, // User not local; will forward
    esp_mail_smtp_status_code_252 = 252, // Cannot verify the user, but it will
                                         // try to deliver the message anyway

    /* Positive Intermediate */
    esp_mail_smtp_status_code_334 = 334, //(Server challenge - the text part
                                         //contains the Base64 - encoded
                                         //challenge)[RFC 4954]
    esp_mail_smtp_status_code_354 = 354, // Start mail input

    /* Transient Negative Completion */
    /* "Transient Negative" means the error condition is temporary, and the action
     may be requested again.*/
    esp_mail_smtp_status_code_421 = 421, // Service is unavailable because the server is shutting down.
    esp_mail_smtp_status_code_432 = 432, // 4.7.12 A password transition is needed [RFC 4954]
    esp_mail_smtp_status_code_450 = 450, // Requested mail action not taken: mailbox unavailable (e.g.,
                                         // mailbox busy or temporarily blocked for policy reasons)
    esp_mail_smtp_status_code_451 = 451, // Requested action aborted : local error in processing
    // e.g.IMAP server unavailable[RFC 4468]
    esp_mail_smtp_status_code_452 = 452, // Requested action not taken : insufficient system storage
    esp_mail_smtp_status_code_454 = 454, // 4.7.0 Temporary authentication failure[RFC 4954]
    esp_mail_smtp_status_code_455 = 455, // Server unable to accommodate parameters

    /* Permanent Negative Completion */
    esp_mail_smtp_status_code_500 = 500, // Syntax error, command unrecognized
                                         // (This may include errors such as
                                         // command line too long)
    // e.g. Authentication Exchange line is too long [RFC 4954]
    esp_mail_smtp_status_code_501 = 501, // Syntax error in parameters or arguments
    // e.g. 5.5.2 Cannot Base64-decode Client responses [RFC 4954]
    // 5.7.0 Client initiated Authentication Exchange (only when the SASL
    // mechanism specified that client does not begin the authentication exchange)
    // [RFC 4954]
    esp_mail_smtp_status_code_502 = 502, // Command not implemented
    esp_mail_smtp_status_code_503 = 503, // Bad sequence of commands
    esp_mail_smtp_status_code_504 = 504, // Command parameter is not implemented
    // e.g. 5.5.4 Unrecognized authentication type [RFC 4954]
    esp_mail_smtp_status_code_521 = 521, // Server does not accept mail [RFC 7504]
    esp_mail_smtp_status_code_523 = 523, // Encryption Needed [RFC 5248]
    esp_mail_smtp_status_code_530 = 530, // 5.7.0 Authentication required [RFC 4954]
    esp_mail_smtp_status_code_534 = 534, // 5.7.9 Authentication mechanism is too weak [RFC 4954]
    esp_mail_smtp_status_code_535 = 535, // 5.7.8 Authentication credentials invalid [RFC 4954]
    esp_mail_smtp_status_code_538 = 538, // 5.7.11 Encryption required for
                                         // requested authentication mechanism[RFC
                                         // 4954]
    esp_mail_smtp_status_code_550 = 550, // Requested action not taken: mailbox unavailable (e.g., mailbox not
                                         // found, no access, or command rejected for policy reasons)
    esp_mail_smtp_status_code_551 = 551, // User not local; please try <forward-path>
    esp_mail_smtp_status_code_552 = 552, // Requested mail action aborted: exceeded storage allocation
    esp_mail_smtp_status_code_553 = 553, // Requested action not taken: mailbox name not allowed
    esp_mail_smtp_status_code_554 = 554, // Transaction has failed (Or, in the
                                         // case of a connection-opening response,
                                         // "No SMTP service here")
    // e.g. 5.3.4 Message too big for system [RFC 4468]
    esp_mail_smtp_status_code_556 = 556, // Domain does not accept mail[RFC 7504]
};

enum esp_mail_smtp_port
{
    esp_mail_smtp_port_25 = 25,   // PLAIN/TLS with STARTTLS
    esp_mail_smtp_port_465 = 465, // SSL
    esp_mail_smtp_port_587 = 587  // TLS with STARTTLS
};

struct esp_mail_smtp_msg_response_t
{
    /* The author Email address to reply */
    MB_String reply_to;

    /* The sender Email address to return the message */
    MB_String return_path;

    /** The Delivery Status Notifications e.g. esp_mail_smtp_notify_never,
   * esp_mail_smtp_notify_success,
   * esp_mail_smtp_notify_failure, and
   * esp_mail_smtp_notify_delay
  */
    int notify = esp_mail_smtp_notify::esp_mail_smtp_notify_never;
};

struct esp_mail_smtp_enable_option_t
{
    /* Enable chunk data sending for large message */
    bool chunking = false;
};

struct esp_mail_attach_blob_t
{
    /* BLOB data (flash or ram) */
    const uint8_t *data = nullptr;

    /* BLOB data size in byte */
    size_t size = 0;
};

struct esp_mail_attach_file_t
{
    MB_String path;
    /** The file storage type e.g. esp_mail_file_storage_type_none,
   * esp_mail_file_storage_type_flash, and
   * esp_mail_file_storage_type_sd
  */
    esp_mail_file_storage_type storage_type = esp_mail_file_storage_type_none;
};

struct esp_mail_attach_descr_t
{
    /* The name of attachment */
    MB_String name;

    /* The attachment file name */
    MB_String filename;

    /* The MIME type of attachment */
    MB_String mime;

    /* The transfer encoding of attachment e.g. base64 */
    MB_String transfer_encoding = "base64";

    /* The content encoding of attachment e.g. base64 */
    MB_String content_encoding;

    /* The content id of attachment file */
    MB_String content_id;
};

struct esp_mail_attach_internal_t
{
    esp_mail_attach_type att_type = esp_mail_att_type_attachment;
    int index = 0;
    int msg_uid = 0;
    bool flash_blob = false;
    bool binary = false;
    bool parallel = false;
    MB_String cid;
};

struct esp_mail_attachment_t
{
    /* The attachment description */
    struct esp_mail_attach_descr_t descr;

    /* The BLOB data config */
    struct esp_mail_attach_blob_t blob;

    /* The file data config */
    struct esp_mail_attach_file_t file;

    /* reserved for internal usage */
    struct esp_mail_attach_internal_t _int;
};

struct esp_mail_smtp_recipient_t
{
    /* The recipient's name */
    MB_String name;

    /* The recipient's Email address */
    MB_String email;
};

struct esp_mail_smtp_recipient_address_t
{
    /* The recipient's Email address */
    MB_String email;
};

struct esp_mail_smtp_send_status_t
{
    /* The status of the message */
    bool completed = false;

    /* The primary recipient mailbox of the message */
    MB_String recipients;

    /* The topic of the message */
    MB_String subject;

    /* The timestamp of the message */
    uint32_t timestamp = 0;
};

struct esp_mail_smtp_capability_t
{
    bool esmtp = false;
    bool binaryMIME = false;
    bool _8bitMIME = false;
    bool chunking = false;
    bool utf8 = false;
    bool pipelining = false;
    bool dsn = false;
};

struct esp_mail_smtp_msg_type_t
{
    bool rfc822 = false;
    int rfc822Idx = 0;
};

enum esp_mail_smtp_command
{
    esp_mail_smtp_cmd_initial_state,
    esp_mail_smtp_cmd_greeting,
    esp_mail_smtp_cmd_start_tls,
    esp_mail_smtp_cmd_login_user,
    esp_mail_smtp_cmd_auth_plain,
    esp_mail_smtp_cmd_auth,
    esp_mail_smtp_cmd_login_psw,
    esp_mail_smtp_cmd_send_header_sender,
    esp_mail_smtp_cmd_send_header_recipient,
    esp_mail_smtp_cmd_send_body,
    esp_mail_smtp_cmd_chunk_termination,
    esp_mail_smtp_cmd_logout
};

enum esp_mail_smtp_priority
{
    esp_mail_smtp_priority_high = 1,
    esp_mail_smtp_priority_normal = 3,
    esp_mail_smtp_priority_low = 5,
};

struct esp_mail_smtp_response_status_t
{
    int respCode = 0;
    int statusCode = 0;
    MB_String text;
};

struct esp_mail_email_info_t
{
    /* The name of Email author*/
    MB_String name;

    /* The Email address */
    MB_String email;
};

#endif

#if defined(ENABLE_IMAP)

enum esp_mail_char_decoding_scheme
{
    esp_mail_char_decoding_scheme_default,
    esp_mail_char_decoding_scheme_iso8859_1,
    esp_mail_char_decoding_scheme_tis620
};

enum esp_mail_imap_port
{
    esp_mail_imap_port_143 = 143, // PLAIN/TLS with STARTTLS
    esp_mail_imap_port_993 = 993, // SSL
};

enum esp_mail_imap_auth_mode
{
    esp_mail_imap_mode_examine,
    esp_mail_imap_mode_select
};

enum esp_mail_imap_response_status
{
    esp_mail_imap_resp_unknown,
    esp_mail_imap_resp_ok,
    esp_mail_imap_resp_no,
    esp_mail_imap_resp_bad
};

enum esp_mail_imap_polling_status_type
{
    imap_polling_status_type_undefined,
    imap_polling_status_type_new_message,
    imap_polling_status_type_remove_message,
    imap_polling_status_type_fetch_message
};

enum esp_mail_imap_header_state
{
    esp_mail_imap_state_from = 1,
    esp_mail_imap_state_sender,
    esp_mail_imap_state_to,
    esp_mail_imap_state_cc,
    esp_mail_imap_state_subject,
    esp_mail_imap_state_content_type,
    esp_mail_imap_state_content_transfer_encoding,
    esp_mail_imap_state_accept_language,
    esp_mail_imap_state_content_language,
    esp_mail_imap_state_date,
    esp_mail_imap_state_msg_id,
    esp_mail_imap_state_return_path,
    esp_mail_imap_state_reply_to,
    esp_mail_imap_state_in_reply_to,
    esp_mail_imap_state_references,
    esp_mail_imap_state_comments,
    esp_mail_imap_state_keywords,
    esp_mail_imap_state_char_set,
    esp_mail_imap_state_boundary
};

enum esp_mail_imap_command
{
    esp_mail_imap_cmd_capability,
    esp_mail_imap_cmd_starttls,
    esp_mail_imap_cmd_login,
    esp_mail_imap_cmd_plain,
    esp_mail_imap_cmd_auth,
    esp_mail_imap_cmd_list,
    esp_mail_imap_cmd_select,
    esp_mail_imap_cmd_examine,
    esp_mail_imap_cmd_close,
    esp_mail_imap_cmd_status,
    esp_mail_imap_cmd_search,
    esp_mail_imap_cmd_fetch_body_header,
    esp_mail_imap_cmd_fetch_body_mime,
    esp_mail_imap_cmd_fetch_body_text,
    esp_mail_imap_cmd_fetch_body_attachment,
    esp_mail_imap_cmd_fetch_body_inline,
    esp_mail_imap_cmd_logout,
    esp_mail_imap_cmd_store,
    esp_mail_imap_cmd_expunge,
    esp_mail_imap_cmd_create,
    esp_mail_imap_cmd_delete,
    esp_mail_imap_cmd_idle,
    esp_mail_imap_cmd_done,
    esp_mail_imap_cmd_get_uid,
    esp_mail_imap_cmd_get_flags,
    esp_mail_imap_cmd_custom,
};

enum esp_mail_imap_mime_fetch_type
{
    esp_mail_imap_mime_fetch_type_part,
    esp_mail_imap_mime_fetch_type_sub_part1,
    esp_mail_imap_mime_fetch_type_sub_part2
};

enum esp_mail_imap_header_type
{
    esp_mail_imap_header_from,
    esp_mail_imap_header_to,
    esp_mail_imap_header_cc,
    esp_mail_imap_header_subject,
    esp_mail_imap_header_date,
    esp_mail_imap_header_msg_id,
    esp_mail_imap_header_cont_lang,
    esp_mail_imap_header_accept_lang
};

enum esp_mail_imap_multipart_sub_type
{
    esp_mail_imap_multipart_sub_type_none = 0,
    esp_mail_imap_multipart_sub_type_mixed,
    esp_mail_imap_multipart_sub_type_alternative,
    esp_mail_imap_multipart_sub_type_parallel,
    esp_mail_imap_multipart_sub_type_digest,
    esp_mail_imap_multipart_sub_type_related,
    esp_mail_imap_multipart_sub_type_report,
};

enum esp_mail_imap_message_sub_type
{
    esp_mail_imap_message_sub_type_none = 0,
    esp_mail_imap_message_sub_type_rfc822,
    esp_mail_imap_message_sub_type_delivery_status,
    esp_mail_imap_message_sub_type_partial,
    esp_mail_imap_message_sub_type_external_body,
};

struct esp_mail_imap_response_status_t
{
    int statusCode = 0;
    MB_String text;
};

struct esp_mail_imap_capability_t
{
    bool imap4 = false;
    bool imap4rev1 = false;
    //rfc2177
    bool idle = false;
};

struct esp_mail_imap_rfc822_msg_header_item_t
{
    MB_String sender;
    MB_String from;
    MB_String subject;
    MB_String messageID;
    MB_String keywords;
    MB_String comments;
    MB_String date;
    MB_String return_path;
    MB_String reply_to;
    MB_String to;
    MB_String cc;
    MB_String bcc;
    MB_String in_reply_to;
    MB_String references;
    MB_String flags;
};

/* descrete media types (rfc 2046) */
struct esp_mail_imap_descrete_media_type_t
{
    /** textual information with subtypes
   * "plain"
   * "enriched" (rfc 1896 revised from richtext in rfc 1341)
   *
   * unrecognized subtypes and charset should be interpreted as
   * application/octet-stream
   *
   * parameters:
   * "charset" (rfc 2045) default is us-ascii
   * for character set includes 8-bit characters
   * and such characters are used in the body, Content-Transfer-Encoding
   * header field and a corresponding encoding on the data are required
   *
   * ISO-8859-X where "X" is to be replaced, as
   * necessary, for the parts of ISO-8859 [ISO-8859].
  */
    static constexpr const char *text = "text";

    /** image data with subtypes (rfc 2048)
   * "jpeg"
   * "gif"
   *
   * unrecognized subtypes should be interpreted as application/octet-stream
  */
    static constexpr const char *image = "image";

    /** audio data with initial subtype
   * "baic" -- for single channel audio encoded using 8bit ISDN mu-law [PCM]
   * at a sample rate of 8000 Hz.
   *
   * Unrecognized subtypes of "audio" should at a miniumum be treated as
   * "application/octet-stream"
  */
    static constexpr const char *audio = "audio";

    /** video data with initial subtype
   * "mpeg"
   *
   * Unrecognized subtypes of "video" should at a minumum be treated as
   * "application/octet-stream"
  */
    static constexpr const char *video = "video";

    /** some other kind of data, typically either
   * uninterpreted binary data or information to be
   * processed by an application with subtypes
   *
   * "octet-stream" -- uninterpreted binary data
   * "PostScript" -- for the transport of PostScript material
   *
   * Other expected uses include spreadsheets, data for mail-based
   * scheduling systems, and languages for "active" (computational)
   * messaging, and word processing formats that are not directly readable.
   *
   * The octet-stream subtype parameters:
   * TYPE, PADDING, NAME
  */
    static constexpr const char *application = "application";
};

/** composite media types (rfc 2046)
 *
 * As stated in the definition of the Content-Transfer-Encoding field
 * [RFC 2045], no encoding other than "7bit", "8bit", or "binary" is
 * permitted for entities of type "multipart".  The "multipart" boundary
 * delimiters and header fields are always represented as 7bit US-ASCII
 * in any case (though the header fields may encode non-US-ASCII header
 * text as per RFC 2047) and data within the body parts can be encoded
 * on a part-by-part basis, with Content-Transfer-Encoding fields for
 * each appropriate body part.
*/
struct esp_mail_imap_composite_media_type_t
{
    /** data consisting of multiple entities of independent data types
   * The Content-Type field for multipart entities requires one parameter,
   * "boundary".
   * The boundary delimiter line is then defined as a line
   * consisting entirely of two hyphen characters ("-", decimal value 45)
   * followed by the boundary parameter value from the Content-Type header
   * field, optional linear whitespace, and a terminating CRLF.
   *
   * NOTE: The CRLF preceding the boundary delimiter line is conceptually
   * attached to the boundary so that it is possible to have a part that
   * does not end with a CRLF (line  break).  Body parts that must be
   * considered to end with line breaks, therefore, must have two CRLFs
   * preceding the boundary delimiter line, the first of which is part of
   * the preceding body part, and the second of which is part of the
   * encapsulation boundary.
   *
   * Boundary delimiters must not appear within the encapsulated material,
   * and must be no longer than 70 characters, not counting the two
   * leading hyphens.
   *
   * The boundary delimiter line following the last body part is a
   * distinguished delimiter that indicates that no further body parts
   * will follow.  Such a delimiter line is identical to the previous
   * delimiter lines, with the addition of two more hyphens after the
   * boundary parameter value.
   *
   * See rfc2049 Appendix A for a Complex Multipart Example
  */
    static constexpr const char *multipart = "multipart";

    /* an encapsulated message */
    static constexpr const char *message = "message";
};

struct esp_mail_imap_media_text_sub_type_t
{
    static constexpr const char *plain = "plain";
    static constexpr const char *enriched = "enriched";
    static constexpr const char *html = "html";
};

/* multipart sub types */
struct esp_mail_imap_multipart_sub_type_t
{
    /* a generic mixed set of parts */
    static constexpr const char *mixed = "mixed";

    /* the same data in multiple formats */
    static constexpr const char *alternative = "alternative";

    /* parts intended to be viewed simultaneously */
    static constexpr const char *parallel = "parallel";

    /* multipart entities in which each part has a default type of
   * "message/rfc822" */
    static constexpr const char *digest = "digest";

    /* for compound objects consisting of several inter-related body parts (rfc
   * 2387) */
    static constexpr const char *related = "related";

    /* rfc 3462 */
    static constexpr const char *report = "report";
};

/* message body sub types */
struct esp_mail_imap_message_sub_type_t
{
    /* body contains  an encapsulated message, with the syntax of an RFC 822
   * message. */
    static constexpr const char *rfc822 = "rfc822";

    /* to allow large objects to be delivered as several separate pieces of mail
   */
    static constexpr const char *Partial = "Partial";

    /* the actual body data are not included, but merely referenced */
    static constexpr const char *External_Body = "External-Body";

    static constexpr const char *delivery_status = "delivery-status";
};

/** content disposition rfc 2183
 *
 * Parameters:
 * "filename", "creation-date","modification-date",
 * "read-date", * "size"
*/
struct esp_mail_imap_content_disposition_type_t
{
    /** if it is intended to be displayed automatically
   * upon display of the message.
  */
    static constexpr const char *inline_ = "inline";

    /** to indicate that they are separate from the main body
   * of the mail message, and that their display should not
   * be automatic, but contingent upon some further action of the user.
  */
    static constexpr const char *attachment = "attachment";
};

/* IMAP polling status */
typedef struct esp_mail_imap_polling_status_t
{
    /** The type of status e.g. imap_polling_status_type_undefined, imap_polling_status_type_new_message, 
   * imap_polling_status_type_fetch_message and imap_polling_status_type_remove_message.
  */
    esp_mail_imap_polling_status_type type = imap_polling_status_type_undefined;

    /** Message number or order from the total number of message that added, fetched or deleted.
  */
    size_t messageNum = 0;

    /** Argument of commands e.g. FETCH
  */
    MB_String argument;
} IMAP_Polling_Status;

struct esp_mail_message_part_info_t
{
    int octetLen = 0;
    int octetCount = 0;
    int attach_data_size = 0;
    int textLen = 0;
    bool sizeProp = false;
    int nestedLevel = 0;
    MB_String partNumStr;
    MB_String partNumFetchStr;
    MB_String text;
    MB_String filename;
    MB_String type;
    MB_String save_path;
    MB_String name;
    MB_String content_disposition;
    MB_String content_type;
    MB_String descr;
    MB_String content_transfer_encoding;
    MB_String creation_date;
    MB_String modification_date;
    MB_String charset;
    MB_String download_error;
    esp_mail_attach_type attach_type = esp_mail_att_type_none;
    esp_mail_message_type msg_type = esp_mail_msg_type_none;
    bool file_open_write = false;
    bool multipart = false;
    esp_mail_imap_multipart_sub_type multipart_sub_type = esp_mail_imap_multipart_sub_type_none;
    esp_mail_imap_message_sub_type message_sub_type = esp_mail_imap_message_sub_type_none;
    bool rfc822_part = false;
    int rfc822_msg_Idx = 0;
    struct esp_mail_imap_rfc822_msg_header_item_t rfc822_header;
    bool error = false;
    bool plain_flowed = false;
    bool plain_delsp = false;
    esp_mail_msg_part_xencoding xencoding = esp_mail_msg_part_xencoding_none;
};

struct esp_mail_message_header_t
{
    int header_data_len = 0;

    struct esp_mail_imap_rfc822_msg_header_item_t header_fields;

    MB_String content_type;
    MB_String content_transfer_encoding;
    uint32_t message_uid;
    uint32_t message_no;
    MB_String boundary;
    MB_String accept_language;
    MB_String content_language;
    MB_String char_set;
    bool multipart = false;
    bool rfc822_part = false;
    bool hasAttachment = false;
    int rfc822Idx = 0;
    MB_String partNumStr;

    esp_mail_imap_multipart_sub_type multipart_sub_type = esp_mail_imap_multipart_sub_type_none;
    esp_mail_imap_message_sub_type message_sub_type = esp_mail_imap_message_sub_type_none;
    MB_String msgID;
    MB_String flags;
    MB_String error_msg;
    bool error = false;
    MB_VECTOR<struct esp_mail_message_part_info_t> part_headers;
    int attachment_count = 0;
    int sd_alias_file_count = 0;
    int total_download_size = 0;
    int downloaded_size = 0;
    int total_attach_data_size = 0;
    int downloaded_bytes = 0;
    int message_data_count = 0;
};

/* Internal use */
struct esp_mail_folder_info_t
{
    MB_String name;
    MB_String attributes;
    MB_String delimiter;
};

struct esp_mail_folder_info_item_t
{
    /* The name of folder */
    const char *name = "";

    /* The attributes of folder */
    const char *attributes = "";

    /* The delimeter of folder */
    const char *delimiter = "";
};

struct esp_mail_imap_download_config_t
{
    /* To download the PLAIN text content of the message */
    bool text = false;

    /* To download the HTML content of the message */
    bool html = false;

    /* To download the attachments of the message */
    bool attachment = false;

    /* To download the inline image of the message */
    bool inlineImg = false;

    /* To download the rfc822 mesages in the message */
    bool rfc822 = false;

    /* To download the message header */
    bool header = false;
};

struct esp_mail_imap_enable_config_t
{
    /* To store the PLAIN text of the message in the IMAPSession */
    bool text = false;

    /* To store the HTML of the message in the IMAPSession */
    bool html = false;

    /* To store the rfc822 messages in the IMAPSession */
    bool rfc822 = false;

    /* To enable the download status via the serial port */
    bool download_status = false;

    /* To sort the message UID of the search result in descending order */
    bool recent_sort = false;

    /* To allow case sesitive in header parsing */
    bool header_case_sensitive = false;
};

struct esp_mail_imap_limit_config_t
{
    /* The maximum messages from the search result */
    size_t search = 10;

    /** The maximum size of the memory buffer to store the message content.
   * This is only limit for data to be stored in the IMAPSession.
  */
    size_t msg_size = 1024;

    /* The maximum size of each attachment to download */
    size_t attachment_size = 1024 * 1024 * 5;

    /* The IMAP idle timeout in ms (1 min to 29 min). Default is 10 min */
    size_t imap_idle_timeout = 10 * 60 * 1000;

    /** The IMAP idle host check interval in ms (30 sec to imap_idle_timeout) 
   * for internet availability checking to ensure the connection is active. 
   * Default is 1 min.
  */
    size_t imap_idle_host_check_interval = 60 * 1000;
};

struct esp_mail_imap_storage_config_t
{
    /* The path to save the downloaded file */
    MB_String saved_path;

    /** The type of file storages e.g.
   * esp_mail_file_storage_type_none,
   * esp_mail_file_storage_type_flash, and
   * esp_mail_file_storage_type_sd
  */
    esp_mail_file_storage_type type = esp_mail_file_storage_type_flash;
};

struct esp_mail_imap_search_config_t
{
    /* The search criteria */
    MB_String criteria;

    /* The option to search the unseen message */
    bool unseen_msg = false;
};

struct esp_mail_imap_fetch_config_t
{
    /* The UID of message to fetch */
    MB_String uid;

    /* Set the message flag as seen */
    bool set_seen = false;
};

struct esp_mail_imap_read_config_t
{
    /* The config for fetching */
    struct esp_mail_imap_fetch_config_t fetch;

    /* The config for search */
    struct esp_mail_imap_search_config_t search;

    /* The config about the limits */
    struct esp_mail_imap_limit_config_t limit;

    /* The config to enable the features */
    struct esp_mail_imap_enable_config_t enable;

    /* The config about downloads */
    struct esp_mail_imap_download_config_t download;

    /* The config about the storage and path to save the downloaded file */
    struct esp_mail_imap_storage_config_t storage;
};

/* Mail and MIME Header Fields */
struct esp_mail_imap_msg_item_t
{
    /* The message number */
    int msgNo = 0;

    /* The message UID */
    int UID = 0;

    /* The message identifier (RFC 4021) */
    const char *ID = "";

    /* The language(s) for auto-responses (RFC 4021) */
    const char *acceptLang = "";

    /* The language of message content (RFC 4021) */
    const char *contentLang = "";

    /* The mailbox of message author (RFC 4021) */
    const char *from = "";

    /* The charset of the mailbox of message author */
    //deprecated
    const char *fromCharset = "";

    /* The primary recipient mailbox (RFC 4021) */
    const char *to = "";

    /* The charset of the primary recipient mailbox */
    //deprecated
    const char *toCharset = "";

    /* The Carbon-copy recipient mailboxes (RFC 4021) */
    const char *cc = "";

    /* The charset of the Carbon-copy recipient mailbox header */
    //deprecated
    const char *ccCharset = "";

    /* The message date and time (RFC 4021) */
    const char *date = "";

    /* The topic of message (RFC 4021) */
    const char *subject = "";

    /* The topic of message charset */
    //deprecated
    const char *subjectCharset = "";

    /* The message flags */
    const char *flags = "";

    /* The PLAIN text content of the message */
    struct esp_mail_imap_plain_body_t text;

    /* The HTML content of the message */
    struct esp_mail_imap_html_body_t html;

    /* rfc822 related */

    /* The sender Email  */
    const char *sender;

    /* The charset of the sender Email */
    //deprecated
    const char *senderCharset = "";

    /* The keywords or phrases, separated by commas */
    const char *keywords = "";

    /* The comments about message */
    const char *comments = "";

    /* The field that contains the parent's message ID of the message to which this one is a reply */
    const char *in_reply_to = "";

    /* The field that contains the parent's references (if any) and followed by the parent's message ID (if any) of the message to which this one is a reply */
    const char *references = "";

    /* The return recipient of the message */
    const char *return_path = "";

    /* The Email address to reply */
    const char *reply_to;

    /* The charset of the Blind carbon-copy recipient mailbox header */
    //deprecated
    const char *bccCharset = "";

    /* The error description from fetching the message */
    const char *fetchError = "";

    /* The info about the attachments in the message */
    MB_VECTOR<struct esp_mail_attachment_info_t> attachments;

    /* The info about the rfc822 messages included in the message */
    MB_VECTOR<esp_mail_imap_msg_item_t> rfc822;

    /* The status for message that contains attachment */
    bool hasAttachment = false;
};

struct esp_mail_imap_msg_list_t
{
    /* The info of a message */
    MB_VECTOR<esp_mail_imap_msg_item_t> msgItems;
};

struct esp_mail_imap_multipart_level_t
{
    uint8_t level = 0;
    bool fetch_rfc822_header = false;
    bool append_body_text = false;
};

#endif

#if defined(ENABLE_SMTP) || defined(ENABLE_IMAP)

struct esp_mail_link_internal_t
{
    MB_String cid;
};

struct esp_mail_auth_capability_t
{
    bool plain = false;
    bool xoauth2 = false;
    bool cram_md5 = false;
    bool digest_md5 = false;
    bool login = false;
    bool start_tls = false;
};

struct esp_mail_sesson_cert_config_t
{
    /* The certificate data (base64 data) */
    const char *cert_data = "";

    /* The certificate file (DER format) */
    const char *cert_file = "";

    /* The storage type */
    esp_mail_file_storage_type cert_file_storage_type;

    /* The cerificate verification option */
    bool verify = false;
};

struct esp_mail_sesson_sever_config_t
{
    /* The hostName of the server */
    MB_String host_name;
    /* The port on the server to connect to */
    uint16_t port = 0;
};

/* The log in credentials */
struct esp_mail_sesson_login_config_t
{
    /* The user Email address to log in */
    MB_String email;

    /* The user password to log in */
    MB_String password;

    /* The OAuth2.0 access token to log in */
    MB_String accessToken;

    /* The user domain or ip of client */
    MB_String user_domain;
};

/* The device time config */
struct esp_mail_sesson_time_config_t
{
    /* set the NTP servers (use comma to separate the servers) to let the library to set the time from NTP server */
    MB_String ntp_server;

    /* the GMT offset or time zone */
    float gmt_offset = 0;

    /* the day light saving offset */
    float day_light_offset = 0;
};

struct esp_mail_sesson_secure_config_t
{
    /* The option to send the command to start the TLS connection */
    bool startTLS = false;
};

struct esp_mail_spi_ethernet_module_t
{
#if defined(ESP8266) && defined(ESP8266_CORE_SDK_V3_X_X)
#ifdef INC_ENC28J60_LWIP
    ENC28J60lwIP *enc28j60;
#endif
#ifdef INC_W5100_LWIP
    Wiznet5100lwIP *w5100;
#endif
#ifdef INC_W5500_LWIP
    Wiznet5500lwIP *w5500;
#endif
#endif
};

struct esp_mail_session_config_t
{
    /* The server config */
    struct esp_mail_sesson_sever_config_t server;

    /* The log in config */
    struct esp_mail_sesson_login_config_t login;

    /* The device time config */
    struct esp_mail_sesson_time_config_t time;

    /* The secure config */
    struct esp_mail_sesson_secure_config_t secure;

    /* The certificate config */
    struct esp_mail_sesson_cert_config_t certificate;

    /* SPI Ethernet Module config for ESP8266 */
    struct esp_mail_spi_ethernet_module_t spi_ethernet_module;

    /* The callback function for WiFi connection */
    NetworkConnectionHandler network_connection_handler = NULL;
};

/** The content transfer encoding
 * enc_7bit or "7bit"
 * enc_qp or "quoted-printable"
 * enc_base64 or "base64"
 * enc_binary or "binary"
 * enc_8bit or "8bit"
*/
typedef struct esp_mail_content_transfer_encoding_t Content_Transfer_Encoding;

/* The session configuations */
typedef struct esp_mail_session_config_t ESP_Mail_Session;

#endif

#if defined(ENABLE_SMTP)
/* The result from sending the Email */
typedef struct esp_mail_smtp_send_status_t SMTP_Result;

/* The attachment details for sending the Email */
typedef struct esp_mail_attachment_t SMTP_Attachment;
#endif

#if defined(ENABLE_IMAP)
/* The info of the selected or open mailbox folder e.g. name, attributes and
 * delimiter */
typedef struct esp_mail_folder_info_item_t FolderInfo;
/* The attachment item details for a message which returned from fetching the
 * Email */
typedef struct esp_mail_attachment_info_t IMAP_Attach_Item;

/** The IMAP operation configuations */
typedef struct esp_mail_imap_read_config_t IMAP_Config;

/* The message item data of the IMAP_MSG_List which contains header, body and
 * attachments info for eacch message*/
typedef struct esp_mail_imap_msg_item_t IMAP_MSG_Item;

/* The list that contains the message items from searching or fetching the Email
 */
typedef struct esp_mail_imap_msg_list_t IMAP_MSG_List;

#endif

#if defined(ENABLE_SMTP)
static const char esp_mail_str_1[] PROGMEM = "Content-Type: multipart/mixed; boundary=\"";
static const char esp_mail_str_3[] PROGMEM = "Mime-Version: 1.0\r\n";
static const char esp_mail_str_4[] PROGMEM = "AUTH LOGIN";
static const char esp_mail_str_5[] PROGMEM = "HELO ";
static const char esp_mail_str_6[] PROGMEM = "EHLO ";
static const char esp_mail_str_7[] PROGMEM = "QUIT";
static const char esp_mail_str_8[] PROGMEM = "MAIL FROM:";
static const char esp_mail_str_9[] PROGMEM = "RCPT TO:";
static const char esp_mail_str_13[] PROGMEM = ",<";
static const char esp_mail_str_14[] PROGMEM = "<";
static const char esp_mail_str_15[] PROGMEM = ">";
static const char esp_mail_str_16[] PROGMEM = "DATA";
static const char esp_mail_str_17[] PROGMEM = "X-Priority: ";
static const char esp_mail_str_18[] PROGMEM = "X-MSMail-Priority: High\r\n";
static const char esp_mail_str_19[] PROGMEM = "X-MSMail-Priority: Normal\r\n";
static const char esp_mail_str_20[] PROGMEM = "X-MSMail-Priority: Low\r\n";
static const char esp_mail_str_21[] PROGMEM = "Importance: High\r\n";
static const char esp_mail_str_22[] PROGMEM = "Importance: Normal\r\n";
static const char esp_mail_str_23[] PROGMEM = "Importance: Low\r\n";
static const char esp_mail_str_26[] PROGMEM = "; Name=\"";
static const char esp_mail_str_28[] PROGMEM = "Content-Type: multipart/parallel; boundary=\"";
static const char esp_mail_str_30[] PROGMEM = "Content-Disposition: attachment; filename=\"";
static const char esp_mail_str_32[] PROGMEM = "application/octet-stream";
static const char esp_mail_str_33[] PROGMEM = "--";
static const char esp_mail_str_35[] PROGMEM = "\"\r\n\r\n";
static const char esp_mail_str_36[] PROGMEM = "\"\r\n";
static const char esp_mail_str_37[] PROGMEM = "\r\n.\r\n";
static const char esp_mail_str_39[] PROGMEM = "SMTP server greeting failed";
static const char esp_mail_str_43[] PROGMEM = "authentication failed";
static const char esp_mail_str_44[] PROGMEM = "mydomain.com";
static const char esp_mail_str_45[] PROGMEM = "AUTH PLAIN";
static const char esp_mail_str_47[] PROGMEM = "login password is not valid";
static const char esp_mail_str_48[] PROGMEM = "send header failed";
static const char esp_mail_str_49[] PROGMEM = "send body failed";
static const char esp_mail_str_98[] PROGMEM = "Content-Disposition: attachment\r\n";
static const char esp_mail_str_104[] PROGMEM = " BODY=BINARYMIME";
static const char esp_mail_str_106[] PROGMEM = "BDAT ";
static const char esp_mail_str_110[] PROGMEM = "delsp=\"no\"";
static const char esp_mail_str_120[] PROGMEM = "Connecting to SMTP server...";
static const char esp_mail_str_121[] PROGMEM = "SMTP server connected, wait for greeting...";
static const char esp_mail_str_122[] PROGMEM = "Sending greeting response...";
static const char esp_mail_str_123[] PROGMEM = "message/rfc822";
static const char esp_mail_str_125[] PROGMEM = "Sending message header...";
static const char esp_mail_str_126[] PROGMEM = "Sending message body...";
static const char esp_mail_str_127[] PROGMEM = "Sending attachments...";
static const char esp_mail_str_128[] PROGMEM = "Closing the session...";
static const char esp_mail_str_129[] PROGMEM = "Message sent successfully";
static const char esp_mail_str_149[] PROGMEM = "Bcc: ";
static const char esp_mail_str_158[] PROGMEM = "file does not exist or can't access";
static const char esp_mail_str_159[] PROGMEM = "msg.html";
static const char esp_mail_str_160[] PROGMEM = "upload ";
static const char esp_mail_str_164[] PROGMEM = "msg.txt";
static const char esp_mail_str_166[] PROGMEM = "binary";
static const char esp_mail_str_167[] PROGMEM = "Sending inline data...";
static const char esp_mail_str_173[] PROGMEM = " LAST";
static const char esp_mail_str_183[] PROGMEM = "*";
static const char esp_mail_str_205[] PROGMEM = "sender Email address is not valid";
static const char esp_mail_str_206[] PROGMEM = "some of the recipient Email address is not valid";
static const char esp_mail_str_207[] PROGMEM = "> C: send Email";
static const char esp_mail_str_208[] PROGMEM = "Sending Email...";
static const char esp_mail_str_222[] PROGMEM = "set recipient failed";
static const char esp_mail_str_236[] PROGMEM = "> C: connect to SMTP server";
static const char esp_mail_str_238[] PROGMEM = "> C: smtp server connected";
static const char esp_mail_str_239[] PROGMEM = "> C: send smtp command, HELO";
static const char esp_mail_str_240[] PROGMEM = "> C: send smtp command, AUTH LOGIN";
static const char esp_mail_str_241[] PROGMEM = "> C: send smtp command, AUTH PLAIN";
static const char esp_mail_str_242[] PROGMEM = "> C: send message header";
static const char esp_mail_str_243[] PROGMEM = "> C: send message body";
static const char esp_mail_str_244[] PROGMEM = "> C: send attachments";
static const char esp_mail_str_245[] PROGMEM = "> C: terminate the SMTP session";
static const char esp_mail_str_246[] PROGMEM = "> C: Message sent successfully";
static const char esp_mail_str_260[] PROGMEM = "< S: ";
static const char esp_mail_str_262[] PROGMEM = " NOTIFY=";
static const char esp_mail_str_264[] PROGMEM = "SUCCESS";
static const char esp_mail_str_265[] PROGMEM = "FAILURE";
static const char esp_mail_str_266[] PROGMEM = "DELAY";
static const char esp_mail_str_267[] PROGMEM = "Sending next Email...";
static const char esp_mail_str_268[] PROGMEM = "> C: send next Email";
static const char esp_mail_str_271[] PROGMEM = "> C: send inline data";
static const char esp_mail_str_272[] PROGMEM = "Content-transfer-encoding: ";
static const char esp_mail_str_288[] PROGMEM = "> C: send smtp command, AUTH XOAUTH2";
static const char esp_mail_str_289[] PROGMEM = "AUTH XOAUTH2 ";
static const char esp_mail_str_293[] PROGMEM = "OAuth2.0 log in was disabled for this server";
static const char esp_mail_str_297[] PROGMEM = "Content-Type: multipart/alternative; boundary=\"";
static const char esp_mail_str_298[] PROGMEM = "Content-Type: multipart/related; boundary=\"";
static const char esp_mail_str_299[] PROGMEM = "Content-Disposition: inline; filename=\"";
static const char esp_mail_str_300[] PROGMEM = "Content-Location: ";
static const char esp_mail_str_301[] PROGMEM = "Content-ID: <";
static const char esp_mail_str_302[] PROGMEM = "cid:";
static const char esp_mail_str_303[] PROGMEM = "Finishing the message sending...";
static const char esp_mail_str_304[] PROGMEM = "> C: Finish the message sending";
static const char esp_mail_str_312[] PROGMEM = "code: ";
static const char esp_mail_str_313[] PROGMEM = ", text: ";
static const char esp_mail_str_325[] PROGMEM = "flash content message";
static const char esp_mail_str_326[] PROGMEM = "file content message";
static const char esp_mail_str_327[] PROGMEM = "\"; size=";

static const char esp_mail_smtp_response_1[] PROGMEM = "AUTH ";
static const char esp_mail_smtp_response_2[] PROGMEM = " LOGIN";
static const char esp_mail_smtp_response_3[] PROGMEM = " PLAIN";
static const char esp_mail_smtp_response_4[] PROGMEM = " XOAUTH2";
static const char esp_mail_smtp_response_5[] PROGMEM = "STARTTLS";
static const char esp_mail_smtp_response_6[] PROGMEM = "8BITMIME";
static const char esp_mail_smtp_response_7[] PROGMEM = "BINARYMIME";
static const char esp_mail_smtp_response_8[] PROGMEM = "CHUNKING";
static const char esp_mail_smtp_response_9[] PROGMEM = "SMTPUTF8";
static const char esp_mail_smtp_response_10[] PROGMEM = "PIPELINING";
static const char esp_mail_smtp_response_11[] PROGMEM = " CRAM-MD5";
static const char esp_mail_smtp_response_12[] PROGMEM = " DIGEST-MD5";
static const char esp_mail_smtp_response_13[] PROGMEM = "DSN";

static const char boundary_table[] PROGMEM = "=_abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

#endif

#if defined(ENABLE_IMAP)

static const char esp_mail_str_2[] PROGMEM = "$ CAPABILITY";
static const char esp_mail_str_27[] PROGMEM = "$";
static const char esp_mail_str_29[] PROGMEM = "7bit";
static const char esp_mail_str_31[] PROGMEM = "base64";
static const char esp_mail_str_40[] PROGMEM = ".dat";
static const char esp_mail_str_41[] PROGMEM = "$ AUTHENTICATE PLAIN ";
static const char esp_mail_str_50[] PROGMEM = "Connecting to IMAP server...";
static const char esp_mail_str_51[] PROGMEM = "* ";
static const char esp_mail_str_54[] PROGMEM = "IMAP server connected";
static const char esp_mail_str_55[] PROGMEM = "> C: download attachment ";
static const char esp_mail_str_57[] PROGMEM = "Downloading messages...";
static const char esp_mail_str_58[] PROGMEM = "Reading the list of mailboxes...";
static const char esp_mail_str_59[] PROGMEM = "> C: download plain TEXT message";
static const char esp_mail_str_60[] PROGMEM = "> C: download HTML message";
static const char esp_mail_str_61[] PROGMEM = "Selecting the ";
static const char esp_mail_str_62[] PROGMEM = "fail to list the mailboxes";
static const char esp_mail_str_63[] PROGMEM = "fail to check the capabilities";
static const char esp_mail_str_64[] PROGMEM = "Check the capability...";
static const char esp_mail_str_65[] PROGMEM = "> C: check the capability";
static const char esp_mail_str_66[] PROGMEM = "Searching messages...";
static const char esp_mail_str_67[] PROGMEM = "message";
static const char esp_mail_str_68[] PROGMEM = "Search limit:";
static const char esp_mail_str_69[] PROGMEM = "Found ";
static const char esp_mail_str_70[] PROGMEM = " messages";
static const char esp_mail_str_71[] PROGMEM = "Show ";
static const char esp_mail_str_72[] PROGMEM = "No message found for search criteria";
static const char esp_mail_str_73[] PROGMEM = "\nNo search criteria provided, then fetching the latest message";
static const char esp_mail_str_74[] PROGMEM = "Fetching message ";
static const char esp_mail_str_75[] PROGMEM = ", UID: ";
static const char esp_mail_str_76[] PROGMEM = ", Number: ";
static const char esp_mail_str_77[] PROGMEM = "> C: fetch message header";
static const char esp_mail_str_78[] PROGMEM = "Attachments (";
static const char esp_mail_str_79[] PROGMEM = ")";
static const char esp_mail_str_80[] PROGMEM = "Downloading attachments...";
static const char esp_mail_str_81[] PROGMEM = "> C: fetch body part header, ";
static const char esp_mail_str_82[] PROGMEM = "rfc822";
static const char esp_mail_str_83[] PROGMEM = "reading";
static const char esp_mail_str_84[] PROGMEM = "Free Heap: ";
static const char esp_mail_str_85[] PROGMEM = "Logging out...";
static const char esp_mail_str_86[] PROGMEM = "> C: fetch body sub part header, ";
static const char esp_mail_str_87[] PROGMEM = "Finished reading Email";
static const char esp_mail_str_88[] PROGMEM = "> C: finished reading Email";
static const char esp_mail_str_90[] PROGMEM = "download";
static const char esp_mail_str_93[] PROGMEM = "connection timeout";
static const char esp_mail_str_94[] PROGMEM = ".html";
static const char esp_mail_str_95[] PROGMEM = ".txt";
static const char esp_mail_str_96[] PROGMEM = " folder...";
static const char esp_mail_str_100[] PROGMEM = "UID:";
static const char esp_mail_str_102[] PROGMEM = "Accept-Language: ";
static const char esp_mail_str_103[] PROGMEM = "Content-Language: ";
static const char esp_mail_str_105[] PROGMEM = "> C: get flags...";
static const char esp_mail_str_108[] PROGMEM = "CC: ";
static const char esp_mail_str_111[] PROGMEM = "> C: UID is ";
static const char esp_mail_str_112[] PROGMEM = " FETCH (FLAGS (";
static const char esp_mail_str_113[] PROGMEM = "Attachment: ";
static const char esp_mail_str_114[] PROGMEM = "File Index: ";
static const char esp_mail_str_115[] PROGMEM = "Filename: ";
static const char esp_mail_str_116[] PROGMEM = "Name: ";
static const char esp_mail_str_117[] PROGMEM = "Size: ";
static const char esp_mail_str_118[] PROGMEM = "Type: ";
static const char esp_mail_str_119[] PROGMEM = "Creation Date: ";
static const char esp_mail_str_124[] PROGMEM = "Saving message header to file...";
static const char esp_mail_str_130[] PROGMEM = "$ LOGIN ";
static const char esp_mail_str_133[] PROGMEM = "$ LIST \"\" *";
static const char esp_mail_str_135[] PROGMEM = "$ EXAMINE \"";
static const char esp_mail_str_137[] PROGMEM = "UID ";
static const char esp_mail_str_138[] PROGMEM = " UID";
static const char esp_mail_str_139[] PROGMEM = " SEARCH";
static const char esp_mail_str_140[] PROGMEM = "UID";
static const char esp_mail_str_141[] PROGMEM = "SEARCH";
static const char esp_mail_str_142[] PROGMEM = "$ UID FETCH ";
static const char esp_mail_str_143[] PROGMEM = "$ FETCH ";
static const char esp_mail_str_144[] PROGMEM = "HEADER.FIELDS (SUBJECT FROM SENDER RETURN-PATH TO REPLY-TO IN-REPLY-TO REFERENCES DATE CC Message-ID COMMENTS KEYWORDS content-type Content-transfer-encoding Content-Language Accept-Language)";
static const char esp_mail_str_146[] PROGMEM = "$ LOGOUT";
static const char esp_mail_str_147[] PROGMEM = " BODY";
static const char esp_mail_str_148[] PROGMEM = ".MIME]";
static const char esp_mail_str_151[] PROGMEM = "no mailbox opened";
static const char esp_mail_str_152[] PROGMEM = ".";
static const char esp_mail_str_153[] PROGMEM = "No mailbox opened";
static const char esp_mail_str_154[] PROGMEM = "Remove FLAG";
static const char esp_mail_str_155[] PROGMEM = "Add FLAG";
static const char esp_mail_str_156[] PROGMEM = "Get UID...";
static const char esp_mail_str_157[] PROGMEM = "Set FLAG";
static const char esp_mail_str_161[] PROGMEM = "/msg";
static const char esp_mail_str_163[] PROGMEM = "/rfc822_msg";
static const char esp_mail_str_169[] PROGMEM = "charset=";
static const char esp_mail_str_170[] PROGMEM = "name=\"";
static const char esp_mail_str_171[] PROGMEM = "name=";
static const char esp_mail_str_172[] PROGMEM = "content-transfer-encoding:";
static const char esp_mail_str_174[] PROGMEM = "content-description:";
static const char esp_mail_str_175[] PROGMEM = "content-disposition:";
static const char esp_mail_str_176[] PROGMEM = "filename=\"";
static const char esp_mail_str_177[] PROGMEM = "filename=";
static const char esp_mail_str_178[] PROGMEM = "size=";
static const char esp_mail_str_179[] PROGMEM = "creation-date=\"";
static const char esp_mail_str_180[] PROGMEM = "creation-date=";
static const char esp_mail_str_181[] PROGMEM = "modification-date=\"";
static const char esp_mail_str_182[] PROGMEM = "modification-date=";
static const char esp_mail_str_187[] PROGMEM = "Message fetch cmpleted";
static const char esp_mail_str_188[] PROGMEM = "fail to close the mailbox";
static const char esp_mail_str_189[] PROGMEM = "> C: get UID...";
static const char esp_mail_str_190[] PROGMEM = "accept-language:";
static const char esp_mail_str_191[] PROGMEM = "content-language:";
static const char esp_mail_str_192[] PROGMEM = ")";
static const char esp_mail_str_193[] PROGMEM = "{";
static const char esp_mail_str_194[] PROGMEM = "}";
static const char esp_mail_str_195[] PROGMEM = "$ CLOSE\r\n";
static const char esp_mail_str_197[] PROGMEM = "> C: close the mailbox folder";
static const char esp_mail_str_198[] PROGMEM = "(";
static const char esp_mail_str_199[] PROGMEM = " EXISTS";
static const char esp_mail_str_200[] PROGMEM = " [UIDNEXT ";
static const char esp_mail_str_203[] PROGMEM = "/header.txt";
static const char esp_mail_str_210[] PROGMEM = "Closing the ";
static const char esp_mail_str_212[] PROGMEM = "FLAGS";
static const char esp_mail_str_213[] PROGMEM = "BODY";
static const char esp_mail_str_214[] PROGMEM = "PEEK";
static const char esp_mail_str_215[] PROGMEM = "TEXT";
static const char esp_mail_str_216[] PROGMEM = "HEADER";
static const char esp_mail_str_217[] PROGMEM = "FIELDS";
static const char esp_mail_str_218[] PROGMEM = "[";
static const char esp_mail_str_219[] PROGMEM = "]";
static const char esp_mail_str_220[] PROGMEM = "MIME";
static const char esp_mail_str_223[] PROGMEM = " NEW";
static const char esp_mail_str_224[] PROGMEM = "ALL";
static const char esp_mail_str_225[] PROGMEM = "> C: connect to IMAP server";
static const char esp_mail_str_226[] PROGMEM = "windows-874";
static const char esp_mail_str_227[] PROGMEM = "iso-8859-1";
static const char esp_mail_str_228[] PROGMEM = "> C: server connected";
static const char esp_mail_str_229[] PROGMEM = "> C: send imap command, LOGIN";
static const char esp_mail_str_230[] PROGMEM = "> C: send imap command, LIST";
static const char esp_mail_str_231[] PROGMEM = "iso-8859-11";
static const char esp_mail_str_232[] PROGMEM = "> C: search messages";
static const char esp_mail_str_233[] PROGMEM = "> C: send imap command, FETCH";
static const char esp_mail_str_234[] PROGMEM = "> C: send imap command, LOGOUT";
static const char esp_mail_str_235[] PROGMEM = "> C: message fetch completed";
static const char esp_mail_str_237[] PROGMEM = "tis-620";
static const char esp_mail_str_247[] PROGMEM = "$ SELECT \"";
static const char esp_mail_str_248[] PROGMEM = "> C: open the mailbox folder";
static const char esp_mail_str_249[] PROGMEM = "$ UID STORE ";
static const char esp_mail_str_250[] PROGMEM = " FLAGS (";
static const char esp_mail_str_251[] PROGMEM = " +FLAGS (";
static const char esp_mail_str_252[] PROGMEM = " -FLAGS (";
static const char esp_mail_str_253[] PROGMEM = "> C: set FLAG";
static const char esp_mail_str_254[] PROGMEM = "> C: add FLAG";
static const char esp_mail_str_255[] PROGMEM = "> C: remove FLAG";
static const char esp_mail_str_256[] PROGMEM = "could not parse flag";
static const char esp_mail_str_257[] PROGMEM = "delsp=\"yes\"";
static const char esp_mail_str_259[] PROGMEM = "delsp=yes";
static const char esp_mail_str_269[] PROGMEM = "header.fields (content-type Content-transfer-encoding)]";
static const char esp_mail_str_273[] PROGMEM = " (FLAGS)";
static const char esp_mail_str_274[] PROGMEM = "UID is ";
static const char esp_mail_str_275[] PROGMEM = "format=flowed";
static const char esp_mail_str_276[] PROGMEM = "Number:";
static const char esp_mail_str_277[] PROGMEM = "boundary=\"";
static const char esp_mail_str_278[] PROGMEM = "quoted-printable";
static const char esp_mail_str_279[] PROGMEM = "Get Flags...";
static const char esp_mail_str_280[] PROGMEM = "> C: no content";
static const char esp_mail_str_281[] PROGMEM = "fail to open the mailbox";
static const char esp_mail_str_290[] PROGMEM = "> C: send imap command, AUTHENTICATE PLAIN";
static const char esp_mail_str_291[] PROGMEM = "> C: send imap command, AUTH XOAUTH2";
static const char esp_mail_str_292[] PROGMEM = "$ AUTHENTICATE XOAUTH2 ";
static const char esp_mail_str_295[] PROGMEM = "0123456789ABCDEF";
static const char esp_mail_str_306[] PROGMEM = "some of the requested messages no longer exist";
static const char esp_mail_str_307[] PROGMEM = "Reading messages...";
static const char esp_mail_str_308[] PROGMEM = "> C: reading plain TEXT message";
static const char esp_mail_str_309[] PROGMEM = "> C: reading HTML message";
static const char esp_mail_str_315[] PROGMEM = " +FLAGS.SILENT (\\Deleted)";
static const char esp_mail_str_316[] PROGMEM = "> C: delete message(s)";
static const char esp_mail_str_317[] PROGMEM = "$ EXPUNGE";
static const char esp_mail_str_318[] PROGMEM = "> C: copy message(s) to ";
static const char esp_mail_str_319[] PROGMEM = "$ UID COPY ";
static const char esp_mail_str_320[] PROGMEM = "> C: create folder";
static const char esp_mail_str_321[] PROGMEM = "> C: delete folder";
static const char esp_mail_str_322[] PROGMEM = "$ CREATE ";
static const char esp_mail_str_323[] PROGMEM = "$ DELETE ";
static const char esp_mail_str_324[] PROGMEM = "HEADER.FIELDS";
static const char esp_mail_str_331[] PROGMEM = "$ IDLE";
static const char esp_mail_str_332[] PROGMEM = "DONE";
static const char esp_mail_str_333[] PROGMEM = " EXPUNGE";
static const char esp_mail_str_334[] PROGMEM = " RECENT";
static const char esp_mail_str_335[] PROGMEM = "> C: listening to mailbox changes...";
static const char esp_mail_str_336[] PROGMEM = "Listening to ";
static const char esp_mail_str_337[] PROGMEM = " folder changes...";
static const char esp_mail_str_338[] PROGMEM = "Polling established";
static const char esp_mail_str_339[] PROGMEM = "> C: polling established";
static const char esp_mail_str_340[] PROGMEM = "Mailbox listening stopped";
static const char esp_mail_str_341[] PROGMEM = "> C: mailbox listening stopped";
static const char esp_mail_str_342[] PROGMEM = " FETCH (UID ";

// Tagged
static const char esp_mail_imap_response_1[] PROGMEM = "$ OK ";
static const char esp_mail_imap_response_2[] PROGMEM = "$ NO ";
static const char esp_mail_imap_response_3[] PROGMEM = "$ BAD ";
// Untagged
static const char esp_mail_imap_response_4[] PROGMEM = "* LIST ";
static const char esp_mail_imap_response_5[] PROGMEM = "* FLAGS ";
static const char esp_mail_imap_response_6[] PROGMEM = "* SEARCH ";
static const char esp_mail_imap_response_7[] PROGMEM = " FETCH ";
static const char esp_mail_imap_response_8[] PROGMEM = " NIL ";
static const char esp_mail_imap_response_9[] PROGMEM = " UID ";
static const char esp_mail_imap_response_10[] PROGMEM = "* CAPABILITY ";
static const char esp_mail_imap_response_11[] PROGMEM = "LOGINDISABLED";
static const char esp_mail_imap_response_12[] PROGMEM = "AUTH=PLAIN";
static const char esp_mail_imap_response_13[] PROGMEM = "AUTH=XOAUTH2";
static const char esp_mail_imap_response_14[] PROGMEM = "STARTTLS";
static const char esp_mail_imap_response_15[] PROGMEM = "CRAM-MD5";
static const char esp_mail_imap_response_16[] PROGMEM = "DIGEST-MD5";
static const char esp_mail_imap_response_17[] PROGMEM = "IDLE"; //rfc2177
static const char esp_mail_imap_response_18[] PROGMEM = "IMAP4";
static const char esp_mail_imap_response_19[] PROGMEM = "IMAP4rev1";

static const char imap_7bit_key1[] PROGMEM = "=20";
static const char imap_7bit_val1[] PROGMEM = " ";
static const char imap_7bit_key2[] PROGMEM = "=2C";
static const char imap_7bit_val2[] PROGMEM = ",";
static const char imap_7bit_key3[] PROGMEM = "=E2=80=99";
static const char imap_7bit_val3[] PROGMEM = "'";
static const char imap_7bit_key4[] PROGMEM = "=0A";
static const char imap_7bit_val4[] PROGMEM = "\r\n";
static const char imap_7bit_key5[] PROGMEM = "=0D";
static const char imap_7bit_val5[] PROGMEM = "\r\n";
static const char imap_7bit_key6[] PROGMEM = "=A0";
static const char imap_7bit_val6[] PROGMEM = " ";
static const char imap_7bit_key7[] PROGMEM = "=B9";
static const char imap_7bit_val7[] PROGMEM = "$sup1";
static const char imap_7bit_key8[] PROGMEM = "=C2=A0";
static const char imap_7bit_val8[] PROGMEM = " ";
static const char imap_7bit_key9[] PROGMEM = "=\r\n";
static const char imap_7bit_val9[] PROGMEM = "";
static const char imap_7bit_key10[] PROGMEM = "=E2=80=A6";
static const char imap_7bit_val10[] PROGMEM = "&hellip;";
static const char imap_7bit_key11[] PROGMEM = "=E2=80=A2";
static const char imap_7bit_val11[] PROGMEM = "&bull;";
static const char imap_7bit_key12[] PROGMEM = "=E2=80=93";
static const char imap_7bit_val12[] PROGMEM = "&ndash;";
static const char imap_7bit_key13[] PROGMEM = "=E2=80=94";
static const char imap_7bit_val13[] PROGMEM = "&mdash;";

#endif

#if defined(ENABLE_SMTP) || defined(ENABLE_IMAP)

static const char esp_mail_str_10[] PROGMEM = "From:";
static const char esp_mail_str_11[] PROGMEM = "To:";
static const char esp_mail_str_12[] PROGMEM = "Cc:";
static const char esp_mail_str_24[] PROGMEM = "Subject:";
static const char esp_mail_str_25[] PROGMEM = "Content-Type:";
static const char esp_mail_str_34[] PROGMEM = "\r\n";
static const char esp_mail_str_38[] PROGMEM = "unable to connect to server";
static const char esp_mail_str_42[] PROGMEM = "the provided SASL authentication mechanism is not support";
static const char esp_mail_str_46[] PROGMEM = "Return-Path:";
static const char esp_mail_str_53[] PROGMEM = "Error, ";
static const char esp_mail_str_56[] PROGMEM = "Logging in...";
static const char esp_mail_str_91[] PROGMEM = ", ";
static const char esp_mail_str_92[] PROGMEM = "%";
static const char esp_mail_str_97[] PROGMEM = ";";
static const char esp_mail_str_99[] PROGMEM = "Date:";
static const char esp_mail_str_101[] PROGMEM = "Message-ID:";
static const char esp_mail_str_107[] PROGMEM = "References:";
static const char esp_mail_str_109[] PROGMEM = "In-Reply-To:";
static const char esp_mail_str_131[] PROGMEM = " ";
static const char esp_mail_str_132[] PROGMEM = "fail to set up the SSL/TLS structure";
static const char esp_mail_str_134[] PROGMEM = "Comments:";
static const char esp_mail_str_136[] PROGMEM = "\"";
static const char esp_mail_str_145[] PROGMEM = "Keywords:";
static const char esp_mail_str_150[] PROGMEM = "Sender:";
static const char esp_mail_str_168[] PROGMEM = "charset=\"";
static const char esp_mail_str_184[] PROGMEM = "Reply-To:";
static const char esp_mail_str_185[] PROGMEM = "> E: ";
static const char esp_mail_str_186[] PROGMEM = "out of memory";
static const char esp_mail_str_196[] PROGMEM = "> C: send STARTTLS command";
static const char esp_mail_str_201[] PROGMEM = "port > ";
static const char esp_mail_str_204[] PROGMEM = "/esp.32";
static const char esp_mail_str_221[] PROGMEM = "connection closed";
static const char esp_mail_str_202[] PROGMEM = "/";
static const char esp_mail_str_209[] PROGMEM = "Send command, STARTTLS";
static const char esp_mail_str_211[] PROGMEM = "host > ";
static const char esp_mail_str_258[] PROGMEM = "session timed out";
static const char esp_mail_str_261[] PROGMEM = "> C: ";
static const char esp_mail_str_263[] PROGMEM = ",";
static const char esp_mail_str_270[] PROGMEM = "format=\"flowed\"";
static const char esp_mail_str_282[] PROGMEM = "file I/O error";
static const char esp_mail_str_285[] PROGMEM = "user=";
static const char esp_mail_str_286[] PROGMEM = "\1auth=Bearer ";
static const char esp_mail_str_287[] PROGMEM = "\1\1";
static const char esp_mail_str_294[] PROGMEM = "{\"status\":";
static const char esp_mail_str_305[] PROGMEM = "connection failed";
static const char esp_mail_str_310[] PROGMEM = "> C: performing the SSL/TLS handshake";
static const char esp_mail_str_311[] PROGMEM = "STARTTLS\r\n";
static const char esp_mail_str_314[] PROGMEM = "> C: ESP Mail Client v";
static const char esp_mail_str_328[] PROGMEM = "0.0.0.0";
static const char esp_mail_str_329[] PROGMEM = ", Fw v";
static const char esp_mail_str_330[] PROGMEM = "+";
static const char esp_mail_str_343[] PROGMEM = " on ";
static const char esp_mail_str_344[] PROGMEM = "data sending failed";
static const char esp_mail_str_345[] PROGMEM = "connection refused";
static const char esp_mail_str_346[] PROGMEM = "Client is not yet initialized";
static const char esp_mail_str_347[] PROGMEM = "/header.json";
static const char esp_mail_str_352[] PROGMEM = "Custom Client is not yet enabled";
#endif

#if defined(MBFS_FLASH_FS) || defined(MBFS_SD_FS)
static const char esp_mail_str_348[] PROGMEM = "Flash Storage is not ready.";
static const char esp_mail_str_349[] PROGMEM = "SD Storage is not ready.";
static const char esp_mail_str_350[] PROGMEM = "File is still opened.";
static const char esp_mail_str_351[] PROGMEM = "File not found.";
#endif

#if defined(ENABLE_SMTP) || defined(ENABLE_IMAP)

static const unsigned char b64_index_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

__attribute__((used)) static bool compFunc(uint32_t i, uint32_t j)
{
    return (i > j);
}

static void __attribute__((used)) esp_mail_debug(const char *msg)
{
    delay(0);
    ESP_MAIL_DEFAULT_DEBUG_PORT.println(msg);
}

static void __attribute__((used))
esp_mail_debug_line(const char *msg, bool newline)
{
    delay(0);
    if (newline)
        ESP_MAIL_DEFAULT_DEBUG_PORT.println(msg);
    else
        ESP_MAIL_DEFAULT_DEBUG_PORT.print(msg);
}

#endif

typedef void (*ConnectionRequestCallback)(const char *, int);
typedef void (*ConnectionUpgradeRequestCallback)(void);
typedef void (*NetworkConnectionRequestCallback)(void);
typedef void (*NetworkStatusRequestCallback)(void);
#endif