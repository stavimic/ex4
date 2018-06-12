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
#define MAX_QUEUD 10
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

int read_data(int s, char *buf, int n) {
    int bcount; /* counts bytes read */
    int br; /* bytes read this pass */
    bcount= 0; br= 0;
    while (bcount < n) { /* loop until full buffer */
        br = read(s, buf, n-bcount);
        if ((br > 0)) {
            bcount += br;
            buf += br;
        }
        if (br < 0) {
            return(-1);
        }
    }
    return(bcount);
}

int establish(unsigned short portnum) {
    char myname[WA_MAX_NAME + 1];
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;  //Official name of the host

    //hostnet initialization
    gethostname(myname, WA_MAX_NAME);
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
        std::cout<<"closing"<<std::endl;

        return(-1);
    }
    listen(s, MAX_QUEUD); /* max # of queued connects */
    return(s);
}
char *buff;

int select_flow(int socket) {
    std::cout << "start Select flow" << std::endl;
    fd_set clientsfds;
    fd_set readfds;
    FD_ZERO(&clientsfds);
    FD_SET(socket, &clientsfds);
    FD_SET(STDIN_FILENO, &clientsfds);
    buff = new char[WA_MAX_MESSAGE];
    int t;
    while (true) {
        readfds = clientsfds;
        if (select(MAX_QUEUD+1, &readfds, NULL, NULL, NULL) < 0) {
//            terminateServer();
            return -1;
        }
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
//            serverStdInput();
        }

        std::cout << "In Select" << std::endl;
        if (FD_ISSET(socket, &readfds)) {
            //will also add the client to the clientsfds

            std::cout << "before accept" << std::endl;
            if((t = accept(socket, nullptr, nullptr)) < 0){
                std::cout << "accept_fail" << std::endl;
                return EXIT_FAILURE;
            }
            std::cout << "after accept" << std::endl;
            read_data(t, buff, WA_MAX_MESSAGE);
            std::cout << "after read" << std::endl;
            print_message(buff, "Connected");
//            connectNewClient();
        }

        else {
            std::cout << "in else" << std::endl;
            //will check each client if it’s in readfds
            //and then receive a message from him
//            handleClientRequest();
        }
        bzero(buff, WA_MAX_NAME);
    }
}


int main(int argc, char** argv)
{
    char * uuu = const_cast<char *>("whatsappServer");
    while (true)
    {
        std::cout << argv[1] << std::endl;
        if (strcmp(argv[1],"whatsappServer") == 0)
        {
            std::cout << boost::lexical_cast<unsigned short>(argv[2])<<std::endl ;
            int s = establish(boost::lexical_cast<unsigned short>(argv[2]));
            select_flow(s);
        }

        break;
    }

}


