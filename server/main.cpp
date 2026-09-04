#include "aoo.h"
#include "aoo_server.hpp"

#include "common/net_utils.hpp"
#include "common/sync.hpp"

#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <optional>
#include <string_view>
#include <thread>
#include <map>

#ifdef _WIN32
# include <windows.h>
#else
# include <signal.h>
# include <stdio.h>
#endif

#include "SonoBusAooMetadata.h"

#define SONOBUS_DEFAULT_SERVER_PORT 10996

AooLogLevel g_loglevel = kAooLogLevelWarning;

void log_function(AooLogLevel level, const AooChar *msg) {
    if (level <= g_loglevel) {
        switch (level) {
            case kAooLogLevelError:
                std::cout << "[error] ";
                break;
            case kAooLogLevelWarning:
                std::cout << "[warning] ";
                break;
            case kAooLogLevelInfo:
                std::cout << "[info] ";
                break;
            case kAooLogLevelDebug:
                std::cout << "[debug] ";
                break;
            case kAooLogLevelVerbose:
                std::cout << "[verbose] ";
                break;
            default:
                break;
        }
        std::cout << msg << std::endl;
    }
}


struct PublicGroupState
{
    AooId creatorClientId;
    AooId creatorUserId;
    AooId groupId;
    std::string groupName;

    std::vector<std::string> users;

    void addUser(const std::string & name) {
        users.push_back(name);
    }

    void removeUser(const std::string & name) {
        users.erase(std::remove(users.begin(), users.end(), name), users.end());
    }
};

std::map<AooId, PublicGroupState> g_public_groups;


struct PublicGroupSubscriberState
{
    AooId clientId;
};

// key is clientId
std::map<AooId, PublicGroupSubscriberState> g_public_group_subscribers;


AooServer::Ptr g_server;

AooError g_error_code = 0;
char g_error_message[256] = {};
aoo::sync::semaphore g_semaphore;
aoo::sync::mutex g_publock;


static void notify_public_group_subscribers(AooId groupId, AooId clientId = kAooIdAll, bool removed = false)
{
    // ASSUMES publock is already taken

    auto found = g_public_groups.find(groupId);
    if (found == g_public_groups.end()) {
        return;
    }

    const auto & state = found->second;

    sonobus::PublicGroupUpdateMetadata update;
    update.groupId = groupId;
    update.removed = removed;
    update.groupName = state.groupName;
    if (!removed) {
        update.users = state.users;
    }
    sonobus::ScopedAooData data;
    if (!sonobus::toAooData(data, update)) {
        std::cerr << "Error serializing public group update data" << std::endl;
        return;
    }

    for (auto iter : g_public_group_subscribers) {
        if (clientId == kAooIdAll || clientId == iter.second.clientId) {
            g_server->notifyClient(iter.second.clientId, data.get());
        }
    }
}

static void notify_public_allgroup_subscribers(AooId clientId = kAooIdAll)
{
    // ASSUMES publock is already taken
    for (auto group : g_public_groups) {
        notify_public_group_subscribers(group.second.groupId, clientId);
    }
}


AooBool AOO_CALL handle_request(void *, AooId client, AooId token, const AooRequest *request)
{
    bool handled = false;

    switch (request->type) {
        case kAooRequestCustom:
        {
            auto r = request->custom;
            sonobus::PublicGroupSubscribeRequestMetadata pubsubscribe;
            if (sonobus::fromAooData(r.data, pubsubscribe)) {
                std::cout << "Received PublicGroupSubscribeRequestMetadata" << std::endl;

                aoo::sync::scoped_lock<aoo::sync::mutex> scopedLock(g_publock);
                if (pubsubscribe.subscribed) {
                    g_public_group_subscribers[client] = { client };
                    std::cout << "Added public group subscriber client: " << client << std::endl;

                    notify_public_allgroup_subscribers(client);
                }
                else {
                    std::cout << "Removing public group subscriber client: " << client << std::endl;
                    g_public_group_subscribers.erase(client);
                }

            }
            handled = true;
            break;
        }
        default:
            break;
    }

    return handled;
}

