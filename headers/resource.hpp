#ifndef RESOURCE_HPP
# define RESOURCE_HPP

# include <unordered_set>
# include <string>

struct resource
{
    std::string                     target;
    /* Representation Metadata RFC7231/3.1. */
    std::string                     content_type;
    std::unordered_set<std::string> content_encoding;
    std::unordered_set<std::string> content_language;
    std::string                     content_location;
    std::unordered_set<std::string> allowed_methods;
    std::string                     content;
};

#endif