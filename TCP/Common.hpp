#pragma once

#define Conv(v) (struct sockaddr*)(v)

static const int gport = 8082;
static const int gfd = -1;

enum STATUS_INFO{
    SOCKET_ERR = 1,
    BIND_ERR,
    LISTEN_ERR,
    ACCEPT_ERR,
    CONNECT_ERR
};