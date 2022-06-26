#pragma once
#include "pressure.h"
#include <arpa/inet.h>
#include <sys/socket.h>

void error_handling(char *message);
int serv2(int port_num);
