#include "header.hpp"
#include "CGI.hpp"
#include <vector>

CGI::CGI(int pipe[2], const std::string &payload)
    : m_payload(payload)
{
    m_pipe[0] = pipe[0];
    m_pipe[1] = pipe[1];
    /* Set up meta_variables RFC3875/4.1., some of this comes from the calling server */
    meta_variables["GATEWAY_INTERFACE"] = "CGI/1.1";
    meta_variables.insert(std::pair<std::string, std::string>("GATEWAY_INTERFACE", "CGI/1.1"));
}

CGI::~CGI()
{

}

void CGI::add_meta_variable(const std::string &key, const std::string &value)
{
    meta_variables[key] = value;
}

void CGI::execute(void)
{
    pid_t pid;
    if ((pid = fork()) == -1)
        TERMINATE("fork failed");
    if (pid == 0)
    {
        close(m_pipe[READ_END]);
        int tmp_pipe[2];
        if (pipe(tmp_pipe) == -1)
            TERMINATE("pipe failed");
        write(tmp_pipe[WRITE_END], m_payload.data(), m_payload.length());
        close(tmp_pipe[WRITE_END]);
        if (dup2(tmp_pipe[READ_END], STDIN_FILENO) == -1)
            TERMINATE("dup2 failed");
        close(tmp_pipe[READ_END]);
        std::vector<char *> args;
        args.push_back(&meta_variables["PATH_TRANSLATED"].at(0));
        args.push_back(&meta_variables["PATH_INFO"].at(0));
        args.push_back(NULL);
        std::vector<char *> env;
        for (std::unordered_map<std::string, std::string>::iterator it = meta_variables.begin(); it != meta_variables.end(); ++it)
        {
            char *tmp = (char *)std::malloc(it->second.length() + 1);
            tmp[it->second.length()] = '\0';
            env.push_back(tmp);
        }
        env.push_back(NULL);
        if (dup2(m_pipe[WRITE_END], STDOUT_FILENO) == -1)
            TERMINATE("dup2 failed");
        close(m_pipe[WRITE_END]);
        if (execve(&meta_variables["PATH_TRANSLATED"].at(0), args.data(), env.data()) == -1)
            TERMINATE("execve failed");
    }
}
