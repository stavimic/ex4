
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
#define MAX_QUEUED 10 // max number of QUEUED clients
#define FAIL_CODE (-1)
#define NUM_OF_ARGS 4  // required num of args
char * auth = const_cast<char *>("$auth_success");
char * duplicate = const_cast<char *>("$dup");
char * shut_down_command = const_cast<char *>("$exit");
struct clientContext
{
    char *name_buffer;
    char *msg_buffer;
    command_type commandT;
    std::string *input_name;
    std::string *msg;
    std::vector<std::string> *recipients;
    char* client_name;
};

// ======================================================================= //


/**
 * Remove "/n" from message
 * @param message
 * @return
 */
std::string trim_message(std::string&  message)
{
    std::string trimmed = std::string(message);
    long len = trimmed.length();
    std::string sub = trimmed.substr(len - 1);

    if( (len >= 1) & (sub == "\n"))
    {
        trimmed = trimmed.substr(0, len - 1);
    }
    return trimmed;
}
/**
 * This function release all the memory allocated by the client
 * @param context
 * @return
 */
int free_resources(clientContext* context)
{
    context->recipients->clear();
    delete context->recipients;
    delete context->input_name;
    delete context->msg;
    delete context->name_buffer;
    delete context->msg_buffer;
    return EXIT_SUCCESS;
}

/**
 * This function connect the client to the server
 * @param context
 * @param hostname server host
 * @param portnum port to connect
 * @return
 */
int call_socket(clientContext* context, const char *hostname,  int portnum)
{
    struct sockaddr_in sa;
    struct hostent *hp;
    int server_socket;
    if ((hp= gethostbyname (hostname)) == nullptr)
    {
        system_call_error("gethostbyname");
        exit(EXIT_FAILURE);

    }
    memset(&sa,0,sizeof(sa));
    memcpy((char *)&sa.sin_addr , hp->h_addr , hp->h_length);

    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);

    if ((server_socket = socket(hp->h_addrtype, SOCK_STREAM,0)) < 0)
    {
        system_call_error("socket");
        exit(EXIT_FAILURE);

    }
    if (connect(server_socket, (struct sockaddr *)&sa , sizeof(sa)) < 0)
    {
        close(server_socket);
        system_call_error("connect");
        exit(EXIT_FAILURE);

    }

    /// send the name of the client in oreder to register in server
    send(server_socket, context->name_buffer, WA_MAX_NAME, 0);
    bzero(context->name_buffer, WA_MAX_NAME);

    /// receive authentication message
    recv(server_socket, context->name_buffer, WA_MAX_NAME, 0);

    ///case success
    if (strcmp(context->name_buffer, auth) == 0){
        print_connection();
    }
        /// case duplicate connection
    else if(strcmp(context->name_buffer, duplicate) == 0)
    {
        print_dup_connection();
        exit(EXIT_FAILURE);
    }
        ///case failed connection
    else
    {
        print_fail_connection();
        exit(EXIT_FAILURE);
    }
    return server_socket;
}

/// Verify client isn't sending to himself
/// \param context
/// \return
int verify_send(clientContext* context)
{
    if(strcmp(context->input_name->c_str(), context->client_name) == 0)
    {
        print_send(false, false, context->client_name, trim_message(*(context->input_name)), " ");
        return FAIL_CODE;
    }
    return EXIT_SUCCESS;
}

/**
 * verify group creation - name as alnum, and enough uniqe members
 * @param context
 * @return
 */
int verify_create_group(clientContext* context)
{
    int i = 0;

    while((*(context->input_name))[i])
    {
        if (! std::isalnum((*(context->input_name))[i]))
        {
            print_create_group(false, false, "",trim_message(*(context->input_name)));
            return FAIL_CODE;
        }
        i++;
    }
    (context->recipients)->push_back(std::string(context->client_name));
    std::sort(context->recipients->begin(), context->recipients->end());
    auto uniqCnt = std::unique(context->recipients->begin(), context->recipients->end()) - context->recipients->begin();

    if(uniqCnt < 2)
    {
        print_create_group(false, false, "",trim_message(*(context->input_name)));
        return FAIL_CODE;
    }
    return EXIT_SUCCESS;

}

/**
 * main function to handle input from user
 * @param context
 * @param fd origin fd
 * @param dest  destination fd
 * @return
 */
