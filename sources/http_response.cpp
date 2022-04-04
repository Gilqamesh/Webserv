#include "http_response.hpp"

http_response::http_response()
    : reject(false), handled_by_cgi(false)
{

}

http_response::~http_response()
{

}

http_response http_response::reject_http_response()
{
    http_response res;
    res.reject = true;
    res.handled_by_cgi = false;
    res.header_fields["Connection"] = "close";
    return (res);
}

http_response http_response::cgi_response()
{
    http_response res;
    res.reject = false;
    res.handled_by_cgi = true;
    return (res);
}

http_response http_response::tooLargeResponse()
{
    http_response res;
    res.http_version = "HTTP/1.1";
    res.status_code = "413";
    res.reason_phrase = "Payload Too Large";
    res.header_fields["Connection"] = "close";
    res.header_fields["Content-Type"] = "text/html";
    res.header_fields["Content-Length"] = "0";
    return (res);
}
