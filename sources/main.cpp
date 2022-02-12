#include "header.hpp"
#include "Utils.hpp"
#include "server.hpp"

int main(void)
{
    /* TEST match_pattern */
    LOG(match_pattern("hello", "hello"));
    LOG(match_pattern("llll", "[l]*"));
    LOG(match_pattern("llll", "[l]"));
    LOG(match_pattern("aaa aa aaaaaa", "[a]+ [a]+ [a]+"));
    LOG(match_pattern("a a", "[a] [a]"));
    LOG(match_pattern("a a", "a a"));
    LOG(match_pattern("aaa aa aaaaaa" + CRLF, "[a]+ [a]+ [a]+" + CRLF));
    LOG(match_pattern("aaa b caaabaa", "[abc]+ [abc]+ [abc]+"));
    LOG(match_pattern("hello", "..l.o"));
    LOG(match_pattern("hello", "c.l.o"));
    LOG(match_pattern("aaa aa aaaaa", "a*.[a]+ [^ ]*"));
    LOG(match_pattern("[hi", "\\[h\\."));
    LOG(match_pattern("[h.", "\\[h\\."));
    LOG(match_pattern("abc", "[a-d]"));
    LOG(match_pattern("abc", "[^a-d]"));
    LOG(match_pattern("abc", "[^d-e]*"));
    LOG(match_pattern("abc", "[^b]*.c"));

    LOG(match_pattern("GET / HTTP/1.1", "[^ ]* [^ ]* [^ ]*"));
    LOG(match_pattern("GET / HTTP/1.1\n", ".*"));
    LOG(match_pattern("GET / HTTP/1.1\n", ".*\n"));
    LOG(match_pattern("Host: 10.12.1.1:8080\n", "[^ \t:]*:[ \t]*[ -~]*[ \t]*"));
    LOG(match_pattern("Host: 10.12.1.1:8080", "[^ \t:]*:[ \t]*[ -~]*[ \t]*"));
    LOG(match_pattern("Host: 10.12.1.1:8080", "[^ \t:]*:[ \t]*.*[ \t]*"));
    LOG(match_pattern("Host: 10.12.1.1:8080" + CRLF, "[^ \t:]*:[ \t]*[ -~]*[ \t]*" + CRLF));
    LOG(match_pattern("Connection: keep-alive", "[^ \t:]*:[ \t]*[ -~]*[ \t]*"));
    LOG(match_pattern("Connection: keep", "[^ \t:]*:[ \t]*[ -~]*"));
}
