#include <luacoap/listener.h>

void store_callback_reference(lua_State *L, lcoap_listener *ltnr) {
  ltnr->lua_func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
}

int execute_callback(lua_State *L, lcoap_listener *ltnr) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, ltnr->lua_func_ref);
  lua_pcall(L, 0, 0, 0);
  return 0;
}

static int method_callback(lua_State* L) { 
  // Get the listener object
  lcoap_listener *ltnr =
      (lcoap_listener *)luaL_checkudata(L, 1, LISTENER_MT_NAME);
  luaL_argcheck(L, ltnr, 1, "Listener expected");

  return execute_callback(L, ltnr);
}


static const struct luaL_Reg luacoap_listener_map[] = {{"callback", method_callback},

                                                       {NULL, NULL}};
void register_listener_table(lua_State *L) {
  // Declare the listener metatable
  luaL_newmetatable(L, LISTENER_MT_NAME);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, luacoap_listener_map, 0);
}