int verify_input(clientContext* context, int fd, int dest)
{
    bzero(context->msg_buffer, WA_MAX_INPUT);
    /// read input
    read(fd, context->msg_buffer, WA_MAX_INPUT);

    /// parse command
    parse_command(context->msg_buffer, context->commandT,
                  *(context->input_name), *(context->msg), *(context->recipients));

    if (context->commandT == INVALID)
    {
        print_invalid_input();
        return FAIL_CODE;
    }
    switch (context->commandT)
    {
        case SEND:
            if(verify_send(context) == FAIL_CODE)
            {
                return FAIL_CODE;
            }
            break;
        case CREATE_GROUP:
            if(verify_create_group(context) == FAIL_CODE)
            {
                return FAIL_CODE;
            }
            break;
        case WHO:
        {
            break;
        }
        case EXIT:
        {
            break;
        }
        default:
        {
            break;
        }
    }

    // send to server desired message
    ssize_t ans = send(dest, context->msg_buffer, WA_MAX_INPUT, 0);

    if (ans == FAIL_CODE)
    {
        system_call_error("send");
        free_resources(context);
        exit(EXIT_FAILURE);
    }

    /// receive authentication message
    if(recv(dest, context->name_buffer, WA_MAX_NAME, 0) == FAIL_CODE)
    {
        system_call_error("recv");
        free_resources(context);
        exit(EXIT_FAILURE);

    }

    /// print messages to client
    switch (context->commandT)
    {
        case SEND:
        {
            print_send(false, strcmp(context->name_buffer, auth) == 0, context->client_name,
                       context->client_name, context->client_name);
            break;
        }
        case CREATE_GROUP:
        {
            print_create_group(false,  strcmp(context->name_buffer, auth) == 0, context->client_name,
                               *context->input_name);
            break;
        }
        case WHO:
        {
            recv(dest, context->msg_buffer, WA_MAX_INPUT, 0);
            std::string s = "create_group GGG " + std::string(context->msg_buffer);
            parse_command(s, context->commandT,
                          *(context->input_name), *(context->msg), *(context->recipients));
            print_who_client(strcmp(context->name_buffer, auth) == 0, *context->recipients);
            break;
        }
        case EXIT:
        {
            print_exit(strcmp(context->name_buffer, auth) == 0, context->client_name);
            free_resources(context);
            exit(0);
        }
        default:
            break;
    }

    return EXIT_SUCCESS;
}

/**
 * Check whether client name is legal or not
 * @param name
 * @return
 */
bool is_client_name_legal(char* name)
{
    int i = 0;
    std::string to_check = std::string(name);
    std::string cur_msg = trim_message(to_check);
    while(cur_msg[i])
    {
        if (!std::isalnum(cur_msg[i]) and (cur_msg[i] != ' '))
        {
            return false;
        }
        i++;
    }
    return true;
}

/**
 * main function to run process
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char** argv)
{
    if (argc != NUM_OF_ARGS)
    {
        print_client_usage();
        exit(0);
    }

    int server;
    char *client_name = argv[1];
    const char *host_name = argv[2];
    int port_num = atoi(argv[3]);

    if(!is_client_name_legal(client_name))
    {
        print_invalid_input();
        exit(1);
    }

    clientContext context;
    command_type T = INVALID;
    std::string* name = new std::string;
    std::string* message = new std::string;
    std::vector<std::string>* recipients = new std::vector<std::string>;

    // define context
    context =
    {
            new char[WA_MAX_NAME],
            new char[WA_MAX_INPUT],
            T,
            name,
            message,
            recipients,
            client_name
    };

    // copy client name to buffer
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
            system_call_error("select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            verify_input(&context, STDIN_FILENO, server);
        }
        //will check this client if it’s in readfds, if so- receive msg from server :
        if (FD_ISSET(server, &readfds))
        {
            bzero(context.msg_buffer, WA_MAX_INPUT);
            recv(server, context.msg_buffer, WA_MAX_INPUT, 0);
            if(strcmp(shut_down_command, context.msg_buffer) == 0){
                free_resources(&context);
                exit(EXIT_FAILURE);
            }
            std::cout<<context.msg_buffer;  // Print the given message
            bzero(context.msg_buffer, WA_MAX_INPUT);
        }
    }

}
