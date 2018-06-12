
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include "whatsappio.h"
#include <string>
#include <sstream>

#include <vector>
#include <iterator>
#include <iostream>

// =======================  Macros & Globals  ============================= //
#define MAX_QUEUD 10
#define FAIL_CODE (-1)

char *buff;

// ======================================================================= //

char * auth = const_cast<char *>("auth_success");

struct Client{
    std::string name;
    int client_socket;
};

struct Group{
    std::string group_name;
    std::vector<Client*> members;
};

template<typename Out>
void split(const std::string &s, char delim, Out result)
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        *(result++) = item;
    }
}

std::vector<std::string> split(std::string &s, char delim)
{
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

int read_data(int s, char *buf, int n)
{
    int bcount; /* counts bytes read */
    int br; /* bytes read this pass */
    bcount= 0; br= 0;
    while (bcount < n)
    { /* loop until full buffer */
//        std::cout << buf << std::endl;
        br = read(s, buf, n-bcount);
        if ((br > 0))
        {
            bcount += br;
            buf += br;
        }
        if (br < 0)
        {
            return FAIL_CODE;
        }
    }
    return bcount;
}

int establish(unsigned short portnum)
{
    char myname[WA_MAX_NAME + 1];
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;  //Official name of the host

    //hostnet initialization
    gethostname(myname, WA_MAX_NAME);
    hp = gethostbyname(myname);
    if (hp == nullptr)
        return FAIL_CODE;

    //sockaddrr_in initlization
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;

    /* this is our host address */
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);

    /* this is our port number */
    sa.sin_port= htons(portnum);

    /* create socket */
    if ((s= socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return FAIL_CODE;
    if (bind(s, (struct sockaddr *)&sa , sizeof(struct sockaddr_in)) < 0)
    {
        close(s);
        std::cout<<"closing- bind did't succeed"<<std::endl;

        return FAIL_CODE;
    }
    listen(s, MAX_QUEUD); /* max # of queued connects */
    return s;
}

int connectNewClient(std::vector<Client>* server_members, int fd){

    bzero(buff, WA_MAX_NAME);
    read_data(fd, buff, 30);
    // check for duplicate
    std::string name = std::string(buff);
    server_members->push_back(
            {
                    name,
                    fd
            });
    print_message(name, "Connected");
    write(fd, auth, WA_MAX_NAME);

}


int select_flow(int connection_socket)
{
    std::vector<Client>* server_members= new std::vector<Client>();
    std::vector<Group*>* server_groups= new std::vector<Group*>();

    std::cout << "start Select flow" << std::endl;
    fd_set clientsfds;
    fd_set readfds;
    FD_ZERO(&clientsfds);
    FD_SET(connection_socket, &clientsfds);
    FD_SET(STDIN_FILENO, &clientsfds);
    buff = new char[WA_MAX_MESSAGE];
    int file_descriptor;
    while (true)
    {
        readfds = clientsfds;
        if (select(MAX_QUEUD+1, &readfds, nullptr, nullptr, nullptr) < 0)
        {
//            terminateServer();
            return FAIL_CODE;
        }
        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
//            serverStdInput();
        }

//        std::cout << "In Select" << std::endl;
        if (FD_ISSET(connection_socket, &readfds)) {
            //will also add the client to the clientsfds

//            std::cout << "before accept" << std::endl;
            if((file_descriptor = accept(connection_socket, nullptr, nullptr)) < 0)
            {
                std::cout << "accept_fail" << std::endl;
                return EXIT_FAILURE;
            }
            connectNewClient(server_members, file_descriptor);
        }

        else
        {
            std::cout << "in else" << std::endl;
            //will check each client if it’s in readfds
            //and then receive a message from him
            handleClientRequest();


        }
        bzero(buff, WA_MAX_NAME);
    }
}


int main(int argc, char** argv)
{
    while (true)
    {
        short port_number = atoi(argv[1]);
        std::cout << port_number << std::endl ;
        int fd = establish(port_number);
        select_flow(fd);
        break;
    }
}


