
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

char* buffer;
char* length_buffer;
// ======================================================================= //




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
    int s;
    if ((hp= gethostbyname (hostname)) == nullptr)
    {
        return FAIL_CODE;
    }
    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr , hp->h_addr , hp->h_length);

    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);

    if ((s = socket(hp->h_addrtype, SOCK_STREAM,0)) < 0)
    {
        std::cout << "Problem Socket" << std::endl;
        return FAIL_CODE;
    }
    if (connect(s, (struct sockaddr *)&sa , sizeof(sa)) < 0)
    {
        close(s);
        std::cout<<"closing, connect didn't succeed in the client side"<<std::endl;
        return FAIL_CODE;
    }
    print_connection();
    write(s, length_buffer, WA_MAX_NAME);

    write(s, buffer, WA_MAX_NAME);
    return s;
}

int main(int argc, char** argv)
{
    int connecting_socket;

    buffer = new char[WA_MAX_MESSAGE];
    length_buffer = new char[5];
    char *client_name = argv[1];
    const char *host_name = argv[2];
    int port_num = atoi(argv[3]);


    buffer = client_name;
    length_buffer[0] = strlen(client_name);
    std::cout<<length_buffer[0]<<"len\n";

    connecting_socket = call_socket(host_name, port_num);
//    std::cout << "S is after call "<< connecting_socket << std::endl;
    bzero(buffer, WA_MAX_NAME);

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
