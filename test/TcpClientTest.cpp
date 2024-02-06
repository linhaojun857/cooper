#include <cooper/net/TcpClient.hpp>
#include <nlohmann/json.hpp>

#define PING_TYPE 100
#define PONG_TYPE 200

using namespace cooper;
using namespace nlohmann;

using ProtocolType = uint32_t;

int main() {
    EventLoop loop;
    InetAddress serverAddr("127.0.0.1", 8888);
    std::shared_ptr<TcpClient> client = std::make_shared<TcpClient>(&loop, serverAddr, "TcpClient");
    client->setMessageCallback([](const TcpConnectionPtr& conn, MsgBuffer* buffer) {
        static int cnt = 5;
        if (cnt > 0) {
            auto packSize = *(static_cast<const uint32_t*>((void*)buffer->peek()));
            if (buffer->readableBytes() < packSize) {
                return;
            } else {
                buffer->retrieve(sizeof(packSize));
                auto str = buffer->read(packSize);
                auto j = json::parse(str);
                auto type = j["type"].get<ProtocolType>();
                if (type == PING_TYPE) {
                    LOG_DEBUG << "receive ping";
                    j["type"] = PONG_TYPE;
                    conn->sendJson(j);
                }
            }
            cnt--;
        }
    });
    client->connect();
    loop.loop();
}
