#ifndef CGI_HPP
# define CGI_HPP

# include <map>
# include <string>
# include "http_request.hpp"

class CGI /* goal: cgi 1.1 complient */
{
public:
    CGI(int pipe[2], http_request *httpRequest);
    ~CGI();
    std::string  out_file_name;

    void add_meta_variable(const std::string &key, const std::string &value);
    void execute(void); /* executes the script with the configured meta_variables */
private:
    http_request                                   *request;
    std::map<std::string, std::string>             meta_variables;
    int                                            m_pipe[2]; /* end point of communication with the process calling the cgi */
};

#endif
