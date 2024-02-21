#ifndef net_HttpServer_hpp
#define net_HttpServer_hpp

#include "cooper/net/EventLoopThread.hpp"
#include "cooper/net/Http.hpp"
#include "cooper/net/TcpServer.hpp"
#include "cooper/util/NonCopyable.hpp"

namespace cooper {

/**
 * @brief Http Server
 */
class HttpServer : public NonCopyable {
public:
    explicit HttpServer(uint16_t port = 8888);

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
     * @brief set keep alive timeout
     * @param timeout
     */
    void setKeepAliveTimeout(size_t timeout);

    /**
     * @brief set max keep alive requests
     * @param maxKeepAliveRequests
     */
    void setMaxKeepAliveRequests(int maxKeepAliveRequests);

    /**
     * @brief add end point
     * @param method
     * @param path
     * @param handler
     */
    void addEndpoint(const std::string& method, const std::string& path, const HttpHandler& handler);

    /**
     * @brief add mount point
     * @param mountPoint
     * @param dir
     * @param headers
     * @return
     */
    bool addMountPoint(const std::string& mountPoint, const std::string& dir, const Headers& headers);

    /**
     * @brief remove mount point
     * @param mountPoint
     * @return
     */
    bool removeMountPoint(const std::string& mountPoint);

    /**
     * @brief set file auth callback
     * @param cb
     */
    void setFileAuthCallback(const FileAuthCallback& cb);

private:
    void recvMsgCallback(const TcpConnectionPtr& conn, MsgBuffer* buffer);

    void handleRequest(HttpRequest& request, HttpResponse& response);

    bool handleFileRequest(const HttpRequest& request, HttpResponse& response);

    bool sendResponse(const TcpConnectionPtr& conn, HttpResponse& response) const;

private:
    EventLoopThread loopThread_;
    std::shared_ptr<TcpServer> server_;
    size_t keepAliveTimeout_{KEEP_ALIVE_TIMEOUT};
    int maxKeepAliveRequests_{MAX_KEEP_ALIVE_REQUESTS};
    HttpRoutes getRoutes_;
    HttpRoutes postRoutes_;
    struct MountPointEntry {
        std::string mountPoint;
        std::string baseDir;
        Headers headers;
    };
    std::vector<MountPointEntry> baseDirs_;
    FileAuthCallback fileAuthCallback_;
};

}  // namespace cooper

#endif
