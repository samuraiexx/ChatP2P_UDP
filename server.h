#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Server {
  private:
    int s;
    struct sockaddr_in address;
    bool itsOn;

  public:
    Server();

    void turnOn();
    void turnOff();
};

#endif
