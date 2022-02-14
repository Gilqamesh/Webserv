#include "utils.hpp"
#include "header.hpp"
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <unordered_set>
#include <sys/time.h>
#ifndef OPEN_MAX
# define OPEN_MAX 1000
#endif
#ifndef BUFFER_SIZE
# define BUFFER_SIZE 10000
#endif

static int read_into_buffer(int fd, char **buffer)
{
    /* read into the buffer and null terminate it */
    int read_ret = read(fd, *buffer, BUFFER_SIZE);
    if (read_ret == -1) { /* read failed */
        std::free(*buffer);
        *buffer = NULL;
        TERMINATE("read failed in get_next_line");
    }
    if (read_ret == 0) { /* immediate EOF */
        std::free(*buffer);
        *buffer = NULL;
        return (1);
    }
    /* EOF might have been reached, null terminate the buffer at the correct position */
    (*buffer)[read_ret] = '\0';
    return (0);
}

std::string get_next_line(int fd)
{
    static char *buffers[OPEN_MAX] = { 0 };

    if (fd < 0 || fd >= OPEN_MAX)
        return (NULL);
    /* first time we use the specific buffer */
    if (buffers[fd] == NULL)
    {
        /*
        * +1 for the null terminating character
        * this also let's us track the end of the string
        */
        buffers[fd] = (char *)std::malloc(BUFFER_SIZE + 1);
        if (buffers[fd] == NULL)
            TERMINATE("malloc failed in get_next_line");
        if (read_into_buffer(fd, buffers + fd))
            return ("");
    }
    /* if we have nothing in the buffer then read into it, return NULL on immediate EOF */
    if (buffers[fd][0] == '\0' && read_into_buffer(fd, buffers + fd))
        return ("");
    /* check to see if we have a newline in the string */
    char *newline_index = std::strchr(buffers[fd], '\n');
    if (newline_index == NULL) /* no newline in the string */
    {
        /* return the concatanation of the string and gnl()
        * handle the buffer first to prepare for the next call
        */
        std::string tmp = std::string(buffers[fd]);
        buffers[fd][0] = '\0';
        return (tmp + get_next_line(fd));
    }
    /* there is a newline in the string
    * rearrange buffer
    * then return the line up to the newline included
    */
    std::string cur_line;
    for (char *cur = buffers[fd]; cur < newline_index; ++cur)
        cur_line += *cur;
    std::memmove(buffers[fd], newline_index + 1, BUFFER_SIZE - (newline_index - buffers[fd]));
    return (cur_line);
}

/*
* Based on this: https://regexr.com/
*
* !! Warning: 'pattern' must be properly formatted !!
* Usage
* works similar to regex, returns true only if the entire 'str'
* matches the 'pattern'
*
* Syntax
* a+            one or more
* a*            zero or more
* [abc]         any of a, b or c
* [^abc]        not a, b or c
* [a-g]         character between a & g
* c             match character c
* .             any character except newline
* \             escape the next character
* 
* Not implemented
// * a?            0 or 1
// * ^abc$         start / end of the string
// * a{5} a{2,}    exactly five, two or more
// * a{1,3}        between one & three
*/
bool match_pattern(const std::string &str, const std::string &pattern)
{
    std::string::const_reverse_iterator s = str.rbegin();
    std::string::const_reverse_iterator p = pattern.rbegin();
    while (s != str.rend() && p != pattern.rend())
    {
        if (p + 1 != pattern.rend() && *(p + 1) == '\\')
        {
            if (*s++ != *p++)
                return (false);
            ++p; /* skip over '\' */
        }
        else if (*p == '*' || *p == '+')
        {
            char current_control_character = *p;
            std::unordered_set<char> token_range;
            bool negate = false;
            ++p; /* skip over current control character */
            if (*p == ']') {
                while (*++p != '[')
                {
                    if (*(p + 1) == '[' && *p == '^')
                        negate = true;
                    else
                    {
                        token_range.insert(*p);
                        if (*(p + 1) == '\\')
                            ++p; /* skip over '\' */
                    }
                }
            } else {
                if (*p == '.') /* skip 's' until next rule in pattern matches */
                {
                    ++p; /* skip over '.' */
                    bool matched = true;
                    while (s != str.rend() && *s != '\n' && match_pattern(std::string(str.begin(), s.base()), std::string(pattern.begin(), p.base())) == false)
                    {
                        matched = false;
                        ++s;
                    }
                    if (current_control_character == '+' && matched == true)
                        return (false);
                    continue ;
                }
                token_range.insert(*p);
            }
            /* [a-g] */
            if (match_pattern(std::string(p.base() + negate, p.base() + 3 + negate), ".-."))
            {
                token_range.clear();
                for (char c = *(p.base() + negate); c != *(p.base() + 2 + negate); ++c)
                    token_range.insert(c);
            }
            ++p; /* skip '[' for next token */
            // bool first_time = true;
            /* check the current pattern in 'str' until the next rule in pattern matches */
            if (current_control_character == '+' && (negate == true ? token_range.count(*s) : token_range.count(*s) == 0))
                return (false);
            if (current_control_character == '+')
                ++s;
            while (s != str.rend() && (negate == true ? token_range.count(*s) == 0 : token_range.count(*s))
                && match_pattern(std::string(str.begin(), s.base()), std::string(pattern.begin(), p.base())) == false)
                ++s;
        }
        else if (*p == ']')
        {
            std::unordered_set<char> token_range;
            bool negate = false;
            while (*++p != '[')
            {
                if (*(p + 1) == '[' && *p == '^')
                    negate = true;
                else
                {
                    token_range.insert(*p);
                    if (*(p + 1) == '\\')
                        ++p; /* skip over '\' */
                }
            }
            /* [a-g] */
            if (match_pattern(std::string(p.base() + negate, p.base() + 3 + negate), ".-."))
            {
                token_range.clear();
                for (char c = *(p.base() + negate); c != *(p.base() + 2 + negate); ++c)
                    token_range.insert(c);
            }
            ++p; /* skip '[' for next token */
            if (negate == true ? token_range.count(*s++) : token_range.count(*s++) == 0)
                return (false); /* match failed */
        }
        else if (*p == '.')
        {
            if (*s++ == '\n')
                return (false);
            ++p;
        }
        else /* match a single character */
        {
            if (*p++ != *s++)
                return (false);
        }
    }
    return (s == str.rend() && p == pattern.rend());
}

/*
* Returns timestamp in microseconds since the Epoch
*/
unsigned long get_current_timestamp(void)
{
    struct timeval cur;

    if (gettimeofday(&cur, NULL) == -1)
        TERMINATE("gettimeofday failed");
    return (cur.tv_sec * 1000000 + cur.tv_usec);
}
