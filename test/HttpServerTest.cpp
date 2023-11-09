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

int main() {
    AsyncLogWriter writer;
    Logger::setLogLevel(Logger::kTrace);
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
    Headers headers;
    server.setFileAuthCallback([](const std::string& path) {
        LOG_DEBUG << "file path: " << path;
        return true;
    });
    server.addMountPoint("/static/", "/home/linhaojun/cpp-code/cooper/test/static", headers);
    server.addEndpoint("GET", "/hello", [](const HttpRequest& req, HttpResponse& resp) {
        resp.body_ =
            "<html>"
            "   <body>"
            "       <h1>Hello, world</h1>"
            "   </body>"
            "</html>";
    });
    server.addEndpoint("POST", "/person/add", [&mysql](const HttpRequest& req, HttpResponse& resp) {
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
    server.addEndpoint("POST", "/person/delete", [&mysql](const HttpRequest& req, HttpResponse& resp) {
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
    server.addEndpoint("POST", "/person/update", [&mysql](const HttpRequest& req, HttpResponse& resp) {
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
    server.addEndpoint("GET", "/person", [&mysql](const HttpRequest& req, HttpResponse& resp) {
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
    server.addEndpoint("GET", "/test1", [&mysql](const HttpRequest& req, HttpResponse& resp) {
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
    server.addEndpoint("POST", "/testMultiPart", [](const HttpRequest& req, HttpResponse& resp) {
        for (const auto& item : req.files_) {
            LOG_INFO << "\n"
                     << "name: " << item.second.name << "\n"
                     << "content: " << item.second.content << "\n"
                     << "filename: " << item.second.filename << "\n";
        }
        json j_resp;
        j_resp["code"] = 200;
        j_resp["msg"] = "success";
        resp.body_ = j_resp.dump();
    });
    server.addEndpoint("POST", "/uploadFile", [](const HttpRequest& req, HttpResponse& resp) {
        LOG_DEBUG << "content-length: " << req.getHeaderValue("content-length");
        for (const auto& item : req.files_) {
            LOG_DEBUG << "\n"
                      << "name: " << item.second.name << "\n"
                      << "filename: " << item.second.filename << "\n"
                      << "fileSize: " << item.second.content.size() << "\n";
        }
        auto iter = req.files_.find("test_file");
        if (iter != req.files_.end()) {
            std::string filename = "/home/linhaojun/cpp-code/cooper/test/static/" + iter->second.filename;
            int fd = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                LOG_ERROR << "open file failed";
                return;
            }
            write(fd, iter->second.content.data(), iter->second.content.size());
            close(fd);
        }
        json j_resp;
        j_resp["code"] = 200;
        j_resp["msg"] = "success";
        resp.body_ = j_resp.dump();
    });
    server.start();
    return 0;
}
