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
#include <set>
#include <iomanip>
#include <sstream>

using namespace std;

#define BUFFERSIZE 2048

class client{
    char buffer[BUFFERSIZE];
    int fd;
    struct sockaddr_in myAddr, recAddr, clAddr;
    int i;
    set<string> not_received;
    
    string to_hex_string( const unsigned int i ) {
        std::stringstream s;
        s << "0x" << setfill('0') << setw(2) << hex << i;
        return s.str();
    }
    
public:
    client(int port, string ip) {
        i = 1;
        if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            throw string("Socket creation failed");
        memset(&myAddr, 0, sizeof(myAddr));
        myAddr.sin_family = AF_INET;
        myAddr.sin_port = htons(0);
        myAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if(bind(fd, (struct sockaddr*) &myAddr, sizeof(myAddr)) < 0)
            throw string("Bind failed");

        memset(&recAddr, 0, sizeof(myAddr));
        recAddr.sin_family = AF_INET;
        recAddr.sin_port = htons(port);

        if(inet_pton(AF_INET, ip.c_str(), &recAddr.sin_addr) <= 0)
            throw "Invalid Adress";
    }
    void sendThread(string id, string message) {
        while(not_received.count(id)){
            int status = sendto(fd, (id + ' ' + message).c_str(), message.size() + 1, 0, 
                         (struct sockaddr*) &recAddr, sizeof(recAddr));
            if(status < 0)
                throw string("Send failed");
        }
    }
    
    void send(string message) {
        string id = to_hex_string(i++);
        not_received.insert(id);
        thread(sendThread, id, message);
    }

    string receive(){
        socklen_t addr_size = sizeof(clAddr);
        recvfrom(fd, buffer, BUFFERSIZE, 0, (struct sockaddr*)&clAddr, &addr_size);
        not_received.erase(string(buffer).substr(5));
    }
};

int main(){
    int port;
    string ip;
    cin >> port >> ip;
    try{
        client c = client(port, ip);
        while(1){
            c.send("oi");
            cout << c.receive() << endl;
        }
        
    }catch(string s){
        cout << s << endl;
    }
}
