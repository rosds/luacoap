#include <luacoap/client.h>

static bool observe;
static int gRet;
static sig_t previous_sigint_handler;
static void signal_interrupt(int sig) {
  gRet = ERRORCODE_INTERRUPT;
  signal(SIGINT, previous_sigint_handler);
}


static smcp_status_t resend_get_request(void* context);
static smcp_status_t get_response_handler(int statuscode, void* context);

request_t create_request(request_t request, coap_code_t method, int get_tt,
                         const char* url, coap_content_type_t ct,
                         const char* payload, size_t payload_length, bool obs,
                         void* data, void* (*cb)(void*, const char*, size_t)) {

  memset(request, 0, sizeof(request_s));
  request->outbound_code = method;
  request->outbound_tt = get_tt;
  request->expected_code = COAP_RESULT_205_CONTENT;
  request->url = url;
  request->content = payload;
  request->content_len = payload_length;
  request->ct = ct;
  request->timeout = obs ? CMS_DISTANT_FUTURE : 30 * MSEC_PER_SEC;
  request->data = data;
  request->callback = cb;

  return request;
}

int send_request(smcp_t smcp, request_t request) {
  gRet = ERRORCODE_INPROGRESS;
  observe = false;

  smcp_status_t status = 0;
  struct smcp_transaction_s transaction;
  previous_sigint_handler = signal(SIGINT, &signal_interrupt);

  int flags = SMCP_TRANSACTION_ALWAYS_INVALIDATE;

  smcp_transaction_end(smcp, &transaction);
  smcp_transaction_init(&transaction, flags, (void*)&resend_get_request,
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

int settup_observe_request(smcp_t smcp, request_t request,
                           smcp_transaction_t t) {
  gRet = ERRORCODE_INPROGRESS;
  observe = true;

  int flags = SMCP_TRANSACTION_ALWAYS_INVALIDATE | SMCP_TRANSACTION_OBSERVE;

  smcp_transaction_end(smcp, t);
  smcp_transaction_init(t, flags, (void*)&resend_get_request,
                        (void*)&get_response_handler, request);

  return 0;
}

static smcp_status_t resend_get_request(void* context) {
  request_s* request = (request_s*)context;
  smcp_status_t status = 0;

  status = smcp_outbound_begin(smcp_get_current_instance(),
                               request->outbound_code, request->outbound_tt);
  require_noerr(status, bail);

  status = smcp_outbound_set_uri(request->url, 0);
  require_noerr(status, bail);

  if (request->content) {
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

static smcp_status_t get_response_handler(int statuscode, void* context) {
  const char* content = smcp_inbound_get_content_ptr();
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
    if (observe && statuscode == SMCP_STATUS_TIMEOUT) {
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

    while ((key = smcp_inbound_next_option(&value, &value_len)) !=
           COAP_OPTION_INVALID) {
      if (key == COAP_OPTION_BLOCK2) {
        last_block = !(value[value_len - 1] & (1 << 3));
      } else if (key == COAP_OPTION_OBSERVE) {
      }
    }

    // TODO: Think better about this
    if (context) {
      request_t req = (request_t)context;

      if (req->callback) {
        req->callback(req->data, content, content_length);
      }
    }
  }

bail:
  return SMCP_STATUS_OK;
}
