#pragma once

#define Conv(v) (struct sockaddr*)(v)

static const int gdefaultsockfd = -1;
static const int gbacklog = 8; // 全连接队列长度