#ifndef util_Semaphore_hpp
#define util_Semaphore_hpp

#include <condition_variable>
#include <mutex>

namespace cooper {
class Semaphore {
public:
    explicit Semaphore(size_t count = 0) {
        m_count = count;
    }

    ~Semaphore() = default;

    void post(size_t n = 1) {
        std::unique_lock<std::recursive_mutex> lock(m_mutex);
        m_count += n;
        if (n == 1) {
            m_cond.notify_one();
        } else {
            m_cond.notify_all();
        }
    }

    void wait() {
        std::unique_lock<std::recursive_mutex> lock(m_mutex);
        while (m_count == 0) {
            m_cond.wait(lock);
        }
        --m_count;
    }

private:
    size_t m_count;
    std::recursive_mutex m_mutex;
    std::condition_variable_any m_cond;
};

}  // namespace cooper

#endif
