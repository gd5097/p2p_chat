#include <iostream>
#include <cstdio>
#include <string>
#include "net.hpp"

using namespace std;

int main()
{
    Net net;

    Connection connection = net.connect("127.0.0.1", 6457);

    char buffer[10000];
    while (true)
    {
        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            break;
        }

        connection.write(buffer, strlen(buffer) + 1);
    }

    net.close();
}