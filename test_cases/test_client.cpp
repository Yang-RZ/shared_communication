#include "shared_socket.h"
#include <iostream>
#include <thread>
#include <random>
#include <time.h>

using namespace std;
using namespace shared_communication;

int main()
{
    try
    {
        unique_ptr<SharedCommBase> com = make_unique<SharedSocket>("127.0.0.1", 34566);

        auto ret = com->Connect();
        if (ret == EnumErrorCode::No_Error)
        {
            cout << "server connected" << endl;
        }
        else
        {
            cout << "Error occurred: " << (int)ret << endl;
            return 0;
        }
        srand(time(nullptr));

        string sClientID = to_string(rand());
        cout << "ClientID: " << sClientID << endl;

        for (size_t i = 0; i < 10; i++)
        {
            string sSend(sClientID + "-hello world:");
            sSend += to_string(i) + "-" + to_string(rand());
            uint8_t bufRecv[100];
            uint32_t recv_len = 0;
            auto ret = com->SendAndRecv((const unsigned char*)(sSend.c_str()), sSend.size(), bufRecv, sizeof(bufRecv), recv_len);
            if (ret == EnumErrorCode::No_Error)
            {
                cout << "recv from server: " << string((char*)(bufRecv), recv_len) << endl;
            }
            else
            {
                cout << "Error occurred: " << (int)ret << endl;
            }

            this_thread::sleep_for(chrono::milliseconds(1000));
        }

        com->Disconnect();
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
    }

    ::system("pause");
}