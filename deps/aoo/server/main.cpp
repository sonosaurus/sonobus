#include "aoo/aoo.h"
#include "aoo/aoo_server.hpp"

#include "common/udp_server.hpp"
#include "common/tcp_server.hpp"
#include "common/sync.hpp"

#include <stdlib.h>
#include <string.h>
#include <iostream>

#ifdef _WIN32
# include <windows.h>
#else
# include <signal.h>
# include <stdio.h>
#endif

#ifndef AOO_DEFAULT_SERVER_PORT
# define AOO_DEFAULT_SERVER_PORT 7078
#endif

AooLogLevel g_loglevel = kAooLogLevelWarning;

void log_function(AooLogLevel level, const AooChar *msg) {
    if (level <= g_loglevel) {
        switch (level) {
        case kAooLogLevelDebug:
            std::cout << "[debug] ";
            break;
        case kAooLogLevelVerbose:
            std::cout << "[verbose] ";
            break;
        case kAooLogLevelWarning:
            std::cout << "[warning] ";
            break;
        case kAooLogLevelError:
            std::cout << "[error] ";
            break;
        default:
            break;
        }
        std::cout << msg << std::endl;
    }
}

AooServer::Ptr g_aoo_server;

int g_error = 0;
aoo::sync::semaphore g_semaphore;

void stop_server(int error) {
    g_error = error;
    g_semaphore.post();
}

aoo::udp_server g_udp_server;

void handle_udp_receive(int e, const aoo::ip_address& addr,
                        const AooByte *data, AooSize size) {
    if (e == 0) {
        g_aoo_server->handleUdpMessage(data, size, addr.address(), addr.length(),
            [](void *, const AooByte *data, AooInt32 size,
                    const void *address, AooAddrSize addrlen, AooFlag) {
                aoo::ip_address addr((const struct sockaddr *)address, addrlen);
                return g_udp_server.send(addr, data, size);
            }, nullptr);
    } else {
        if (g_loglevel >= kAooLogLevelError)
            std::cout << "UDP server: recv() failed: " << aoo::socket_strerror(e) << std::endl;
        stop_server(e);
    }
}

aoo::tcp_server g_tcp_server;

AooId handle_tcp_accept(int e, const aoo::ip_address& addr) {
    if (e == 0) {
        // add new client
        AooId id;
        g_aoo_server->addClient([](void *, AooId client, const AooByte *data, AooSize size) {
            return g_tcp_server.send(client, data, size);
        }, nullptr, &id);
        if (g_loglevel >= kAooLogLevelVerbose) {
            std::cout << "Add new client " << id << std::endl;
        }
        return id;
    } else {
        // error
        if (g_loglevel >= kAooLogLevelError)
            std::cout << "TCP server: accept() failed: " << aoo::socket_strerror(e) << std::endl;
    #if 1
        stop_server(e);
    #endif
        return kAooIdInvalid;
    }
}

void handle_tcp_receive(int e, AooId client, const aoo::ip_address& addr,
                        const AooByte *data, AooSize size) {
    if (e == 0 && size > 0) {
        // handle client message
        if (auto err = g_aoo_server->handleClientMessage(client, data, size); err != kAooOk) {
            // remove misbehaving client
            g_aoo_server->removeClient(client);
            g_tcp_server.close(client);
            if (g_loglevel >= kAooLogLevelWarning)
                std::cout << "Close client " << client << " after error: " << aoo_strerror(err) << std::endl;
        }
    } else {
        // close client
        if (e != 0) {
            if (g_loglevel >= kAooLogLevelWarning)
                std::cout << "Close client after error: " << aoo::socket_strerror(e) << std::endl;
        } else {
            if (g_loglevel >= kAooLogLevelVerbose)
                std::cout << "Client " << client << " has disconnected" << std::endl;
        }
        g_aoo_server->removeClient(client);
    }
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
        << "  -p, --port             port number (default = " << AOO_DEFAULT_SERVER_PORT << ")\n"
        << "  -r, --relay            enable server relay\n"
        << "  -l, --log-level=LEVEL  set log level\n"
        << std::endl;
}

