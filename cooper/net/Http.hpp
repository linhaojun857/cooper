#ifndef net_Http_hpp
#define net_Http_hpp

#include <cooper/net/Socket.hpp>
#include <cooper/net/TcpConnection.hpp>
#include <set>
#include <string>
#include <unordered_map>

#define COOPER_VERSION "1.0"
#define KEEP_ALIVE_TIMEOUT 60
#define MAX_KEEP_ALIVE_REQUESTS 10

namespace cooper {

class Http {
public:
    static const std::set<std::string> methods;
};

/**
 * Http status.
 */
class HttpStatus {
public:
    /**
     * Continue.
     */
    static const HttpStatus CODE_100;  // Continue

    /**
     * Switching Protocols.
     */
    static const HttpStatus CODE_101;  // Switching

    /**
     * Processing.
     */
    static const HttpStatus CODE_102;  // Processing

    /**
     * OK.
     */
    static const HttpStatus CODE_200;  // OK

    /**
     * Created.
     */
    static const HttpStatus CODE_201;  // Created

    /**
     * Accepted.
     */
    static const HttpStatus CODE_202;  // Accepted

    /**
     * Non-Authoritative Information.
     */
    static const HttpStatus CODE_203;  // Non-Authoritative Information

    /**
     * No Content.
     */
    static const HttpStatus CODE_204;  // No Content

    /**
     * Reset Content.
     */
    static const HttpStatus CODE_205;  // Reset Content

    /**
     * Partial Content.
     */
    static const HttpStatus CODE_206;  // Partial Content

    /**
     * Multi-HttpStatus.
     */
    static const HttpStatus CODE_207;  // Multi-HttpStatus

    /**
     * IM Used.
     */
    static const HttpStatus CODE_226;  // IM Used

    /**
     * Multiple Choices.
     */
    static const HttpStatus CODE_300;  // Multiple Choices

    /**
     * Moved Permanently.
     */
    static const HttpStatus CODE_301;  // Moved Permanently

    /**
     * Moved Temporarily.
     */
    static const HttpStatus CODE_302;  // Moved Temporarily

    /**
     * See Other.
     */
    static const HttpStatus CODE_303;  // See Other

    /**
     * Not Modified.
     */
    static const HttpStatus CODE_304;  // Not Modified

    /**
     * Use Proxy.
     */
    static const HttpStatus CODE_305;  // Use Proxy

    /**
     * Reserved.
     */
    static const HttpStatus CODE_306;  // Reserved

    /**
     * Temporary Redirect.
     */
    static const HttpStatus CODE_307;  // Temporary Redirect

    /**
     * Bad Request.
     */
    static const HttpStatus CODE_400;  // Bad Request

    /**
     * Unauthorized.
     */
    static const HttpStatus CODE_401;  // Unauthorized

    /**
     * Payment Required.
     */
    static const HttpStatus CODE_402;  // Payment Required

    /**
     * Forbidden.
     */
    static const HttpStatus CODE_403;  // Forbidden

    /**
     * Not Found.
     */
    static const HttpStatus CODE_404;  // Not Found

    /**
     * Method Not Allowed.
     */
    static const HttpStatus CODE_405;  // Method Not Allowed

    /**
     * Not Acceptable.
     */
    static const HttpStatus CODE_406;  // Not Acceptable

    /**
     * Proxy Authentication Required.
     */
    static const HttpStatus CODE_407;  // Proxy Authentication Required

    /**
     * Request Timeout.
     */
    static const HttpStatus CODE_408;  // Request Timeout

    /**
     * Conflict.
     */
    static const HttpStatus CODE_409;  // Conflict

    /**
     * Gone
     */
    static const HttpStatus CODE_410;  // Gone

    /**
     * Length Required.
     */
    static const HttpStatus CODE_411;  // Length Required

    /**
     * Precondition Failed.
     */
    static const HttpStatus CODE_412;  // Precondition Failed

    /**
     * Request Entity Too Large.
     */
    static const HttpStatus CODE_413;  // Request Entity Too Large

    /**
     * Request-URI Too Large.
     */
    static const HttpStatus CODE_414;  // Request-URI Too Large

    /**
     * Unsupported Media Type.
     */
    static const HttpStatus CODE_415;  // Unsupported Media Type

