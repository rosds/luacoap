#include <luacoap/listener.h>

void init_listener(lcoap_listener_t ltnr) {
  ltnr->transaction = (smcp_transaction_t) malloc (sizeof(struct smcp_transaction_s));
}

static void destroy_listener(lcoap_listener_t ltnr) {
  free(ltnr->transaction);
}

static int coap_listener_gc(lua_State *L) {
  int stack = 1;
  lcoap_listener_t ltnr = (lcoap_listener_t)luaL_checkudata(L, stack, LISTENER_MT_NAME);
  destroy_listener(ltnr);
  return 0;
}

static int method_callback(lua_State *L) {
  // Get the listener object
  lcoap_listener *ltnr =
      (lcoap_listener *)luaL_checkudata(L, 1, LISTENER_MT_NAME);
  luaL_argcheck(L, ltnr, 1, "Listener expected");

  return execute_callback(L, ltnr);
}

static const struct luaL_Reg luacoap_listener_map[] = {
    {"callback", method_callback}, {"__gc", coap_listener_gc}, {NULL, NULL}};

void store_callback_reference(lua_State *L, lcoap_listener *ltnr) {
  ltnr->lua_func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
}

int execute_callback(lua_State *L, lcoap_listener *ltnr) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, ltnr->lua_func_ref);
  lua_pcall(L, 0, 0, 0);
  return 0;
}

void register_listener_table(lua_State *L) {
  luaL_newmetatable(L, LISTENER_MT_NAME);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, luacoap_listener_map, 0);
}
