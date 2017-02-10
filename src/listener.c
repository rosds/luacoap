#include <luacoap/listener.h>

inline static int next_ref_index() {
  static int i = 0;
  return i++;
}

void store_callback_reference(lua_State *L, lcoap_listener* ltnr) {
  int index = next_ref_index();
  lua_pushvalue(L, index);
  ltnr->lua_func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
}
