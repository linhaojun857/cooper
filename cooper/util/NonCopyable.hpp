#ifndef util_NonCopyable_hpp
#define util_NonCopyable_hpp

class NonCopyable {
protected:
    NonCopyable() {
    }
    ~NonCopyable() {
    }
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) noexcept(true) = default;
    NonCopyable& operator=(NonCopyable&&) noexcept(true) = default;
};

#endif
