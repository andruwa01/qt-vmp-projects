#include "clientvmp.h"

ClientVmp::ClientVmp(std::string ipv4_vmp_new
                    , int vmp_port_ctrl_new
                    , int vmp_port_data_new
                    , int frequency_new)
    : ipv4_vmp(ipv4_vmp_new),
      vmp_port_ctrl(vmp_port_ctrl_new),
      vmp_port_data(vmp_port_data_new),
      vmp_frequency_hz(frequency_new)
{
    qDebug() << "ClientVmp constructor called";
}

ClientVmp::~ClientVmp()
{
    close(socket_ctrl);
    close(socket_data);

    qDebug() << "ClientVmp destructor called";
}

bool ClientVmp::initSockets()
{
    socket_ctrl = initSocket(ipv4_vmp, vmp_port_ctrl, 0);
    if (socket_ctrl == -1)
    {
        qCritical() << "initSocket(): " << "failed for ctrl";
        return false;
    }

    struct sockaddr_in local_addr;
    std::memset(&local_addr, 0, sizeof(sockaddr_in));
    socklen_t hints_size = sizeof(local_addr);
    getsockname(socket_ctrl, (struct sockaddr*)&local_addr, &hints_size);

    socket_data = initSocket(ipv4_vmp, vmp_port_data, ntohs(local_addr.sin_port) - 1);
    if (socket_data == -1)
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

    // set socket O_NONBLOCK

    int flags = fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int status = fcntl(sockfd, F_SETFL, flags);
    if (status == -1)
    {
        qCritical() << "fcntl(): " << std::strerror(errno);
    }

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

    qInfo() << "socket binded(), connected()";

    return sockfd;
}

void ClientVmp::sendCommand(const CommandInfo &commandInfo)
{
    std::lock_guard<std::mutex> lg(mutexSocket);

    std::vector<uint8_t> command(0);

    makeCommand(command, commandInfo.commandByte, commandInfo.params);

    if (send(socket_ctrl, command.data(), command.size(), MSG_NOSIGNAL) == -1)
    {
        qCritical() << "send(): " << strerror(errno);
    }

    QString commandHex = QString::fromStdString(messToStr(command[12]));

    qInfo() << "====================================================================>>>" << commandHex;
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

    // offset == 16
    std::memcpy(&command_pkg[offset], params.data(), params.size());

    while (command_pkg.size() % 4 != 0)
    {
        command_pkg.push_back(0);
    }

    // calculate full parts
    command_pkg[3] = command_pkg.size() / 4 - 1;
}

ssize_t ClientVmp::receiveRespFromCommand(const CommandInfo &commandInfo)
{
    std::lock_guard<std::mutex> lg(mutexSocket);

    const int command = commandInfo.commandByte;

    std::vector<uint8_t> buffer(MAX_UDP_SIZE);
    ssize_t read_size = recv(socket_ctrl,  buffer.data(), MAX_UDP_SIZE, 0);
    if (read_size == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            qInfo() << "no data available on ctrl socket";
        }
        else
        {
            qCritical() << QString::fromStdString(messToStr(command)) << " answer recv():" << strerror(errno);
        }

        return -1;
    }

    buffer.resize(read_size);

//    qDebug()  << QString::fromStdString(messToStr(command)) << " answer recv():" << "get" << read_size << "bytes";

    uint8_t ackByte = buffer[12];
    if (command == VPrm::MessId::GetCurrentState && ackByte != VPrm::MessId::AnsCurrentState)
    {
        qCritical() << "ERROR! don't get" << QString::fromStdString(messToStr(VPrm::MessId::AnsCurrentState)) << "\n";
        return -1;
    }

    if (command == VPrm::MessId::SetRtpCtrl && ackByte != VPrm::MessId::AckRtpCtrl)
    {
        qCritical() << "ERROR! don't get" << QString::fromStdString(messToStr(VPrm::MessId::AckRtpCtrl)) << "\n";
        return -1;
    }

    if (command == VPrm::MessId::SetFrequency && ackByte != VPrm::MessId::AckFrequency)
    {
        qCritical() << "ERROR! don't get" << QString::fromStdString(messToStr(VPrm::MessId::AckFrequency)) << "\n";
        return -1;
    }

    QString respByteHex = QString::fromStdString(messToStr(ackByte));
    qInfo() << "<<<====================================================================" << respByteHex;

//    debugPrintHexPkg(buffer);

    return read_size;
}

ssize_t ClientVmp::receiveDataPkg(std::vector<uint8_t> &pkg)
{
    std::lock_guard<std::mutex> lg(mutexSocket);

    ssize_t read_size = recv(socket_data, pkg.data(), MAX_UDP_SIZE, 0);
    if (read_size == -1)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            qInfo() << "no data to read on data socket";
        }
        else
        {
            qCritical() << "data pkg recv():" << strerror(errno);
        }

        return -1;
    }

    qInfo() << "data pkg recv():" << "get" << read_size << "bytes";

    pkg.resize(read_size);

//    debugPrintHexPkg(pkg);

    return read_size;
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

int ClientVmp::getVmpFreq()
{
    return vmp_frequency_hz;
}

int ClientVmp::getSocketCtrl()
{
    return socket_ctrl;
}

int ClientVmp::getSocketData()
{
    return socket_data;
}

void ClientVmp::setVmpIp(std::string new_vmp_ip)
{
    ipv4_vmp = new_vmp_ip;
}

void ClientVmp::setVmpCtrlPort(int new_vmp_ctrl_port)
{
    vmp_port_ctrl = new_vmp_ctrl_port;
}

void ClientVmp::setVmpDataPort(int new_vmp_data_port)
{
    vmp_port_data = new_vmp_data_port;
}

void ClientVmp::setVmpFreq(int new_vmp_freq)
{
    vmp_frequency_hz = new_vmp_freq;
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

void ClientVmp::debugPrintHexPkg(std::vector<uint8_t> pkg)
{
    qDebug() << "pkg data: <=========================================================>" << "\n";

    size_t chunk_size = 0x10; // 16 элементов в строке

    // Используем qDebug()
    QString header = "           "; // Выравнивание для индексов

    // Печатаем индексы в шестнадцатеричном формате
    for (size_t i = 0; i < chunk_size; ++i) {
        header += QString(" %1").arg(i, 2, 16, QChar('0')); // Индексы в hex формате с выравниванием
    }
    qDebug().noquote() << header;

    // Печатаем данные по 16 символов на строку
    for (size_t i = 0; i < pkg.size(); i += chunk_size) {
        QString line;

        // Печатаем индекс строки в шестнадцатеричном формате
        line += QString("%1 | ").arg(i, 8, 16, QChar('0'));

        // Печатаем строку данных
        for (size_t j = i; j < i + chunk_size && j < pkg.size(); ++j) {
            line += QString(" %1").arg(static_cast<unsigned char>(pkg[j]), 2, 16, QChar('0'));
        }

        qDebug().noquote() << line; // Печать строки в qDebug()
    }

    qDebug() << "\nnpkg data: <=========================================================>";
}
