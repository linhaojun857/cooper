#ifndef util_Funcs_hpp
#define util_Funcs_hpp

#include <algorithm>
#include <cstdint>
#include <vector>
namespace cooper {
inline uint64_t hton64(uint64_t n) {
    static const int one = 1;
    static const char sig = *(char*)&one;
    if (sig == 0)
        return n;  // for big endian machine just return the input
    char* ptr = reinterpret_cast<char*>(&n);
    std::reverse(ptr, ptr + sizeof(uint64_t));
    return n;
}
inline uint64_t ntoh64(uint64_t n) {
    return hton64(n);
}
inline std::vector<std::string> splitString(const std::string& s, const std::string& delimiter,
                                            bool acceptEmptyString = false) {
    if (delimiter.empty())
        return std::vector<std::string>{};
    std::vector<std::string> v;
    size_t last = 0;
    size_t next = 0;
    while ((next = s.find(delimiter, last)) != std::string::npos) {
        if (next > last || acceptEmptyString)
            v.push_back(s.substr(last, next - last));
        last = next + delimiter.length();
    }
    if (s.length() > last || acceptEmptyString)
        v.push_back(s.substr(last));
    return v;
}
}  // namespace cooper

#endif
