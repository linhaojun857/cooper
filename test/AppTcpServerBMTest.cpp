#include <cooper/net/AppTcpServer.hpp>
#include <cooper/util/AsyncLogWriter.hpp>
#include <dbng.hpp>
#include <mysql.hpp>

using namespace cooper;
using namespace ormpp;

#define TEST 1

struct Person {
    int id{};
    std::string name;
    std::optional<int> age;
};
REFLECTION(Person, id, name, age)

int main() {
    AsyncLogWriter writer;
    Logger::setLogLevel(Logger::kTrace);
    Logger::setOutputFunction(std::bind(&AsyncLogWriter::write, &writer, std::placeholders::_1, std::placeholders::_2),
                              std::bind(&AsyncLogWriter::flushAll, &writer));
    AppTcpServer server;
    server.setMode(BUSINESS_MODE);
    dbng<mysql> mysql;
    bool ret;
    ret = mysql.connect("172.18.48.1", "root", "20030802", "test");
    if (!ret) {
        LOG_ERROR << "connect mysql failed";
    }
    mysql.create_datatable<Person>(ormpp_auto_key{"id"});
    server.setConnectionCallback([](const TcpConnectionPtr& connPtr) {
        if (connPtr->connected()) {
            LOG_DEBUG << "AppTcpServerTest new connection";
        } else if (connPtr->disconnected()) {
            LOG_DEBUG << "AppTcpServerTest connection disconnected";
        }
    });
    server.registerBusinessHandler(TEST, [&mysql](const TcpConnectionPtr& conn, const json& j) {
        Person p;
        p.name = j["name"];
        p.age = j["age"];
        auto ret = mysql.insert(p);
        if (ret) {
            LOG_INFO << "insert success";
        } else {
            LOG_INFO << "insert failed";
        }
        conn->send("hello, world");
    });
    server.start();
    return 0;
}