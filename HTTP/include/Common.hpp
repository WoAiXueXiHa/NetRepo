#pragma once
#include <iostream>
#include <string>
#include <cstdlib>

enum {
    USAGE_ERR = 1,
    SOCKET_ERR,
    BIND_ERR,
    LISTEN_ERR
};
#define Conv(v) (struct sockaddr*)(v)
const int gdefaultsockfd = -1;
const int gbacklog = 8;