#ifndef HTTPOBJECT_HPP
# define HTTPOBJECT_HPP

# include <vector>
# include <string>
# include "header.hpp"

struct HttpObject
{
    HttpObject();
    ~HttpObject();
    std::vector<std::string>    headerFields;
    bool                        found_content_length;
    bool                        finished_reading;
    bool                        header_is_parsed;
    bool                        chunked;
    bool                        is_post;
    bool                        cutConnection;
    size_t                      content_length;
    size_t                      readRequestPosition;
    size_t                      nOfBytesRead;
    std::string                 *request_body;
    std::vector<char>           *main_vec;
    long long                   chunks_size;
    std::string                 current_header_field;
    // http_response               *response;
};

#endif
