#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "InventoryManager.hpp"


// Read one line until '\n'
bool recv_line(int fd, std::string& out) {
    out.clear();
    char c;
    while (true) {
        ssize_t n = recv(fd, &c, 1, 0);
        if (n <= 0) return false;
        if (c == '\n') break;
        out.push_back(c);
    }
    return true;
}

// Send the entire message
void send_all(int fd, const std::string& msg) {
    size_t sent = 0;
    while (sent < msg.size()) {
        ssize_t n = send(fd, msg.data() + sent, msg.size() - sent, 0);
        if (n <= 0) return;
        sent += n;
    }
}
bool is_number(const std::string& s) {
    for (char c : s) {
        if (!isdigit(c)) return false;
    }
    return true;
}

void handle_client(int client_fd, InventoryManager& inventory) {
    bool is_authenticated=false; 
    std::string username;   
    
    while(true){
        bool check_username=true;
        std::string line;
        if(!recv_line(client_fd, line)) break;

        std::string command;
        std::string arg;
        size_t space_pos=line.find(' ');

        if(space_pos!=std::string::npos){
            command=line.substr(0,space_pos);
            arg=line.substr(space_pos+1);

        }else{
            command=line;
        }

        if (command=="HELLO"){
                username=arg;
                if(username.empty()){
                    send_all(client_fd, "ERR PROTOCOL missing_username\n");
                    check_username=false;
                    continue;
                }
                
                for (char c : username){
                    if(!isalpha(c)){
                        is_authenticated=false;
                        send_all(client_fd, "ERR PROTOCOL invalid_username\n");
                        check_username=false;
                        break;
                       
                    }
                }
                if(check_username){
                    is_authenticated=true;
                    send_all(client_fd, "OK HELLO\n");

                }
                
                    
           
                }  
                else if(!is_authenticated){
                    send_all(client_fd, "ERR STATE not_authenticated\n");
                    continue;
            }

                

                else if(command=="LIST"){
                    std::string response=inventory.listItems();
                    send_all(client_fd,response+"\n");
            
                }
                else if(command=="BORROW"){
                    int itemID;
                    try{
                        
                        itemID=std::stoi(arg);
                        inventory.borrowItem(itemID,username);
                        send_all(client_fd, "OK BORROW "+std::to_string(itemID)+"\n");
                    }
                    catch(const std::invalid_argument&){
                        send_all(client_fd, "ERR PROTOCOL invalid_id\n");
                    }
                //  catch if item is not available
                    catch(const std::runtime_error& e){
                        send_all(client_fd, e.what());
                    }
                        
                    
                    
                }
                else if(command=="RETURN"){
                    int itemID;
                    try{
                        itemID=std::stoi(arg);
                        inventory.returnItem(itemID,username);
                        send_all(client_fd, "OK RETURN "+std::to_string(itemID)+"\n");
                    }

                    catch(const std::invalid_argument&){
                        send_all(client_fd, "ERR PROTOCOL invalid_id\n");
                    }
                    catch(const std::runtime_error& e){
                        send_all(client_fd, e.what());
                    }
                }
                else if(command=="WAIT"){
                    int itemID;
                    try{
                        itemID=std::stoi(arg);
                        inventory.waitUntilAvailable(itemID,username);
                        send_all(client_fd, "OK WAIT "+std::to_string(itemID)+"\n");
                    }
                    catch(const std::invalid_argument&){
                        send_all(client_fd, "ERR PROTOCOL invalid_id\n");
                    }
                    catch(const std::runtime_error& e){
                        send_all(client_fd, e.what());
                    }
                }
                // Stops the connection between server and client
                else if(command=="QUIT"){
                    send_all(client_fd, "OK QUIT\n");
                    break;
                }
                
            } 
            close(client_fd);
            }
            
       

        


int main(int argc, char *argv){
    InventoryManager items;
   
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(5555);

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    std::cout << "Server listening on port 5555\n";

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        std::thread(handle_client, client_fd, std::ref(items)).detach();
    }
    close(server_fd);
    return 0;
    
    
} ;  
