//
// Created by hareld10 on 6/10/18.
//

#include "whatsappClient.h"
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
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

int main(int argc, char** argv)
{
    while (true){
        std::string str;
        getline(std::cin, str);
        std::vector<std::string> splitted = split(str, ' ');
        if (splitted[0] == ("whatsappClient"))
        {
//            std::cout << boost::lexical_cast<unsigned short>(splitted[1])<<std::endl ;
            std::string name = splitted[1];
            std::string host_name = splitted[2];
            int s;
            unsigned short port_num = boost::lexical_cast<unsigned short>(splitted[3]);
            s = whatsappClient::call_socket(host_name.c_str(), port_num);
        }
        break;
    }

}

int whatsappClient::call_socket(const char *hostname, unsigned short portnum) {
    struct sockaddr_in sa;
    struct hostent *hp;
    int s;
    if ((hp= gethostbyname (hostname)) == NULL) {
        return(-1);
    }
    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr , hp->h_addr ,
           hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);
    if ((s = socket(hp->h_addrtype, SOCK_STREAM,0))
        < 0) {
        return(-1);
    }
    if (connect(s, (struct sockaddr *)&sa , sizeof(sa)) < 0) {
        close(s);
        return(-1);
    }
    return(s);
}
