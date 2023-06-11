#include "aoo/aoo_source.h"
#include "aoo/aoo_sink.h"
#if AOO_NET
# include "aoo/aoo_server.h"
# include "aoo/aoo_client.h"
#endif

int main(int argc, const char * arg[]) {
    AooSource *source = AooSource_new(0, NULL);
    AooSink *sink = AooSink_new(0, NULL);
#if AOO_NET
    AooClient *client;
    AooServer *server;
    client = AooClient_new(NULL);
    server = AooServer_new(NULL);
#endif

    AooSource_free(source);
    AooSink_free(sink);
#if AOO_NET
    AooClient_free(client);
    AooServer_free(server);
#endif

    return 0;
}
