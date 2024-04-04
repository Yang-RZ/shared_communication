#include "shared_socket.h"
#include <boost/asio.hpp>
#include <iostream>

using namespace std;
using namespace boost;
using namespace boost::asio;

namespace shared_communication
{
    class SharedSocket::PrivateMembersForSocket
    {
    public:
        PrivateMembersForSocket() : 
            io_context_(),
            sock_(io_context_),
            ip_addr_(),
            port_(-1)
        {
        }

    public:
        std::string ip_addr_;
        int port_;

    public:
        boost::asio::io_context io_context_;
        boost::asio::ip::tcp::socket sock_;
    };

    SharedSocket::SharedSocket(std::string ip_addr, int port) : 
        SharedCommBase(),
        ptr_private_members_(new PrivateMembersForSocket)
    {
        ptr_private_members_->ip_addr_ = ip_addr;
        ptr_private_members_->port_ = port;
    }

    SharedSocket::~SharedSocket()
    {
        if (ptr_private_members_)
        {
            delete ptr_private_members_;
        }
    }

    void SharedSocket::DoConnect()
    {
        if (IsConnected())
        {
            throw std::runtime_error("Trying to reconnect the device...");
        }

        ptr_private_members_->sock_.connect(ip::tcp::endpoint(ip::address::from_string(ptr_private_members_->ip_addr_), ptr_private_members_->port_));
    }

    void SharedSocket::DoDisconnect()
    {
        if (!IsConnected())
        {
            throw std::runtime_error("Trying to disconnect to an disconnected device...");
        }

        ptr_private_members_->sock_.close();
    }

    uint32_t SharedSocket::DoSend(uint8_t* buf, uint32_t len)
    {
        if (!IsConnected())
        {
            throw std::runtime_error(std::string("Device unconnected. Caller: ") + std::string(__func__));
        }

        return ptr_private_members_->sock_.send(asio::buffer(buf, len));
    }

    uint32_t SharedSocket::DoRecv(uint8_t* buf, uint32_t len)
    {
        if (!IsConnected())
        {
            throw std::runtime_error(std::string("Device unconnected. Caller: ") + std::string(__func__));
        }

        uint32_t iRecv = ptr_private_members_->sock_.receive(asio::buffer(buf, len));

        return iRecv;
    }

    std::string SharedSocket::UniqueName()
    {
        return ptr_private_members_->ip_addr_ + "-" + std::to_string(ptr_private_members_->port_);
    }

    void SharedSocket::OnReceivingBaton()
    {
        std::cout << "Receving baton..." << std::endl;
        connected_ = false;
        Connect();
        //ptr_private_members_->sock_.connect(ip::tcp::endpoint(ip::address::from_string(ptr_private_members_->ip_addr_), ptr_private_members_->port_));
    }
} // namespace shared_communication
