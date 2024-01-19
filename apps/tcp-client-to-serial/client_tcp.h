#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <log_policy.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

template <class LogPolicy = LogPolicyStdout> class ClientTCP {
  private:
    int _socket_fd;

    bool _set_keep_alive_options(int socket) {
        int optval = 1;
        if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &optval,
                       sizeof(optval))) {
            LogPolicy::log("Error on setsockopt: %s\n", strerror(errno));
            return false;
        }

        // Set the keepalive parameters
        int keepidle = 15;  // Time in seconds before starting keepalive probes
        int keepintvl = 10; // Time in seconds between keepalive probes
        int keepcnt = 3;    // Number of keepalive probes before considering the
                            // connection dead

        if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle,
                       sizeof(keepidle)) == -1) {
            LogPolicy::log("Error on setsockopt: %s\n", strerror(errno));
            return false;
        }

        if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl,
                       sizeof(keepintvl)) == -1) {
            LogPolicy::log("Error on setsockopt: %s\n", strerror(errno));
            return false;
        }

        if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt,
                       sizeof(keepcnt)) == -1) {
            LogPolicy::log("Error on setsockopt: %s\n", strerror(errno));
            return false;
        }

        return true;
    }

  public:
    ClientTCP() : _socket_fd(-1) {}
    ~ClientTCP() { disconnect(); }

    size_t tx(const std::uint8_t* data, const std::size_t data_sz) {
        if (!isConnect())
            return 0;

        ssize_t bytesSent = send(_socket_fd, data, data_sz, 0);
        if (bytesSent == -1) {
            LogPolicy::log("Error on send: %s\n", strerror(errno));
            disconnect();
            return 0;
        }

        return static_cast<size_t>(bytesSent);
    }
    size_t rx(std::uint8_t* data, const std::size_t max_data_sz) {
        if (!isConnect())
            return 0;

        ssize_t bytesRead = recv(_socket_fd, data, max_data_sz, 0);

        if (bytesRead == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No data was available for reading
                return 0;
            } else {
                // Error on recv
                LogPolicy::log("Error on recv: %s\n", strerror(errno));
            }
        }

        return static_cast<size_t>(bytesRead);
    }

    bool connect(const char* server_host, uint16_t server_port) {
        struct addrinfo hints, *server_info;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        char port_str[6];
        snprintf(port_str, sizeof(port_str), "%u", server_port);

        int errcode = getaddrinfo(server_host, port_str, &hints, &server_info);
        if (errcode != 0) {
            LogPolicy::log("Error on getaddrinfo: %s\n", gai_strerror(errcode));
            return false;
        }

        _socket_fd = socket(server_info->ai_family, server_info->ai_socktype,
                            server_info->ai_protocol);
        if (_socket_fd == -1) {
            LogPolicy::log("Error on socket: %s\n", strerror(errno));
            freeaddrinfo(server_info);
            return false;
        }

        if (!_set_keep_alive_options(_socket_fd)) {
            close(_socket_fd);
            _socket_fd = -1;
            freeaddrinfo(server_info);
            return false;
        }

        // keep-alive
        int optval = 1;
        if (setsockopt(_socket_fd, SOL_SOCKET, SO_KEEPALIVE, &optval,
                       sizeof(optval))) {
            LogPolicy::log("Error on setsockopt: %s\n", strerror(errno));
            close(_socket_fd);
            _socket_fd = -1;
            freeaddrinfo(server_info);
            return false;
        }

        // Set the keepalive parameters
        int keepidle = 60;  // Time in seconds before starting keepalive probes
        int keepintvl = 10; // Time in seconds between keepalive probes
        int keepcnt = 6;    // Number of keepalive probes before considering the
                            // connection dead

        if (setsockopt(_socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle,
                       sizeof(keepidle)) == -1) {
            LogPolicy::log("Error on setsockopt: %s\n", strerror(errno));
            return 1;
        }

        if (setsockopt(_socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl,
                       sizeof(keepintvl)) == -1) {
            perror("setsockopt TCP_KEEPINTVL");
            return 1;
        }

        if (setsockopt(_socket_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt,
                       sizeof(keepcnt)) == -1) {
            perror("setsockopt TCP_KEEPCNT");
            return 1;
        }

        // set socket nonblock
        int flags = fcntl(_socket_fd, F_GETFL, 0);
        if (flags == -1) {
            LogPolicy::log("Error on fcntl: %s\n", strerror(errno));
            freeaddrinfo(server_info);
            return false;
        }
        fcntl(_socket_fd, F_SETFL, flags | O_NONBLOCK);

        if (::connect(_socket_fd, server_info->ai_addr,
                      server_info->ai_addrlen) == -1) {
            if (errno == EINPROGRESS) {
                fd_set write_set;
                FD_ZERO(&write_set);
                FD_SET(_socket_fd, &write_set);

                struct timeval tv;
                tv.tv_sec = 5;
                tv.tv_usec = 0;

                int retval =
                    select(_socket_fd + 1, NULL, &write_set, NULL, &tv);
                if (retval == -1) {
                    LogPolicy::log("Error on select: %s\n", strerror(errno));
                    close(_socket_fd);
                    _socket_fd = -1;
                    freeaddrinfo(server_info);
                    return false;
                } else if (retval == 0) {
                    LogPolicy::log("Timeout on select\n");
                    close(_socket_fd);
                    _socket_fd = -1;
                    freeaddrinfo(server_info);
                    return false;
                } else {
                    int error = 0;
                    socklen_t len = sizeof(error);
                    int retval = getsockopt(_socket_fd, SOL_SOCKET, SO_ERROR,
                                            &error, &len);
                    if (retval != 0 || error != 0) {
                        // LogPolicy::log("Error on getsockopt: %s\n",
                        // strerror(errno));
                        LogPolicy::log("Couldn't connect to server\n");
                        close(_socket_fd);
                        _socket_fd = -1;
                        freeaddrinfo(server_info);
                        return false;
                    }
                }
            } else {
                LogPolicy::log("Error on connect: %s\n", strerror(errno));
                close(_socket_fd);
                _socket_fd = -1;
                freeaddrinfo(server_info);
                return false;
            }
        }

        freeaddrinfo(server_info);
        return true;
    }

    void disconnect() {
        if (isConnect()) {
            close(_socket_fd);
            _socket_fd = -1;
        }
    }

    bool isConnect() {
        if (_socket_fd == -1) {
            return false;
        } else {
            int error = 0;
            socklen_t len = sizeof(error);
            int retval =
                getsockopt(_socket_fd, SOL_SOCKET, SO_ERROR, &error, &len);
            if (retval != 0 || error != 0) {
                LogPolicy::log("Error on getsockopt: %s\n", strerror(errno));
                return false;
            }
        }

        // check if my tcp connection is still alive
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(_socket_fd, &read_set);

        int retval = select(_socket_fd + 1, &read_set, NULL, NULL, &tv);
        if (retval == -1) {
            LogPolicy::log("Error on select: %s\n", strerror(errno));
            return false;
        } else if (retval == 0) {
            // LogPolicy::log("Timeout on select\n");
            return true;
        } else {
            char buf[1];
            ssize_t bytesRead = recv(_socket_fd, buf, sizeof(buf), MSG_PEEK);
            if (bytesRead == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No data was available for reading
                    return true;
                } else {
                    // Error on recv
                    LogPolicy::log("Error on recv: %s\n", strerror(errno));
                    return false;
                }
            }
        }

        return true;
    }
};
