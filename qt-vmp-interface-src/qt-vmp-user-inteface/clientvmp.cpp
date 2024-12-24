#include "clientvmp.h"

ClientVmp::ClientVmp(std::string ipv4_vmp_new
                    , int vmp_port_ctrl_new
                    , int vmp_port_data_new)
    : ipv4_vmp(ipv4_vmp_new),
      vmp_port_ctrl(vmp_port_ctrl_new),
      vmp_port_data(vmp_port_data_new)
{
    qDebug() << "ClientVmp constructor called";
}

ClientVmp::~ClientVmp()
{
    qDebug() << "ClientVmp destructor called";
}

bool ClientVmp::initSockets()
{
    rtcp_socket_ctrl = initSocket(ipv4_vmp, vmp_port_ctrl, 0);
    if (rtcp_socket_ctrl == -1)
    {
        qCritical() << "initSocket(): " << "failed for ctrl";
        return false;
    }

    struct sockaddr_in local_addr;
    std::memset(&local_addr, 0, sizeof(sockaddr_in));
    socklen_t hints_size = sizeof(local_addr);
    getsockname(rtcp_socket_ctrl, (struct sockaddr*)&local_addr, &hints_size);

    rtcp_socket_data = initSocket(ipv4_vmp, vmp_port_data, ntohs(local_addr.sin_port) - 1);
    if (rtcp_socket_data == -1)
    {
        qCritical() << "initSocket(): " << "failed for data";
        return false;
    }

    return true;
}

int ClientVmp::initSocket(std::string ipv4_vmp, const int port_vmp, const int port_client)
{
    struct sockaddr_in hints;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == -1)
    {
        qCritical() << "socket()" << std::strerror(errno);
        return -1;
    }

    qInfo() << "socket(): " << "created";

    // add functionality to receive data from vmp (to use recv() instead of recvfrom())

    std::memset(&hints, 0, sizeof(sockaddr_in));
    hints.sin_family = AF_INET;
    hints.sin_port   = htons(port_client);
    hints.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, (struct sockaddr*)&hints, sizeof(hints)) == -1)
    {
        qCritical() << "bind(): " << std::strerror(errno);
        return -1;
    }

    // add functionality to send data to vmp (to use send() instead of sendto() )

    std::memset(&hints, 0, sizeof(sockaddr_in));
    hints.sin_family = AF_INET;
    hints.sin_port   = htons(port_vmp);
    if (inet_pton(AF_INET, ipv4_vmp.c_str(), &(hints.sin_addr.s_addr)) == -1)
    {
        qCritical() << "inet_pton(): " << std::strerror(errno);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&hints, sizeof(hints)) == -1)
    {
        qCritical() << "connect(): " << std::strerror(errno);
        close(sockfd);
        return -1;
    }

    qInfo() << "socket bind(), connect()";

    return sockfd;
}

void ClientVmp::sendCommand(std::vector<uint8_t> buffer)
{
    if (send(rtcp_socket_ctrl, buffer.data(), buffer.size(), MSG_NOSIGNAL) == -1)
    {
        qCritical() << "send(): " << strerror(errno);
    }

    QString commandHex = QString::fromStdString(messToStr(buffer[12]));
    qInfo() << "send(): " << "command " << commandHex << " sent";
}

void ClientVmp::makeCommand(std::vector<uint8_t> &command_pkg, uint8_t mess_id, const std::vector<uint8_t> &params)
{
    std::size_t size_vprm_rtcp_head      = sizeof(VPRM_RTCP_HEAD)     - 1;
    std::size_t size_vprm_rtcp_name_req  = sizeof(VPRM_RTCP_NAME_REQ) - 1;
    command_pkg.resize(size_vprm_rtcp_head + size_vprm_rtcp_name_req + 4 + params.size(), 0);

    // initial offset of package
    int32_t offset  = 0;

    // RTCP_HEAD 8 bytes
    std::memcpy(&command_pkg[offset], VPRM_RTCP_HEAD, size_vprm_rtcp_head);
    offset += size_vprm_rtcp_head;
    // RTCP_NAME_REQ 4 bytes
    std::memcpy(&command_pkg[offset], VPRM_RTCP_NAME_REQ, size_vprm_rtcp_name_req);
    offset += size_vprm_rtcp_name_req;
    // command 4 bytes
    std::memcpy(&command_pkg[offset], &mess_id, sizeof(mess_id));
    offset += sizeof(mess_id);
    offset += 3 * sizeof(uint8_t);

    std::memcpy(&command_pkg[offset], params.data(), params.size());

    while (command_pkg.size() % 4 != 0)
    {
        command_pkg.push_back(0);
    }

    // calculate full parts
    command_pkg[3] = command_pkg.size() / 4 - 1;
}

