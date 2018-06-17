
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include "whatsappio.h"
#include <string>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <iterator>
#include <iostream>

// =======================  Macros & Globals  ============================= //

#define MAX_QUEUED 10
#define FAIL_CODE (-1)
#define PORT_INDEX 1
#define NUM_OF_ARGS 2
#define ERASE_USER_CODE 3
char * auth = const_cast<char *>("$auth_success");
char * command_fail = const_cast<char *>("$command_fail");
char * duplicate = const_cast<char *>("$dup");
char * shut_down_command = const_cast<char *>("$exit");
// ======================================================================= //

struct Client
{
    std::string name;
    int client_socket;
};

struct Group
{
    std::string group_name;
    std::vector<Client*>* members;
};

struct serverContext
{
    char *name_buffer;
    char *msg_buffer;
    std::vector<Client*>* server_members;
    std::vector<Group*>* server_groups;

    command_type commandT;
    std::string *name;
    std::string *msg;
    std::vector<std::string> *recipients;
};

void free_resources(serverContext* context)
{
    delete context-> msg_buffer;
    delete context-> name_buffer;
    context-> server_groups->clear();
    context-> server_members->clear();
    delete context-> name;
    delete context-> msg;
    delete context-> recipients;
    delete context->server_groups;
    delete context->server_members;
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
    {
        close(s);
        system_call_error("socket");
        exit(EXIT_FAILURE);
    }
    if (bind(s, (struct sockaddr *)&sa , sizeof(struct sockaddr_in)) < 0)
    {
        close(s);
        system_call_error("bind");
        exit(EXIT_FAILURE);
    }
    listen(s, MAX_QUEUED); /* max # of queued connects */
    return s;
}

Client* get_client_by_name(serverContext* context, std::string& name)
{
    for(auto client: *((*context).server_members))
    {
        long len = name.length();
        std::string sub = name.substr(len - 1);

        if( (len >= 1) & (sub == "\n"))
        {
            name = name.substr(0, len - 1);
        }
        if(client->name == name)
        {
            return client;
        }
    }
    return nullptr;
}


Group* getGroupByName(serverContext* context, std::string& name)
{
    for(auto &group: *(context->server_groups))
    {
        if(group->group_name == name)
        {
            return group;
        }
    }
    return nullptr;

}



