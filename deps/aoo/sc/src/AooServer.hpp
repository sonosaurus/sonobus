#include "Aoo.hpp"
#include "aoo/aoo_server.hpp"

#include "common/udp_server.hpp"
#include "common/tcp_server.hpp"

#include <thread>

namespace sc {

class AooServer {
public:
    AooServer(World *world, int port, const char *password);
    ~AooServer();
private:
    World* world_;
    int port_;
    ::AooServer::Ptr server_;
    aoo::udp_server udpserver_;
    aoo::tcp_server tcpserver_;
    std::thread udpthread_;
    std::thread tcpthread_;

    void handleEvent(const AooEvent *event);

    AooId handleAccept(int e, const aoo::ip_address& addr);

    void handleReceive(int e, AooId client, const aoo::ip_address& addr,
                       const AooByte *data, AooSize size);

    void handleUdpReceive(int e, const aoo::ip_address& addr,
                          const AooByte *data, AooSize size);

    void addClient(AooId client, const aoo::ip_address& addr);

    void removeClient(AooId client);
};

struct AooServerCmd {
    int port;
};

struct AooServerCreateCmd : AooServerCmd {
    char password[64];
};

} // sc
