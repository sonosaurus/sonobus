#include "aoo/aoo_source.h"
#include "aoo/aoo_sink.h"
#if USE_AOO_NET
# include "aoo/aoo_client.h"
# include "aoo/aoo_server.h"
# ifdef _WIN32
#  include <ws2tcpip.h>
# else
#  include <netdb.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <arpa/inet.h>
#  include <errno.h>
# endif
#endif

int main(int argc, const char * arg[]) {
    AooSource *source = AooSource_new(0, 0, NULL);
    AooSink *sink = AooSink_new(0, 0, NULL);
#if USE_AOO_NET
    AooClient *client;
    AooServer *server;
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(0x7F000001);
    sa.sin_port = htons(50000);
    client = AooClient_new(&sa, sizeof(sa), 0, NULL);
    server = AooServer_new(40000, 0, NULL);
#endif

    AooSource_free(source);
    AooSink_free(sink);
#if USE_AOO_NET
    AooClient_free(client);
    AooServer_free(server);
#endif

    return 0;
}
