#include <fcntl.h>

#include <cooper/net/HttpServer.hpp>
#include <cooper/util/AsyncLogWriter.hpp>
#include <dbng.hpp>
#include <mysql.hpp>
#include <nlohmann/json.hpp>

using namespace cooper;
using namespace nlohmann;
using namespace ormpp;

struct Person {
    int id{};
    std::string name;
    std::optional<int> age;
};
REFLECTION(Person, id, name, age)

struct Course {
    int id{};
    std::string name;
};
REFLECTION(Course, id, name)

std::string getCurrentTime() {
    time_t now = time(nullptr);
    tm* tm = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return buf;
}

int main() {
    AsyncLogWriter writer;
    Logger::setLogLevel(Logger::kError);
    Logger::setOutputFunction(std::bind(&AsyncLogWriter::write, &writer, std::placeholders::_1, std::placeholders::_2),
                              std::bind(&AsyncLogWriter::flushAll, &writer));
    dbng<mysql> mysql;
    bool ret;
    ret = mysql.connect("172.18.48.1", "root", "20030802", "test");
    if (!ret) {
        LOG_ERROR << "connect mysql failed";
    }
    mysql.create_datatable<Person>(ormpp_auto_key{"id"});
    mysql.create_datatable<Course>(ormpp_auto_key{"id"});
    mysql.execute(
        "CREATE TABLE IF NOT EXISTS p_c (\n"
        "\tid INT PRIMARY KEY AUTO_INCREMENT,\n"
        "\tperson_id INT,\n"
        "\tcourse_id INT\n"
        ");");
    HttpServer server;
    server.setKeepAliveTimeout(60);
    server.setMaxKeepAliveRequests(1000);
    Headers headers;
    server.setFileAuthCallback([](const std::string& path) {
        LOG_DEBUG << "file path: " << path;
        return true;
    });
    server.addMountPoint("/static/", "/home/linhaojun/cpp-code/cooper/test/static", headers);
    server.addEndpoint("GET", "/hello", [](HttpRequest& req, HttpResponse& resp) {
        resp.body_ =
            "<html>"
            "   <body>"
            "       <h1>Hello, world</h1>"
            "   </body>"
            "</html>";
    });
    server.addEndpoint("GET", "/hello1", [](HttpRequest& req, HttpResponse& resp) {
        json j;
        j["code"] = 20000;
        j["msg"] = "Hello World!";
        resp.body_ = j.dump();
    });
    server.addEndpoint("POST", "/person/add", [&mysql](HttpRequest& req, HttpResponse& resp) {
        auto j = json::parse(req.body_);
        LOG_INFO << "json: \n" << j.dump();
        Person p;
        p.name = j["name"];
        p.age = j["age"];
        auto ret = mysql.insert(p);
        if (ret) {
            LOG_INFO << "insert success";
        } else {
            LOG_INFO << "insert failed";
        }
        json j_resp;
        j_resp["code"] = 200;
        j_resp["msg"] = "success";
        resp.body_ = j_resp.dump();
    });
    server.addEndpoint("POST", "/person/delete", [&mysql](HttpRequest& req, HttpResponse& resp) {
        auto j = json::parse(req.body_);
        LOG_INFO << "json: \n" << j.dump();
        Person p;
        p.id = j["id"];
        auto ret = mysql.delete_records<Person>("id = " + std::to_string(p.id));
        if (ret) {
            LOG_INFO << "delete success";
        } else {
            LOG_INFO << "delete failed";
        }
        json j_resp;
        j_resp["code"] = 200;
        j_resp["msg"] = "success";
        resp.body_ = j_resp.dump();
    });
    server.addEndpoint("POST", "/person/update", [&mysql](HttpRequest& req, HttpResponse& resp) {
        auto j = json::parse(req.body_);
        LOG_INFO << "json: \n" << j.dump();
        Person p;
        p.id = j["id"];
        p.name = j["name"];
        p.age = j["age"];
        auto ret = mysql.update(p);
        if (ret) {
            LOG_INFO << "update success";
        } else {
            LOG_INFO << "update failed";
        }
        json j_resp;
        j_resp["code"] = 200;
        j_resp["msg"] = "success";
        resp.body_ = j_resp.dump();
    });
    server.addEndpoint("GET", "/person", [&mysql](HttpRequest& req, HttpResponse& resp) {
        auto persons = mysql.query<Person>();
        json j_resp;
        j_resp["code"] = 200;
        j_resp["msg"] = "success";
        for (auto& p : persons) {
            json j;
            j["id"] = p.id;
            j["name"] = p.name;
            j["age"] = p.age.value_or(0);
            j_resp["data"].push_back(j);
        }
        resp.body_ = j_resp.dump();
    });
    server.addEndpoint("GET", "/test1", [&mysql](HttpRequest& req, HttpResponse& resp) {
        auto results = mysql.query<std::tuple<std::string, std::string>>(
            "SELECT\n"
            "\tperson.NAME AS person_name,\n"
            "\tcourse.NAME AS course_name \n"
            "FROM\n"
            "\tperson\n"
            "\tLEFT JOIN p_c ON person.id = p_c.person_id\n"
            "\tLEFT JOIN course ON p_c.course_id = course.id;");
        json j_resp;
        j_resp["code"] = 200;
        j_resp["msg"] = "success";
        if (!results.empty()) {
            for (auto& result : results) {
                json j;
                j["person_name"] = std::get<0>(result);
                j["course_name"] = std::get<1>(result);
                j_resp["data"].push_back(j);
            }
        }
        resp.body_ = j_resp.dump();
    });
    server.addEndpoint("POST", "/testMultiPart", [](HttpRequest& req, HttpResponse& resp) {
        MultiPartWriteCallbackMap writeCallbacks;
        std::string content;
        writeCallbacks["test_name"] = [&](const MultipartFormData& file, const char* data, size_t len, int flag) {
            if (flag == FLAG_CONTENT) {
                content.append(data, len);
            } else {
                LOG_DEBUG << "error flag";
            }
        };
        if (!req.parseMultiPartFormData(writeCallbacks)) {
            LOG_ERROR << "parse multi part form data failed";
            json j_resp;
            j_resp["code"] = 500;
            j_resp["msg"] = "error";
            resp.body_ = j_resp.dump();
            return;
        }
        LOG_DEBUG << "content: " << content;
        json j_resp;
        j_resp["code"] = 200;
        j_resp["msg"] = "success";
        resp.body_ = j_resp.dump();
    });
    server.addEndpoint("POST", "/uploadFile", [](HttpRequest& req, HttpResponse& resp) {
        MultiPartWriteCallbackMap writeCallbacks;
        int fd;
        std::string filename;
        writeCallbacks["test_file"] = [&](const MultipartFormData& file, const char* data, size_t len, int flag) {
            if (flag == FLAG_FILENAME) {
                filename = "/home/linhaojun/cpp-code/cooper/test/static/" + file.filename;
                fd = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
                if (fd < 0) {
                    LOG_ERROR << "open file failed";
                    return;
                }
            } else if (flag == FLAG_CONTENT) {
                if (fd > 0) {
                    write(fd, data, len);
                }
            } else {
                LOG_DEBUG << "error flag";
            }
        };
        LOG_DEBUG << "transfer begin time: " << getCurrentTime();
        if (!req.parseMultiPartFormData(writeCallbacks)) {
            LOG_ERROR << "parse multi part form data failed";
            if (access(filename.c_str(), F_OK) == 0) {
                remove(filename.c_str());
            }
            if (fd > 0) {
                close(fd);
            }
            json j_resp;
            j_resp["code"] = 500;
            j_resp["msg"] = "error";
            resp.body_ = j_resp.dump();
            return;
        }
        if (fd > 0) {
            close(fd);
        }
        LOG_DEBUG << "transfer end time: " << getCurrentTime();
        LOG_DEBUG << "filename: " << filename;
        json j_resp;
        j_resp["code"] = 200;
        j_resp["msg"] = "success";
        resp.body_ = j_resp.dump();
    });
    server.start(10);
    return 0;
}
