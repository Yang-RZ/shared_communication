#include "sync_helper.h"

#include <boost/thread/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/process.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/log/attributes/current_process_name.hpp>

using namespace boost::interprocess;

namespace shared_communication
{
    SyncHelper::SyncHelper(std::string dev_address)
    {
        dev_address_ = dev_address;
        named_mutex mtx(open_or_create, NEW_CONNECTION_MUTEX_NAME);
        scoped_lock<named_mutex> lock(mtx);

        ptr_sync_data_ = nullptr;
        try
        {
            shared_memory_object shm(open_only, dev_address.c_str(), read_write);
            region_sync_data_ = mapped_region(shm, read_write);
            ptr_sync_data_ = (SyncData*)region_sync_data_.get_address();


        }
        catch (const std::exception& e)
        {
            std::cout << "Failed to open shm: " << e.what() << std::endl;
        }
        if (ptr_sync_data_ == nullptr)
        {
            try
            {
                shared_memory_object shm(create_only, dev_address.c_str(), read_write);
                shm.truncate(sizeof(SyncData));
                region_sync_data_ = mapped_region(shm, read_write);
                new (region_sync_data_.get_address()) SyncData;

                ptr_sync_data_ = (SyncData*)region_sync_data_.get_address();
            }
            catch (const std::exception& e)
            {
                std::cout << "Failed to create shm: " << e.what() << std::endl;
                throw e;
            }
        }
    }

    SyncHelper::~SyncHelper()
    {
        bool bHasAliveConnection = false;
        ptr_sync_data_->App_Connected[app_id] = false;
        ptr_sync_data_->App_Infos[app_id] = AppInfo();
        if (is_master_connection_)
        {
            for (size_t i = 0; i < MAX_APP_NUM; i++)
            {
                if (ptr_sync_data_->App_Connected[i])
                {
                    bHasAliveConnection = true;
                }
            }
            if (!bHasAliveConnection)
            {
                named_mutex::remove(NEW_CONNECTION_MUTEX_NAME);
                shared_memory_object::remove(dev_address_.c_str());
            }
        }
        else
        {

        }
    }

    void SyncHelper::Initialize()
    {
        if (!ptr_sync_data_->Initialized)
        {
            ptr_sync_data_->Initialized = true;
            is_master_connection_ = true;
        }
        else
        {
            is_master_connection_ = false;
        }
    }

    void SyncHelper::RepairCommunication(std::string dev_address)
    {
        named_mutex::remove(NEW_CONNECTION_MUTEX_NAME);
        shared_memory_object::remove(dev_address.c_str());
    }

    void SyncHelper::Connect()
    {
        if (app_id >= 0)
        {
            ptr_sync_data_->App_Connected[app_id] = false;
            ptr_sync_data_->App_Infos[app_id] = AppInfo();
            app_id = -1;
        }

        for (size_t i = 0; i < MAX_APP_NUM; i++)
        {
            if (!ptr_sync_data_->App_Connected[i])
            {
                app_id = i;
                break;
            }
        }
        if (app_id < 0)
        {
            throw std::runtime_error("App number limit exceeded!");
        }
        ptr_sync_data_->App_Connected[app_id] = true;
        ptr_sync_data_->App_Infos[app_id].PID = ipcdetail::get_current_process_id();
        std::string sLocation = boost::dll::program_location().string();
        ptr_sync_data_->App_Infos[app_id].Location_String_Len = sLocation.size();
        ::memcpy(ptr_sync_data_->App_Infos[app_id].Location, sLocation.c_str(), sLocation.size());

        if (IsMasterConnection())
        {
            ptr_sync_data_->Connected = true;
        }
    }

    void SyncHelper::Disconnect()
    {
        ptr_sync_data_->App_Connected[app_id] = false;
        ptr_sync_data_->App_Infos[app_id] = AppInfo();

        if (IsMasterConnection())
        {
            std::cout << "Event dev disconnected..." << std::endl;
            ptr_sync_data_->Initialized = false;
            ptr_sync_data_->Connected = false;
            ptr_sync_data_->Sem_Dev_Disconnect.post();
        }
    }

