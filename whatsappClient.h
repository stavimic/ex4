//
// Created by hareld10 on 6/10/18.
//

#ifndef EX4_WHATSAPPCLIENT_H
#define EX4_WHATSAPPCLIENT_H

#define MAX_QUEUD 10
class whatsappClient {
public:
    static int call_socket(const char *hostname, unsigned short portnum);
};



#endif //EX4_WHATSAPPCLIENT_H
