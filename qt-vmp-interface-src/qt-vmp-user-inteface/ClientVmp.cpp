#include "ClientVmp.h"

ClientVmp::ClientVmp(std::string ipv4_vmp_new, int vmp_port_ctrl_new, int vmp_port_data_new)
    : ipv4_vmp(ipv4_vmp_new),
      vmp_port_ctrl(vmp_port_ctrl_new),
      vmp_port_data(vmp_port_data_new)
{}

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

    qInfo() << "socket(): " << "socket was created";

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

    qInfo() << "socket bind(), connect() finished successfully";

    return sockfd;
}

void ClientVmp::sendRTCP(std::vector<uint8_t> buffer)
{
    if (send(rtcp_socket_data, buffer.data(), buffer.size(), MSG_NOSIGNAL) == -1)
    {
        qCritical() << "send(): " << strerror(errno);
    }
}

void ClientVmp::makeCommand(std::vector<uint8_t> &command_result, uint8_t mess_id, const std::vector<uint8_t> &buffer_data)
{
    qInfo() << "message makeCommand(): " << QString::fromStdString(messToStr(mess_id));

    // all size in bytes - without null terminated symbol
    // 8 bytes
    std::size_t size_vprm_rtcp_head      = sizeof(VPRM_RTCP_HEAD) - 1;
    // 4 bytes
    std::size_t size_vprm_rtcp_name_req  = sizeof(VPRM_RTCP_NAME_REQ) - 1;
    // 4 bytes
    std::size_t size_command = 4;

    command_result.resize(size_vprm_rtcp_head + size_vprm_rtcp_name_req + size_command + buffer_data.size(), 0);

    // add RTCP header
    int32_t index = 0;
    // 8 bytes
    std::memcpy(&command_result[index], VPRM_RTCP_HEAD, size_vprm_rtcp_head);
    index += size_vprm_rtcp_head;
    // 4 bytes
    std::memcpy(&command_result[index], VPRM_RTCP_NAME_REQ, size_vprm_rtcp_name_req);
    index += size_vprm_rtcp_name_req;
    // 1 byte
    std::memcpy(&command_result[index], &mess_id, sizeof(mess_id));
    index += sizeof(mess_id);
    // 3 bytes step (reserver 3 bytes after mess_id)
    index += 3 * sizeof(uint8_t);

    // add buffer with data
    std::memcpy(&command_result[index], buffer_data.data(), buffer_data.size());

    // Add zeroes until package.size % 4 == 0 because it is requirements of protocol
    while (command_result.size() % 4 != 0) {
        command_result.push_back(0);
    }

    // Calculate full parts
    if (command_result.size() >= 4) {
        command_result[3] = command_result.size() / 4 - 1;
    } else {
        qCritical() << "command_result does not have enough elements";
    }
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
}
