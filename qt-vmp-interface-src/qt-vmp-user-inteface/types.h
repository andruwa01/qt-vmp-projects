#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include <cstdint>

struct CommandInfo
{
    int8_t commandByte = 0;
    std::vector<uint8_t> params = {0};
    bool isSent = false;
    bool isWaitingForResponse = false;
};

#endif // TYPES_H
