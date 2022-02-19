#ifndef CGI_HPP
# define CGI_HPP

# include <unordered_map>
# include <string>

class CGI
{
public:
    CGI();
    ~CGI();

    void add_meta_variable(const std::string&, const std::string&);
private:
    std::unordered_map<std::string, std::string>    meta_variables;
};

#endif
