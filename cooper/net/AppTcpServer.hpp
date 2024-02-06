#ifndef net_AppTcpServer_hpp
#define net_AppTcpServer_hpp

#include <nlohmann/json.hpp>
#include <utility>

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
class AppTcpServer : public NonCopyable, public std::enable_shared_from_this<AppTcpServer> {
public:
    using ModeType = uint8_t;
    using ProtocolType = uint32_t;
    using BusinessHandler = std::function<void(const TcpConnectionPtr&, const json&)>;
    using MediaHandler = std::function<void(const TcpConnectionPtr&, const char*, size_t len)>;

    explicit AppTcpServer(uint16_t port = 8888, bool pingPong = true, size_t pingPongInterval = 10,
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
    void resetKickoffEntry(const TcpConnectionPtr& connPtr);

    void recvMsgCallback(const TcpConnectionPtr& conn, MsgBuffer* buffer);
    void recvBusinessMsgCallback(const TcpConnectionPtr& conn, MsgBuffer* buffer);
    void recvMediaMsgCallback(const TcpConnectionPtr& conn, MsgBuffer* buffer);

    class PingPongEntry {
    public:
        explicit PingPongEntry(size_t pingPongInterval, size_t pingPongTimeout,
                               const std::weak_ptr<TcpConnection>& conn, const std::weak_ptr<TimingWheel>& timingWheel,
                               const std::weak_ptr<AppTcpServer>& server) {
            pingPongInterval_ = pingPongInterval;
            pingPongTimeout_ = pingPongTimeout;
            conn_ = conn;
            timingWheel_ = timingWheel;
            server_ = server;
        }

        void reset() {
            conn_.reset();
            timingWheel_.reset();
            server_.reset();
        }

        ~PingPongEntry() {
            auto conn = conn_.lock();
            auto timingWheel = timingWheel_.lock();
            auto server = server_.lock();
            if (conn && timingWheel && conn->connected()) {
                json j;
                j["type"] = PING_TYPE;
                conn->sendJson(j);
                auto kickoffEntry = std::make_shared<KickoffEntry>(conn);
                timingWheel->insertEntry(pingPongTimeout_, kickoffEntry);
                server->kickoffEntries_[conn] = kickoffEntry;
                auto pingPongEntry =
                    std::make_shared<PingPongEntry>(pingPongInterval_, pingPongTimeout_, conn_, timingWheel_, server_);
                timingWheel->insertEntry(pingPongInterval_, pingPongEntry);
                server->pingPongEntries_[conn] = pingPongEntry;
            }
        }

    private:
        size_t pingPongInterval_;
        size_t pingPongTimeout_;
        std::weak_ptr<TcpConnection> conn_;
        std::weak_ptr<TimingWheel> timingWheel_;
        std::weak_ptr<AppTcpServer> server_;
    };

    class KickoffEntry {
    public:
        explicit KickoffEntry(const std::weak_ptr<TcpConnection>& conn) {
            conn_ = conn;
        }
        void reset() {
            conn_.reset();
        }
        ~KickoffEntry() {
            auto conn = conn_.lock();
            if (conn) {
                conn->forceClose();
            }
        }

    private:
        std::weak_ptr<TcpConnection> conn_;
    };

    ModeType mode_ = BUSINESS_MODE;
    bool pingPong_;
    size_t pingPongInterval_;
    size_t pingPongTimeout_;
    EventLoopThread loopThread_;
    std::shared_ptr<TcpServer> server_;
    std::unordered_map<EventLoop*, std::shared_ptr<TimingWheel>> timingWheelMap_;
    std::unordered_map<ProtocolType, BusinessHandler> businessHandlers_;
    std::unordered_map<ProtocolType, MediaHandler> mediaHandlers_;
    std::unordered_map<TcpConnectionPtr, std::weak_ptr<PingPongEntry>> pingPongEntries_;
    std::unordered_map<TcpConnectionPtr, std::weak_ptr<KickoffEntry>> kickoffEntries_;
    ConnectionCallback connectionCallback_;
    SockOptCallback sockOptCallback_;
};
}  // namespace cooper

#endif
