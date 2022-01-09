#include "Aoo.hpp"
#include "aoo/aoo_server.hpp"

#include <thread>

namespace sc {

class AooServer {
public:
    AooServer(World *world, int port);
    ~AooServer();

    void handleEvent(const AooEvent *event);
private:
    World* world_;
    ::AooServer::Ptr server_;
    int32_t port_;
    std::thread thread_;
};

struct AooServerCmd {
    int port;
};

} // sc
