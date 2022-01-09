#include "Aoo.hpp"

#include "common/time.hpp"

#include "oscpack/osc/OscReceivedElements.h"

#include <string>
#include <thread>

namespace sc {

class AooClient {
public:
    AooClient(World *world, int32_t port);
    ~AooClient();

    void connect(const char* host, int port,
                 const char* user, const char* pwd);

    void disconnect();

    void joinGroup(const char* name, const char* pwd);

    void leaveGroup(const char* name);

    void forwardMessage(const char *data, int32_t size,
                        aoo::time_tag time);

    void handleEvent(const AooEvent* e);
private:
    World* world_;
    std::shared_ptr<INode> node_;

    void sendReply(const char *cmd, bool success,
                   const char *errmsg = nullptr);

    void sendGroupReply(const char* cmd, const char *group,
                        bool success, const char* errmsg = nullptr);

    void handlePeerMessage(const char *data, int32_t size,
                           const aoo::ip_address& address, aoo::time_tag t);

    void handlePeerBundle(const osc::ReceivedBundle& bundle,
                          const aoo::ip_address& address);
};

struct AooClientCmd {
    int port;
};

struct ConnectCmd : AooClientCmd {
    char serverName[256];
    int32_t serverPort;
    char userName[64];
    char userPwd[64];
};

struct GroupCmd : AooClientCmd {
    char name[64];
    char pwd[64];
};

struct GroupRequest {
    AooClient* obj;
    std::string group;
};

} // namespace sc
