//
// Created by hareld10 on 6/10/18.
//

#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include "whatsappServer.h"
#include "whatsappio.h"
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>
#include <boost/lexical_cast.hpp>

template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> split(std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

int whatsappServer::establish(unsigned short portnum) {
    char myname[MAX_NAME+1];
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;  //Official name of the host

    //hostnet initialization
    gethostname(myname, MAX_NAME);
    hp = gethostbyname(myname);
    if (hp == NULL)
        return(-1);

    //sockaddrr_in initlization
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;

    /* this is our host address */
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);

    /* this is our port number */
    sa.sin_port= htons(portnum);

    /* create socket */
    if ((s= socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return(-1);
    if (bind(s , (struct sockaddr *)&sa , sizeof(struct sockaddr_in)) < 0) {
        close(s);
        return(-1);
    }
    listen(s, MAX_QUEUD); /* max # of queued connects */
    return(s);
}

int whatsappServer::select_flow(int socket) {
    fd_set clientsfds;
    fd_set readfds;
    FD_ZERO(&clientsfds);
    FD_SET(socket, &clientsfds);
    FD_SET(STDIN_FILENO, &clientsfds);
    while (true) {
        readfds = clientsfds;
        if (select(MAX_QUEUD+1, &readfds, NULL, NULL, NULL) < 0) {
//            terminateServer();
            return -1;
        }
        if (FD_ISSET(socket, &readfds)) {
            //will also add the client to the clientsfds
//            connectNewClient();
        }
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
//            serverStdInput();
        }
        else {
            
            //will check each client if it’s in readfds
            //and then receive a message from him
//            handleClientRequest();
        }
    }
}

int main(int argc, char** argv)
{
    char * uuu = const_cast<char *>("whatsappServer");
    while (true){
//        std::string str;
//        getline(std::cin, str);
//        std::vector<std::string> splitted = split(str, ' ');

        std::cout << argv[1] << std::endl;
        if (strcmp(argv[1],"whatsappServer") == 0)
        {
            std::cout << boost::lexical_cast<unsigned short>(argv[2])<<std::endl ;
            int s = whatsappServer::establish(boost::lexical_cast<unsigned short>(argv[2]));
            std::cout << s;
            whatsappServer::select_flow(s);
        }

        break;
    }

}