    /**
     * Requested Range Not Satisfiable.
     */
    static const HttpStatus CODE_416;  // Requested Range Not Satisfiable

    /**
     * Expectation Failed.
     */
    static const HttpStatus CODE_417;  // Expectation Failed

    /**
     * I'm a Teapot (rfc7168 2.3.3)
     */
    static const HttpStatus CODE_418;  // I'm a teapot

    /**
     * Unprocessable Entity.
     */
    static const HttpStatus CODE_422;  // Unprocessable Entity

    /**
     * Locked.
     */
    static const HttpStatus CODE_423;  // Locked

    /**
     * Failed Dependency.
     */
    static const HttpStatus CODE_424;  // Failed Dependency

    /**
     * Unordered Collection.
     */
    static const HttpStatus CODE_425;  // Unordered Collection

    /**
     * Upgrade Required.
     */
    static const HttpStatus CODE_426;  // Upgrade Required

    /**
     * Precondition Required.
     */
    static const HttpStatus CODE_428;  // Precondition Required

    /**
     * Too Many Requests.
     */
    static const HttpStatus CODE_429;  // Too Many Requests

    /**
     * Request HttpHeader Fields Too Large.
     */
    static const HttpStatus CODE_431;  // Request HttpHeader Fields Too Large

    /**
     * Requested host unavailable.
     */
    static const HttpStatus CODE_434;  // Requested host unavailable

    /**
     * Close connection withot sending headers.
     */
    static const HttpStatus CODE_444;  // Close connection withot sending headers

    /**
     * Retry With.
     */
    static const HttpStatus CODE_449;  // Retry With

    /**
     * Unavailable For Legal Reasons.
     */
    static const HttpStatus CODE_451;  // Unavailable For Legal Reasons

    /**
     * Internal Server Error.
     */
    static const HttpStatus CODE_500;  // Internal Server Error

    /**
     * Not Implemented.
     */
    static const HttpStatus CODE_501;  // Not Implemented

    /**
     * Bad Gateway.
     */
    static const HttpStatus CODE_502;  // Bad Gateway

    /**
     * Service Unavailable.
     */
    static const HttpStatus CODE_503;  // Service Unavailable

    /**
     * Gateway Timeout.
     */
    static const HttpStatus CODE_504;  // Gateway Timeout

    /**
     * HTTP Version Not Supported.
     */
    static const HttpStatus CODE_505;  // HTTP Version Not Supported

    /**
     * Variant Also Negotiates.
     */
    static const HttpStatus CODE_506;  // Variant Also Negotiates

    /**
     * Insufficient Storage.
     */
    static const HttpStatus CODE_507;  // Insufficient Storage

    /**
     * Loop Detected.
     */
    static const HttpStatus CODE_508;  // Loop Detected

    /**
     * Bandwidth Limit Exceeded.
     */
    static const HttpStatus CODE_509;  // Bandwidth Limit Exceeded

    /**
     * Not Extended.
     */
    static const HttpStatus CODE_510;  // Not Extended

    /**
     * Network Authentication Required.
     */
    static const HttpStatus CODE_511;  // Network Authentication Required

    /**
     * Constructor.
     */
    HttpStatus() : code(0), description(nullptr) {
    }

    /**
     * Constructor.
     * @param pCode - status code.
     * @param pDesc - description.
     */
    HttpStatus(int pCode, const char* pDesc) : code(pCode), description(pDesc) {
    }

    /**
     * HttpStatus code.
     */
    int code;

    /**
     * Description.
     */
    const char* description;

    bool operator==(const HttpStatus& other) const {
        return this->code == other.code;
    }

    bool operator!=(const HttpStatus& other) const {
        return this->code != other.code;
    }
};

class HttpHeader {
public:
    /**
     * Possible values for headers.
     */
    class Value {
    public:
        static const char* const CONNECTION_CLOSE;
        static const char* const CONNECTION_KEEP_ALIVE;
        static const char* const CONNECTION_UPGRADE;

        static const char* const SERVER;
        static const char* const USER_AGENT;

        static const char* const TRANSFER_ENCODING_CHUNKED;
        static const char* const CONTENT_TYPE_APPLICATION_JSON;

