#include "Http.hpp"

#include <regex>

#include "cooper/util/Logger.hpp"
#include "cooper/util/Utilities.hpp"

namespace cooper {
const std::set<std::string> Http::methods = {"GET",     "HEAD",    "POST",  "PUT",   "DELETE",
                                             "CONNECT", "OPTIONS", "TRACE", "PATCH", "PRI"};

const HttpStatus HttpStatus::CODE_100(100, "Continue");
const HttpStatus HttpStatus::CODE_101(101, "Switching");
const HttpStatus HttpStatus::CODE_102(102, "Processing");

const HttpStatus HttpStatus::CODE_200(200, "OK");
const HttpStatus HttpStatus::CODE_201(201, "Created");
const HttpStatus HttpStatus::CODE_202(202, "Accepted");
const HttpStatus HttpStatus::CODE_203(203, "Non-Authoritative Information");
const HttpStatus HttpStatus::CODE_204(204, "No Content");
const HttpStatus HttpStatus::CODE_205(205, "Reset Content");
const HttpStatus HttpStatus::CODE_206(206, "Partial Content");
const HttpStatus HttpStatus::CODE_207(207, "Multi-HttpStatus");
const HttpStatus HttpStatus::CODE_226(226, "IM Used");

const HttpStatus HttpStatus::CODE_300(300, "Multiple Choices");
const HttpStatus HttpStatus::CODE_301(301, "Moved Permanently");
const HttpStatus HttpStatus::CODE_302(302, "Moved Temporarily");
const HttpStatus HttpStatus::CODE_303(303, "See Other");
const HttpStatus HttpStatus::CODE_304(304, "Not Modified");
const HttpStatus HttpStatus::CODE_305(305, "Use Proxy");
const HttpStatus HttpStatus::CODE_306(306, "Reserved");
const HttpStatus HttpStatus::CODE_307(307, "Temporary Redirect");

const HttpStatus HttpStatus::CODE_400(400, "Bad Request");
const HttpStatus HttpStatus::CODE_401(401, "Unauthorized");
const HttpStatus HttpStatus::CODE_402(402, "Payment Required");
const HttpStatus HttpStatus::CODE_403(403, "Forbidden");
const HttpStatus HttpStatus::CODE_404(404, "Not Found");
const HttpStatus HttpStatus::CODE_405(405, "Method Not Allowed");
const HttpStatus HttpStatus::CODE_406(406, "Not Acceptable");
const HttpStatus HttpStatus::CODE_407(407, "Proxy Authentication Required");
const HttpStatus HttpStatus::CODE_408(408, "Request Timeout");
const HttpStatus HttpStatus::CODE_409(409, "Conflict");
const HttpStatus HttpStatus::CODE_410(410, "Gone");
const HttpStatus HttpStatus::CODE_411(411, "Length Required");
const HttpStatus HttpStatus::CODE_412(412, "Precondition Failed");
const HttpStatus HttpStatus::CODE_413(413, "Request Entity Too Large");
const HttpStatus HttpStatus::CODE_414(414, "Request-URI Too Large");
const HttpStatus HttpStatus::CODE_415(415, "Unsupported Media Type");
const HttpStatus HttpStatus::CODE_416(416, "Requested Range Not Satisfiable");
const HttpStatus HttpStatus::CODE_417(417, "Expectation Failed");
const HttpStatus HttpStatus::CODE_418(418, "I'm a Teapot");
const HttpStatus HttpStatus::CODE_422(422, "Unprocessable Entity");
const HttpStatus HttpStatus::CODE_423(423, "Locked");
const HttpStatus HttpStatus::CODE_424(424, "Failed Dependency");
const HttpStatus HttpStatus::CODE_425(425, "Unordered Collection");
const HttpStatus HttpStatus::CODE_426(426, "Upgrade Required");
const HttpStatus HttpStatus::CODE_428(428, "Precondition Required");
const HttpStatus HttpStatus::CODE_429(429, "Too Many Requests");
const HttpStatus HttpStatus::CODE_431(431, "Request HttpHeader Fields Too Large");
const HttpStatus HttpStatus::CODE_434(434, "Requested host unavailable");
const HttpStatus HttpStatus::CODE_444(444, "Close connection without sending headers");
const HttpStatus HttpStatus::CODE_449(449, "Retry With");
const HttpStatus HttpStatus::CODE_451(451, "Unavailable For Legal Reasons");

const HttpStatus HttpStatus::CODE_500(500, "Internal Server Error");
const HttpStatus HttpStatus::CODE_501(501, "Not Implemented");
const HttpStatus HttpStatus::CODE_502(502, "Bad Gateway");
const HttpStatus HttpStatus::CODE_503(503, "Service Unavailable");
const HttpStatus HttpStatus::CODE_504(504, "Gateway Timeout");
const HttpStatus HttpStatus::CODE_505(505, "HTTP Version Not Supported");
const HttpStatus HttpStatus::CODE_506(506, "Variant Also Negotiates");
const HttpStatus HttpStatus::CODE_507(507, "Insufficient Storage");
const HttpStatus HttpStatus::CODE_508(508, "Loop Detected");
const HttpStatus HttpStatus::CODE_509(509, "Bandwidth Limit Exceeded");
const HttpStatus HttpStatus::CODE_510(510, "Not Extended");
const HttpStatus HttpStatus::CODE_511(511, "Network Authentication Required");

const char* const HttpHeader::Value::CONNECTION_CLOSE = "close";
const char* const HttpHeader::Value::CONNECTION_KEEP_ALIVE = "keep-alive";
const char* const HttpHeader::Value::CONNECTION_UPGRADE = "Upgrade";
const char* const HttpHeader::Value::SERVER = "cooper/" COOPER_VERSION;
const char* const HttpHeader::Value::USER_AGENT = "cooper/" COOPER_VERSION;
const char* const HttpHeader::Value::TRANSFER_ENCODING_CHUNKED = "chunked";
const char* const HttpHeader::Value::CONTENT_TYPE_APPLICATION_JSON = "application/json";
const char* const HttpHeader::Value::EXPECT_100_CONTINUE = "100-continue";

const char* const HttpHeader::ACCEPT = "Accept";
const char* const HttpHeader::AUTHORIZATION = "Authorization";
const char* const HttpHeader::WWW_AUTHENTICATE = "WWW-Authenticate";
const char* const HttpHeader::CONNECTION = "Connection";
const char* const HttpHeader::TRANSFER_ENCODING = "Transfer-Encoding";
const char* const HttpHeader::CONTENT_ENCODING = "Content-Encoding";
const char* const HttpHeader::CONTENT_LENGTH = "Content-Length";
const char* const HttpHeader::CONTENT_TYPE = "Content-Type";
const char* const HttpHeader::CONTENT_RANGE = "Content-Range";
const char* const HttpHeader::RANGE = "Range";
const char* const HttpHeader::HOST = "Host";
const char* const HttpHeader::USER_AGENT = "User-Agent";
const char* const HttpHeader::SERVER = "Server";
const char* const HttpHeader::UPGRADE = "Upgrade";
const char* const HttpHeader::CORS_ORIGIN = "Access-Control-Allow-Origin";
const char* const HttpHeader::CORS_METHODS = "Access-Control-Allow-Methods";
const char* const HttpHeader::CORS_HEADERS = "Access-Control-Allow-Headers";
const char* const HttpHeader::CORS_MAX_AGE = "Access-Control-Max-Age";
const char* const HttpHeader::ACCEPT_ENCODING = "Accept-Encoding";
const char* const HttpHeader::EXPECT = "Expect";

bool parseMultipartBoundary(const std::string& contentType, std::string& boundary) {
    auto boundaryKeyword = "boundary=";
    auto pos = contentType.find(boundaryKeyword);
    if (pos == std::string::npos) {
        return false;
    }
    auto end = contentType.find(';', pos);
    auto beg = pos + strlen(boundaryKeyword);
    boundary = utils::trimDoubleQuotesCopy(contentType.substr(beg, end - beg));
    return !boundary.empty();
}

void MultipartFormDataParser::setBoundary(std::string&& boundary) {
    boundary_ = boundary;
    dashBoundaryCrlf_ = dash_ + boundary_ + crlf_;
    crlfDashBoundary_ = crlf_ + dash_ + boundary_;
}

bool MultipartFormDataParser::parse(cooper::MsgBuffer* buffer, cooper::HttpRequest& request, ParseContext& context) {
    MultipartFormDataMap::iterator cur;
    auto begin = buffer->peek();
    auto contentLengthStr = request.headers_[HttpHeader::CONTENT_LENGTH];
    size_t contentLength = -1;
    if (!contentLengthStr.empty()) {
        contentLength = std::stoi(contentLengthStr);
    }
    bool needToReadMore = false;
    int sockfd = context.socketPtr->fd();
    auto readMore = std::function<int()>([buffer, sockfd, &needToReadMore]() {
        int err;
        ssize_t n = buffer->readFd(sockfd, &err);
        if (n < 0) {
            if (errno == EPIPE || errno == ECONNRESET) {
                LOG_TRACE << "EPIPE or ECONNRESET, errno=" << errno << " fd=" << sockfd;
            }
            if (errno == EAGAIN) {
                LOG_TRACE << "EAGAIN, errno=" << errno << " fd=" << sockfd;
                return 2;
            }
            LOG_SYSERR << "read socket error";
            return -1;
        }
        needToReadMore = false;
        return 1;
    });
    while (true) {
        if (needToReadMore) {
            auto ret = readMore();
            if (ret == -1) {
                break;
            }
            if (ret == 2) {
                continue;
            }
        }
        switch (state_) {
            case 0: {
                if (buffer->readableBytes() < dashBoundaryCrlf_.size()) {
                    continue;
                }
                auto dashBoundaryCrlf = buffer->find(dashBoundaryCrlf_);
                if (!dashBoundaryCrlf) {
                    return false;
                }
                buffer->retrieveUntil(dashBoundaryCrlf + dashBoundaryCrlf_.size());
                state_ = 1;
                break;
            }
            case 1: {
                clearFileInfo();
                state_ = 2;
                break;
            }
            case 2: {
                if (buffer->readableBytes() < crlf_.size()) {
                    needToReadMore = true;
                    continue;
                }
                auto crlf = buffer->find(crlf_);
                while (crlf) {
                    if (crlf == buffer->peek()) {
                        cur = request.files.emplace(file_.name, file_);
                        buffer->retrieve(crlf_.size());
                        state_ = 3;
                        break;
                    }
                    static const std::string headerName = "content-type:";
                    const auto header = std::string(buffer->peek(), crlf - buffer->peek());
                    if (startWithCaseIgnore(header, headerName)) {
                        file_.contentType = utils::trimCopy(header.substr(headerName.size()));
                    } else {
                        static const std::regex reContentDisposition(R"~(^Content-Disposition:\s*form-data;\s*(.*)$)~",
                                                                     std::regex_constants::icase);
                        std::smatch m;
                        if (std::regex_match(header, m, reContentDisposition)) {
                            std::multimap<std::string, std::string> params;
                            utils::parseDispositionParams(m[1], params);
                            auto iter = params.find("name");
                            if (iter != params.end()) {
                                file_.name = iter->second;
                            } else {
                                return false;
                            }

                            iter = params.find("filename");
                            if (iter != params.end()) {
                                file_.filename = iter->second;
                            }
                        } else {
                            return false;
                        }
                    }
                    buffer->retrieve(crlf - buffer->peek() + crlf_.size());
                    crlf = buffer->find(crlf_);
                }
                if (state_ != 3) {
                    return false;
                }
                break;
            }
            case 3: {
                if (buffer->readableBytes() < crlfDashBoundary_.size()) {
                    needToReadMore = true;
                    continue;
                }
                auto crlfDashBoundary = buffer->find(crlfDashBoundary_);
                if (crlfDashBoundary) {
                    auto len = crlfDashBoundary - buffer->peek();
                    auto& content = cur->second.content;
                    if (content.size() + len > content.max_size()) {
                        return false;
                    }
                    content.append(buffer->peek(), len);
                    buffer->retrieve(len + crlfDashBoundary_.size());
                    state_ = 4;
                } else {
                    auto len = buffer->readableBytes() - crlfDashBoundary_.size();
                    if (len > 0) {
                        auto& content = cur->second.content;
                        if (content.size() + len > content.max_size()) {
                            return false;
                        }
                        content.append(buffer->peek(), len);
                        buffer->retrieve(len);
                    }
                    needToReadMore = true;
                }
                break;
            }
            case 4: {
                if (buffer->readableBytes() < crlf_.size()) {
                    needToReadMore = true;
                    continue;
                }
                auto crlf = buffer->find(crlf_);
                if (crlf == buffer->peek()) {
                    buffer->retrieve(crlf_.size());
                    state_ = 1;
                } else {
                    if (buffer->readableBytes() < dash_.size()) {
                        needToReadMore = true;
                        continue;
                    }
                    auto dash = buffer->find(dash_);
                    if (dash == buffer->peek()) {
                        buffer->retrieve(dash_.size());
                        if (contentLength != -1) {
                            buffer->retrieve(contentLength - (buffer->peek() - begin));
                        } else {
                            buffer->retrieveAll();
                        }
                        return true;
                    } else {
                        return false;
                    }
                }
                break;
            }
        }
    }
    return false;
}

void MultipartFormDataParser::clearFileInfo() {
    file_.name.clear();
    file_.filename.clear();
    file_.contentType.clear();
}

bool MultipartFormDataParser::startWithCaseIgnore(const std::string& a, const std::string& b) {
    if (a.size() < b.size()) {
        return false;
    }
    for (size_t i = 0; i < b.size(); i++) {
        if (::tolower(a[i]) != ::tolower(b[i])) {
            return false;
        }
    }
    return true;
}

bool HttpRequest::parseRequestStartingLine() {
    auto ret = buffer_->findCRLF();
    if (!ret) {
        return false;
    }
    auto line = buffer_->readUntil(ret);
    buffer_->retrieve(2);  // skip \r\n
    size_t count = 0;
    utils::split(line.data(), line.data() + line.size(), ' ', [&count, this](const char* b, const char* e) {
        if (count == 0) {
            method_ = std::string(b, e);
        } else if (count == 1) {
            path_ = std::string(b, e);
        } else if (count == 2) {
            version_ = std::string(b, e);
        }
        count++;
    });
    if (count != 3) {
        return false;
    }
    if (Http::methods.find(method_) == Http::methods.end()) {
        return false;
    }
    if (version_ != "HTTP/1.1" && version_ != "HTTP/1.0") {
        return false;
    }
    return true;
}
bool HttpRequest::parseHeaders() {
    while (true) {
        auto ret = buffer_->findCRLF();
        if (!ret) {
            return false;
        }
        auto line = buffer_->readUntil(ret);
        buffer_->retrieve(2);  // skip \r\n
        if (line.empty()) {
            break;
        }
        // parse header
        const char* begin = line.data();
        const char* end = line.data() + line.size();
        while (begin < end && utils::isSpaceOrTab(end[-1])) {
            end--;
        }
        auto pos = begin;
        while (pos < end && *pos != ':') {
            pos++;
        }
        if (pos == end) {
            return false;
        }
        auto key_end = pos;
        if (*pos++ != ':') {
            return false;
        }
        while (pos < end && utils::isSpaceOrTab(*pos)) {
            pos++;
        }
        if (pos < end) {
            auto key = std::string(begin, key_end);
            auto value = std::string(pos, end);
            headers_[key] = value;
        }
    }
    return true;
}

std::string HttpRequest::getHeaderValue(const std::string& key) const {
    auto iter = headers_.find(key);
    if (iter == headers_.end()) {
        return "";
    }
    return iter->second;
}

bool HttpRequest::parseBody(ParseContext& context) {
    if (isMultipartFormData()) {
        MultipartFormDataParser multipartFormDataParser;
        const auto& contentType = headers_[HttpHeader::CONTENT_TYPE];
        std::string boundary;
        if (!parseMultipartBoundary(contentType, boundary)) {
            return false;
        }
        multipartFormDataParser.setBoundary(std::move(boundary));
        return multipartFormDataParser.parse(buffer_, *this, context);
    } else {
        auto contentLength = headers_[HttpHeader::CONTENT_LENGTH];
        if (contentLength.empty()) {
            body_ = std::string(buffer_->peek(), buffer_->readableBytes());
            buffer_->retrieveAll();
            return true;
        }
        size_t len = std::stoul(contentLength);
        if (len > buffer_->readableBytes()) {
            return false;
        }
        body_ = buffer_->read(len);
        return true;
    }
}

bool HttpRequest::isMultipartFormData() {
    auto iter = headers_.find(HttpHeader::CONTENT_TYPE);
    if (iter == headers_.end()) {
        return false;
    }
    const auto& contentType = iter->second;
    return !contentType.rfind("multipart/form-data", 0);
}

void HttpContentWriter::write(const cooper::TcpConnectionPtr& conn) {
    if (file_.empty() || size_ == 0) {
        return;
    }
    conn->sendFile(file_.c_str(), 0, size_);
}

}  // namespace cooper
