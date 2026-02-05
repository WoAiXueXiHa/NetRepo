#pragma once
#include <iostream>

#define Conv(v) (struct sockaddr*)(v)

static const std::string gip = "127.0.0.1";
static const int gport = 8082;
static const int gfd = -1;
static const int BACKLOG = 16;
enum STATUS_INFO{
    SOCKET_ERR = 1,
    BIND_ERR,
    LISTEN_ERR,
    ACCEPT_ERR,
    CONNECT_ERR
};