void AOO_CALL handle_event(void *, const AooEvent *event, AooThreadLevel level)
{
    switch (event->type) {
        case kAooEventClientLogin:
        {
            auto e = event->clientLogin;
            if (e.error == kAooOk) {
                std::cout << "New client with ID " << e.id << std::endl;
            } else {
                std::cout << "Client " << e.id << " failed to log in" << std::endl;
            }
            break;
        }
        case kAooEventClientLogout:
        {
            auto e = event->clientLogout;
            if (e.errorCode == kAooOk) {
                std::cout << "Client " << e.id << " logged out" << std::endl;
            } else {
                std::cout << "Client " << e.id << " logged out after error: "
                << e.errorMessage << std::endl;
            }

            {
                aoo::sync::scoped_lock<aoo::sync::mutex> scopedLock(g_publock);
                g_public_group_subscribers.erase(e.id);
            }


            break;
        }
        case kAooEventGroupAdd:
        {
            auto e = event->groupAdd;
            std::cout << "Add new group '" << e.name << "'" << std::endl;

            if (e.metadata != nullptr) {
                sonobus::GroupMetadata grpmetadata;
                if (sonobus::fromAooData(*e.metadata, grpmetadata)) {
                    if (grpmetadata.isPublic) {
                        PublicGroupState groupstate;
                        groupstate.groupId = e.id;
                        groupstate.groupName = e.name;

                        aoo::sync::scoped_lock<aoo::sync::mutex> scopedLock(g_publock);
                        g_public_groups[groupstate.groupId] = groupstate;

                        std::cout << "Added to public groups" << std::endl;
                        notify_public_group_subscribers(groupstate.groupId);
                    }
                }
            }

            break;
        }
        case kAooEventGroupRemove:
        {
            auto e = event->groupRemove;
            std::cout << "Remove group '" << e.name << "'" << std::endl;

            auto found = g_public_groups.find(e.id);
            if (found != g_public_groups.end()) {
                aoo::sync::scoped_lock<aoo::sync::mutex> scopedLock(g_publock);
                notify_public_group_subscribers(e.id, kAooIdAll, true);

                g_public_groups.erase(found);
                std::cout << "Removed from public groups" << std::endl;
            }

            break;
        }

        case kAooEventGroupJoin:
        {
            auto e = event->groupJoin;

            std::cout << "Peer joined group '" << e.groupName << "'  username: '" << e.userName <<  "'" <<  std::endl;

            aoo::sync::scoped_lock<aoo::sync::mutex> scopedLock(g_publock);

            auto found = g_public_groups.find(e.groupId);
            if (found != g_public_groups.end()) {
                auto & state = found->second;
                state.addUser(e.userName);
                notify_public_group_subscribers(state.groupId);
            }


            break;
        }
        case kAooEventGroupLeave:
        {
            auto e = event->groupLeave;

            std::cout << "Peer left group '" << e.groupName << "'  username: '" << e.userName <<  "'" <<  std::endl;

            aoo::sync::scoped_lock<aoo::sync::mutex> scopedLock(g_publock);

            auto found = g_public_groups.find(e.groupId);
            if (found != g_public_groups.end()) {
                auto & state = found->second;
                state.removeUser(e.userName);
                notify_public_group_subscribers(state.groupId);
            }

            break;
        }
        case kAooEventGroupUpdate:
        {
            auto e = event->groupUpdate;

            std::cout << "Updated group id: " << e.groupId << std::endl;

            break;
        }
        case kAooEventNotification:
        {
            std::cout << "Custom Notifications" << std::endl;



            break;
        }

        default:
            break;
    }
}


void stop_server(int error, const char *msg = nullptr) {
    if (msg) {
        snprintf(g_error_message, sizeof(g_error_message), "%s", msg);
    }
    g_error_code = error;
    g_semaphore.post();
}

