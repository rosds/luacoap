#include <luacoap/luacoap.h>

static int coap_create_client(lua_State *L) {
  lcoap_client *cud;
  cud = (lcoap_client *)lua_newuserdata(L, sizeof(lcoap_client));
  luaL_getmetatable(L, CLIENT_MT_NAME);
  lua_setmetatable(L, -2);

  // Creates the client
  cud->smcp = smcp_create(61616);
  return 1;
}

static const struct luaL_Reg luacoap_map[] = {{"Client", coap_create_client},
                                              {NULL, NULL}};

int luaopen_coap(lua_State *L) {
  // Declare the client metatable
  register_client_table(L);

  // Register the listener object
  register_listener_table(L);

  // Register the coap library
  luaL_newlib(L, luacoap_map);
  luaL_setfuncs(L, luacoap_map, 0);
  lua_pushnumber(L, COAP_TRANS_TYPE_CONFIRMABLE);
  lua_setfield(L, -2, "CON");
  lua_pushnumber(L, COAP_TRANS_TYPE_NONCONFIRMABLE);
  lua_setfield(L, -2, "NON");

  return 1;
}
