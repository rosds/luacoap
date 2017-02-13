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

typedef enum {
  LISTENER_STATUS_LISTENING,
  LISTENER_STATUS_PAUSED,
  LISTENER_STATUS_STOPPING,
  LISTENER_STATUS_STOPPED
} listener_status_t;

/**
 *  Handle observation subscriptions
 */
typedef struct {
  smcp_t smcp;                           /**< CoAP client reference */
  int lua_func_ref;                      /**< Reference to the lua callback function */
  request_s request;                     /**< CoAP request structure */
  struct smcp_transaction_s transaction; /**< SMCP transaction structure */

  // Thread Control
  pthread_t thread;                      /**< SMCP process thread */
  pthread_mutex_t suspend_mutex;         /**< Mutex to access the thread status */
  int suspend;                           /**< Suspend thread flag */
  int stop;                              /**< Stop thread flag */
  pthread_cond_t cond_resume;            /**< Thread status variables */
} lcoap_listener;

typedef lcoap_listener* lcoap_listener_t;

/**
 *  Stores a reference in the listener structure to the object in the top of
 *  the stack of the lua state L.
 */
void store_callback_reference(lua_State* L, lcoap_listener* ltnr);

/**
 *  Register the listener table.
 */
void register_listener_table(lua_State* L);

// TODO: remove this, only for debugging
int method_listen(lua_State* L);

#endif /* ifndef LUA_COAP_LISTENER__ */
