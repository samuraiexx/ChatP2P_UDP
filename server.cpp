#include "server.h"

#define PORT 2222 
#define BUFSIZE 1024

using namespace std;

Server::Server(){
//Creating a server socket
  //First of all, generate the socket
  if((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
    cerr << "Erro: Não foi possível criar socket." << endl;
    return;
  }

  //Then, fill address comonents and bind it to the socket.
  memset((char *)&address, 0, sizeof address);
  address.sin_family = AF_INET;
  address.sin_port = htons(PORT);
  address.sin_addr.s_addr = htonl(INADDR_ANY); //0.0.0.0, or htos(0)
  if(bind(s, (struct sockaddr *)&address, sizeof address) < 0){
    cerr << "Erro: Não foi possível realizar o bind." << endl;
    return;
  }

  //At first, it's turned off.
  itsOn = false;

}

void Server::turnOn(){
  struct sockaddr_in clientAddress;
  socklen_t addressSize = sizeof clientAddress;
  char buf[BUFSIZE];
  char opt;

  itsOn = true;
  while(itsOn){
    cout << "Esperando..." << endl;

    int recvlen = recvfrom(s, buf, BUFSIZE, 0, (struct sockaddr *)&clientAddress, &addressSize);
    cout << "Pacote recebido com " << recvlen << " bytes!" << endl;
    if(recvlen > 0) {
      string buff = buf;
      cout << buff << endl;
    }
    string reply = "Olar";
    sendto(s, reply.c_str(), (int) reply.size(), 0, (struct sockaddr *)&clientAddress, addressSize);

    cout << "Deseja continuar? [Y/N] ";
    cin >> opt;
    if(opt == 'N' or opt == 'n') turnOff();
  }
}

void Server::turnOff(){
  itsOn = false;
}
