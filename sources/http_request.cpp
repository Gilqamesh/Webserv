#include "http_request.hpp"
#include "header.hpp"

http_request::http_request()
    : reject(false)
{
    chunked = false;
}

http_request::http_request(bool r)
    : reject(r)
{
    chunked = false;
}

http_request::~http_request()
{
    if (chunked == true)
    {
        PRINT_HERE();
        delete payload;
        PRINT_HERE();
    }
}

http_request http_request::reject_http_request(void)
{
    return (http_request(true));
}

http_request http_request::chunked_http_request(void)
{
    return (http_request(false));
}

http_request http_request::payload_too_large(void)
{
    http_request req;
    req.too_large = true;
    req.redirected = false;
    req.reject = false;
    req.chunked = false;
    return (req);
}
