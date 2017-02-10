#include <luacoap/listener.h>

inline static int next_ref_index() {
  static int i = 0;
  return i++;
}

void store_callback_reference(lua_State *L, lcoap_listener* ltnr) {
  ltnr->lua_func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
}

void execute_callback(lua_State *L, lcoap_listener* ltnr) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, ltnr->lua_func_ref);
  lua_pcall(L, 0, 0, 0);
}
