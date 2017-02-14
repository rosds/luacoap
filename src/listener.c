#include <luacoap/listener.h>

/*****************************************************************************/
// LUA C Interface for a Resource Observation
//
// Methods:
// * callback : executes the associated callback
// * listen : Launchs a thread that processes the CoAP notifications
// * pause : Suspends the thread
// * continue : Resume the execution of the thread
// * stop : Shutdowns the thread
//
/*****************************************************************************/

// Garbage collector routine
static int coap_listener_gc(lua_State *L);

// Execute callback function
static int method_callback(lua_State *L);

// Control observing thread
static int start_listening(lua_State *L);
static int stop_listening(lua_State *L);
static int pause_listening(lua_State *L);
static int continue_listening(lua_State *L);

static const struct luaL_Reg luacoap_listener_map[] = {
    {"callback", method_callback},
    {"listen", start_listening},
    {"pause", pause_listening},
    {"continue", continue_listening},
    {"stop", stop_listening},
    {"__gc", coap_listener_gc},
    {NULL, NULL}};

void register_listener_table(lua_State *L) {
  luaL_newmetatable(L, LISTENER_MT_NAME);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, luacoap_listener_map, 0);
}

/*****************************************************************************/

lcoap_listener_t lua_create_listener(lua_State *L, smcp_t smcp, int func_ref) {
  lcoap_listener_t ltnr =
      (lcoap_listener_t)lua_newuserdata(L, sizeof(lcoap_listener));
  luaL_getmetatable(L, LISTENER_MT_NAME);
  lua_setmetatable(L, -2);

  // Keep a reference to the smcp client
  ltnr->L = L;
  ltnr->smcp = smcp;
  ltnr->lua_func_ref = func_ref;

  return ltnr;
}

static int coap_listener_gc(lua_State *L) {
  stop_listening(L);
  return 0;
}

int execute_listener_callback(lcoap_listener_t ltnr) {
  lua_rawgeti(ltnr->L, LUA_REGISTRYINDEX, ltnr->lua_func_ref);
  
  lua_pcall(ltnr->L, 1, 0, 0);
  return 0;
}

int execute_listener_callback_with_payload(lcoap_listener_t ltnr, const char* p, size_t l) {
  lua_rawgeti(ltnr->L, LUA_REGISTRYINDEX, ltnr->lua_func_ref);
  
  // push payload argument
  lua_pushlstring(ltnr->L, p, l);

  lua_pcall(ltnr->L, 1, 0, 0);
  return 0;
}

static int method_callback(lua_State *L) {
  // Get the listener object
  lcoap_listener *ltnr =
      (lcoap_listener *)luaL_checkudata(L, 1, LISTENER_MT_NAME);
  luaL_argcheck(L, ltnr, 1, "Listener expected");

  return execute_listener_callback(ltnr);
}

static void *thread_function(void *listener) {
  lcoap_listener_t ltnr = (lcoap_listener_t)listener;

  do {
    pthread_mutex_lock(&ltnr->suspend_mutex);
    if (ltnr->suspend != 0) {
      pthread_cond_wait(&ltnr->cond_resume, &ltnr->suspend_mutex);
    } else if (ltnr->stop != 0) {
      break;
    }
    pthread_mutex_unlock(&ltnr->suspend_mutex);

    smcp_wait(ltnr->smcp, 1000);
    smcp_process(ltnr->smcp);
  } while (true);
}

static int start_listening(lua_State *L) {
  lcoap_listener_t ltnr =
      (lcoap_listener_t)luaL_checkudata(L, -1, LISTENER_MT_NAME);

  pthread_mutex_init(&ltnr->suspend_mutex, NULL);
  pthread_cond_init(&ltnr->cond_resume, NULL);
  ltnr->suspend = 0;
  ltnr->stop = 0;

  smcp_status_t status;
  status = smcp_transaction_begin(ltnr->smcp, &ltnr->transaction,
                                  30 * MSEC_PER_SEC);

  if (status) {
    fprintf(stderr, "smcp_begin_transaction_old() returned %d(%s).\n",
            status, smcp_status_to_cstr(status));
  }

  // Launch the thread
  pthread_create(&ltnr->thread, NULL, thread_function, (void *)ltnr);
  return 0;
}

static int stop_listening(lua_State *L) {
  lcoap_listener_t ltnr =
      (lcoap_listener_t)luaL_checkudata(L, -1, LISTENER_MT_NAME);

  pthread_mutex_lock(&ltnr->suspend_mutex);
  ltnr->stop = 1;
  pthread_mutex_unlock(&ltnr->suspend_mutex);

  // Stop the smcp processing
  pthread_join(ltnr->thread, NULL);

  // Finish the transacition
  smcp_transaction_end(ltnr->smcp, &ltnr->transaction);

  pthread_mutex_destroy(&ltnr->suspend_mutex);
  pthread_cond_destroy(&ltnr->cond_resume);
  ltnr->suspend = 0;
  ltnr->stop = 0;
  return 0;
}

static int pause_listening(lua_State *L) {
  lcoap_listener_t ltnr =
      (lcoap_listener_t)luaL_checkudata(L, -1, LISTENER_MT_NAME);

  pthread_mutex_lock(&ltnr->suspend_mutex);
  ltnr->suspend = 1;
  pthread_mutex_unlock(&ltnr->suspend_mutex);

  return 0;
}

static int continue_listening(lua_State *L) {
  lcoap_listener_t ltnr =
      (lcoap_listener_t)luaL_checkudata(L, -1, LISTENER_MT_NAME);

  pthread_mutex_lock(&ltnr->suspend_mutex);
  ltnr->suspend = 0;
  pthread_cond_signal(&ltnr->cond_resume);
  pthread_mutex_unlock(&ltnr->suspend_mutex);

  return 0;
}
