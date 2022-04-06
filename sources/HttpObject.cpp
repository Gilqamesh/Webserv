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
    PRINT_HERE();
    main_vec = new std::vector<char>();
    PRINT_HERE();
    request_body = new std::string();
    PRINT_HERE();
}

HttpObject::~HttpObject()
{
    delete main_vec;
    delete request_body;
}
