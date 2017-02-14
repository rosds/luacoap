#include <luacoap/listener.h>

static int coap_listener_gc(lua_State *L) {
  lcoap_listener_t ltnr =
      (lcoap_listener_t)luaL_checkudata(L, 1, LISTENER_MT_NAME);

  // Stop the smcp processing
  pthread_cancel(ltnr->thread);

  // Finish the transacition
  smcp_transaction_end(ltnr->smcp, &ltnr->transaction);
  pthread_mutex_destroy(&ltnr->suspend_mutex);
  pthread_cond_destroy(&ltnr->cond_resume);

  return 0;
}

static int execute_callback(lcoap_listener_t ltnr) {
  lua_rawgeti(ltnr->L, LUA_REGISTRYINDEX, ltnr->lua_func_ref);
  
  lua_pcall(ltnr->L, 1, 0, 0);
  return 0;
}

static int execute_callback_with_payload(lcoap_listener_t ltnr, const char* p, size_t l) {
  lua_rawgeti(ltnr->L, LUA_REGISTRYINDEX, ltnr->lua_func_ref);
  
  // push payload argument
  lua_pushlstring(ltnr->L, p, l);

  lua_pcall(ltnr->L, 1, 0, 0);
  return 0;
}

void execute_listener_callback(lcoap_listener_t ltnr) {
  execute_callback(ltnr);
}

void execute_listener_callback_with_payload(lcoap_listener_t ltnr, const char* p, size_t l) {
  execute_callback_with_payload(ltnr, p, l);
}

static int method_callback(lua_State *L) {
  // Get the listener object
  lcoap_listener *ltnr =
      (lcoap_listener *)luaL_checkudata(L, 1, LISTENER_MT_NAME);
  luaL_argcheck(L, ltnr, 1, "Listener expected");

  return execute_callback(ltnr);
}

static void *thread_function(void *listener) {
  listener_status_t status;
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

  } while (status == LISTENER_STATUS_LISTENING);
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

int method_listen(lua_State *L) {
  // Get the listener object
  lcoap_listener *ltnr =
      (lcoap_listener *)luaL_checkudata(L, -1, LISTENER_MT_NAME);

  start_listening(L);

  return 0;
}

static const struct luaL_Reg luacoap_listener_map[] = {
    {"callback", method_callback},
    {"listen", start_listening},
    {"pause", pause_listening},
    {"continue", continue_listening},
    {"stop", stop_listening},
    {"__gc", coap_listener_gc},
    {NULL, NULL}};

void store_callback_reference(lua_State *L, lcoap_listener *ltnr) {
  ltnr->L = L;
  ltnr->lua_func_ref = luaL_ref(L, LUA_REGISTRYINDEX);
}

void register_listener_table(lua_State *L) {
  luaL_newmetatable(L, LISTENER_MT_NAME);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, luacoap_listener_map, 0);
}
