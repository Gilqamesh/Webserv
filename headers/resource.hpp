#ifndef RESOURCE_HPP
# define RESOURCE_HPP

# include <set>
# include <string>

struct resource
{
    std::string                     target;
    std::string                     path;
    /* Representation Metadata RFC7231/3.1. */
    std::string                     content_type;
    std::set<std::string>           content_encoding;
    std::set<std::string>           content_language;
    std::string                     content_location;
    std::set<std::string>           allowed_methods;
    std::string                     content;
    /* If the resource is static, then:
    *   easy to cache
    *   does not require cgi as there exist only one representation
    */
    bool                            is_static;
    std::string                     script_path; // this might just be the same as 'target'
    /* Optional */
    // bool                            needs_authentication;
};

#endif
