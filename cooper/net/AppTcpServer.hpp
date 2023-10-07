#ifndef net_AppTcpServer_hpp
#define net_AppTcpServer_hpp

#include <nlohmann/json.hpp>

#include "cooper/net/EventLoopThread.hpp"
#include "cooper/net/TcpServer.hpp"
#include "cooper/util/Logger.hpp"
#include "cooper/util/NonCopyable.hpp"

using namespace nlohmann;

namespace cooper {
/**
 * @brief Application Tcp Server
 */
class AppTcpServer : public NonCopyable {
public:
    using protocolType = uint32_t;
    using Handler = std::function<void(const TcpConnectionPtr&, const json&)>;

    explicit AppTcpServer(uint16_t port = 8888);

    ~AppTcpServer();

    /**
     * @brief start server
     * @param loopNum
     */
    void start(int loopNum = 3);

    /**
     * @brief stop server
     */
    void stop();

    /**
     * @brief register protocol handler
     * @param header
     * @param handler
     */
    void registerProtocolHandler(protocolType header, const Handler& handler);

private:
    void recvMsgCallback(const TcpConnectionPtr& conn, MsgBuffer* buffer);

    EventLoopThread loopThread_;
    std::shared_ptr<TcpServer> server_;
    std::unordered_map<protocolType, Handler> handlers_;
};
}  // namespace cooper

#endif