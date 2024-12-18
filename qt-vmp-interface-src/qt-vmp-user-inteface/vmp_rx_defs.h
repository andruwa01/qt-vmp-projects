#ifndef VMP_RX_DEFS_H
#define VMP_RX_DEFS_H

#include <stdint.h>
#include <string>

#define VPRM_IGNORE_FAIL_SIZE                   3

#define VPRM_RTCP_TIMEOUT                       98
#define VPRM_RTCP_SETFREQTIMEOUT                4000
#define VPRM_RTCP_ADDMEMCHTIMEOUT               8000
#define VPRM_RTCP_SETGEN                        1000
#define VPRM_RTCP_SETANT                        4000

#define VPRM_QUEUE_LEN                          100
#define VPRM_WAIT_LEN                           4

#define VPRM_RTCP_NAME_REQ                      "RPRM"
#define VPRM_RTCP_NAME_ANS                      "APRM"
#define VPRM_RTCP_HEAD                          "\x80\xCC\x00\x00\x00\x00\x00\x00"

#define VPRM_DEF_FREQ                           5000000

#define VSYNC_CTRL_PORT                         5151
#define VSYNC_CTRL_CONNECT_TIMEOUT              1000
#define VSYNC_CTRL_TIMEOUT                      100

#define VPRM_PRESET_MAX                         249
#define VPRM_FREQ_MIN                           3000000
#define VPRM_FREQ_MAX                           30000000

namespace VPrm
{
    enum MessId
    {
        GetDiag                 = 0x01, AnsDiag                 = 0x81,
        GetCurrentState         = 0x02, AnsCurrentState         = 0x82,
        SetFilterMode           = 0x03, AckFilterMode           = 0x83,
        SetFrequency            = 0x04, AckFrequency            = 0x84,
        GetStat                 = 0x05, AnsStat                 = 0x85,
        ResetStat               = 0x06, AckResentStat           = 0x86,
        ResetRtpSeqNum          = 0x07, AckResetRtpSeqNum       = 0x87,
        GetAddr                 = 0x09, AnsAddr                 = 0x89,
        SetAddr                 = 0x0a, AckAddr                 = 0x8a,
        SetAtt20dB              = 0x0c, AckAtt20dB              = 0x8c,
        SetAtt45dB              = 0x0d, AckAtt45dB              = 0x8d,
        SetCustomFilter         = 0x0e, AckCustomFilter         = 0x8e,
        SetAgcDelay             = 0x0f, AckAgcDelay             = 0x8f,
        SetAnt                  = 0x10, AckAnt                  = 0x90,
        SetGen                  = 0x11, AckGen                  = 0x91,
        AddMemChannel           = 0x12, AckAddMemChannel        = 0x92,
        SetMemChannel           = 0x13, AckMemChannel           = 0x93,
        SetRefFreqTrimmer       = 0x15, AckRefFreqTrimmer       = 0x95,
        GetSoftVers             = 0x16, AnsSoftVers             = 0x96,
        SetPhase                = 0x17, AckPhase                = 0x97,
        SetRtpCtrl              = 0x18, AckRtpCtrl              = 0x98,
        SetAdcProtect           = 0x19, AckAdcProtect           = 0x99,
        SetRtpSyncMode          = 0x1a, AckRtpSyncMode          = 0x9a,
        SetAntCtrl              = 0x1b, AckAntCtrl              = 0x9b,
        GetRefFreqTrimmer       = 0x20, AnsRefFreqTrimmer       = 0xa0,
                                        Error                   = 0xff
    };

    enum AntType
    {
        Internal                = 0x00,
        External                = 0x01
    };

    #pragma pack (push,1)

    struct PrmState
    {
        int32_t freq;
        int32_t phase;
        int32_t ant;
        int32_t att20dB;
        int32_t att45dB;
        int32_t gen;
        int32_t filterMode;
        int32_t agc;
        int32_t filterLow;
        int32_t filterHi;
    };

    struct MemPrmState
    {
        int32_t chN;
        int32_t freq;
        int32_t phase;
        int32_t ant;
        int32_t att20dB;
        int32_t att45dB;
        int32_t gen;
        int32_t filterMode;
        int32_t agc;
        int32_t filterLow;
        int32_t filterHi;
    };

    struct PrmMess
    {
        int32_t messId;
        union
        {
            int32_t testResult;
            PrmState prmState;
            int32_t filterMode;
            int32_t freq;
            int32_t packetsCount;
            struct
            {
                int32_t ip;
                int32_t mask;
                int16_t rtp;
                int16_t rtcp;
                int64_t mac;
            } prmAddr;
            int32_t att20dB;
            int32_t att45dB;
            struct
            {
                int32_t filterLowFreq;
                int32_t filterHiFreq;
            } customFilter;
            int32_t agcDelay;
            int32_t ant;
            int32_t gen;
            MemPrmState memPrmState;
            int32_t chN;
            int32_t refFreqTrimmer;
            char softVersStr[8];
            int32_t phase;
            int32_t rtpCtrl;
            int32_t adcProtect;
            int32_t rtpSyncMode;
            int32_t antCtrl;
            struct
            {
                int32_t errN;
                int32_t errCode;
            } error;
        }value;
    };

    #pragma pack (pop)
}

#endif // VMP_RX_DEFS_H
