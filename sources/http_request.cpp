#include "http_request.hpp"

http_request::http_request()
    : reject(false)
{

}

http_request::http_request(bool r)
    : reject(r)
{

}

http_request::http_request(const http_request &other)
    : method_token(other.method_token), target(other.target), protocol_version(other.protocol_version),
    header_fields(other.header_fields), reject(other.reject)
{

}

http_request &http_request::operator=(const http_request &other)
{
    if (this != &other)
    {
        method_token = other.method_token;
        target = other.target;
        protocol_version = other.protocol_version;
        header_fields = other.header_fields;
        reject = other.reject;
    }
    return (*this);
}

http_request::~http_request()
{

}

http_request http_request::reject_http_request(void)
{
    return (http_request(true));
}
