#ifndef COAP_CLIENT_HH__
#define COAP_CLIENT_HH__

#include <signal.h>

#include <smcp/assert-macros.h>
#include <smcp/smcp.h>

#define ERRORCODE_OK (0)
#define ERRORCODE_HELP (1)
#define ERRORCODE_BADARG (2)
#define ERRORCODE_NOCOMMAND (3)
#define ERRORCODE_UNKNOWN (4)
#define ERRORCODE_BADCOMMAND (5)
#define ERRORCODE_NOREADLINE (6)
#define ERRORCODE_QUIT (7)
#define ERRORCODE_INIT_FAILURE (8)
#define ERRORCODE_TIMEOUT (9)
#define ERRORCODE_COAP_ERROR (10)

#define ERRORCODE_INTERRUPT (128 + SIGINT)
#define ERRORCODE_SIGHUP (128 + SIGHUP)

#define ERRORCODE_INPROGRESS (127)

/**
 *  Request Context Structure.
 */
typedef struct {
  const char *url;                        /**< CoAP resource URL */
  const char *content;                    /**< Playload */
  coap_size_t content_len;                /**< Playload length */
  coap_content_type_t ct;                 /**< Content type */

  cms_t timeout;                          /**< Request timeout */

  coap_code_t outbound_code;
  coap_transaction_type_t outbound_tt;
  coap_code_t expected_code;

  // Callback
  void* data;
  void* (*callback)(void *, const char* payload, size_t payload_length);

} request_s;

typedef request_s *request_t;

request_t create_request(request_t request, coap_code_t method, int get_tt, const char *url,
                         coap_content_type_t ct, const char *payload,
                         size_t payload_length, bool observe, void* data, void*(*cb)(void*, const char*, size_t));

int send_request(smcp_t smcp, coap_code_t method, int get_tt, const char *url,
                 coap_content_type_t ct, const char *payload,
                 size_t payload_length, bool observe, char *output_content,
                 size_t *content_size);

int settup_observe_request(smcp_t smcp, request_t request, smcp_transaction_t t);

#endif /* COAP_CLIENT_HH__ */
