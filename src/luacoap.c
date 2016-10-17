#include <smcp/smcp.h>
#include <smcp/assert-macros.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <signal.h>

#include <luacoap/client/get.h>

#define CLIENT_MT_NAME "coap_client"

typedef struct lcoap_userdata {
  smcp_t smcp;
} lcoap_userdata;

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
  // Get the coap client
  int stack = 1;
  lcoap_userdata *cud = (lcoap_userdata *)luaL_checkudata(L, stack, CLIENT_MT_NAME);
  luaL_argcheck(L, cud, stack, "Client expected");
  if (cud == NULL) {
    return luaL_error(L, "get: first argument is not a a client.");
  }
  stack++;

  // Get the url
  if (!lua_isstring(L, stack)) {
    return luaL_error(L, "get: second argument is not a string.");
  }
  size_t ln;
  const char *url = luaL_checklstring(L, stack, &ln);

  if (send_get_request(cud->smcp, url) != 0) {
    luaL_error(L, "get: error sending request");
  }

  return 0;
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