bool check_arguments(const char **argv, int argc, int numargs) {
    if (argc > numargs) {
        return true;
    } else {
        std::cout << "Missing argument(s) for option '" << argv[0] << "'";
        return false;
    }
}

bool match_option(const char *str, const char *short_option, const char *long_option) {
    return (short_option && !strcmp(str, short_option))
           || (long_option && !strcmp(str, long_option));
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
    int port = AOO_DEFAULT_SERVER_PORT;
    bool relay = false;

    argc--; argv++;

    try {
        while ((argc > 0) && (argv[0][0] == '-')) {
            if (match_option(argv[0], "-h", "--help")) {
                print_usage();
                return EXIT_SUCCESS;
            } else if (match_option(argv[0], "-v", "--version")) {
                std::cout << "aooserver " << aoo_getVersionString() << std::endl;
                return EXIT_SUCCESS;
            } else if (match_option(argv[0], "-p", "--port")) {
                if (!check_arguments(argv, argc, 1)) {
                    return EXIT_FAILURE;
                }
                port = std::stoi(argv[1]);
                if (port <= 0 || port > 65535) {
                    std::cout << "Port number " << port << " out of range" << std::endl;
                    return EXIT_FAILURE;
                }
                argc--; argv++;
            } else if (match_option(argv[0], "-r", "--relay")) {
                relay = true;
            } else if (match_option(argv[0], "-l", "--log-level")) {
                if (!check_arguments(argv, argc, 1)) {
                    return EXIT_FAILURE;
                }
                g_loglevel = std::stoi(argv[1]);
                argc--; argv++;
            } else {
                std::cout << "Unknown command line option '" << argv[0] << "'" << std::endl;
                print_usage();
                return EXIT_FAILURE;
            }
            argc--; argv++;
        }
    } catch (const std::exception& e) {
        std::cout << "Bad argument for option '" << argv[0] << "'" << std::endl;
        return EXIT_FAILURE;
    }

    AooSettings settings;
    AooSettings_init(&settings);
    settings.logFunc = log_function;
    if (auto err = aoo_initialize(&settings); err != kAooOk) {
        std::cout << "Could not initialize AOO library: "
                  << aoo_strerror(err) << std::endl;
        return EXIT_FAILURE;
    }

    AooError err;
    g_aoo_server = AooServer::create(&err);
    if (!g_aoo_server) {
        std::cout << "Could not create AooServer: "
                  << aoo_strerror(err) << std::endl;
        return EXIT_FAILURE;
    }

    // setup UDP server
    // TODO: increase socket receive buffer for relay? Use threaded receive?
    try {
        g_udp_server.start(port, handle_udp_receive);
    } catch (const std::exception& e) {
        std::cout << "Could not start UDP server: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // setup TCP server
    try {
        g_tcp_server.start(port, handle_tcp_accept, handle_tcp_receive);
    } catch (const std::exception& e) {
        std::cout << "Could not start TCP server: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // setup AooServer
    auto flags = aoo::socket_family(g_udp_server.socket()) == aoo::ip_address::IPv6 ?
                     kAooSocketDualStack : kAooSocketIPv4;

    if (auto err = g_aoo_server->setup(port, flags); err != kAooOk) {
        std::cout << "Could not setup AooServer: " << aoo_strerror(err) << std::endl;
        return EXIT_FAILURE;
    }

    g_aoo_server->setServerRelay(relay);

    // finally start network threads
    auto udp_thread = std::thread([]() {
        g_udp_server.run();
    });
    auto tcp_thread = std::thread([]() {
        g_tcp_server.run();
    });

    if (g_loglevel >= kAooLogLevelVerbose) {
        std::cout << "Listening on port " << port << std::endl;
    }

    // wait for stop signal
    g_semaphore.wait();

    if (g_error == 0) {
        std::cout << "Program stopped by the user" << std::endl;
    } else {
        std::cout << "Program stopped because of an error: "
                  << aoo::socket_strerror(g_error) << std::endl;
    }

    // stop UDP and TCP server and exit
    g_udp_server.stop();
    udp_thread.join();

    g_tcp_server.stop();
    tcp_thread.join();

    aoo_terminate();

    return EXIT_SUCCESS;
}
