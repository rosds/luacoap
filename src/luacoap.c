#include <smcp/smcp.h>
#include <smcp/assert-macros.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <signal.h>

#define CLIENT_MT_NAME "coap_client"

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
static int retries = 0;
static sig_t previous_sigint_handler;
static struct smcp_transaction_s transaction;
static coap_transaction_type_t get_tt;
static cms_t get_timeout = 30 * MSEC_PER_SEC;
static int last_observe_value = -1;
static const char *url;
static void
signal_interrupt(int sig) {
  gRet = ERRORCODE_INTERRUPT;
  signal(SIGINT, previous_sigint_handler);
}
static coap_content_type_t request_accept_type = -1;


typedef struct lcoap_userdata {
  smcp_t smcp;
} lcoap_userdata;

static smcp_status_t
get_response_handler(int statuscode, void* context) {
  const char* content = (const char*)smcp_inbound_get_content_ptr();
  coap_size_t content_length = smcp_inbound_get_content_len();

  if(statuscode>=0) {
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

    last_observe_value = observe_value;
  }

bail:
  return SMCP_STATUS_OK;
}

smcp_status_t
resend_get_request(void* context) {
  smcp_status_t status = 0;

  status = smcp_outbound_begin(smcp_get_current_instance(),COAP_METHOD_GET, get_tt);
  require_noerr(status, bail);

  status = smcp_outbound_set_uri(url, 0);
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


static int coap_create_client(lua_State *L) {
  lcoap_userdata *cud;
  cud = (lcoap_userdata *)lua_newuserdata(L, sizeof(lcoap_userdata));
  luaL_getmetatable(L, CLIENT_MT_NAME);
  lua_setmetatable(L, -2);

  // Creates the client
  cud->smcp = smcp_create(61616);

  return 1;
}

static int coap_client_gc(lua_State *L) {
  int stack = 1;
  lcoap_userdata *cud = (lcoap_userdata *)luaL_checkudata(L, stack, CLIENT_MT_NAME);
  luaL_argcheck(L, cud, stack, "Server/Client expected");
  if (cud != NULL) {
    free(cud->smcp);
  }
  return 0;
}

static int coap_client_get(lua_State *L) {
  smcp_status_t status = 0;
  int flags = SMCP_TRANSACTION_ALWAYS_INVALIDATE;
  gRet = ERRORCODE_INPROGRESS;  
  previous_sigint_handler = signal(SIGINT, &signal_interrupt);
  retries = 0;

  // Get the coap client
  int stack = 1;
  lcoap_userdata *cud = (lcoap_userdata *)luaL_checkudata(L, stack, CLIENT_MT_NAME);
  luaL_argcheck(L, cud, stack, "Client expected");
  if (cud == NULL) {
    return luaL_error(L, "first argument is not a a client.");
  }
  stack++;

  // Get the url
  if (!lua_isstring(L, stack)) {
    return luaL_error(L, "second argument is not a string.");
  }
  size_t ln;
  url = luaL_checklstring(L, stack, &ln);

  smcp_transaction_end(cud->smcp, &transaction);
  smcp_transaction_init(
    &transaction,
    flags,
    (void*)&resend_get_request,
    (void*)&get_response_handler,
    (void*)url
  );
  status = smcp_transaction_begin(cud->smcp, &transaction, get_timeout);

  if(status) {
    fprintf(stderr,
      "smcp_begin_transaction_old() returned %d(%s).\n",
      status,
      smcp_status_to_cstr(status));
    return false;
  }

  while(ERRORCODE_INPROGRESS == gRet) {
    smcp_wait(cud->smcp,1000);
    smcp_process(cud->smcp);
  }

  smcp_transaction_end(cud->smcp, &transaction);
  signal(SIGINT, previous_sigint_handler);
  url = NULL;
  return gRet;
}

static const struct luaL_Reg luacoap_client_map[] = {
  {"get", coap_client_get},
  {"__gc", coap_client_gc},
  {NULL, NULL}
};

static const struct luaL_Reg luacoap_map[] = {
  {"Client", coap_create_client},
  {NULL, NULL}
};

int luaopen_libluacoap(lua_State *L) {
  // Declare the client metatable
  luaL_newmetatable(L, CLIENT_MT_NAME);
  luaL_setfuncs(L, luacoap_client_map, 0);
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");

  luaL_newlib(L, luacoap_map);
  luaL_setfuncs(L, luacoap_map, 0);
  lua_setglobal(L, "coap");
  return 1;
}
