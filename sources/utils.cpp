#include "utils.hpp"
#include "header.hpp"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <cstdlib>
#include <unordered_set>
#include <sys/time.h>
#ifndef OPEN_MAX
# define OPEN_MAX 1000
#endif

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

/* Returns 'str' where each alpha char is upper-cased
* Only with ASCII
*/
std::string to_upper(const std::string &str)
{
    std::string res;

    for (std::string::const_iterator cit = str.begin(); cit != str.end(); ++cit)
        res.push_back(::toupper(*cit));
    return (res);
}

bool match(const std::string &what, const std::string &pattern)
{
    regex_t re;
    if (regcomp(&re, pattern.c_str(), REG_EXTENDED | REG_NOSUB))
        TERMINATE("regcomp failed");
    int ret = regexec(&re, what.c_str(), (size_t)0, 0, 0);
    regfree(&re);
    return (ret == 0);
}

/* -------------------- get_next_line() ------------------------ */

static char	*ft_strjoin(char *s1, char *s2)
{
	char	*s_join;
	int		len_s1;
	int		len_s2;
	int		i;
	int		j;

	if (s1 == 0 || s2 == 0)
		return (0);
	len_s1 = strlen(s1);
	len_s2 = strlen(s2);
	s_join = (char *)malloc(sizeof(*s1) * (len_s1 + len_s2) + 1);
	if (s_join == NULL)
		return (NULL);
	i = 0;
	while (s1[i] != '\0')
	{
		s_join[i] = s1[i];
		i++;
	}
	j = 0;
	while (s2[j] != '\0')
		s_join[i++] = s2[j++];
	s_join[i] = '\0';
	free(s1);
	return (s_join);
}

static char	*get_temp(char *temp_old, int len_line)
{
	char	*temp_new;
	int		j;
	int		k;
	int		l;

	if (temp_old[len_line] == '\0')
		k = len_line;
	else
		k = len_line + 1;
	j = 0;
	l = k;
	while (temp_old[k++] != '\0')
		j++;
	temp_new = (char *)malloc(sizeof(char) * j + 1);
	if (temp_new == NULL)
		return (NULL);
	j = 0;
	while (temp_old[l] != '\0')
		temp_new[j++] = temp_old[l++];
	temp_new[j] = '\0';
	free(temp_old);
	return (temp_new);
}

static char	*get_line(char *text)
{
	char	*line;
	int		i;

	i = 0;
	while (text[i] != '\n' && text[i] != '\0')
		i++;
	line = (char *)malloc(sizeof(char) * i + 1);
	if (line == NULL)
		return (NULL);
	i = 0;
	while (text[i] != '\n' && text[i] != '\0')
	{
		line[i] = text[i];
		i++;
	}
	line[i] = '\0';
	return (line);
}

static char	*output(char *text)
{
	char		*line;
	static char	*temp;

	if (temp == NULL)
		temp = (char *)calloc(1, 1);
	if (temp == NULL)
		return (NULL);
	if (strlen(text) > 0)
		temp = ft_strjoin(temp, text);
	free(text);
	if (strlen(temp) == 0)
	{
		free(temp);
		temp = NULL;
		return (NULL);
	}
	line = get_line(temp);
	if (temp[strlen(line)] == '\n')
	{
		temp = get_temp(temp, strlen(line));
		return (ft_strjoin(line, (char *)"\n"));
	}
	else
	{
		free(temp);
		temp = NULL;
		return (line);
	}
}

char	*get_next_line(int fd)
{
	char		*buffer;
	char		*text;
	int			buff_size;

	buffer = (char *)malloc(sizeof(char) * 1 + 1);
	text = (char *)calloc(1, 1);
	if (text == NULL || buffer == NULL)
		return (NULL);
	buff_size = read(fd, buffer, 1);
	while (buff_size > 0)
	{
		buffer[buff_size] = '\0';
		if (strchr(buffer, '\n'))
		{
			text = ft_strjoin(text, buffer);
			break ;
		}
		else
		{
			text = ft_strjoin(text, buffer);
			buff_size = read(fd, buffer, 1);
		}
	}
	free(buffer);
	return (output(text));
}
