#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>

/*
* returns false if EOF
*/
std::pair<std::string, bool> get_next_line(int fd);

/* returns true if 'str' matches the 'pattern' */
bool match_pattern(const std::string &str, const std::string &pattern);

unsigned long get_current_timestamp(void);

std::string to_upper(const std::string &str);

#endif