        static const char* const EXPECT_100_CONTINUE;
    };

public:
    static const char* const ACCEPT;             // "Accept"
    static const char* const AUTHORIZATION;      // "Authorization"
    static const char* const WWW_AUTHENTICATE;   // "WWW-Authenticate"
    static const char* const CONNECTION;         // "Connection"
    static const char* const TRANSFER_ENCODING;  // "Transfer-Encoding"
    static const char* const CONTENT_ENCODING;   // "Content-Encoding"
    static const char* const CONTENT_LENGTH;     // "Content-Length"
    static const char* const CONTENT_TYPE;       // "Content-Type"
    static const char* const CONTENT_RANGE;      // "Content-Range"
    static const char* const RANGE;              // "Range"
    static const char* const HOST;               // "Host"
    static const char* const USER_AGENT;         // "User-Agent"
    static const char* const SERVER;             // "Server"
    static const char* const UPGRADE;            // "Upgrade"
    static const char* const CORS_ORIGIN;        // Access-Control-Allow-Origin
    static const char* const CORS_METHODS;       // Access-Control-Allow-Methods
    static const char* const CORS_HEADERS;       // Access-Control-Allow-Headers
    static const char* const CORS_MAX_AGE;       // Access-Control-Max-Age
    static const char* const ACCEPT_ENCODING;    // Accept-Encoding
    static const char* const EXPECT;             // Expect
};

struct ci {
    bool operator()(const std::string& s1, const std::string& s2) const {
        return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
                                            [](unsigned char c1, unsigned char c2) {
                                                return std::tolower(c1) == std::tolower(c2);
                                            });
    }
};

struct hash {
    size_t operator()(const std::string& str) const {
        std::string lowerStr = str;
        for (char& c : lowerStr) {
            c = static_cast<char>(std::tolower(c));
        }
        return std::hash<std::string>{}(lowerStr);
    }
};

struct ParseContext {
    std::shared_ptr<Socket> socketPtr;
};

using HttpPath = std::string;
using Headers = std::unordered_map<std::string, std::string, hash, ci>;
using Body = std::string;

class HttpRequest;

struct MultipartFormData {
    std::string name;
    std::string content;
    std::string filename;
    std::string contentType;
};

using MultipartFormDataMap = std::multimap<std::string, MultipartFormData>;

class MultipartFormDataParser {
public:
    MultipartFormDataParser() = default;

    void setBoundary(std::string&& boundary);

    bool parse(MsgBuffer* buffer, HttpRequest& request, ParseContext& context);

private:
    void clearFileInfo();

    static bool startWithCaseIgnore(const std::string& a, const std::string& b);

    const std::string dash_ = "--";
    const std::string crlf_ = "\r\n";
    std::string boundary_;
    std::string dashBoundaryCrlf_;
    std::string crlfDashBoundary_;

    size_t state_ = 0;
    MultipartFormData file_;
};

class HttpRequest {
    friend class HttpServer;

public:
    std::string getHeaderValue(const std::string& key) const;

private:
    bool parseRequestStartingLine();

    bool parseHeaders();

    bool parseBody(ParseContext& context);

    bool isMultipartFormData();

public:
    std::string method_;
    std::string path_;
    std::string version_;
    Headers headers_;
    Body body_;
    MultipartFormDataMap files;

private:
    TcpConnectionPtr conn_;
    MsgBuffer* buffer_;
};

class HttpResponse;
class HttpContentWriter {
public:
    friend class HttpServer;
    using Ptr = std::shared_ptr<HttpContentWriter>;

    HttpContentWriter(const std::string& file, const std::string& contentType) {
        file_ = file;
        size_ = 0;
        contentType_ = contentType;
    }

    void write(const TcpConnectionPtr& conn);

private:
    std::string file_;
    size_t size_;
    std::string contentType_;
};

class HttpResponse {
public:
    friend class HttpServer;
    HttpResponse() {
        version_ = "HTTP/1.1";
        statusCode_ = HttpStatus::CODE_200;
    }

public:
    std::string version_;
    HttpStatus statusCode_;
    Headers headers_;
    Body body_;

private:
    HttpContentWriter::Ptr contentWriter_;
};

using HttpHandler = std::function<void(const HttpRequest&, HttpResponse&)>;
using HttpRoutes = std::unordered_map<HttpPath, HttpHandler>;
}  // namespace cooper

#endif
