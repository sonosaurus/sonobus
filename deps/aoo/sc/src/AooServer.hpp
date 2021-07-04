#include "Aoo.hpp"
#include "aoo/aoo_net.hpp"

#include <thread>

class AooServer {
public:
    AooServer(World *world, int port);
    ~AooServer();

    void handleEvent(const aoo_event *event);
private:
    World* world_;
    aoo::net::server::pointer server_;
    int32_t port_;
    std::thread thread_;
};

struct AooServerCmd {
    int port;
};
