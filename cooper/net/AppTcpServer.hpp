#ifndef net_AppTcpServer_hpp
#define net_AppTcpServer_hpp

#include <nlohmann/json.hpp>

#include "cooper/net/EventLoopThread.hpp"
#include "cooper/net/TcpServer.hpp"
#include "cooper/util/Logger.hpp"
#include "cooper/util/NonCopyable.hpp"

#define PING_TYPE 100
#define PONG_TYPE 200

#define BUSINESS_MODE 1
#define MEDIA_MODE 2

using namespace nlohmann;

namespace cooper {
/**
 * @brief Application Tcp Server
 */
class AppTcpServer : public NonCopyable {
public:
    using ModeType = uint8_t;
    using ProtocolType = uint32_t;
    using BusinessHandler = std::function<void(const TcpConnectionPtr&, const json&)>;
    using MediaHandler = std::function<void(const TcpConnectionPtr&, const char*, size_t len)>;

    explicit AppTcpServer(uint16_t port = 8888, bool pingPong = true, double pingPongInterval = 10,
                          size_t pingPongTimeout = 3);

    ~AppTcpServer();

    /**
     * @brief set mode
     * @param mode
     */
    void setMode(ModeType mode);

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
     * @brief register business handler
     * @param type
     * @param handler
     */
    void registerBusinessHandler(ProtocolType type, const BusinessHandler& handler);

    /**
     * @brief register media handler
     * @param type
     * @param handler
     */
    void registerMediaHandler(ProtocolType type, const MediaHandler& handler);

    /**
     * @brief set connection callback
     * @param cb
     */
    void setConnectionCallback(const ConnectionCallback& cb);

    /**
     * set sock opt callback
     * @param cb
     */
    void setSockOptCallback(const SockOptCallback& cb);

private:
    static void resetPingPongEntry(const TcpConnectionPtr& connPtr);

    void recvMsgCallback(const TcpConnectionPtr& conn, MsgBuffer* buffer);
    void recvBusinessMsgCallback(const TcpConnectionPtr& conn, MsgBuffer* buffer);
    void recvMediaMsgCallback(const TcpConnectionPtr& conn, MsgBuffer* buffer);

    // 1: use BusinessHandler, 2: use MediaHandler
    ModeType mode_ = 1;
    EventLoopThread loopThread_;
    std::shared_ptr<TcpServer> server_;
    std::unordered_map<ProtocolType, BusinessHandler> businessHandlers_;
    std::unordered_map<ProtocolType, MediaHandler> mediaHandlers_;
    bool pingPong_;
    double pingPongInterval_;
    size_t pingPongTimeout_;
    std::unordered_map<TcpConnectionPtr, TimerId> timerIds_;
    std::shared_ptr<TimingWheel> timingWheel_;
    ConnectionCallback connectionCallback_;
    SockOptCallback sockOptCallback_;
};
}  // namespace cooper

#endif
