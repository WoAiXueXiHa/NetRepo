#pragma once

enum{
    USAGE_ERR = 1,
    SOCKET_ERR,
    BIND_ERR
};

#define CONV(v) (struct sockaddr* )(v)