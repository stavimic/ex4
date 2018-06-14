
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
#include <algorithm>

// =======================  Macros & Globals  ============================= //
#define MAX_QUEUED 10
#define FAIL_CODE (-1)

char * auth = const_cast<char *>("auth_success");
char * command_fail = const_cast<char *>("command_fail");

struct clientContext{
    char *name_buffer;
    char *msg_buffer;
    command_type commandT;
    std::string *input_name;
    std::string *msg;
    std::vector<std::string> *recipients;
    char* client_name;
};

// ======================================================================= //


int call_socket(clientContext* context, const char *hostname,  int portnum)
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

    write(server_socket, context->name_buffer, WA_MAX_NAME);
    bzero(context->name_buffer, WA_MAX_NAME);
    read(server_socket, context->name_buffer, WA_MAX_NAME);
//    std::cout << context->name_buffer << std::endl;
    if (strcmp(context->name_buffer, auth) == 0){
        print_connection();
    }
    else{
        //print duplicate name
        print_fail_connection();
    }
    return server_socket;
}

//int auth_func(int server)
//{
//    read(server, name_buffer, WA_MAX_NAME);
//    std::cout << name_buffer << std::endl;
//    if (strcmp(name_buffer, auth) == 0){
//        print_connection();
//    }
//    else{
//        //print duplicate name
//        print_fail_connection();
//    }
//
//}

int verify_send(clientContext* context)
{
    int i = 0;
    while((*(context->input_name))[i])
    {
        if (! std::isalnum((*(context->input_name))[i]))
        {
            print_create_group(false, false, "",*(context->input_name));
            return FAIL_CODE;
        }
        i++;
    }

    if(strcmp(context->input_name->c_str(), context->client_name) == 0) // Verify client isn't sending to himself
    {
        print_send(false, false, context->client_name, *(context->input_name), " ");
        return FAIL_CODE;
    }
    return EXIT_SUCCESS;
}
int verify_create_group(clientContext* context)
{
    int i = 0;
//    if(context->recipients->size() < 1)
//    {
//        print_create_group(false, false, "",*(context->input_name));
//        return FAIL_CODE;
//    }
    while((*(context->input_name))[i])
    {
        if (! std::isalnum((*(context->input_name))[i]))
        {
            print_create_group(false, false, "",*(context->input_name));
            return FAIL_CODE;
        }
        i++;
    }

    (context->recipients)->push_back(std::string(context->client_name));
    std::sort(context->recipients->begin(), context->recipients->end());
    auto uniqCnt = std::unique(context->recipients->begin(), context->recipients->end()) - context->recipients->begin();
    for(auto elem: *context->recipients){
        std::cout << elem << std::endl;
    }
    if(uniqCnt < 2)
    {
        print_create_group(false, false, "",*(context->input_name));
        return FAIL_CODE;
    }
    return EXIT_SUCCESS;

}

int verify_who(clientContext* context){return 0;}

int verify_exit(clientContext* context){return 0;}

int verify_input(clientContext* context, int fd, int dest){
    bzero(context->msg_buffer, WA_MAX_MESSAGE);
    read(fd, context->msg_buffer, WA_MAX_MESSAGE);

    parse_command(context->msg_buffer, context->commandT,
                  *(context->input_name), *(context->msg), *(context->recipients));

    if (context->commandT == INVALID)
    {
        print_invalid_input();
    }
    switch (context->commandT){
        case SEND:
            if(verify_send(context) == FAIL_CODE){
                return FAIL_CODE;
            }
            break;
        case CREATE_GROUP:
            if(verify_create_group(context) == FAIL_CODE){
                return FAIL_CODE;
            }
            break;
        case WHO:
            break;
        case EXIT:
            break;
        default:
            break;
    }


    ssize_t ans = send(dest, context->msg_buffer, WA_MAX_MESSAGE, 0);

    if (ans == FAIL_CODE)
    {
        system_call_error("send");
        exit(1);
    }

    if(recv(dest, context->name_buffer, WA_MAX_NAME, 0) == FAIL_CODE)
    {
        system_call_error("recv");
        exit(1);
    }

    switch (context->commandT) {
        case SEND:
            print_send(false, strcmp(context->name_buffer, auth) == 0, context->client_name, context->client_name, context->client_name);
            break;
        case CREATE_GROUP:
            print_create_group(false,  strcmp(context->name_buffer, auth) == 0, context->client_name, *context->input_name);
        case WHO:
            break;
        case EXIT:
            break;
        default:
            break;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char** argv)
{
    int server;
    char *client_name = argv[1];
    const char *host_name = argv[2];
    int port_num = atoi(argv[3]);

    clientContext context;
    command_type T = INVALID;
    std::string* name = new std::string;
    std::string* message = new std::string;
    std::vector<std::string>* recipients = new std::vector<std::string>;

    context = {
            new char[WA_MAX_NAME],
            new char[WA_MAX_MESSAGE],
            T,
            name,
            message,
            recipients,
            client_name
    };

    strcpy(context.name_buffer, client_name);
    server = call_socket(&context, host_name, port_num);
    bzero(context.name_buffer, WA_MAX_NAME);

    fd_set clientsfds;
    fd_set readfds;
    FD_ZERO(&clientsfds);
    FD_SET(server, &clientsfds);
    FD_SET(STDIN_FILENO, &clientsfds);

    while (true)
    {
        readfds = clientsfds;
        if (select(MAX_QUEUED + 1, &readfds, nullptr, nullptr, nullptr) < 0)
        {
//            terminateServer();
            return FAIL_CODE;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            verify_input(&context, STDIN_FILENO, server);

//            auth_func(server);
            // todo: Check if message is valid and if not print ERROR -----
        }
        //will check this client if it’s in readfds, if so- receive msg from server :
        if (FD_ISSET(server, &readfds))
        {
            bzero(context.msg_buffer, WA_MAX_MESSAGE);
            read(server, context.msg_buffer, WA_MAX_MESSAGE);
            // todo Check if message is valid -----
            std::cout<<context.msg_buffer;  // Print the given message
        }
    }

}
