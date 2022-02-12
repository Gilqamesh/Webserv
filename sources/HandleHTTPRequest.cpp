#include "HandleHTTPRequest.hpp"

HandleHTTPRequest::HandleHTTPRequest()
{

}

HandleHTTPRequest::HandleHTTPRequest(const http_message &message)
    : request_message(message), request_code()
{
    
}

HandleHTTPRequest::HandleHTTPRequest(const HandleHTTPRequest &other)
    : request_message(other.request_message), request_code(other.request_code)
{

}

HandleHTTPRequest &HandleHTTPRequest::operator=(const HandleHTTPRequest& other)
{
    if (this != &other)
    {
        request_message = other.request_message;
        request_code = other.request_code;
    }
    return (*this);
}

HandleHTTPRequest::~HandleHTTPRequest()
{

}

int HandleHTTPRequest::get_request_code(void)
{
    /* Controls
    * controls are request header fields (key-value pairs) that direct
    * specific handling of the request
    */
    handle_cache_control();
    handle_expect();
    handle_host();
    handle_max_forwards();
    handle_pragma();
    handle_range();
    handle_TE();
    return (request_code);
}

/* CONTROLS */

void HandleHTTPRequest::handle_cache_control(void)
{

}

void HandleHTTPRequest::handle_expect(void)
{

}

void HandleHTTPRequest::handle_host(void)
{

}

void HandleHTTPRequest::handle_max_forwards(void)
{

}

void HandleHTTPRequest::handle_pragma(void)
{

}

void HandleHTTPRequest::handle_range(void)
{

}

void HandleHTTPRequest::handle_TE(void)
{

}
