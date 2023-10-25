#include <cooper/util/AsyncLogWriter.hpp>
#include <cooper/util/Logger.hpp>
#include <cooper/util/ThreadPool.hpp>

using namespace cooper;

int main() {
    AsyncLogWriter writer;
    Logger::setLogLevel(Logger::kTrace);
    Logger::setOutputFunction(std::bind(&AsyncLogWriter::write, &writer, std::placeholders::_1, std::placeholders::_2),
                              std::bind(&AsyncLogWriter::flushAll, &writer));
    ThreadPool pool(10, "threadPool");
    for (int i = 0; i < 10000; ++i) {
        pool.addTask([i]() {
            LOG_INFO << "task " << i << " is running";
        });
    }
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
