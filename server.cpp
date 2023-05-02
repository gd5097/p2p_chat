#include <cstdio>
#include <cstdlib>
#include "net.hpp"

int count(bool opened[100])
{
    int count = 0;
    for (int i = 0; i < 100; i++)
    {
        if (opened[i])
            count++;
    }
    return count;
}

int main()
{
    Net net;

    NetEvent net_events[100];
    ConnectionEvent connection_events[100];
    Connection connections[100];
    int connection_count = 0;
    bool opened[100] = {0};

    net.listen("0.0.0.0", 6457);

    char messages[1000][1000];
    int message_count = 0;

    while (true)
    {
        // net 이벤트를 불러온다
        int event_count = net.poll(net_events);

        // net에서 발생하는 이벤트 순회
        for (int i = 0; i < event_count; i++)
        {
            NetEvent event = net_events[i];

            // 이벤트 타입이 새로운 연결일 경우
            if (event.type == NetEventType::NEW_CONNECTION)
            {
                // 연결 배열에 넣는다
                opened[connection_count] = true;
                connections[connection_count++] = event.data.connection;
                cout << "new connection" << endl;
            }
        }

        // 각 연결에 대해 순회
        for (int i = 0; i < connection_count; i++)
        {
            if (opened[i] == false)
                continue;

            auto connection = connections[i];

            // 연결에서 발생하는 이벤트를 불러온다
            event_count = connection.poll(connection_events);

            for (int j = 0; j < event_count; j++)
            {
                ConnectionEvent event = connection_events[j];

                // 연결 이벤트가 새로운 메시지인 경우
                if (event.type == ConnectionEventType::NEW_MESSAGE)
                {
                    // 연결 id와 함께 출력한다
                    cout << "new message: " << event.data.message.buffer << endl;
                    strncpy(messages[message_count++], event.data.message.buffer, event.data.message.size);
                }

                else if (event.type == ConnectionEventType::CLOSED)
                {
                    cout << "connection closed" << endl;
                    opened[i] = false;
                }
            }
        }

        system("clear");
        cout << "Message count: " << message_count << endl;
        cout << count(opened) << " connections" << endl;
        if (message_count > 0)
        {
            cout << "Last message: " << messages[message_count - 1] << endl;
        }
    }

    net.close();
}