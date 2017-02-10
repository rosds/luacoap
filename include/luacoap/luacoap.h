#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <luacoap/client.h>
#include <luacoap/listener.h>
#include <smcp/smcp.h>

#define LISTENER_MT_NAME "coap_listener"

#define CLIENT_MT_NAME "coap_client"
#define COAP_METHOD_OBSERVE 5

typedef struct { smcp_t smcp; } lcoap_userdata;
