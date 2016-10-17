#ifndef COAP_CLIENT_GET_HH__
#define COAP_CLIENT_GET_HH__

#include <smcp/smcp.h>
#include <smcp/assert-macros.h>
#include <signal.h>

#define ERRORCODE_OK            (0)
#define ERRORCODE_HELP          (1)
#define ERRORCODE_BADARG        (2)
#define ERRORCODE_NOCOMMAND     (3)
#define ERRORCODE_UNKNOWN       (4)
#define ERRORCODE_BADCOMMAND    (5)
#define ERRORCODE_NOREADLINE    (6)
#define ERRORCODE_QUIT          (7)
#define ERRORCODE_INIT_FAILURE  (8)
#define ERRORCODE_TIMEOUT       (9)
#define ERRORCODE_COAP_ERROR    (10)

#define ERRORCODE_INTERRUPT     (128+SIGINT)
#define ERRORCODE_SIGHUP    (128+SIGHUP)

#define ERRORCODE_INPROGRESS    (127)

int send_get_request(smcp_t smcp, int get_tt, const char *url);

#endif // COAP_CLIENT_GET_HH__