uint32_t ClientVmp::parseIQPkg(std::vector<uint8_t> &iq_pkg, uint32_t pkg_size)
{
    uint32_t offset = 0; 		 // offset for stepping in pkg
    int step = sizeof(uint32_t); // 4 bytes step
    int32_t subpkg_size = 0;	 // track package size up to start of each iteration
    do
    {
        qDebug() << "offset: " << offset;

        // check if we have enough data to read 4 bytes
        if (pkg_size < step + offset)
        {
            return offset;
        }

        // check if we have enough data to read all package
        subpkg_size = *(int32_t*)(iq_pkg.data() + offset);
        if (pkg_size < offset + step + subpkg_size)
        {
            return offset;
        }

        // header 12 bytes

        // swap (endianess)
        std::swap(iq_pkg[offset + step + 2], iq_pkg[offset + step + 3]);
        uint16_t seq_package_num = *(uint16_t*)&iq_pkg[offset + step + 2];

        qDebug() << "seq_package_num" << seq_package_num;

        if (iq_pkg[offset + step] 	  != (uint8_t)0x80 ||
            iq_pkg[offset + step + 1] != (uint8_t)0x7F ||
            subpkg_size != package_data_and_header_size)
        {
            return offset;
        }

        // Filing zeroes if there are missed packets
        if (seq_package_num != uint16_t(last_seq_package_num + 1))
        {
            int32_t missed_packages_cnt = 0;
            if (missed_packages_cnt >= last_seq_package_num)
            {
                missed_packages_cnt = missed_packages_cnt - last_seq_package_num - 1;
            }

            else
            {
                // in case seq_package_num counter was enter 65535 - we start counting missed packages
                // from UINT16_MAX (65535) - last_seq_package_num
                missed_packages_cnt = UINT16_MAX - last_seq_package_num + seq_package_num;
            }

            qInfo() << "PRM DROPOUT: " << missed_packages_cnt;

            if (missed_packages_cnt < 30)
            {
                if (zero_buffer.size() < (package_data_and_header_size - 12) * missed_packages_cnt)
                {
                    zero_buffer.resize(  (package_data_and_header_size - 12) * missed_packages_cnt, 0);
                }

                qInfo() << "zero filled: " << missed_packages_cnt;
            }
        }

        last_seq_package_num = seq_package_num;

        // send data to gui

        offset += subpkg_size + step;
    }
    while (offset < pkg_size);

    return offset;
}

std::string ClientVmp::getVmpIp()
{
    return ipv4_vmp;
}

int ClientVmp::getVmpCtrlPort()
{
    return vmp_port_ctrl;
}

int ClientVmp::getVmpDataPort()
{
    return vmp_port_data;
}

