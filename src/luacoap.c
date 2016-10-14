#include <smcp/smcp.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#define CLIENT_MT_NAME "coap_client"

typedef struct lcoap_userdata {
  smcp_t *smcp;
} lcoap_userdata;

static int coap_create_client(lua_State *L) {
  lcoap_userdata *cud;
  cud = (lcoap_userdata *)lua_newuserdata(L, sizeof(lcoap_userdata));
  luaL_getmetatable(L, CLIENT_MT_NAME);
  lua_setmetatable(L, -2);
  return 1;
}

static int my_func(lua_State *L) {
  lua_pushnumber(L, luaL_checknumber(L, 1) + 42);
  return 1;
}

static const struct luaL_Reg luacoap_client_map[] = {
  {"get", my_func},
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
