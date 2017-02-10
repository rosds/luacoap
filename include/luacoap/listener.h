#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <smcp/smcp.h>

/**
 *  Handle observation subscriptions
 */
typedef struct {
  smcp_t* smcp;
  int lua_func_ref; /**< Reference to the lua callback function */
} lcoap_listener;

/**
 *  Stores a reference in the listener structure to the object in the top of
 *  the stack of the lua state L.
 */
void store_callback_reference(lua_State* L, lcoap_listener* ltnr);
