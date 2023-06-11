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

    void connect(int token, const char* host, int port, const char* pwd);

    void disconnect(int token);

    void joinGroup(int token, const char* groupName, const char *groupPwd,
                   const char* userName, const char *userPwd);

    void leaveGroup(int token, AooId group);

    void forwardMessage(const char *data, int32_t size, aoo::time_tag time);

    void handleEvent(const AooEvent* e);
private:
    World* world_;
    std::shared_ptr<INode> node_;

    void sendError(const char *cmd, AooId token,
                   AooError error, const AooResponse& response) {
        sendError(cmd, token, error, response.error.errorCode,
                  response.error.errorMessage);
    }

    void sendError(const char *cmd, AooId token, AooError error,
                   int code = 0, const char *msg = "");

    void handlePeerMessage(AooId group, AooId user, AooNtpTime time,
                           const AooData& data);

    template<typename T>
    T * createCommand(int port, int token);
};

struct AooClientCmd {
    int port;
    int token;
};

struct ConnectCmd : AooClientCmd {
    char serverHost[256];
    int32_t serverPort;
    char password[64];
    // TODO metadata
};

struct GroupJoinCmd : AooClientCmd {
    char groupName[256];
    char userName[256];
    char groupPwd[64];
    char userPwd[64];
    // TODO metadata
};

struct GroupLeaveCmd : AooClientCmd {
    AooId group;
};

struct RequestData {
    AooClient *client;
    AooId token;
};

} // namespace sc
