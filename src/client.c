#include <luacoap/client.h>

typedef struct {
  const char* url;

  bool with_payload;
  const char* content;
  coap_size_t content_len;
  coap_content_type_t ct;

  coap_code_t outbound_code;
  coap_transaction_type_t outbound_tt;
  coap_code_t expected_code;
} request_s;

static int gRet;
static sig_t previous_sigint_handler;
static void signal_interrupt(int sig) {
  gRet = ERRORCODE_INTERRUPT;
  signal(SIGINT, previous_sigint_handler);
}

static smcp_status_t get_response_handler(int statuscode, void* context) {
  const char* content = (const char*)smcp_inbound_get_content_ptr();
  coap_size_t content_length = smcp_inbound_get_content_len();

  if (statuscode >= 0) {
    if (content_length > (smcp_inbound_get_packet_length() - 4)) {
      fprintf(stderr,
              "INTERNAL ERROR: CONTENT_LENGTH LARGER THAN "
              "PACKET_LENGTH-4!(content_length=%u, packet_length=%u)\n",
              content_length, smcp_inbound_get_packet_length());
      gRet = ERRORCODE_UNKNOWN;
      goto bail;
    }

    if (!coap_verify_packet((void*)smcp_inbound_get_packet(),
                            smcp_inbound_get_packet_length())) {
      fprintf(stderr, "INTERNAL ERROR: CALLBACK GIVEN INVALID PACKET!\n");
      gRet = ERRORCODE_UNKNOWN;
      goto bail;
    }
  }

  if (statuscode == SMCP_STATUS_TRANSACTION_INVALIDATED) {
    gRet = 0;
  }

  if (((statuscode < COAP_RESULT_200) || (statuscode >= COAP_RESULT_400)) &&
      (statuscode != SMCP_STATUS_TRANSACTION_INVALIDATED) &&
      (statuscode != HTTP_TO_COAP_CODE(HTTP_RESULT_CODE_PARTIAL_CONTENT))) {
    if (statuscode == SMCP_STATUS_TIMEOUT) {
      gRet = 0;
    } else {
      gRet = (statuscode == SMCP_STATUS_TIMEOUT) ? ERRORCODE_TIMEOUT
                                                 : ERRORCODE_COAP_ERROR;
      fprintf(stderr, "get: Result code = %d (%s)\n", statuscode,
              (statuscode < 0) ? smcp_status_to_cstr(statuscode)
                               : coap_code_to_cstr(statuscode));
    }
  }

  if ((statuscode > 0) && content && content_length) {
    coap_option_key_t key;
    const uint8_t* value;
    coap_size_t value_len;
    bool last_block = true;
    int32_t observe_value = -1;

    while ((key = smcp_inbound_next_option(&value, &value_len)) !=
           COAP_OPTION_INVALID) {
      if (key == COAP_OPTION_BLOCK2) {
        last_block = !(value[value_len - 1] & (1 << 3));
      } else if (key == COAP_OPTION_OBSERVE) {
        if (value_len)
          observe_value = value[0];
        else
          observe_value = 0;
      }
    }

    fwrite(content, content_length, 1, stdout);

    if (last_block) {
      // Only print a newline if the content doesn't already print one.
      if (content_length && (content[content_length - 1] != '\n')) printf("\n");
    }

    fflush(stdout);
  }

bail:
  return SMCP_STATUS_OK;
}

static smcp_status_t resend_get_request(void* context) {
  request_s* request = (request_s*)context;
  smcp_status_t status = 0;

  status = smcp_outbound_begin(smcp_get_current_instance(),
                               request->outbound_code, request->outbound_tt);
  require_noerr(status, bail);

  status = smcp_outbound_set_uri(request->url, 0);
  require_noerr(status, bail);

  if (request->with_payload) {
    status =
        smcp_outbound_add_option_uint(COAP_OPTION_CONTENT_TYPE, request->ct);
    require_noerr(status, bail);

    status =
        smcp_outbound_append_content(request->content, request->content_len);
    require_noerr(status, bail);
  }

  status = smcp_outbound_send();

  if (status) {
    fprintf(stderr, "smcp_outbound_send() returned error %d(%s).\n", status,
            smcp_status_to_cstr(status));
  }

bail:
  return status;
}

static int create_transaction(smcp_t smcp, void* request) {
  smcp_status_t status = 0;
  struct smcp_transaction_s transaction;
  previous_sigint_handler = signal(SIGINT, &signal_interrupt);
  smcp_transaction_end(smcp, &transaction);
  smcp_transaction_init(&transaction, SMCP_TRANSACTION_ALWAYS_INVALIDATE,
                        (void*)&resend_get_request,
                        (void*)&get_response_handler, request);

  status = smcp_transaction_begin(smcp, &transaction, 30 * MSEC_PER_SEC);

  if (status) {
    fprintf(stderr, "smcp_begin_transaction_old() returned %d(%s).\n", status,
            smcp_status_to_cstr(status));
    return false;
  }

  while (ERRORCODE_INPROGRESS == gRet) {
    smcp_wait(smcp, 1000);
    smcp_process(smcp);
  }

  smcp_transaction_end(smcp, &transaction);
  signal(SIGINT, previous_sigint_handler);
  return gRet;
}

int send_request(smcp_t smcp, coap_code_t method, int get_tt,
                     const char* url) {
  gRet = ERRORCODE_INPROGRESS;

  request_s request_data;
  memset(&request_data, 0, sizeof(request_data));
  request_data.outbound_code = method;
  request_data.outbound_tt = get_tt;
  request_data.expected_code = COAP_RESULT_205_CONTENT;
  request_data.url = url;
  request_data.with_payload = false;

  return create_transaction(smcp, (void*)&request_data);
}

int send_request_with_payload(smcp_t smcp, coap_code_t method, int get_tt,
                                  const char* url, coap_content_type_t ct,
                                  const char* payload, size_t payload_length) {
  gRet = ERRORCODE_INPROGRESS;

  request_s request_data;
  memset(&request_data, 0, sizeof(request_data));
  request_data.outbound_code = method;
  request_data.outbound_tt = get_tt;
  request_data.expected_code = COAP_RESULT_205_CONTENT;
  request_data.url = url;
  request_data.content = payload;
  request_data.content_len = payload_length;
  request_data.ct = ct;
  request_data.with_payload = true;

  return create_transaction(smcp, (void*)&request_data);
}
