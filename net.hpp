#pragma once

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

struct Message
{
    char buffer[10000];
    int size;
};

union ConnectionEventData
{
    Message message;
};

enum ConnectionEventType
{
    NEW_MESSAGE,
    CLOSED
};

struct ConnectionEvent
{
    ConnectionEventType type;
    ConnectionEventData data;
};

class Connection
{
private:
    int socket_fd;
    int epoll_fd;

public:
    Connection() {}
    Connection(int socket_fd) : socket_fd(socket_fd)
    {
        epoll_fd = epoll_create1(0);
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = socket_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev);
    }

    int poll(ConnectionEvent *events)
    {
        epoll_event epoll_events[10000];
        int nfds = epoll_wait(epoll_fd, epoll_events, 10000, 100);
        if (nfds == -1)
        {
            throw "failed to epoll_wait";
        }

        int event_count = 0;
        for (int i = 0; i < nfds; ++i)
        {
            if (epoll_events[i].data.fd == socket_fd)
            {
                char buffer[10000];
                int recv_length = recv(socket_fd, buffer, sizeof(buffer), 0);

                if (recv_length < 0)
                {
                    throw "failed to recv";
                }

                ConnectionEvent connection_event;
                if (recv_length == 0)
                {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, NULL);
                    connection_event.type = CLOSED;
                    events[event_count++] = connection_event;
                    continue;
                }

                connection_event.type = NEW_MESSAGE;
                connection_event.data.message.size = recv_length;
                memcpy(connection_event.data.message.buffer, buffer, recv_length);
                events[event_count++] = connection_event;
            }
        }

        return event_count;
    }

    void write(const char *message, int size)
    {
        send(socket_fd, message, size, 0);
    }
};

enum NetEventType
{
    NEW_CONNECTION,
};

union NetEventData
{
    Connection connection;
    NetEventData() {}
};

struct NetEvent
{
    NetEventType type;
    NetEventData data;
    NetEvent() {}
};

class Net
{
private:
    int server_fd;
    int epoll_fd;

public:
    Connection connect(const char *ip, short port)
    {
        int epoll_fd = epoll_create1(0);
        if (epoll_fd == -1)
        {
            perror("epoll_create1");
            exit(EXIT_FAILURE);
        }
        int socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (socket_fd == -1)
        {
            perror("socket");
            exit(EXIT_FAILURE);
        }

        struct epoll_event event, events[100];
        event.data.fd = socket_fd;
        event.events = EPOLLOUT | EPOLLIN | EPOLLET;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1)
        {
            perror("epoll_ctl");
            exit(EXIT_FAILURE);
        }

        sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(ip);

        ::connect(socket_fd, (sockaddr *)&server_addr, sizeof(server_addr));

        int nfds = epoll_wait(epoll_fd, events, 100, -1);
        if (nfds == -1)
        {
            perror("epoll_wait");
            return -1;
        }

        int event_count = 0;
        for (int i = 0; i < nfds; ++i)
        {
            if (events[i].data.fd == socket_fd)
            {
                return Connection(socket_fd);
            }
        }

        return Connection();
    }

    void listen(const char *ip, short port)
    {
        server_fd = socket(PF_INET, SOCK_STREAM, 0);
        if (server_fd < 0)
        {
            throw "failed to create socket";
        }

        if (fcntl(server_fd, F_SETFL, O_NONBLOCK) < 0)
        {
            throw "failed to set socket to nonblock";
        }

        sockaddr_in host_addr;
        memset(&host_addr, 0, sizeof(host_addr));
        host_addr.sin_family = AF_INET;
        host_addr.sin_port = htons(port);
        host_addr.sin_addr.s_addr = inet_addr(ip);

        if (bind(server_fd, (sockaddr *)&host_addr, sizeof(sockaddr)) < 0)
        {
            throw "failed to bind";
        }

        if (::listen(server_fd, 10) < 0)
        {
            throw "failed to listen";
        }

        epoll_fd = epoll_create1(0);
        if (epoll_fd < 0)
        {
            throw "failed to create epoll";
        }
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = server_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0)
        {
            throw "failed to add fd to epoll";
        }
    }

    int poll(NetEvent *events)
    {
        epoll_event epoll_events[10000];
        int nfds = epoll_wait(epoll_fd, epoll_events, 10000, 100);
        if (nfds < 0)
            throw "failed to epoll wait";

        int event_count = 0;
        for (int i = 0; i < nfds; ++i)
        {
            if (epoll_events[i].data.fd == server_fd)
            {
                sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);
                int client_fd = accept(server_fd, (sockaddr *)&client_addr, &len);
                NetEvent net_event;
                net_event.type = NEW_CONNECTION;
                NetEventData net_event_data;
                net_event_data.connection = Connection(client_fd);
                net_event.data = net_event_data;
                events[event_count++] = net_event;
            }
        }

        return event_count;
    }

    void close()
    {
        ::close(server_fd);
        ::close(epoll_fd);
    }
};