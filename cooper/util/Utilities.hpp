#ifndef util_Utilities_hpp
#define util_Utilities_hpp

#include <functional>
#include <map>
#include <string>

namespace cooper {
/**
 * @brief cooper helper functions.
 *
 */
namespace utils {
/**
 * @brief Convert a wide string to a UTF-8.
 * @details UCS2 on Windows, UTF-32 on Linux & Mac
 *
 * @param str String to convert
 *
 * @return converted string.
 */
std::string toUtf8(const std::wstring& wstr);
/**
 * @brief Convert a UTF-8 string to a wide string.
 * @details UCS2 on Windows, UTF-32 on Linux & Mac
 *
 * @param str String to convert
 *
 * @return converted string.
 */
std::wstring fromUtf8(const std::string& str);

/**
 * @details Convert a wide string path with arbitrary directory separators
 * to a UTF-8 portable path for use with cooper.
 *
 * This is a helper, mainly for Windows and multi-platform projects.
 *
 * @note On Windows, backslash directory separators are converted to slash to
 * keep portable paths.
 *
 * @remarks On other OSes, backslashes are not converted to slash, since they
 * are valid characters for directory/file names.
 *
 * @param strPath Wide string path.
 *
 * @return std::string UTF-8 path, with slash directory separator.
 */
std::string fromWidePath(const std::wstring& strPath);
/**
 * @details Convert a UTF-8 path with arbitrary directory separator to a wide
 * string path.
 *
 * This is a helper, mainly for Windows and multi-platform projects.
 *
 * @note On Windows, slash directory separators are converted to backslash.
 * Although it accepts both slash and backslash as directory separator in its
 * API, it is better to stick to its standard.

 * @remarks On other OSes, slashes are not converted to backslashes, since they
 * are not interpreted as directory separators and are valid characters for
 * directory/file names.
 *
 * @param strUtf8Path Ascii path considered as being UTF-8.
 *
 * @return std::wstring path with, on windows, standard backslash directory
 * separator to stick to its standard.
 */
std::wstring toWidePath(const std::string& strUtf8Path);

/**
 * @details Convert an UTF-8 path to a native path.
 *
 * This is a helper for non-Windows OSes for multi-platform projects.
 *
 * Does nothing.
 *
 * @param strPath UTF-8 path.
 *
 * @return \p strPath.
 */
inline const std::string& toNativePath(const std::string& strPath) {
    return strPath;
}
/**
 * @details Convert an wide string path to a UTF-8 path.
 *
 * This is a helper, mainly for Windows and multi-platform projects.
 *
 * @note On Windows, backslash directory separators are converted to slash to
 * keep portable paths.

 * @warning On other OSes, backslashes are not converted to slash, since they
 * are valid characters for directory/file names.
 *
 * @param strPath wide string path.
 *
 * @return Generic path, with slash directory separators
 */
inline std::string toNativePath(const std::wstring& strPath) {
    return fromWidePath(strPath);
}
/**
 * @note NoOP on all OSes
 */
inline const std::string& fromNativePath(const std::string& strPath) {
    return strPath;
}
/**
 * @note fromWidePath() on all OSes
 */
// Convert on all systems
inline std::string fromNativePath(const std::wstring& strPath) {
    return fromWidePath(strPath);
}

/**
 * @brief Check if the name supplied by the SSL Cert matchs a FQDN
 * @param certName The name supplied by the SSL Cert
 * @param hostName The FQDN to match
 *
 * @return true if matches. false otherwise
 */
bool verifySslName(const std::string& certName, const std::string& hostname);

/**
 * @brief Returns the TLS backend used by cooper. Could be "None", "OpenSSL" or
 * "Botan"
 */
std::string tlsBackend();

/**
 * @brief Generates cryptographically secure random bytes
 * @param ptr Pointer to the buffer to fill
 * @param size Size of the buffer
 * @return true if successful, false otherwise
 *
 * @note This function really sholdn't fail, but it's possible that
 *
 *   - OpenSSL can't access /dev/urandom
 *   - Compiled with glibc that supports getentropy() but the kernel doesn't
 *
 * When using Botan or on *BSD/macOS, this function will always succeed.
 */
bool secureRandomBytes(void* ptr, size_t size);

bool isSpaceOrTab(char c);

std::pair<size_t, size_t> trim(const char* b, const char* e, size_t left, size_t right);

void split(const char* b, const char* e, char d, const std::function<void(const char*, const char*)>& fn);

bool isValidPath(const std::string& path);

bool isDir(const std::string& path);

bool isFile(const std::string& path);

std::string fileExtension(const std::string& path);

std::string findContentType(const std::string& path);

size_t getFileSize(const std::string& path);

std::string trimDoubleQuotesCopy(const std::string& s);

std::string trimCopy(const std::string& s);

void parseDispositionParams(const std::string& s, std::multimap<std::string, std::string>& params);

}  // namespace utils

}  // namespace cooper

#endif
