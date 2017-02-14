#ifndef LUA_COAP_LISTENER_HH__
#define LUA_COAP_LISTENER_HH__

#include <pthread.h>
#include <unistd.h>

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <smcp/smcp.h>

#include <luacoap/client.h>
#include <luacoap/luaclient.h>

// Name for the Lua meta-table
#define LISTENER_MT_NAME "coap_listener"

/**
 *  Listener Structure
 */
typedef struct {
  // smcp stuff
  smcp_t smcp;                           /**< CoAP client reference */
  request_s request;                     /**< CoAP request structure */
  struct smcp_transaction_s transaction; /**< SMCP transaction structure */

  // Callback 
  int lua_func_ref;                      /**< Reference to the lua callback function */
  lua_State* L;                          /**< Lua state with the callback function */

  // Thread Control
  pthread_t thread;                      /**< SMCP process thread */
  pthread_mutex_t suspend_mutex;         /**< Mutex to access the thread status */
  int suspend;                           /**< Suspend thread flag */
  int stop;                              /**< Stop thread flag */
  pthread_cond_t cond_resume;            /**< Thread status variables */
} lcoap_listener;

typedef lcoap_listener* lcoap_listener_t;

/**
 *  Register the listener table.
 */
void register_listener_table(lua_State* L);

/**
 *  Create a Lua table Listener
 */
lcoap_listener_t lua_create_listener(lua_State* L, smcp_t smcp, int func_ref);

/**
 * Executes the callback in the referenced lua state.
 */
int execute_listener_callback(lcoap_listener_t ltnr);
int execute_listener_callback_with_payload(lcoap_listener_t ltnr, const char* p, size_t l);

#endif /* ifndef LUA_COAP_LISTENER__ */