#ifdef _WIN32
BOOL WINAPI console_handler(DWORD signal) {
    switch (signal) {
        case CTRL_C_EVENT:
            stop_server(0);
            return TRUE;
        case CTRL_CLOSE_EVENT:
            return TRUE;
            // Pass other signals to the next handler.
        default:
            return FALSE;
    }
}
#else
bool set_signal_handler(int sig, sig_t handler) {
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(sig, &sa, nullptr) == 0) {
        return true;
    } else {
        perror("sigaction");
        return false;
    }
}

bool set_signal_handlers() {
    // NB: stop_server() is async-signal-safe!
    auto handler = [](int) { stop_server(0); };
    return set_signal_handler(SIGINT, handler)
    && set_signal_handler(SIGTERM, handler);
}
#endif

void print_usage() {
    std::cout
    << "Usage: aooserver [OPTIONS]...\n"
    << "Run an AOO server instance\n"
    << "Options:\n"
    << "  -h, --help             display help and exit\n"
    << "  -v, --version          print version and exit\n"
    << "  -p, --port=PORT        port number (default = " << SONOBUS_DEFAULT_SERVER_PORT << ")\n"
    << "  -P, --password=PWD     password\n"
    << "  -r, --relay            enable server relay\n"
    << "  -l, --log-level=LEVEL  set log level\n"
    << std::endl;
}

// match option with a single argument
template<typename T>
std::optional<T> match_option(const char **& argv, int& argc,
                              const char* short_option, const char* long_option) {
    assert(argc > 0);
    if ((short_option && !strcmp(argv[0], short_option))
        || (long_option && !strcmp(argv[0], long_option))) {
        // -f <arg> or --foo <arg>
        if (argc < 2) {
            std::stringstream msg;
            msg << "Missing argument for option '" << argv[0] << "'";
            throw std::runtime_error(msg.str());
        }
        T result;
        std::stringstream ss;
        ss << argv[1];
        ss >> result;
        if (ss) {
            argv += 2; argc -= 2;
            return result;
        } else {
            std::stringstream msg;
            msg << "Bad argument '" << argv[1] << "' for option '" << argv[0] << "'";
            throw std::runtime_error(msg.str());
        }
    } else if (long_option) {
        // --foo=<arg>
        std::string_view str(argv[0]);
        if (auto pos = str.find('='); pos != std::string::npos) {
            if (auto opt = str.substr(0, pos); opt == long_option) {
                auto arg = str.substr(pos + 1);
                T result;
                std::stringstream ss;
                ss << arg;
                ss >> result;
                if (ss) {
                    argv++; argc--;
                    return result;
                } else {
                    std::stringstream msg;
                    msg << "Bad argument '" << arg << "' for option '" << opt << "'";
                    throw std::runtime_error(msg.str());
                }
            }
        }
    }
    return std::nullopt;
}

// match option without argument
bool match_option(const char **& argv, int& argc,
                  const char* short_option, const char* long_option) {
    assert(argc > 0);
    if ((short_option && !strcmp(argv[0], short_option))
        || (long_option && !strcmp(argv[0], long_option))) {
        argv++; argc--;
        return true;
    } else {
        return false;
    }
}

