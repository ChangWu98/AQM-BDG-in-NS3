#ifndef TCP_UTILS_H
#define TCP_UTILS_H

#include <string>
#include <stdint.h>
#include "ns3/inet-socket-address.h"
namespace ns3{
class TcpUtils{
public:
    static std::string ConvertIpString(uint32_t ip);
};
}

#endif