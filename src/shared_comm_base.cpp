#include <boost/bind.hpp>
#include "shared_comm_base.h"
#include "sync_helper.h"

namespace shared_communication
{
    class SharedCommBase::PrivateMembers
    {
    public:
        std::shared_ptr<SyncHelper> Ptr_Sync;
    };

    void SharedCommBase::RunMonitorDevDisconnectionTask()
    {
        auto sync = ptr_private_members_->Ptr_Sync;

        while (IsConnected())
        {
            if (sync->DeviceDisconnected())
            {
                OnReceivingBaton();
                break;
            }
        }
    }

    void SharedCommBase::RunCommunicationTask()
    {
        auto sync = ptr_private_members_->Ptr_Sync;
        EnumMessageType typeMsg = EnumMessageType::Normal;
        std::shared_ptr<uint8_t> msg_buf(new uint8_t[MTU_SIZE]);
        std::shared_ptr<uint8_t> reply_buf(new uint8_t[MTU_SIZE]);
        uint32_t msg_size = 0;
        uint32_t msg_send_len = 0;
        uint32_t reply_len = 0;

        while (IsConnected())
        {
            if (sync->WaitNewMsg(typeMsg, msg_buf.get(), MTU_SIZE, msg_size, 1000))
            {
                EnumErrorCode error_code = EnumErrorCode::No_Error;
                switch (typeMsg)
                {
                    case shared_communication::EnumMessageType::Normal:
                        try
                        {
                            msg_send_len = DoSend(msg_buf.get(), msg_size);
                        }
                        catch (const std::exception& e)
                        {
                            error_code = EnumErrorCode::Send_Msg_Failed;
                            sync->LogError(e.what());
                        }
                        try
                        {
                            reply_len = DoRecv(reply_buf.get(), MTU_SIZE);
                        }
                        catch (const std::exception& e)
                        {
                            error_code = EnumErrorCode::Recv_Msg_Failed;
                            sync->LogError(e.what());
                        }
                        break;
                    case shared_communication::EnumMessageType::Disconnect:
                        try
                        {
                            DoDisconnect();
                            connected_ = false;
                            sync->Disconnect();
                        }
                        catch (const std::exception& e)
                        {
                            error_code = EnumErrorCode::Unknown_Error;
                            sync->LogError(e.what());
                        }
                        break;
                    default:
                        break;
                }

                sync->NotifyReplyDone(typeMsg, reply_buf.get(), reply_len, error_code);
            }
        }
    }

    SharedCommBase::SharedCommBase() : connected_(false),
        ptr_private_members_(nullptr)
    {
        ptr_private_members_ = new PrivateMembers();
    }

    SharedCommBase::~SharedCommBase()
    {
        if (ptr_private_members_)
        {
            delete ptr_private_members_;
            ptr_private_members_ = nullptr;
        }
    }

    void SharedCommBase::RepairCommunication()
    {
        SyncHelper::RepairCommunication(UniqueName());
    }

    EnumErrorCode SharedCommBase::Connect()
    {
        try
        {
            InitializeSync(UniqueName());

            ptr_private_members_->Ptr_Sync->Connect();
            if (ptr_private_members_->Ptr_Sync->IsMasterConnection())
            {
                DoConnect();
            }
            connected_ = true;

            if (ptr_private_members_->Ptr_Sync->IsMasterConnection())
            {
                ptr_private_members_->Ptr_Sync->t_communication = new std::thread(std::bind(&SharedCommBase::RunCommunicationTask, this));
            }
            else
            {
                ptr_private_members_->Ptr_Sync->t_wait_dev_disconnect = new std::thread(std::bind(&SharedCommBase::RunMonitorDevDisconnectionTask, this));
            }
        }
        catch (const std::exception& ex)
        {
            std::cout << "Error: " << ex.what() << std::endl;
            return EnumErrorCode::Unknown_Error;
        }

        return EnumErrorCode::No_Error;
    }

    EnumErrorCode SharedCommBase::Disconnect()
    {
        auto& mtx = ptr_private_members_->Ptr_Sync->GetSendMutex();
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(mtx);

        std::cout << "Disconnecting..." << std::endl;
        if (ptr_private_members_->Ptr_Sync->IsMasterConnection())
        {
            ptr_private_members_->Ptr_Sync->SendMsg(EnumMessageType::Disconnect, nullptr, 0);
            ptr_private_members_->Ptr_Sync->WaitSendMsgDone();
        }
        else
        {
            ptr_private_members_->Ptr_Sync->Disconnect();
            connected_ = false;
        }
        if (ptr_private_members_->Ptr_Sync->IsMasterConnection())
        {
            if (ptr_private_members_->Ptr_Sync->t_communication != nullptr)
            {
                if (ptr_private_members_->Ptr_Sync->t_communication->joinable())
                {
                    ptr_private_members_->Ptr_Sync->t_communication->join();
                }
                delete ptr_private_members_->Ptr_Sync->t_communication;
                ptr_private_members_->Ptr_Sync->t_communication = nullptr;
            }
        }
        else
        {
            if (ptr_private_members_->Ptr_Sync->t_wait_dev_disconnect != nullptr)
            {
                if (ptr_private_members_->Ptr_Sync->t_wait_dev_disconnect->joinable())
                {
                    ptr_private_members_->Ptr_Sync->t_wait_dev_disconnect->join();
                }
                delete ptr_private_members_->Ptr_Sync->t_wait_dev_disconnect;
                ptr_private_members_->Ptr_Sync->t_wait_dev_disconnect = nullptr;
            }
        }

        return ptr_private_members_->Ptr_Sync->GetErrorCode();
    }

    bool SharedCommBase::IsConnected()
    {
        return connected_;
    }

    EnumErrorCode SharedCommBase::SendAndRecv(const uint8_t* buf_send, uint32_t send_len, uint8_t* buf_recv, uint32_t buf_recv_len, uint32_t& len_recv)
    {
        auto& mtx = ptr_private_members_->Ptr_Sync->GetSendMutex();
        boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> lock(mtx);

        ptr_private_members_->Ptr_Sync->SendMsg(EnumMessageType::Normal, buf_send, send_len);
        ptr_private_members_->Ptr_Sync->RecvReply(EnumMessageType::Normal, buf_recv, buf_recv_len, len_recv);

        return ptr_private_members_->Ptr_Sync->GetErrorCode();
    }

    void SharedCommBase::InitializeSync(std::string unique_name)
    {
        if (ptr_private_members_->Ptr_Sync == nullptr)
        {
            ptr_private_members_->Ptr_Sync = std::make_shared<SyncHelper>(unique_name);
        }
        ptr_private_members_->Ptr_Sync->Initialize();
    }
}