std::string ClientVmp::messToStr(uint8_t messId)
{
    if(messId == VPrm::MessId::GetDiag) 			return "GetDiag";
    if(messId == VPrm::MessId::GetCurrentState)	 	return "GetCurrentState";
    if(messId == VPrm::MessId::SetFilterMode) 		return "SetFilterMode";
    if(messId == VPrm::MessId::SetFrequency)	 	return "SetFrequency";
    if(messId == VPrm::MessId::GetStat) 			return "GetStat";
    if(messId == VPrm::MessId::ResetStat) 			return "ResetStat";
    if(messId == VPrm::MessId::GetAddr) 			return "GetAddr";
    if(messId == VPrm::MessId::SetAddr) 			return "SetAddr";
    if(messId == VPrm::MessId::SetAtt20dB) 			return "SetAtt20dB";
    if(messId == VPrm::MessId::SetAtt45dB) 			return "SetAtt45dB";
    if(messId == VPrm::MessId::SetCustomFilter) 	return "SetCustomFilter";
    if(messId == VPrm::MessId::SetAgcDelay) 		return "SetAgcDelay";
    if(messId == VPrm::MessId::SetAnt) 				return "SetAnt";
    if(messId == VPrm::MessId::SetGen) 				return "SetGen";
    if(messId == VPrm::MessId::AddMemChannel) 		return "AddMemChannel";
    if(messId == VPrm::MessId::SetMemChannel) 		return "SetMemChannel";
    if(messId == VPrm::MessId::SetRefFreqTrimmer) 	return "SetRefFreqTrimmer";
    if(messId == VPrm::MessId::GetSoftVers) 		return "GetSoftVers";
    if(messId == VPrm::MessId::SetPhase) 			return "SetPhase";
    if(messId == VPrm::MessId::SetRtpCtrl) 			return "SetRtpCtrl";
    if(messId == VPrm::MessId::SetAdcProtect) 		return "SetAdcProtect";
    if(messId == VPrm::MessId::SetRtpSyncMode) 		return "SetRtpSyncMode";
    if(messId == VPrm::MessId::SetAntCtrl) 			return "SetAntCtrl";
    if(messId == VPrm::MessId::GetRefFreqTrimmer)   return "GetRefFreqTrimmer";
    if(messId == VPrm::MessId::AnsDiag) 			return "AnsDiag";
    if(messId == VPrm::MessId::AnsCurrentState) 	return "AnsCurrentState";
    if(messId == VPrm::MessId::AckFilterMode) 		return "AckFilterMode";
    if(messId == VPrm::MessId::AckFrequency) 		return "AckFrequency";
    if(messId == VPrm::MessId::AnsStat) 			return "AnsStat";
    if(messId == VPrm::MessId::AckResentStat) 		return "AckResentStat";
    if(messId == VPrm::MessId::AckResetRtpSeqNum) 	return "AckResetRtpSeqNum";
    if(messId == VPrm::MessId::AnsAddr) 			return "AnsAddr";
    if(messId == VPrm::MessId::AckAddr) 			return "AckAddr";
    if(messId == VPrm::MessId::AckAtt20dB) 			return "AckAtt20dB";
    if(messId == VPrm::MessId::AckAtt45dB) 			return "AckAtt45dB";
    if(messId == VPrm::MessId::AckCustomFilter) 	return "AckCustomFilter";
    if(messId == VPrm::MessId::AckAgcDelay) 		return "AckAgcDelay";
    if(messId == VPrm::MessId::AckAnt) 				return "AckAnt";
    if(messId == VPrm::MessId::AckGen) 				return "AckGen";
    if(messId == VPrm::MessId::AckAddMemChannel) 	return "AckAddMemChannel";
    if(messId == VPrm::MessId::AckMemChannel) 		return "AckMemChannel";
    if(messId == VPrm::MessId::AckRefFreqTrimmer)   return "AckRefFreqTrimmer";
    if(messId == VPrm::MessId::AnsSoftVers) 		return "AnsSoftVers";
    if(messId == VPrm::MessId::AckPhase) 			return "AckPhase";
    if(messId == VPrm::MessId::AckRtpCtrl) 			return "AckRtpCtrl";
    if(messId == VPrm::MessId::AckAdcProtect) 		return "AckAdcProtect";
    if(messId == VPrm::MessId::AckRtpSyncMode)  	return "AckRtpSyncMode";
    if(messId == VPrm::MessId::AckAntCtrl) 			return "AckAntCtrl";
    if(messId == VPrm::MessId::AnsRefFreqTrimmer) 	return "AnsRefFreqTrimmer";
    if(messId == VPrm::MessId::Error) 				return "Error";
    return "unknown messId: 0x" + messIdToHex(messId);
}

std::string ClientVmp::messIdToHex(uint8_t messId) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(messId);
    return oss.str();
}
