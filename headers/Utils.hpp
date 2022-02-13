#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>

/*
* Returns one line read from 'fd'
* If there was nothing read, returns with an empty string
*/
std::string get_next_line(int fd);

/* returns true if 'str' matches the 'pattern' */
bool match_pattern(const std::string &str, const std::string &pattern);

unsigned long get_current_timestamp(void);

#endif
