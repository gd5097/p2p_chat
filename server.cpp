#include <stdio.h>
#include "net.hpp"

int main()
{
    Net net;

    NetEvent net_events[100];
    ConnectionEvent connection_events[100];
    Connection connections[100];
    int connection_count = 0;

    net.listen("0.0.0.0", 6457);

    while (true)
    {
        // net 이벤트를 불러온다
        int event_count = net.poll(net_events);

        printf("net event count: %d\n", event_count);

        // net에서 발생하는 이벤트 순회
        for (int i = 0; i < event_count; i++)
        {
            NetEvent event = net_events[i];

            // 이벤트 타입이 새로운 연결일 경우
            if (event.type == NetEventType::NEW_CONNECTION)
            {
                // 연결 배열에 넣는다
                connections[connection_count++] = event.data.connection;
            }
        }

        // 각 연결에 대해 순회
        for (int i = 0; i < connection_count; i++)
        {
            auto connection = connections[i];

            // 연결에서 발생하는 이벤트를 불러온다
            event_count = connection.poll(connection_events);

            for (int j = 0; j < connection_count; j++)
            {
                ConnectionEvent event = connection_events[j];

                // 연결 이벤트가 새로운 메시지인 경우
                if (event.type == ConnectionEventType::NEW_MESSAGE)
                {
                    // 연결 id와 함께 출력한다
                    printf("new message from connection: %s\n", event.data.message.buffer);
                }
            }
        }
    }

    net.close();
}