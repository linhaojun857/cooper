#include "InetAddress.hpp"

#include <netdb.h>
#include <netinet/tcp.h>
#include <strings.h>

#include <cstring>

#include "cooper/net/InetAddress.hpp"
#include "cooper/util/Logger.hpp"

// INADDR_ANY use (type)value casting.
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

//     struct sockaddr_in6 {
//         sa_family_t     sin6_family;   /* address family: AF_INET6 */
//         uint16_t        sin6_port;     /* port in network byte order */
//         uint32_t        sin6_flowinfo; /* IPv6 flow information */
//         struct in6_addr sin6_addr;     /* IPv6 address */
//         uint32_t        sin6_scope_id; /* IPv6 scope-id */
//     };

using namespace cooper;

/*
#ifdef __linux__
#if !(__GNUC_PREREQ(4, 6))
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
#endif
*/

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6) : isIpV6_(ipv6) {
    if (ipv6) {
        memset(&addr6_, 0, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
        addr6_.sin6_addr = ip;
        addr6_.sin6_port = htons(port);
    } else {
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
        addr_.sin_addr.s_addr = htonl(ip);
        addr_.sin_port = htons(port);
    }
    isUnspecified_ = false;
}

InetAddress::InetAddress(const std::string& ip, uint16_t port, bool ipv6) : isIpV6_(ipv6) {
    if (ipv6) {
        memset(&addr6_, 0, sizeof(addr6_));
        addr6_.sin6_family = AF_INET6;
        addr6_.sin6_port = htons(port);
        if (::inet_pton(AF_INET6, ip.c_str(), &addr6_.sin6_addr) <= 0) {
            return;
        }
    } else {
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        if (::inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0) {
            return;
        }
    }
    isUnspecified_ = false;
}

std::string InetAddress::toIpPort() const {
    char buf[64] = "";
    uint16_t port = ntohs(addr_.sin_port);
    snprintf(buf, sizeof(buf), ":%u", port);
    return toIp() + std::string(buf);
}
std::string InetAddress::toIpPortNetEndian() const {
    std::string buf;
    static constexpr auto bytes = sizeof(addr_.sin_port);
    buf.resize(bytes);
    std::memcpy(&buf[0], &addr_.sin_port, bytes);
    return toIpNetEndian() + buf;
}
bool InetAddress::isIntranetIp() const {
    if (addr_.sin_family == AF_INET) {
        uint32_t ip_addr = ntohl(addr_.sin_addr.s_addr);
        if ((ip_addr >= 0x0A000000 && ip_addr <= 0x0AFFFFFF) || (ip_addr >= 0xAC100000 && ip_addr <= 0xAC1FFFFF) ||
            (ip_addr >= 0xC0A80000 && ip_addr <= 0xC0A8FFFF) || ip_addr == 0x7f000001)

        {
            return true;
        }
    } else {
        auto addrP = ip6NetEndian();
        // Loopback ip
        if (*addrP == 0 && *(addrP + 1) == 0 && *(addrP + 2) == 0 && ntohl(*(addrP + 3)) == 1)
            return true;
        // Privated ip is prefixed by FEC0::/10 or FE80::/10, need testing
        auto i32 = (ntohl(*addrP) & 0xffc00000);
        if (i32 == 0xfec00000 || i32 == 0xfe800000)
            return true;
        if (*addrP == 0 && *(addrP + 1) == 0 && ntohl(*(addrP + 2)) == 0xffff) {
            // the IPv6 version of an IPv4 IP address
            uint32_t ip_addr = ntohl(*(addrP + 3));
            if ((ip_addr >= 0x0A000000 && ip_addr <= 0x0AFFFFFF) || (ip_addr >= 0xAC100000 && ip_addr <= 0xAC1FFFFF) ||
                (ip_addr >= 0xC0A80000 && ip_addr <= 0xC0A8FFFF) || ip_addr == 0x7f000001) {
                return true;
            }
        }
    }
    return false;
}

bool InetAddress::isLoopbackIp() const {
    if (!isIpV6()) {
        uint32_t ip_addr = ntohl(addr_.sin_addr.s_addr);
        if (ip_addr == 0x7f000001) {
            return true;
        }
    } else {
        auto addrP = ip6NetEndian();
        if (*addrP == 0 && *(addrP + 1) == 0 && *(addrP + 2) == 0 && ntohl(*(addrP + 3)) == 1)
            return true;
        // the IPv6 version of an IPv4 loopback address
        if (*addrP == 0 && *(addrP + 1) == 0 && ntohl(*(addrP + 2)) == 0xffff && ntohl(*(addrP + 3)) == 0x7f000001)
            return true;
    }
    return false;
}

std::string InetAddress::toIp() const {
    char buf[64];
    if (addr_.sin_family == AF_INET) {
        ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    } else if (addr_.sin_family == AF_INET6) {
        ::inet_ntop(AF_INET6, &addr6_.sin6_addr, buf, sizeof(buf));
    }
    return buf;
}

std::string InetAddress::toIpNetEndian() const {
    std::string buf;
    if (addr_.sin_family == AF_INET) {
        static constexpr auto bytes = sizeof(addr_.sin_addr.s_addr);
        buf.resize(bytes);
        std::memcpy(&buf[0], &addr_.sin_addr.s_addr, bytes);
    } else if (addr_.sin_family == AF_INET6) {
        static constexpr auto bytes = sizeof(addr6_.sin6_addr);
        buf.resize(bytes);
        std::memcpy(&buf[0], ip6NetEndian(), bytes);
    }
    return buf;
}

uint32_t InetAddress::ipNetEndian() const {
    // assert(family() == AF_INET);
    return addr_.sin_addr.s_addr;
}

const uint32_t* InetAddress::ip6NetEndian() const {
    // assert(family() == AF_INET6);
    return addr6_.sin6_addr.s6_addr32;
}

uint16_t InetAddress::toPort() const {
    return ntohs(portNetEndian());
}
