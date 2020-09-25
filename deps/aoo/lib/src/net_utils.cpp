#include "net_utils.hpp"

#include <stdio.h>

namespace aoo {
namespace net {

void socket_close(int sock){
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

std::string socket_strerror(int err)
{
    char buf[1024];
#ifdef _WIN32
    buf[0] = 0;
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
                          err, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), buf,
                          1024, NULL);
#else
    snprintf(buf, 1024, "%s", strerror(err));
#endif
    return buf;
}

int socket_errno(){
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

int socket_set_nonblocking(int socket, int nonblocking)
{
#ifdef _WIN32
    u_long modearg = nonblocking;
    if (ioctlsocket(socket, FIONBIO, &modearg) != NO_ERROR)
        return -1;
#else
    int sockflags = fcntl(socket, F_GETFL, 0);
    if (nonblocking)
        sockflags |= O_NONBLOCK;
    else
        sockflags &= ~O_NONBLOCK;
    if (fcntl(socket, F_SETFL, sockflags) < 0)
        return -1;
#endif
    return 0;
}

// kudos to https://stackoverflow.com/a/46062474/6063908
int socket_connect(int socket, const ip_address& addr, float timeout)
{
    // set nonblocking and connect
    socket_set_nonblocking(socket, 1);

    if (connect(socket, (const struct sockaddr *)&addr.address, addr.length) < 0)
    {
        int status;
        struct timeval timeoutval;
        fd_set writefds, errfds;
    #ifdef _WIN32
        if (socket_errno() != WSAEWOULDBLOCK)
    #else
        if (socket_errno() != EINPROGRESS)
    #endif
            return -1; // break on "real" error

        // block with select using timeout
        if (timeout < 0) timeout = 0;
        timeoutval.tv_sec = (int)timeout;
        timeoutval.tv_usec = (timeout - timeoutval.tv_sec) * 1000000;
        FD_ZERO(&writefds);
        FD_SET(socket, &writefds); // socket is connected when writable
        FD_ZERO(&errfds);
        FD_SET(socket, &errfds); // catch exceptions

        status = select(socket+1, NULL, &writefds, &errfds, &timeoutval);
        if (status < 0) // select failed
        {
            fprintf(stderr, "socket_connect: select failed");
            return -1;
        }
        else if (status == 0) // connection timed out
        {
        #ifdef _WIN32
            WSASetLastError(WSAETIMEDOUT);
        #else
            errno = ETIMEDOUT;
        #endif
            return -1;
        }

        if (FD_ISSET(socket, &errfds)) // connection failed
        {
            int err; socklen_t len = sizeof(err);
            getsockopt(socket, SOL_SOCKET, SO_ERROR, (char *)&err, &len);
        #ifdef _WIN32
            WSASetLastError(err);
        #else
            errno = err;
        #endif
            return -1;
        }
    }
    // done, set blocking again
    socket_set_nonblocking(socket, 0);
    return 0;
}

} // net
} // aoo
