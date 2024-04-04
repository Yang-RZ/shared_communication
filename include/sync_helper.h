#pragma once

#include <iostream>
#include <thread>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

#include "error_code.h"

namespace shared_communication
{
#define MTU_SIZE 1500
#define MAX_APP_NUM 32
#define MAX_ERROR_INFO_SIZE 1024
#define NEW_CONNECTION_MUTEX_NAME "MTX_NEW_CONNECTION_SHARED_COMM"

    enum class EnumMessageType
    {
        Normal,
        Disconnect,
    };

    struct AppInfo
    {
        int PID = 0;
        char Location[256] = { 0 };
        int Location_String_Len = 0;
    };

    struct SyncData
    {
        bool Initialized = false;
        bool Connected = false;

        boost::interprocess::interprocess_mutex Mtx_Msg_Send;
        EnumMessageType Msg_Type;
        boost::interprocess::interprocess_semaphore Sem_New_Msg_Prepare_Send;
        boost::interprocess::interprocess_semaphore Sem_New_Msg_Send_Done;
        boost::interprocess::interprocess_semaphore Sem_Dev_Disconnect;

        bool App_Connected[MAX_APP_NUM] = { false };
        AppInfo App_Infos[MAX_APP_NUM];

        uint32_t Msg_Size;
        uint8_t Message[MTU_SIZE] = { 0 };
        uint32_t Reply_Size;
        uint8_t Reply[MTU_SIZE] = { 0 };

        EnumErrorCode Error_Code = EnumErrorCode::No_Error;
        uint32_t Error_Info_Size = 0;
        uint8_t Error_Info[MAX_ERROR_INFO_SIZE] = { 0 };

    public:
        SyncData() : Sem_New_Msg_Prepare_Send(0), Sem_New_Msg_Send_Done(0), Mtx_Msg_Send(), Sem_Dev_Disconnect(0)
        {
        }
    };

    class SyncHelper
    {
    public:
        SyncHelper(std::string dev_address);
        ~SyncHelper();
        void Initialize();

    public:
        static void RepairCommunication(std::string dev_address);

    public:
        void Connect();
        void Disconnect();
        //void TermiateOtherConnection();

        bool WaitNewMsg(EnumMessageType& msg_type, uint8_t* msg_buffer, uint32_t msg_buffer_size, uint32_t& size, int timeout_ms = 1000);
        void NotifyReplyDone(EnumMessageType msg_type, uint8_t* reply_buffer, uint32_t reply_size, EnumErrorCode error_code);

        inline auto& GetSendMutex() { return ptr_sync_data_->Mtx_Msg_Send; };
        void SendMsg(EnumMessageType msg_type, const uint8_t* msg_buffer, uint32_t msg_size);
        void RecvReply(EnumMessageType msg_type, uint8_t* reply_buffer, uint32_t reply_buffer_size, uint32_t& reply_size, uint32_t timeout_ms = 5000);
        void WaitSendMsgDone(uint32_t timeout_ms = 5000);

        EnumErrorCode GetErrorCode();
        void LogError(std::string error_info);

        bool DeviceDisconnected();

    public:
        inline bool IsMasterConnection() { return is_master_connection_; };

    public:
        std::thread* t_communication = nullptr;
        std::thread* t_wait_dev_disconnect = nullptr;

    private:
        boost::interprocess::mapped_region region_sync_data_;
        SyncData* ptr_sync_data_ = nullptr;
        std::string dev_address_ = "";
        bool is_master_connection_ = false;
        int app_id = -1;
    };
}