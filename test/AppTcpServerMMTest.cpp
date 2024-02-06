#include <cooper/net/AppTcpServer.hpp>
#include <cooper/util/AsyncLogWriter.hpp>

using namespace cooper;

#define TEST 1

int main() {
    AsyncLogWriter writer;
    Logger::setLogLevel(Logger::kTrace);
    Logger::setOutputFunction(std::bind(&AsyncLogWriter::write, &writer, std::placeholders::_1, std::placeholders::_2),
                              std::bind(&AsyncLogWriter::flushAll, &writer));

    std::shared_ptr<AppTcpServer> server = std::make_shared<AppTcpServer>();
    server->setMode(MEDIA_MODE);

    server->setConnectionCallback([](const TcpConnectionPtr& connPtr) {
        if (connPtr->connected()) {
            LOG_DEBUG << "AppTcpServerTest new connection";
        } else if (connPtr->disconnected()) {
            LOG_DEBUG << "AppTcpServerTest connection disconnected";
        }
    });

    server->registerMediaHandler(TEST, [](const TcpConnectionPtr& conn, const char* buf, size_t len) {
        LOG_DEBUG << "recv " << std::string(buf, len);
    });

    server->start();
    return 0;
}
