#include "http_response.hpp"

http_response::http_response()
{

}

http_response::http_response(const http_response &other)
    : http_version(other.http_version), status_code(other.status_code),
    reason_phrase(other.reason_phrase), header_fields(other.header_fields)
{

}

http_response &http_response::operator=(const http_response &other)
{
    if (this != &other)
    {
        http_version = other.http_version;
        status_code = other.status_code;
        reason_phrase = other.reason_phrase;
        header_fields = other.header_fields;
        payload = other.payload;
    }
    return (*this);
}

http_response::~http_response()
{

}
