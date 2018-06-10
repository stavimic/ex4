//
// Created by hareld10 on 6/10/18.
//

#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include "whatsappio.h"
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <sys/select.h>

#define MAX_QUEUD 10

char* buffer;
template<typename Out>
void split2(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> split2(std::string &s, char delim) {
    std::vector<std::string> elems;
    split2(s, delim, std::back_inserter(elems));
    return elems;
}

int call_socket(const char *hostname, unsigned short portnum) {
    struct sockaddr_in sa;
    struct hostent *hp;
    int s;
    if ((hp= gethostbyname (hostname)) == NULL) {
        return(-1);
    }
    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr , hp->h_addr , hp->h_length);

    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);

    if ((s = socket(hp->h_addrtype, SOCK_STREAM,0)) < 0) {
        std::cout << "Problem Socket" << std::endl;
        return(-1);
    }

    std::cout << "S is before connect" << s << std::endl;
    if (connect(s, (struct sockaddr *)&sa , sizeof(sa)) < 0) {
        std::cout << "Problem Connect Client" << std::endl;
        close(s);
        std::cout<<"closing"<<std::endl;
        return(-1);
    }
    std::cout << "S is after connect" << s << std::endl;
    print_connection();
    std::cout << "Before write name" << std::endl;
    write(s, buffer, MAX_NAME);
    std::cout << "After write name" << std::endl;
    return(s);
}

int main(int argc, char** argv)
{
    int s;
    if (strcmp(argv[1],"whatsappClient") == 0)
    {
        buffer = new char[MAX_MESSAGE];
        char *client_name = argv[2];
        const char *host_name = argv[3];
        unsigned short port_num = boost::lexical_cast<unsigned short>(argv[4]);
        buffer = client_name;
        s = call_socket(host_name, port_num);
        std::cout << "S is after call s" << s << std::endl;
        bzero(buffer, MAX_NAME);
    }

    fd_set clientsfds;
    fd_set readfds;
    FD_ZERO(&clientsfds);
    FD_SET(s, &clientsfds);
    FD_SET(STDIN_FILENO, &clientsfds);

    while (true){
        readfds = clientsfds;
        if (select(MAX_QUEUD+1, &readfds, NULL, NULL, NULL) < 0) {
//            terminateServer();
            return -1;
        }
        std::cout << "In Select Client" << std::endl;

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
//            serverStdInput();
        }
        else {
            std::cout << "in else" << std::endl;
            //will check each client if it’s in readfds
            //and then receive a message from him
//            handleClientRequest();
        }
    }

}
