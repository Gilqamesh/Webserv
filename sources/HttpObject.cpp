#include "HttpObject.hpp"

HttpObject::HttpObject()
{
    found_content_length = false;
    finished_reading = false;
    header_is_parsed = false;
    chunked = false;
    is_post = false;
    cutConnection = false;
    content_length = 0;
    readRequestPosition = 0;
    nOfBytesRead = 0;
    chunks_size = 0;
    main_vec = new std::vector<char>();
    request_body = new std::string();
}

HttpObject::~HttpObject()
{
    delete main_vec;
    delete request_body;
}
