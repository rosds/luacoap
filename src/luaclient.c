#include <luacoap/luaclient.h>

/*****************************************************************************/
// LUA C Interface for the CoAP client table
//
// Methods:
// * get : Sends a GET request
// * post : Sends a POST request
// * put : Sends a PUT request
// * observe : Subscribes and observes a resource
//
/*****************************************************************************/

static int coap_client_send_request(coap_code_t method, lua_State *L);

static int coap_client_get(lua_State *L) {
  return coap_client_send_request(COAP_METHOD_GET, L);
}
static int coap_client_put(lua_State *L) {
  return coap_client_send_request(COAP_METHOD_PUT, L);
}
static int coap_client_post(lua_State *L) {
  return coap_client_send_request(COAP_METHOD_POST, L);
}
static int coap_client_observe(lua_State *L) {
  return coap_client_send_request(COAP_METHOD_OBSERVE, L);
}

// Client Garbage Collector routine
static int coap_client_gc(lua_State *L);

static const struct luaL_Reg luacoap_client_map[] = {
    {"get", coap_client_get},   {"put", coap_client_put},
    {"post", coap_client_post}, {"observe", coap_client_observe},
    {"__gc", coap_client_gc},   {NULL, NULL}};

void register_client_table(lua_State *L) {
  luaL_newmetatable(L, CLIENT_MT_NAME);
  luaL_setfuncs(L, luacoap_client_map, 0);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
}

/*****************************************************************************/

static void *execute_callback(void *l, const char *p, size_t n) {
  lcoap_listener_t ltnr = (lcoap_listener_t)l;
  execute_listener_callback_with_payload(ltnr, p, n);
}


static int coap_client_send_request(coap_code_t method, lua_State *L) {
  coap_transaction_type_t tt = COAP_TRANS_TYPE_CONFIRMABLE;
  coap_content_type_t ct = COAP_CONTENT_TYPE_TEXT_PLAIN;

  // Get the coap client
  int stack = 1;
  lcoap_client *cud = (lcoap_client *)luaL_checkudata(L, stack, CLIENT_MT_NAME);
  luaL_argcheck(L, cud, stack, "Client expected");
  if (cud == NULL) {
    return luaL_error(L, "First argument is not of class Client");
  }
  stack++;

  // Get transaction type
  if (lua_isnumber(L, stack)) {
    tt = lua_tointeger(L, stack);
    stack++;

    if ((tt != COAP_TRANS_TYPE_CONFIRMABLE) &&
        tt != (COAP_TRANS_TYPE_NONCONFIRMABLE)) {
      return luaL_error(L,
                        "Invalid transaction type, use coap.CON or coap.NON");
    }
  }

  // Get the url
  size_t ln;
  const char *url = luaL_checklstring(L, stack, &ln);
  stack++;

  if (url == NULL) return luaL_error(L, "Invalid URL");

  size_t payload_len;
  const char *payload = NULL;

  // Optional content type and payload
  if (lua_isnumber(L, stack)) {
    ct = lua_tointeger(L, stack);
    stack++;

    // get the payload
    payload_len;
    payload = luaL_checklstring(L, stack, &payload_len);
    stack++;
  }

  // Check if the last argument is the callback function
  int func_ref = lua_isfunction(L, -1) ? luaL_ref(L, LUA_REGISTRYINDEX) : 0;

  if (method == COAP_METHOD_OBSERVE) {
    lcoap_listener_t ltnr = lua_create_listener(L, cud->smcp, func_ref);

    // Create the CoAP request
    create_request(&ltnr->request, COAP_METHOD_GET, tt, url, ct, payload,
                   payload_len, true, ltnr, execute_callback);

    // Send the request
    settup_observe_request(cud->smcp, &ltnr->request, &ltnr->transaction);

    return 1;

  } else {

    void *f(void *d, const char *p, size_t n) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, func_ref);
      lua_pushlstring(L, p, n);
      lua_pcall(L, 1, 0, 0);
    }

    request_s req;
    create_request(&req, method, tt, url, ct, payload, payload_len, false, NULL, f);

    if (send_request(cud->smcp, &req) != 0) {
      luaL_error(L, "Error sending request");
    }
  }

  return 0;
}

static int coap_client_gc(lua_State *L) {
  lcoap_client *cud = (lcoap_client *)luaL_checkudata(L, -1, CLIENT_MT_NAME);
  if (cud) {
    free(cud->smcp);
  }
  return 0;
}
