#pragma once

enum class EnumErrorCode
{
    No_Error,
    Device_Unconnected,
    Send_Msg_Failed,
    Recv_Msg_Failed,
    Unknown_Error = 100000,
};
