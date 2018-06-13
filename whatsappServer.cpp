
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


// ======================================================================= //

char * auth = const_cast<char *>("auth_success");

struct Client{
    std::string name;
    int client_socket;
};

struct Group{
    std::string group_name;
    std::vector<Client*>* members;
};

struct serverContext{
    char *name_buffer;
    char *msg_buffer;
    std::vector<Client*>* server_members;
    std::vector<Group*>* server_groups;

    command_type commandT;
    std::string *name;
    std::string *msg;
    std::vector<std::string> *recipients;
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

int connectNewClient(serverContext* context, int fd)
{

    bzero(context->name_buffer, WA_MAX_NAME);
    read_data(fd, context->name_buffer, WA_MAX_NAME);
    // check for duplicate
    std::string name = std::string(context->name_buffer);
    Client* new_client = new Client();
    *new_client = {name, fd};
    (context->server_members)->push_back(new_client);

    print_message(name, "Connected");
    write(fd, auth, WA_MAX_NAME);

}

Client* get_client_by_fd(serverContext* context, int fd)
{
    for(auto client: *((*context).server_members))
    {
        if(client->client_socket == fd)
        {
            return client;
        }
    }
    return nullptr;
}

Client* get_client_by_name(serverContext* context, std::string& name)
{
    for(auto client: *((*context).server_members))
    {
        if(client->name == name)
        {
            return client;
        }
    }
    return nullptr;
}

int getFdByName(serverContext* context, std::string& name){
    for(auto &client: *(context->server_members)){
        if(client->name == name)
        {
            return client->client_socket;
        }
    }
    return FAIL_CODE;
}

void send_msg(serverContext* context, int fd,  std::string& msg, int origin_fd)
{

//    bzero(context->msg_buffer, WA_MAX_MESSAGE);
    context->msg_buffer = const_cast<char *>(msg.c_str());
    Client* origin_client = get_client_by_fd(context, origin_fd); // todo check if nullptr
    std::string final_msg = origin_client->name + ": " + msg;
    send(fd, final_msg.c_str(), WA_MAX_MESSAGE, 0);
}


int does_name_exist(serverContext* context, std::string& name)
{
    if (getFdByName(context, name) != FAIL_CODE)
    {
        return true;
    }
    for(auto &group: *(context->server_groups)){
        if(group->group_name == name)
        {
            return true;
        }
    }
    return false;
}


int handel_group_creation(serverContext* context, int origin_fd)
{
    // Make sure the given name doesn't already exist:
    if (does_name_exist(context,  *(context->name)))
    {
        return FAIL_CODE;
    }

    // Check the name is legal todo move to the client side:
    int i = 0;
    while((*(context->name))[i])
    {
        if (! std::isalnum((*(context->name))[i]))
        {
            return FAIL_CODE;
        }
    }

    // Check that every client exists in the sever:
    for(std::string& name: *(context->recipients))
    {
        if (get_client_by_name(context, name) == nullptr)
        {
            return FAIL_CODE;
        }
    }

    std::vector<Client*>* group_members = new std::vector<Client*>();
    Client* admin = get_client_by_fd(context, origin_fd);
    group_members->push_back(admin);  // Add the admin to the group

    for(std::string& name: *(context->recipients))
    {
        Client* cur_client = get_client_by_name(context, name);
        for(auto& member: *group_members)
        {
            if (cur_client->name == name)
            {
                break;
            }
        }
        // If the client isn't already in the group, add it:
        group_members->push_back(cur_client);
    }
    Group* new_group = new Group();
    *new_group = {(*(context->name), group_members};
    (context->server_groups)->push_back(new_group);
    return EXIT_SUCCESS;
}


int handleClientRequest(serverContext* context, int fd)
{
    bzero(context->msg_buffer, WA_MAX_MESSAGE);
    read_data(fd, context->msg_buffer, WA_MAX_MESSAGE);
    parse_command(context->msg_buffer, context->commandT,
                  *(context->name), *(context->msg), *(context->recipients));

    if((context->commandT) == INVALID){
        // update client
    }

    if(context->commandT == SEND){
        int dest_fd = getFdByName(context, *(context->name));
        // if not -1
        send_msg(context, dest_fd, *(context->msg), fd);
    }

    else if (context->commandT == CREATE_GROUP)
    {
        // todo check if succeeded
        handel_group_creation(context, fd);
    }
}



int select_flow(int connection_socket)
{
    serverContext context;
    command_type T;
    std::string* name = new std::string;
    std::string* message = new std::string;
    std::vector<std::string>* recipients = new std::vector<std::string>;

    context = {
        new char[WA_MAX_NAME],
        new char[WA_MAX_MESSAGE],
        new std::vector<Client*>(),
        new std::vector<Group*>(),
        T,
        name,
        message,
        recipients
    };

    fd_set clientsfds;
    fd_set readfds;
    FD_ZERO(&clientsfds);
    FD_SET(connection_socket, &clientsfds);
    FD_SET(STDIN_FILENO, &clientsfds);
    int file_descriptor;
    while (true)
    {
        readfds = clientsfds;

//        std::cout << "before select" << std::endl;

        if (select(MAX_QUEUD + 1, &readfds, nullptr, nullptr, nullptr) < 0)
        {
//            terminateServer();
            return FAIL_CODE;
        }

//        std::cout << "after select" << std::endl;


        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
//            serverStdInput();
        }

        if (FD_ISSET(connection_socket, &readfds)) {
            //will also add the client to the clientsfds

            if((file_descriptor = accept(connection_socket, nullptr, nullptr)) < 0)
            {

                std::cout << "accept_fail" << std::endl;
                return EXIT_FAILURE;
            }
            FD_SET(file_descriptor, &clientsfds);
            connectNewClient(&context, file_descriptor);
        }

        else
        {
//            std::cout << "in else" << std::endl;
            //will check each client if it’s in readfds
            //and then receive a message from him
            for(const auto client: *(context.server_members)){
                if(FD_ISSET((*client).client_socket, &readfds)){
                    handleClientRequest(&context, client->client_socket);
                }
            }
        }
        bzero(context.name_buffer, WA_MAX_NAME);
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


