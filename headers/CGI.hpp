#ifndef CGI_HPP
# define CGI_HPP

# include <map>
# include <string>

class CGI /* goal: cgi 1.1 complient */
{
public:
    CGI(int pipe[2], const std::string &payload);
    ~CGI();

    void add_meta_variable(const std::string &key, const std::string &value);
    void execute(void); /* executes the script with the configured meta_variables */
private:
    std::map<std::string, std::string>             meta_variables;
    int                                            m_pipe[2]; /* end point of communication with the process calling the cgi */
    std::string                                    m_payload;
};

#endif