int connectNewClient(serverContext* context, int fd)
{
    bzero(context->name_buffer, WA_MAX_NAME);
    recv(fd, context->name_buffer, WA_MAX_NAME, 0);
    // check for duplicate
    std::string name = std::string(context->name_buffer);
    if(get_client_by_name(context, name) != nullptr)  // name already exists
    {
        // Client name already exists, inform the client that the creation failed
        if(send(fd, duplicate, WA_MAX_NAME, 0) < 0 )
        {
            system_call_error("send");
            exit(EXIT_FAILURE);
        }
        return FAIL_CODE;
    }

    if (getGroupByName(context, name) != nullptr) // name already exists
    {
        // Group name already exists, inform the client that the creation failed
        if(send(fd, duplicate, WA_MAX_NAME, 0) < 0 )
        {
            system_call_error("send");
            exit(EXIT_FAILURE);
        }
        return FAIL_CODE;

    }
    Client* new_client = new Client();
    *new_client = {name, fd};
    (context->server_members)->push_back(new_client);
    print_connection_server(name);
    if(send(fd, auth, WA_MAX_NAME, 0) < 0)
    {
        system_call_error("send");
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;

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

int getFdByName(serverContext* context, std::string& name)
{
    for(auto &client: *(context->server_members)){
        if(client->name == name)
        {
            return client->client_socket;
        }
    }
    return FAIL_CODE;
}


int send_msg(serverContext* context, int fd,  std::string& msg, int origin_fd)
{
    Client* origin_client = get_client_by_fd(context, origin_fd);
    Client* dest = get_client_by_fd(context, fd);

    std::string final_msg = origin_client->name + ": " + msg;

    if(send(fd, final_msg.c_str(), WA_MAX_INPUT, 0) == FAIL_CODE)
    {
        system_call_error("send");
        exit(EXIT_FAILURE);
    }
    // Success:
    print_send(true, true, origin_client->name, dest->name, trim_message(msg));
    return EXIT_SUCCESS;
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
    if (does_name_exist(context, *(context->name)))
    {
        return FAIL_CODE;
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
    bool flag = true;
    // Check if the is already in the group:
    for(std::string& name: *(context->recipients))
    {
        Client* cur_client = get_client_by_name(context, name);
        for(auto& member: *group_members)
        {
            if (member->name == name)
            {
                flag = false;  // No need to add the client to the group
            }
        }
        // If the client isn't already in the group, add it:
        if(flag)
        {
            group_members->push_back(cur_client);
        }
        flag = true;
    }
    // Add the new group to the server's vector of groups:
    Group* new_group = new Group();
    *new_group = {*(context->name), group_members};
    (context->server_groups)->push_back(new_group);
    return EXIT_SUCCESS;
}


/**
 * Handle the request being currently sent from an existing client
 * @param context server context
 * @param fd client fd
 * @return 0 or -1 if succeeded / failed
 */
int handleClientRequest(serverContext* context, int fd)
{
    bzero(context->msg_buffer, WA_MAX_INPUT);
    recv(fd, context->msg_buffer, WA_MAX_INPUT, 0);  // Get command
    parse_command(
            context->msg_buffer,
            context->commandT,
            *(context->name),
            *(context->msg),
            *(context->recipients)
    );

    Client* sender = get_client_by_fd(context, fd);
    switch(context->commandT)
    {
        case SEND:
        {
            int dest_fd = getFdByName(context, *(context->name));
            if(dest_fd != FAIL_CODE)  // The message is for a single client
            {
                if(send_msg(context, dest_fd, *(context->msg), fd) == FAIL_CODE)
                {
                    if(send(fd, command_fail, WA_MAX_NAME, 0) < 0)  // inform client that sending failed
                    {
                        system_call_error("send");
                        exit(EXIT_FAILURE);
                    }
                    return FAIL_CODE;
                }
                if(send(fd, auth, WA_MAX_NAME, 0) < 0)  // inform client that sending succeeded
                {
                    system_call_error("send");
                    exit(EXIT_FAILURE);
                }
                return EXIT_SUCCESS;
            }

            // Check if this is a group message, and if the group exists:
            Group* curGroup = getGroupByName(context, *(context->name));
            if(curGroup != nullptr)
            {
                // Make sure the sender is part of the group:
                bool message_is_legal = false;
                for(Client* current_client: *(curGroup->members))
                {
                    if (current_client->name == sender->name) // the sending is legal
                    {
                        message_is_legal = true;
                        break;
                    }
                }
                if(!message_is_legal)
                {
                    std::string trimmed_msg = trim_message(*(context->msg));
                    print_send(true, false, sender->name, *(context->name), trimmed_msg); // message FAIL
                    if(send(fd, command_fail, WA_MAX_NAME, 0) < 0) // inform client that sending failed
                    {
                        system_call_error("send");
                        exit(EXIT_FAILURE);
                    }
                    return FAIL_CODE;
                }
                for(auto member: *(curGroup->members))
                {
                    if(member->name != sender->name)
                    {
                        std::string final_msg = sender->name + ": " + *(context->msg);
                        send(member->client_socket, final_msg.c_str(), WA_MAX_INPUT, 0);
                    }
                }
                print_send(true, true, sender->name, *(context->name), trim_message(*(context->msg))); // message success
                if(send(fd, auth, WA_MAX_NAME, 0) < 0 )  // inform client that sending succeeded
                {
                    system_call_error("send");
                    exit(EXIT_FAILURE);
                }
                return EXIT_SUCCESS;
            }
            else
            {
                std::string trimmed_msg = trim_message(*(context->msg));
                print_send(true, false, sender->name, *(context->name), trimmed_msg); // message FAIL
                // inform client that sending failed:
                if (send(fd, command_fail, WA_MAX_NAME, 0)< 0)
                {
                    system_call_error("send");
                    exit(EXIT_FAILURE);
                }
                return FAIL_CODE;
            }
        }

        case CREATE_GROUP:
        {
            if(handel_group_creation(context, fd) == FAIL_CODE)
            {
                if(send(fd, command_fail, WA_MAX_NAME, 0)< 0 )  // inform client that group failed
                {
                    system_call_error("send");
                    exit(EXIT_FAILURE);
                }
                print_create_group(true, false, sender->name,  *(context->name)); // Print fail message
            }
            else
            {
                if(send(fd, auth, WA_MAX_NAME, 0)<0)  // inform client that group succeeded
                {
                    system_call_error("send");
                    exit(EXIT_FAILURE);
                }
                print_create_group(true, true, sender->name,  *(context->name)); // Print success message
            }
            break;
        }

        case WHO:
        {
            std::string total;
            for(auto& client: *context->server_members)
            {
                std::string name = client->name;
                total += name;
                total += ",";
            }
            total = total.substr(0, total.length() - 1);
            if(send(fd, auth, WA_MAX_NAME, 0)<0)  // inform client that group succeeded
            {
                system_call_error("send");
                exit(EXIT_FAILURE);
            }
            if(send(fd, total.c_str(), WA_MAX_INPUT, 0)<0)  // send the list of clients
            {
                system_call_error("send");
                exit(EXIT_FAILURE);
            }
            print_who_server(sender->name);
            break;
        }

        case EXIT:
        {
            std::string name_to_delete = sender->name;
            // Un-register the client from the server:
            auto iter = std::begin(*context->server_members);
            while (iter != std::end(*context->server_members))
            {
                if ((*iter)->name == name_to_delete)
                {
                    iter = (*context->server_members).erase(iter);
                    break;
                }
                ++iter;
            }

            // Un-register the client from all groups:
            for(Group* group : *context->server_groups)
            {
                auto iter = std::begin(*((*group).members));
                while (iter != std::end(*((*group).members)))
                {
                    if ((*iter)->name == name_to_delete)
                    {
                        iter = group->members->erase(iter);
                        break;
                    }
                    ++iter;
                }
            }
            if(send(fd, auth, WA_MAX_NAME, 0) < 0 )  // inform client that un-registering succeeded
            {
                system_call_error("send");
                exit(EXIT_FAILURE);
            }
            print_exit(true, name_to_delete);
            return ERASE_USER_CODE;
        }
        default:
        {
            break;
        }

    }
    return EXIT_SUCCESS;
}


int serverStdInput(serverContext* context)
{
    bzero(context->msg_buffer, WA_MAX_INPUT);
    if(read(STDIN_FILENO, context->name_buffer, WA_MAX_NAME) == FAIL_CODE)  // Get command from STDIN
    {
        system_call_error("read");
        exit(EXIT_FAILURE);
    }
    auto msg = std::string(context->name_buffer);
    if(strcmp(trim_message(msg).c_str(), "EXIT"))
    {
        print_invalid_input();
        bzero(context->name_buffer, WA_MAX_NAME);
        return FAIL_CODE;
    }
    // Now tell all the clients to terminate:
    for(Client* client : *context->server_members)
    {
        if(send(client->client_socket, shut_down_command, WA_MAX_NAME, 0) == FAIL_CODE)
        {
            system_call_error("send");
            exit(EXIT_FAILURE);
        }

    }
    free_resources(context);
    print_exit();
    exit(EXIT_SUCCESS);
}


int select_flow(int connection_socket)
{
    serverContext context;
    std::string* name = new std::string;
    std::string* message = new std::string;
    std::vector<std::string>* recipients = new std::vector<std::string>;

    // Create the context for the current server:
    context =
    {
        new char[WA_MAX_NAME],
        new char[WA_MAX_INPUT],
        new std::vector<Client*>(),
        new std::vector<Group*>(),
        INVALID,
        name,
        message,
        recipients
    };

    // Initialize FD set
    fd_set clientsfds;
    fd_set readfds;
    FD_ZERO(&clientsfds);
    FD_SET(connection_socket, &clientsfds);
    FD_SET(STDIN_FILENO, &clientsfds);
    int file_descriptor;
    while (true)
    {
        readfds = clientsfds;
        if (select(MAX_QUEUED + 1, &readfds, nullptr, nullptr, nullptr) < 0)
        {
            system_call_error("select");
            exit(EXIT_FAILURE);
        }
        std::cerr<<"out of select\n";


        if (FD_ISSET(STDIN_FILENO, &readfds))  // Message from stdin
        {
            serverStdInput(&context);
        }

        if (FD_ISSET(connection_socket, &readfds))  // Connection from new client
        {
            if((file_descriptor = accept(connection_socket, nullptr, nullptr)) < 0)
            {
                system_call_error("accept");
                exit(EXIT_FAILURE);
            }
            FD_SET(file_descriptor, &clientsfds); // add the client to the clientsfds
            connectNewClient(&context, file_descriptor);
        }
        else  // Connection from existing client
        {
            //will check each client if it’s in readfds and then receive a message from him
            for(const auto client: *(context.server_members))
            {
                if(client == nullptr)
                {
                    std::cerr<<"client == nullptr\n";
                }

                if(FD_ISSET((*client).client_socket, &readfds))
                {
                    std::cerr<<"entering client request- socket number" << (*client).client_socket << "\n";

                    if(handleClientRequest(&context, client->client_socket) == ERASE_USER_CODE)
                    {
                        FD_CLR(client->client_socket, &clientsfds);
                    }
                }
            }
        }

        std::cerr<<"end of loop\n";
        if(context.name_buffer == nullptr)
        {
            std::cerr<<"name_buffer == nullptr\n";
        }
        bzero(context.name_buffer, WA_MAX_NAME);
    }
}


int main(int argc, char** argv)
{

    if(argc != NUM_OF_ARGS)
    {
        print_server_usage();
        exit(0);
    }
    unsigned short port_number = atoi(argv[PORT_INDEX]);
    int fd = establish(port_number);
    select_flow(fd);
    return EXIT_SUCCESS; // todo check if this is what we need to return
}


