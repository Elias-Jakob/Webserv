#ifndef SERVER_HPP
# define SERVER_HPP
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <map>
#include <vector>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>

#include "src/http/HttpResponse.hpp"
#include "src/http/ClientConnection.hpp"

#endif