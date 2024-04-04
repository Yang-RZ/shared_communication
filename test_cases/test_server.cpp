#include <iostream>
#include"boost/asio.hpp"

using namespace std;
using namespace boost;
using asio::ip::tcp;

int main()
{
    cout << "server start ......" << endl;

    asio::io_context io;
    tcp::acceptor acptr(io, tcp::endpoint(tcp::v4(), 34566));

    while (true)
    {
        tcp::socket sock(io);
        acptr.accept(sock);

        try
        {
            cout << "client:" << sock.remote_endpoint().address() << endl;

            while (true)
            {
                char buf[0xFF];
                size_t iRecv = sock.receive(asio::buffer(buf));
                cout << "recv from client:" << std::string(buf, iRecv) << endl;
                sock.send(asio::buffer(buf, iRecv));
            }
        }
        catch (std::exception& e)
        {
            cout << e.what() << endl;
        }
    }

    //sock.close();
    ::system("pause");
}