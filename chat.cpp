#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <queue>
#include <unistd.h>
#include <sys/fcntl.h>

using namespace std;

#define BUFFERSIZE 2048

string to_hex_string( const unsigned int i ) {
  std::stringstream s;
  s << "0x" << setfill('0') << setw(2) << hex << i;
  return s.str();
}

class ChatHandler{
  mutex qMu;
  priority_queue<string, vector<string>, greater<string>> queued_msgs; // true for messages false for confirmations
  string last_confirmed;
  bool on;

  friend class Client;
  friend class Server;
};

class Client{
  protected:
    char buffer[BUFFERSIZE];
    int send_s; //socket for send and receive
    struct sockaddr_in localAddr, remoteAddr, fromAddr;
    int count;
    socklen_t addr_size;
    ChatHandler* handler;

  public:
    Client(int remote_port, string ip, ChatHandler &chandler){
      handler = &chandler;
      count = 0;
      handler->on = true;
      addr_size = sizeof fromAddr;

      if((send_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw string("Socket creation failed");

      memset(&localAddr, 0, sizeof(localAddr));
      localAddr.sin_family = AF_INET;
      localAddr.sin_port = htons(0);
      localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
      if(bind(send_s, (struct sockaddr*) &localAddr, sizeof(localAddr)) < 0)
        throw string("Bind failed1");

      if(fcntl(send_s, F_SETFL, O_NONBLOCK) < 0)
        throw string("Failed to set socket to non-blocking.");

      memset(&remoteAddr, 0, sizeof(remoteAddr));
      remoteAddr.sin_family = AF_INET;
      remoteAddr.sin_port = htons(remote_port);

      if(inet_pton(AF_INET, ip.c_str(), &remoteAddr.sin_addr) <= 0)
        throw "Invalid Adress";

    }

    void client_thread(){
      while(handler->on) if(handler->queued_msgs.size()) {
        unique_lock<mutex> locker(handler->qMu);
        string msg = handler->queued_msgs.top(); handler->queued_msgs.pop();
        locker.unlock();

        string pkt = msg.substr(0, 4);
        if(pkt.compare(to_hex_string(0))){ //if its a message, not a confirmation
          while(handler->last_confirmed.compare(pkt) != 0) {
            locker.lock();
            string top;
            while(handler->queued_msgs.size() && (top = handler->queued_msgs.top()).substr(0, 4).compare(to_hex_string(0)))
              sendto(send_s, top.c_str(), top.size() + 1, 0,
                  (struct sockaddr*) &remoteAddr, sizeof(remoteAddr));
            locker.unlock();
            sendto(send_s, msg.c_str(), msg.size() + 1, 0,
                (struct sockaddr*) &remoteAddr, sizeof(remoteAddr));
          }
        } else { //its is a confirmation
          sendto(send_s, msg.c_str(), msg.size() + 1, 0,
              (struct sockaddr*) &remoteAddr, sizeof(remoteAddr));
        }
      }
    }

    void turn_off(){ handler->on = false; }

    void send(string msg) {
      if(msg == "EXIT")
        throw string("Chat has ended.");
      while(handler->queued_msgs.size() > 0 && count == 255); //Id reset, must wait the queue
      if(count == 255) count = 0;
      lock_guard<mutex> locker(handler->qMu);
      handler->queued_msgs.push(to_hex_string(++count) + msg);
    }

};

class Server{
  protected:
    char buffer[BUFFERSIZE];
    int send_s, receive_s; //socket for send and receive
    struct sockaddr_in localAddr, remoteAddr, fromAddr;
    socklen_t addr_size;
    string last_readed;
    ChatHandler* handler;

  public:
    Server(int local_port, ChatHandler &chandler){
      handler = &chandler;
      handler->on = true;
      addr_size = sizeof fromAddr;

      if((receive_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        throw string("Socket creation failed");

      memset(&localAddr, 0, sizeof(localAddr));
      localAddr.sin_family = AF_INET;
      localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
      localAddr.sin_port = htons(local_port);
      if(bind(receive_s, (struct sockaddr*) &localAddr, sizeof(localAddr)) < 0)
        throw string("Bind failed4");

      if(fcntl(receive_s, F_SETFL, O_NONBLOCK) < 0)
        throw string("Failed to set socket to non-blocking.");

    }

    void turn_off() { handler->on = false; }

    void server_thread(){
      while(handler->on){
        if(recvfrom(receive_s, buffer, BUFFERSIZE, 0, (struct sockaddr*)&fromAddr, &addr_size) < 4) continue;
        string msg(buffer);
        string pkt = msg.substr(0, 4);
        msg = msg.substr(4);
        if(pkt.compare(to_hex_string(0)) == 0) //confirmation message, update last packet received
          handler->last_confirmed = msg;
        else {
          if(pkt.compare(last_readed)) cout << "\t\t\t\t\t" << msg << endl, last_readed = pkt;
          unique_lock<mutex> locker(handler->qMu);
          handler->queued_msgs.push(to_hex_string(0) + pkt);
          locker.unlock();
        }
      }
    }
};

int main(){
  try{
    string msg, dest_ip;
    int port_in, port_out;

    cout << "Escolha uma porta para enviar mensagens: ";
    cin >> port_out;
    cout << "Escolha uma porta para receber mensagens: ";
    cin >> port_in;
    cout << "Insira o IP do destino: ";
    cin >> dest_ip;
    getline(cin, msg);
    cout << "Conversa iniciada." << endl;

    ChatHandler cHandler;
    Client clt(port_out, dest_ip, cHandler);
    Server svr(port_in, cHandler);
    thread client(&Client::client_thread, &clt);
    thread server(&Server::server_thread, &svr);

    try{
      while(getline(cin, msg)) clt.send(msg);
    } catch(string s){
      cout << s << endl;
      clt.turn_off();
      svr.turn_off();
      client.join();
      server.join();
    }

  } catch(string s){
    cout << s << endl;
  }

}