int main(int argc, const char **argv) {
    // set control handler
#ifdef _WIN32
    if (!SetConsoleCtrlHandler(console_handler, TRUE)) {
        std::cout << "Could not set console handler" << std::endl;
        return EXIT_FAILURE;
    }
#else
    if (!set_signal_handlers()) {
        return EXIT_FAILURE;
    }
#endif

    // parse command line options
    int port = SONOBUS_DEFAULT_SERVER_PORT;
    bool relay = false;
    std::string password;

    argc--; argv++;

    try {
        while ((argc > 0) && (argv[0][0] == '-')) {
            if (match_option(argv, argc, "-h", "--help")) {
                print_usage();
                return EXIT_SUCCESS;
            } else if (match_option(argv, argc, "-v", "--version")) {
                std::cout << "aooserver " << aoo_getVersionString() << std::endl;
                return EXIT_SUCCESS;
            } else if (auto arg = match_option<int>(argv, argc, "-p", "--port")) {
                port = *arg;
                if (port <= 0 || port > 65535) {
                    std::cout << "Port number " << port << " out of range" << std::endl;
                    return EXIT_FAILURE;
                }
            } else if (auto arg = match_option<std::string>(argv, argc, "-P", "--password")) {
                password = *arg;
            } else if (match_option(argv, argc, "-r", "--relay")) {
                relay = true;
            } else if (auto arg = match_option<int>(argv, argc, "-l", "--log-level")) {
                auto level = *arg;
                if (level < kAooLogLevelSilent || level > kAooLogLevelVerbose) {
                    std::cout << "Log level " << level << " out of range" << std::endl;
                    return EXIT_FAILURE;
                }
                g_loglevel = level;
            } else {
                std::cout << "Unknown command line option '" << argv[0] << "'" << std::endl;
                print_usage();
                return EXIT_FAILURE;
            }
        }
        if (argc > 0) {
            std::cout << "Ignoring excess arguments: ";
            for (int i = 0; i < argc; ++i) {
                std::cout << argv[i] << " ";
            }
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    AooSettings settings;
    settings.logFunc = log_function;
    if (auto err = aoo_initialize(&settings); err != kAooOk) {
        std::cout << "Could not initialize AOO library: "
        << aoo_strerror(err) << std::endl;
        return EXIT_FAILURE;
    }

    g_server = AooServer::create();
    if (!g_server) {
        std::cout << "Could not create AooServer" << std::endl;
        return EXIT_FAILURE;
    }

    g_server->setEventHandler(handle_event, nullptr, kAooEventModeCallback);

    g_server->setRequestHandler(handle_request, nullptr, 0);


    if (!password.empty()) {
        g_server->setPassword(password.c_str());
    }

    AooServerSettings server_settings;
    server_settings.portNumber = port;

    auto err = g_server->setup(server_settings);
    if (err != kAooOk) {
        std::string msg;
        if (err == kAooErrorSocket) {
            msg = aoo::socket::strerror(aoo::socket::get_last_error());
        } else {
            msg = aoo_strerror(err);
        }
        std::cout << "Could not setup AooServer: " << msg << std::endl;
        return EXIT_FAILURE;
    }

    g_server->setUseInternalRelay(relay);

    if (g_loglevel >= kAooLogLevelInfo) {
        std::cout << "Listening on port " << port << std::endl;
    }

    // run server threads
    // NB: we *could* just block on the run() method, but then there
    // would be no "safe" way to break from a signal/control handler.
    auto thread = std::thread([]() {
        auto err = g_server->run(kAooInfinite);
        if (err != kAooOk) {
            std::string msg;
            if (err == kAooErrorSocket) {
                msg = aoo::socket::strerror(aoo::socket::get_last_error());
            } else {
                msg = aoo_strerror(err);
            }
            // break from the main thread
            stop_server(err, msg.c_str());
        }
    });

    auto udp_thread = std::thread([]() {
        auto err = g_server->receive(kAooInfinite);
        if (err != kAooOk) {
            std::string msg;
            if (err == kAooErrorSocket) {
                msg = aoo::socket::strerror(aoo::socket::get_last_error());
            } else {
                msg = aoo_strerror(err);
            }
            // break from the main thread
            stop_server(err, msg.c_str());
        }
    });

    // wait for stop signal
    g_semaphore.wait();

    if (g_error_code == 0) {
        std::cout << "Program stopped by the user" << std::endl;
    } else {
        std::cout << "Program stopped because of an error: "
        << g_error_message << std::endl;
    }

    // stop server and join threads
    g_server->stop();
    if (thread.joinable()) {
        thread.join();
    }
    if (udp_thread.joinable()) {
        udp_thread.join();
    }

    aoo_terminate();

    return g_error_code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
