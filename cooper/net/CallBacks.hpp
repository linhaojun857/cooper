#ifndef net_CallBacks_hpp
#define net_CallBacks_hpp

#include <functional>
#include <memory>

namespace cooper {
enum class SSLError { kSSLHandshakeError, kSSLInvalidCertificate, kSSLProtocolError };
using TimerCallback = std::function<void()>;

// the data has been read to (buf, len)
class TcpConnection;
class MsgBuffer;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
// tcp server and connection callback
using RecvMessageCallback = std::function<void(const TcpConnectionPtr&, MsgBuffer*)>;
using ConnectionErrorCallback = std::function<void()>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, const size_t)>;
using SSLErrorCallback = std::function<void(SSLError)>;
using SockOptCallback = std::function<void(int)>;

}  // namespace cooper

#endif