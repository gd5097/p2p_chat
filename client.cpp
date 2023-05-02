#include <iostream>
#include <stdio.h>
#include <string>
#include "net.hpp"

using namespace std;

int main()
{
    Net net;

    Connection connection = net.connect("127.0.0.1", 6457);

    while (true)
    {
        string msg;
        cout << "> ";
        cin >> msg;

        connection.write(msg.c_str(), msg.length() + 1);
    }

    net.close();
}