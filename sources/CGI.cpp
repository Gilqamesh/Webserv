#include "header.hpp"
#include "CGI.hpp"

CGI::CGI()
{
    /* Set up meta_variables RFC3875/4.1., some of this comes from the calling server */
    meta_variables["GATEWAY_INTERFACE"] = "CGI/1.1";
}

CGI::~CGI()
{

}

void CGI::add_meta_variable(const std::string &key, const std::string &value)
{
    meta_variables[key] = value;
}
