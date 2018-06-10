//
// Created by hareld10 on 6/10/18.
//
#include <iostream>
#include <string>
#include "whatsappio.h"
#include "whatsappServer.h"
#include <boost/algorithm/string.hpp>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>

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
        if (splitted[0] == ("whatsappServer"))
        {
            whatsappServer
        }

        break;
    }

}


