#pragma once

#include <string>
#include "error_code.h"

#ifdef WIN32
#define EXPORTED_OBJECT __declspec(dllexport)
#else
#define EXPORTED_OBJECT
#endif

namespace shared_communication
{
    class EXPORTED_OBJECT SharedCommBase
    {
    public:
        SharedCommBase();
        virtual ~SharedCommBase();

        void RepairCommunication();

        EnumErrorCode Connect();
        EnumErrorCode Disconnect();

        bool IsConnected();

        EnumErrorCode SendAndRecv(const uint8_t* buf_send, uint32_t send_len, uint8_t* buf_recv, uint32_t buf_recv_len, uint32_t& len_recv);

    private:
        void InitializeSync(std::string unique_name);
        
    public:
        class PrivateMembers;

    private:
        PrivateMembers* ptr_private_members_;

    private:
        void RunMonitorDevDisconnectionTask();
        void RunCommunicationTask();

    protected:
        virtual std::string UniqueName() = 0;
        virtual void DoConnect() = 0;
        virtual void DoDisconnect() = 0;
        virtual uint32_t DoSend(uint8_t* buf, uint32_t len) = 0;
        virtual uint32_t DoRecv(uint8_t* buf, uint32_t len) = 0;
        virtual void OnReceivingBaton() = 0;

    protected:
        bool connected_;
    };
}
