#include "Utilities.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <regex>
#if __cplusplus < 201103L || __cplusplus >= 201703L
#include <clocale>
#include <cstdlib>
#else  // __cplusplus
#include <codecvt>
#include <locale>
#endif  // __cplusplus

#include <openssl/rand.h>

#include <cassert>
#include <functional>
#include <limits>
#include <memory>

#include "cooper/util/Logger.hpp"

namespace cooper {
namespace utils {
std::string toUtf8(const std::wstring& wstr) {
    if (wstr.empty())
        return {};

    std::string strTo;
#if __cplusplus < 201103L || __cplusplus >= 201703L
    // Note: Introduced in c++11 and deprecated with c++17.
    // Revert to C99 code since there no replacement yet
    strTo.resize(3 * wstr.length(), 0);
    auto nLen = wcstombs(&strTo[0], wstr.c_str(), strTo.length());
    strTo.resize(nLen);
#else  // c++11 to c++14
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8conv;
    strTo = utf8conv.to_bytes(wstr);
#endif
    return strTo;
}
std::wstring fromUtf8(const std::string& str) {
    if (str.empty())
        return {};
    std::wstring wstrTo;
#if __cplusplus < 201103L || __cplusplus >= 201703L
    // Note: Introduced in c++11 and deprecated with c++17.
    // Revert to C99 code since there no replacement yet
    wstrTo.resize(str.length(), 0);
    auto nLen = mbstowcs(&wstrTo[0], str.c_str(), wstrTo.length());
    wstrTo.resize(nLen);
#else  // c++11 to c++14
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8conv;
    try {
        wstrTo = utf8conv.from_bytes(str);
    } catch (...)  // Should never fail if str valid UTF-8
    {
    }
#endif
    return wstrTo;
}

std::wstring toWidePath(const std::string& strUtf8Path) {
    auto wstrPath{fromUtf8(strUtf8Path)};
    return wstrPath;
}

std::string fromWidePath(const std::wstring& wstrPath) {
    auto& srcPath{wstrPath};
    return toUtf8(srcPath);
}

bool verifySslName(const std::string& certName, const std::string& hostname) {
    if (certName.find('*') == std::string::npos) {
        return certName == hostname;
    }

    size_t firstDot = certName.find('.');
    size_t hostFirstDot = hostname.find('.');
    size_t pos, len, hostPos, hostLen;

    if (firstDot != std::string::npos) {
        pos = firstDot + 1;
    } else {
        firstDot = pos = certName.size();
    }

    len = certName.size() - pos;

    if (hostFirstDot != std::string::npos) {
        hostPos = hostFirstDot + 1;
    } else {
        hostFirstDot = hostPos = hostname.size();
    }

    hostLen = hostname.size() - hostPos;

    // *. in the beginning of the cert name
    if (certName.compare(0, firstDot, "*") == 0) {
        return certName.compare(pos, len, hostname, hostPos, hostLen) == 0;
    }
    // * in the left most. but other chars in the right
    else if (certName[0] == '*') {
        // compare if `hostname` ends with `certName` but without the leftmost
        // should be fine as domain names can't be that long
        intmax_t hostnameIdx = hostname.size() - 1;
        intmax_t certNameIdx = certName.size() - 1;
        while (hostnameIdx >= 0 && certNameIdx != 0) {
            if (hostname[hostnameIdx] != certName[certNameIdx]) {
                return false;
            }
            hostnameIdx--;
            certNameIdx--;
        }
        if (certNameIdx != 0) {
            return false;
        }
        return true;
    }
    // * in the right of the first dot
    else if (firstDot != 0 && certName[firstDot - 1] == '*') {
        if (certName.compare(pos, len, hostname, hostPos, hostLen) != 0) {
            return false;
        }
        for (size_t i = 0; i < hostFirstDot && i < firstDot && certName[i] != '*'; i++) {
            if (hostname[i] != certName[i]) {
                return false;
            }
        }
        return true;
    }
    // else there's a * in  the middle
    else {
        if (certName.compare(pos, len, hostname, hostPos, hostLen) != 0) {
            return false;
        }
        for (size_t i = 0; i < hostFirstDot && i < firstDot && certName[i] != '*'; i++) {
            if (hostname[i] != certName[i]) {
                return false;
            }
        }
        intmax_t hostnameIdx = hostFirstDot - 1;
        intmax_t certNameIdx = firstDot - 1;
        while (hostnameIdx >= 0 && certNameIdx >= 0 && certName[certNameIdx] != '*') {
            if (hostname[hostnameIdx] != certName[certNameIdx]) {
                return false;
            }
            hostnameIdx--;
            certNameIdx--;
        }
        return true;
    }

    assert(false && "This line should not be reached in verifySslName");
    // should not reach
    return certName == hostname;
}

std::string tlsBackend() {
    return "openssl";
}

std::string toHexString(const void* data, size_t len) {
    std::string str;
    str.resize(len * 2);
    for (size_t i = 0; i < len; i++) {
        unsigned char c = ((const unsigned char*)data)[i];
        str[i * 2] = "0123456789ABCDEF"[c >> 4];
        str[i * 2 + 1] = "0123456789ABCDEF"[c & 0xf];
    }
    return str;
}

bool secureRandomBytes(void* data, size_t len) {
    // OpenSSL's RAND_bytes() uses int as the length parameter
    for (size_t i = 0; i < len; i += (std::numeric_limits<int>::max)()) {
        int fillSize = (int)(std::min)(len - i, (size_t)(std::numeric_limits<int>::max)());
        if (!RAND_bytes((unsigned char*)data + i, fillSize))
            return false;
    }
    return true;
}

bool isSpaceOrTab(char c) {
    return c == ' ' || c == '\t';
}

std::pair<size_t, size_t> trim(const char* b, const char* e, size_t left, size_t right) {
    while (b + left < e && isSpaceOrTab(b[left])) {
        left++;
    }
    while (right > 0 && isSpaceOrTab(b[right - 1])) {
        right--;
    }
    return std::make_pair(left, right);
}

void split(const char* b, const char* e, char d, const std::function<void(const char*, const char*)>& fn) {
    size_t i = 0;
    size_t beg = 0;

    while (e ? (b + i < e) : (b[i] != '\0')) {
        if (b[i] == d) {
            auto r = trim(b, e, beg, i);
            if (r.first < r.second) {
                fn(&b[r.first], &b[r.second]);
            }
            beg = i + 1;
        }
        i++;
    }

    if (i) {
        auto r = trim(b, e, beg, i);
        if (r.first < r.second) {
            fn(&b[r.first], &b[r.second]);
        }
    }
}
bool isValidPath(const std::string& path) {
    size_t level = 0;
    size_t i = 0;

    // Skip slash
    while (i < path.size() && path[i] == '/') {
        i++;
    }

    while (i < path.size()) {
        // Read component
        auto beg = i;
        while (i < path.size() && path[i] != '/') {
            i++;
        }

        auto len = i - beg;
        assert(len > 0);

        if (!path.compare(beg, len, ".")) {
        } else if (!path.compare(beg, len, "..")) {
            if (level == 0) {
                return false;
            }
            level--;
        } else {
            level++;
        }

        // Skip slash
        while (i < path.size() && path[i] == '/') {
            i++;
        }
    }

    return true;
}
bool isDir(const std::string& path) {
    struct stat st {};
    return stat(path.c_str(), &st) >= 0 && S_ISDIR(st.st_mode);
}

bool isFile(const std::string& path) {
    struct stat st {};
    return stat(path.c_str(), &st) >= 0 && S_ISREG(st.st_mode);
}
std::string fileExtension(const std::string& path) {
    std::smatch m;
    static auto re = std::regex("\\.([a-zA-Z0-9]+)$");
    if (std::regex_search(path, m, re)) {
        return m[1].str();
    }
    return {};
}
inline constexpr unsigned int str2tagCore(const char* s, size_t l, unsigned int h) {
    return (l == 0) ? h
                    : str2tagCore(s + 1, l - 1,
                                  // Unsets the 6 high bits of h, therefore no overflow happens
                                  (((std::numeric_limits<unsigned int>::max)() >> 6) & h * 33) ^
                                      static_cast<unsigned char>(*s));
}

inline unsigned int str2tag(const std::string& s) {
    return str2tagCore(s.data(), s.size(), 0);
}
namespace udl {

inline constexpr unsigned int operator"" _t(const char* s, size_t l) {
    return str2tagCore(s, l, 0);
}
}  // namespace udl

std::string findContentType(const std::string& path) {
    auto ext = fileExtension(path);
    using udl::operator""_t;
    switch (str2tag(ext)) {
        default:
            return "application/octet-stream";
        case "css"_t:
            return "text/css";
        case "csv"_t:
            return "text/csv";
        case "htm"_t:
        case "html"_t:
            return "text/html";
        case "js"_t:
        case "mjs"_t:
            return "text/javascript";
        case "txt"_t:
            return "text/plain";
        case "vtt"_t:
            return "text/vtt";
        case "apng"_t:
            return "image/apng";
        case "avif"_t:
            return "image/avif";
        case "bmp"_t:
            return "image/bmp";
        case "gif"_t:
            return "image/gif";
        case "png"_t:
            return "image/png";
        case "svg"_t:
            return "image/svg+xml";
        case "webp"_t:
            return "image/webp";
        case "ico"_t:
            return "image/x-icon";
        case "tif"_t:
        case "tiff"_t:
            return "image/tiff";
        case "jpg"_t:
        case "jpeg"_t:
            return "image/jpeg";
        case "mp4"_t:
            return "video/mp4";
        case "mpeg"_t:
            return "video/mpeg";
        case "webm"_t:
            return "video/webm";
        case "mp3"_t:
            return "audio/mp3";
        case "mpga"_t:
            return "audio/mpeg";
        case "weba"_t:
            return "audio/webm";
        case "wav"_t:
            return "audio/wave";
        case "otf"_t:
            return "font/otf";
        case "ttf"_t:
            return "font/ttf";
        case "woff"_t:
            return "font/woff";
        case "woff2"_t:
            return "font/woff2";
        case "7z"_t:
            return "application/x-7z-compressed";
        case "atom"_t:
            return "application/atom+xml";
        case "pdf"_t:
            return "application/pdf";
        case "json"_t:
            return "application/json";
        case "rss"_t:
            return "application/rss+xml";
        case "tar"_t:
            return "application/x-tar";
        case "xht"_t:
        case "xhtml"_t:
            return "application/xhtml+xml";
        case "xslt"_t:
            return "application/xslt+xml";
        case "xml"_t:
            return "application/xml";
        case "gz"_t:
            return "application/gzip";
        case "zip"_t:
            return "application/zip";
        case "wasm"_t:
            return "application/wasm";
    }
}

size_t getFileSize(const std::string& path) {
    struct stat st {};
    if (stat(path.c_str(), &st) < 0) {
        return 0;
    }
    return st.st_size;
}

}  // namespace utils
}  // namespace cooper
