#include "http_response.hpp"

http_response::http_response()
    : reject(false)
{

}

http_response::~http_response()
{

}

http_response http_response::reject_http_response(void)
{
    http_response res;
    res.reject = true;
    return (res);
}
