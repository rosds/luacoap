#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>


static int my_func(lua_State *L) {
  lua_pushnumber(L, luaL_checknumber(L, 1) + 42);
  return 1;
}

static const struct luaL_Reg luacoap_map[] = {
  {"myFunc", my_func},
  {"__index", luacoap_map},
  {NULL, NULL}
};

int luaopen_libluacoap(lua_State *L) {
  luaL_newlib(L, luacoap_map);
  luaL_setfuncs(L, luacoap_map, 0);
  lua_setglobal(L, "coap");
  return 1;
}
