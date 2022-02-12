#include "http_message.hpp"

http_message::http_message()
    : reject(false)
{

}

http_message::http_message(bool r)
    : reject(r)
{

}

http_message::http_message(const http_message &other)
    : method_token(other.method_token), target(other.target), protocol_version(other.protocol_version),
    header_fields(other.header_fields), reject(other.reject)
{

}

http_message &http_message::operator=(const http_message &other)
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

http_message::~http_message()
{

}

http_message http_message::reject_http_message(void)
{
    return (http_message(true));
}