    bool SyncHelper::WaitNewMsg(EnumMessageType& msg_type, uint8_t* msg_buffer, uint32_t msg_buffer_size, uint32_t& size, int timeout_ms)
    {
        boost::system_time const timeoutPeriod = boost::get_system_time() + boost::posix_time::milliseconds(timeout_ms);

        if (ptr_sync_data_->Sem_New_Msg_Prepare_Send.timed_wait(timeoutPeriod))
        {
            msg_type = ptr_sync_data_->Msg_Type;
            switch (msg_type)
            {
                case EnumMessageType::Normal:
                    size = ptr_sync_data_->Msg_Size;
                    assert(size <= MTU_SIZE);
                    assert(msg_buffer_size >= size);
                    ::memcpy(msg_buffer, ptr_sync_data_->Message, size);
                    break;
                case EnumMessageType::Disconnect:
                    break;
                default:
                    throw std::runtime_error(std::string("Unsupported msg type: ") + std::to_string((int)msg_type));
                    break;
            }

            return true;
        }
        else
        {
            return false;
        }
    }

    void SyncHelper::NotifyReplyDone(EnumMessageType msg_type, uint8_t* reply_buffer, uint32_t reply_size, EnumErrorCode error_code)
    {
        switch (msg_type)
        {
            case EnumMessageType::Normal:
                assert(reply_size <= MTU_SIZE);
                ptr_sync_data_->Reply_Size = reply_size;
                ::memcpy(ptr_sync_data_->Reply, reply_buffer, reply_size);
                break;
            case EnumMessageType::Disconnect:
                break;
            default:
                throw std::runtime_error(std::string("Unsupported msg type: ") + std::to_string((int)msg_type));
                break;
        }
        ptr_sync_data_->Sem_New_Msg_Send_Done.post();
    }

    void SyncHelper::SendMsg(EnumMessageType msg_type, const uint8_t* msg_buffer, uint32_t msg_size)
    {
        switch (msg_type)
        {
            case EnumMessageType::Normal:
                assert(msg_size <= MTU_SIZE);
                ptr_sync_data_->Msg_Size = msg_size;
                ::memcpy(ptr_sync_data_->Message, msg_buffer, msg_size);
                break;
            case EnumMessageType::Disconnect:
                break;
            default:
                throw std::runtime_error(std::string("Unsupported msg type: ") + std::to_string((int)msg_type));
                break;
        }

        ptr_sync_data_->Msg_Type = msg_type;
        ptr_sync_data_->Sem_New_Msg_Prepare_Send.post();
    }

    void SyncHelper::RecvReply(EnumMessageType msg_type, uint8_t* reply_buffer, uint32_t reply_buffer_size, uint32_t& reply_size, uint32_t timeout_ms)
    {
        boost::system_time const timeoutPeriod = boost::get_system_time() + boost::posix_time::milliseconds(timeout_ms);

        if (ptr_sync_data_->Sem_New_Msg_Send_Done.timed_wait(timeoutPeriod))
        {
            switch (msg_type)
            {
                case EnumMessageType::Normal:
                    reply_size = ptr_sync_data_->Reply_Size;
                    assert(reply_buffer_size >= reply_size);
                    ::memcpy(reply_buffer, ptr_sync_data_->Reply, reply_size);
                    break;
                case EnumMessageType::Disconnect:
                    break;
                default:
                    throw std::runtime_error(std::string("Unsupported msg type: ") + std::to_string((int)msg_type));
                    break;
            }
        }
        else
        {
            throw std::runtime_error("Recv timeout!");
        }
    }

    void SyncHelper::WaitSendMsgDone(uint32_t timeout_ms)
    {
        boost::system_time const timeoutPeriod = boost::get_system_time() + boost::posix_time::milliseconds(timeout_ms);

        if (ptr_sync_data_->Sem_New_Msg_Send_Done.timed_wait(timeoutPeriod))
        {

        }
        else
        {
            throw std::runtime_error("Recv timeout!");
        }
    }

    EnumErrorCode SyncHelper::GetErrorCode()
    {
        return ptr_sync_data_->Error_Code;
    }

    void SyncHelper::LogError(std::string error_info)
    {
        ptr_sync_data_->Error_Info_Size = error_info.size();
        ::memcpy(ptr_sync_data_->Error_Info, error_info.c_str(), error_info.size());

        std::cout << "Error: " << error_info << std::endl;
    }
    bool SyncHelper::DeviceDisconnected()
    {
        boost::system_time const timeoutPeriod = boost::get_system_time() + boost::posix_time::milliseconds(1000);

        if (ptr_sync_data_->Sem_Dev_Disconnect.timed_wait(timeoutPeriod))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}
