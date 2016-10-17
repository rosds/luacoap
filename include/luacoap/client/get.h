#ifndef COAP_CLIENT_GET_HH__
#define COAP_CLIENT_GET_HH__

#define ERRORCODE_OK            (0)
#define ERRORCODE_HELP          (1)
#define ERRORCODE_BADARG        (2)
#define ERRORCODE_NOCOMMAND     (3)
#define ERRORCODE_UNKNOWN       (4)
#define ERRORCODE_BADCOMMAND    (5)
#define ERRORCODE_NOREADLINE    (6)
#define ERRORCODE_QUIT          (7)
#define ERRORCODE_INIT_FAILURE  (8)
#define ERRORCODE_TIMEOUT       (9)
#define ERRORCODE_COAP_ERROR    (10)

#define ERRORCODE_INTERRUPT     (128+SIGINT)
#define ERRORCODE_SIGHUP    (128+SIGHUP)

#define ERRORCODE_INPROGRESS    (127)

static int gRet;
static sig_t previous_sigint_handler;
static const char *url_data;
static void
signal_interrupt(int sig) {
  gRet = ERRORCODE_INTERRUPT;
  signal(SIGINT, previous_sigint_handler);
}
static coap_content_type_t request_accept_type = -1;

static smcp_status_t
get_response_handler(int statuscode, void* context) {
  const char* content = (const char*)smcp_inbound_get_content_ptr();
  coap_size_t content_length = smcp_inbound_get_content_len();

  if(statuscode >= 0) {
    if(content_length>(smcp_inbound_get_packet_length()-4)) {
      fprintf(stderr, "INTERNAL ERROR: CONTENT_LENGTH LARGER THAN PACKET_LENGTH-4!(content_length=%u, packet_length=%u)\n",content_length,smcp_inbound_get_packet_length());
      gRet = ERRORCODE_UNKNOWN;
      goto bail;
    }

    if(!coap_verify_packet((void*)smcp_inbound_get_packet(), smcp_inbound_get_packet_length())) {
      fprintf(stderr, "INTERNAL ERROR: CALLBACK GIVEN INVALID PACKET!\n");
      gRet = ERRORCODE_UNKNOWN;
      goto bail;
    }
  }

  if(statuscode == SMCP_STATUS_TRANSACTION_INVALIDATED) {
    gRet = 0;
  }

  if( ((statuscode < COAP_RESULT_200) ||(statuscode >= COAP_RESULT_400))
      && (statuscode != SMCP_STATUS_TRANSACTION_INVALIDATED)
      && (statuscode != HTTP_TO_COAP_CODE(HTTP_RESULT_CODE_PARTIAL_CONTENT))
    ) {
    if(statuscode == SMCP_STATUS_TIMEOUT) {
      gRet = 0;
    } else {
      gRet = (statuscode == SMCP_STATUS_TIMEOUT)?ERRORCODE_TIMEOUT:ERRORCODE_COAP_ERROR;
      fprintf(stderr, "get: Result code = %d (%s)\n", statuscode,
          (statuscode < 0) ? smcp_status_to_cstr(
            statuscode) : coap_code_to_cstr(statuscode));
    }
  }

  if((statuscode>0) && content && content_length) {
    coap_option_key_t key;
    const uint8_t* value;
    coap_size_t value_len;
    bool last_block = true;
    int32_t observe_value = -1;

    while((key=smcp_inbound_next_option(&value, &value_len))!=COAP_OPTION_INVALID) {

      if(key == COAP_OPTION_BLOCK2) {
        last_block = !(value[value_len-1]&(1<<3));
      } else if(key == COAP_OPTION_OBSERVE) {
        if(value_len)
          observe_value = value[0];
        else observe_value = 0;
      }

    }

    fwrite(content, content_length, 1, stdout);

    if(last_block) {
      // Only print a newline if the content doesn't already print one.
      if(content_length && (content[content_length - 1] != '\n'))
        printf("\n");
    }

    fflush(stdout);
  }

bail:
  return SMCP_STATUS_OK;
}

static smcp_status_t
resend_get_request(void* context) {
  smcp_status_t status = 0;

  status = smcp_outbound_begin(smcp_get_current_instance(),COAP_METHOD_GET, COAP_TRANS_TYPE_CONFIRMABLE);
  require_noerr(status, bail);

  status = smcp_outbound_set_uri(url_data, 0);
  require_noerr(status, bail);

  if(request_accept_type!=COAP_CONTENT_TYPE_UNKNOWN) {
    status = smcp_outbound_add_option_uint(COAP_OPTION_ACCEPT, request_accept_type);
    require_noerr(status, bail);
  }

  status = smcp_outbound_send();

  if(status) {
    fprintf(stderr,
      "smcp_outbound_send() returned error %d(%s).\n",
      status,
      smcp_status_to_cstr(status));
  }

bail:
  return status;
}

int send_get_request(smcp_t smcp, const char *url) {
  url_data = url;
  gRet = ERRORCODE_INPROGRESS;  
  smcp_status_t status = 0;
  previous_sigint_handler = signal(SIGINT, &signal_interrupt);
  struct smcp_transaction_s transaction;

  smcp_transaction_end(smcp, &transaction);
  smcp_transaction_init(
    &transaction,
    SMCP_TRANSACTION_ALWAYS_INVALIDATE,
    (void*)&resend_get_request,
    (void*)&get_response_handler,
    (void*)url
  );

  status = smcp_transaction_begin(smcp, &transaction, 30 * MSEC_PER_SEC);

  if(status) {
    fprintf(stderr,
      "smcp_begin_transaction_old() returned %d(%s).\n",
      status,
      smcp_status_to_cstr(status));
    return false;
  }

  while(ERRORCODE_INPROGRESS == gRet) {
    smcp_wait(smcp,1000);
    smcp_process(smcp);
  }

  smcp_transaction_end(smcp, &transaction);
  signal(SIGINT, previous_sigint_handler);
  url = NULL;
  return gRet;
}

#endif // COAP_CLIENT_GET_HH__
