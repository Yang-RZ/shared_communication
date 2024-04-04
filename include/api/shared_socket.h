#pragma once

#include <string>

#include "shared_comm_base.h"

namespace shared_communication
{
    extern "C"
    {
        class EXPORTED_OBJECT SharedSocket : public SharedCommBase
        {
        public:
            SharedSocket(std::string ip_addr, int port);
            ~SharedSocket();

        protected:
            void DoConnect() override;
            void DoDisconnect() override;

            uint32_t DoSend(uint8_t *buf, uint32_t len) override;
            uint32_t DoRecv(uint8_t *buf, uint32_t len) override;

        protected:
            // Inherited via SharedCommBase
            std::string UniqueName() override;
            void OnReceivingBaton() override;

        private:
            class PrivateMembersForSocket;
            PrivateMembersForSocket *ptr_private_members_;
        };
    }
} // namespace shared_communication
