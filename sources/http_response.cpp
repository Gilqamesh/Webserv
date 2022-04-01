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
