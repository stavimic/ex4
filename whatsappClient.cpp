
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include "whatsappio.h"
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <sys/select.h>

// =======================  Macros & Globals  ============================= //
#define MAX_QUEUED 10
#define FAIL_CODE (-1)

char* name_buffer;
char* msg_buffer;

// ======================================================================= //



char * auth = const_cast<char *>("auth_success");
template<typename Out>
void split2(const std::string &s, char delim, Out result)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        *(result++) = item;
    }
}

std::vector<std::string> split2(std::string &s, char delim)
{
    std::vector<std::string> elems;
    split2(s, delim, std::back_inserter(elems));
    return elems;
}

int call_socket(const char *hostname,  int portnum)
{
    struct sockaddr_in sa;
    struct hostent *hp;
    int server_socket;
    if ((hp= gethostbyname (hostname)) == nullptr)
    {
        return FAIL_CODE;
    }
    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr , hp->h_addr , hp->h_length);

    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);

    if ((server_socket = socket(hp->h_addrtype, SOCK_STREAM,0)) < 0)
    {
        std::cout << "Problem Socket" << std::endl;
        return FAIL_CODE;
    }
    if (connect(server_socket, (struct sockaddr *)&sa , sizeof(sa)) < 0)
    {
        close(server_socket);
        std::cout<<"closing, connect didn't succeed in the client side"<<std::endl;
        return FAIL_CODE;
    }

    write(server_socket, name_buffer, WA_MAX_NAME);
    bzero(name_buffer, WA_MAX_NAME);
    read(server_socket, name_buffer, WA_MAX_NAME);
    std::cout << name_buffer << std::endl;
    if (strcmp(name_buffer, auth) == 0){
        print_connection();
    }
    else{
        //print duplicate name
        print_fail_connection();
    }
    std::cout<< "S is: " << server_socket << std::endl;
    return server_socket;
}


//int read_message(int fd)



int main(int argc, char** argv)
{
    int connecting_socket;

    name_buffer = new char[WA_MAX_NAME];
    msg_buffer = new char[WA_MAX_MESSAGE];
    char *client_name = argv[1];
    const char *host_name = argv[2];
    int port_num = atoi(argv[3]);


    name_buffer = client_name;

    connecting_socket = call_socket(host_name, port_num);
    bzero(name_buffer, WA_MAX_NAME);

    fd_set clientsfds;
    fd_set readfds;
    FD_ZERO(&clientsfds);
    FD_SET(connecting_socket, &clientsfds);
    FD_SET(STDIN_FILENO, &clientsfds);

    while (true)
    {
        readfds = clientsfds;
        if (select(MAX_QUEUED + 1, &readfds, nullptr, nullptr, nullptr) < 0)
        {
//            terminateServer();
            return FAIL_CODE;
        }
//        std::cout << "In Select Client" << std::endl;

        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            bzero(name_buffer, WA_MAX_MESSAGE);
            read(STDIN_FILENO, msg_buffer, WA_MAX_MESSAGE);
            // todo Check if message is valid -----
            std::cout<<msg_buffer<<std::endl;  // Print the given message
            
        }

        //will check this client if it’s in readfds, if so- receive msg from server :
        if (FD_ISSET(connecting_socket, &readfds))
        {
            std::cout << "in else" << std::endl;
            bzero(name_buffer, WA_MAX_MESSAGE);
            read(connecting_socket, msg_buffer, WA_MAX_MESSAGE);
            // todo Check if message is valid -----
            std::cout<<msg_buffer<<std::endl;  // Print the given message
        }

    }

}
