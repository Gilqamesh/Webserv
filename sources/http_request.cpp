#include "http_request.hpp"

http_request::http_request()
    : reject(false)
{

}

http_request::http_request(bool r)
    : reject(r)
{

}

http_request::~http_request()
{

}

http_request http_request::reject_http_request(void)
{
    return (http_request(true));
}

http_request http_request::chunked_http_request(void)
{
    return (http_request(false));
}
