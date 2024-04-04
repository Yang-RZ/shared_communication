#include <iostream>
#include "shared_socket.h"

using namespace std;
using namespace shared_communication;

int main()
{
    try
    {
        unique_ptr<SharedCommBase> com = make_unique<SharedSocket>("127.0.0.1", 34566);
        com->RepairCommunication();
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
    }

    ::system("pause");
}