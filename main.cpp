#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include "hardwareAPI.h"

int main(int argc, char ** argv)
{
    if(argc < 3)
    {
        std::cout << "You are bad and should feel bad." << std::endl;
        return 0;
    }
    int port = atoi(argv[2]);

    if(port < 1)
    {
        std::cout << "Wrong portnumber: " << port << std::endl;
    }

    // Initialize the connection.
    initHW(argv[1], port);




    std::cout << "Hejsan!" << std::endl;
    return 0;
}
