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

using namespace std;

#define BUFFERSIZE 2048

string to_hex_string( const unsigned int i ) {
    std::stringstream s;
    s << "0x" << setfill('0') << setw(2) << hex << i;
    return s.str();
}

class Chat{
    char buffer[BUFFERSIZE];
    int send_s, receive_s; //socket for send and receive
    int local_port, remote_port; // receive and send ports
    struct sockaddr_in localAddr, remoteAddr, fromAddr;
    int count;
    bool on;
    socklen_t addr_size;
    mutex qMu;
    string last_confirmed;
    queue<pair<string, bool>> queued_msgs; // true for messages false for confirmations

    public:
    Chat(int local_port, int remote_port, string ip) {
        count = 0;
        on = true;
        addr_size = sizeof(fromAddr);

        this->local_port = local_port;
        this->remote_port = remote_port;
        if((send_s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
            throw string("Socket creation failed");

        memset(&localAddr, 0, sizeof(localAddr));
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = htons(0);
        localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        if(bind(send_s, (struct sockaddr*) &localAddr, sizeof(localAddr)) < 0)
            throw string("Bind failed");

        memset(&remoteAddr, 0, sizeof(remoteAddr));
        remoteAddr.sin_family = AF_INET;
        remoteAddr.sin_port = htons(remote_port);

        if(inet_pton(AF_INET, ip.c_str(), &remoteAddr.sin_addr) <= 0)
            throw "Invalid Adress";
    }

    void server_thread(){
        while(on){
            recvfrom(receive_s, buffer, BUFFERSIZE, 0, (struct sockaddr*)&fromAddr, &addr_size);
            string msg(buffer);
            string pkt = msg.substr(0, 4);
            msg = msg.substr(5);
            
            if(pkt.compare(to_hex_string(0)) == 0) //confirmation message, update last packet received
                last_confirmed = msg;
            else {
                cout << "Menssagem:: " << msg << endl;
                unique_lock<mutex> locker(qMu);
                queued_msgs.push({to_hex_string(0) + pkt, 0});
                locker.unlock();
            }

        }
    }

    void client_thread(){
        while(on) if(queued_msgs.size()) {
                unique_lock<mutex> locker(qMu);
                string msg = queued_msgs.front().first;
                bool type = queued_msgs.front().second; queued_msgs.pop();
                locker.unlock();

                if(type){ //if its a message, not a confirmation
                    string pkt = to_hex_string(++count);
                    string msg = pkt + msg;
                    while(last_confirmed.compare(pkt) != 0)
                        sendto(send_s, msg.c_str(), msg.size() + 1, 0,
                                (struct sockaddr*) &remoteAddr, sizeof(remoteAddr));
                } else { //its is a confirmation
                    string pkt = to_hex_string(0);
                    string msg = pkt + msg;
                    sendto(send_s, msg.c_str(), msg.size() + 1, 0,
                            (struct sockaddr*) &remoteAddr, sizeof(remoteAddr));
                }
            }
    }

    void send(string msg) {
        lock_guard<mutex> locker(qMu);
        queued_msgs.push({msg, true});
    }
    
    void turn_off() { on = false; }
};

int main(){
    try{
        Chat chat(2222, 2223, "127.0.0.1");
        chat.send("oi");
    }catch(string s){
        cout << s << endl;
    }
}
