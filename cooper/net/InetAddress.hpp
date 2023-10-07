#ifndef net_InetAddress_hpp
#define net_InetAddress_hpp

#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <mutex>
#include <string>
#include <unordered_map>

#include "cooper/util/Date.hpp"

namespace cooper {
/**
 * @brief Wrapper of sockaddr_in. This is an POD interface class.
 *
 */
class InetAddress {
public:
    /**
     * @brief Constructs an endpoint with given port number. Mostly used in
     * TcpServer listening.
     *
     * @param port
     * @param loopbackOnly
     * @param ipv6
     */
    InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

    /**
     * @brief Constructs an endpoint with given ip and port.
     *
     * @param ip A IPv4 or IPv6 address.
     * @param port
     * @param ipv6
     */
    InetAddress(const std::string& ip, uint16_t port, bool ipv6 = false);

    /**
     * @brief Constructs an endpoint with given struct `sockaddr_in`. Mostly
     * used when accepting new connections
     *
     * @param addr
     */
    explicit InetAddress(const struct sockaddr_in& addr) : addr_(addr), isUnspecified_(false) {
    }

    /**
     * @brief Constructs an IPv6 endpoint with given struct `sockaddr_in6`.
     * Mostly used when accepting new connections
     *
     * @param addr
     */
    explicit InetAddress(const struct sockaddr_in6& addr) : addr6_(addr), isIpV6_(true), isUnspecified_(false) {
    }

    /**
     * @brief Return the sin_family of the endpoint.
     *
     * @return sa_family_t
     */
    sa_family_t family() const {
        return addr_.sin_family;
    }

    /**
     * @brief Return the IP string of the endpoint.
     *
     * @return std::string
     */
    std::string toIp() const;

    /**
     * @brief Return the IP and port string of the endpoint.
     *
     * @return std::string
     */
    std::string toIpPort() const;

    /**
     * @brief Return the IP bytes of the endpoint in net endian byte order
     *
     * @return std::string
     */
    std::string toIpNetEndian() const;

    /**
     * @brief Return the IP and port bytes of the endpoint in net endian byte
     * order
     *
     * @return std::string
     */
    std::string toIpPortNetEndian() const;

    /**
     * @brief Return the port number of the endpoint.
     *
     * @return uint16_t
     */
    uint16_t toPort() const;

    /**
     * @brief Check if the endpoint is IPv4 or IPv6.
     *
     * @return true
     * @return false
     */
    bool isIpV6() const {
        return isIpV6_;
    }

    /**
     * @brief Return true if the endpoint is an intranet endpoint.
     *
     * @return true
     * @return false
     */
    bool isIntranetIp() const;

    /**
     * @brief Return true if the endpoint is a loopback endpoint.
     *
     * @return true
     * @return false
     */
    bool isLoopbackIp() const;

    /**
     * @brief Get the pointer to the sockaddr struct.
     *
     * @return const struct sockaddr*
     */
    const struct sockaddr* getSockAddr() const {
        return static_cast<const struct sockaddr*>((void*)(&addr6_));
    }

    /**
     * @brief Set the sockaddr_in6 struct in the endpoint.
     *
     * @param addr6
     */
    void setSockAddrInet6(const struct sockaddr_in6& addr6) {
        addr6_ = addr6;
        isIpV6_ = (addr6_.sin6_family == AF_INET6);
        isUnspecified_ = false;
    }

    /**
     * @brief Return the integer value of the IP(v4) in net endian byte order.
     *
     * @return uint32_t
     */
    uint32_t ipNetEndian() const;

    /**
     * @brief Return the pointer to the integer value of the IP(v6) in net
     * endian byte order.
     *
     * @return const uint32_t*
     */
    const uint32_t* ip6NetEndian() const;

    /**
     * @brief Return the port number in net endian byte order.
     *
     * @return uint16_t
     */
    uint16_t portNetEndian() const {
        return addr_.sin_port;
    }

    /**
     * @brief Set the port number in net endian byte order.
     *
     * @param port
     */
    void setPortNetEndian(uint16_t port) {
        addr_.sin_port = port;
    }

    /**
     * @brief Return true if the address is not initalized.
     */
    inline bool isUnspecified() const {
        return isUnspecified_;
    }

private:
    union {
        struct sockaddr_in addr_;
        struct sockaddr_in6 addr6_;
    };
    bool isIpV6_{false};
    bool isUnspecified_{true};
};

}  // namespace cooper

#endif

#endif
