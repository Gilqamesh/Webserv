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
        LOG_E(__LINE__);
        close(m_pipe[READ_END]);
        LOG_E(__LINE__);
        int tmp_pipe[2];
        LOG_E(__LINE__);
        if (pipe(tmp_pipe) == -1)
            TERMINATE("pipe failed");
        LOG_E(__LINE__);
        write(tmp_pipe[WRITE_END], m_payload.data(), m_payload.length());
        LOG_E(__LINE__);
        close(tmp_pipe[WRITE_END]);
        LOG_E(__LINE__);
        if (dup2(tmp_pipe[READ_END], STDIN_FILENO) == -1)
            TERMINATE("dup2 failed");
        LOG_E(__LINE__);
        close(tmp_pipe[READ_END]);
        LOG_E(__LINE__);
        std::vector<char *> args;
        LOG_E(__LINE__);
        args.push_back(&meta_variables["PATH_TRANSLATED"].at(0));
        LOG_E(__LINE__);
        args.push_back(&meta_variables["PATH_INFO"].at(0));
        LOG_E(__LINE__);
        args.push_back(NULL);
        LOG_E(__LINE__);
        std::vector<char *> env;
        LOG_E(__LINE__);
        for (std::map<std::string, std::string>::iterator it = meta_variables.begin(); it != meta_variables.end(); ++it)
        {
            LOG_E(__LINE__);
            char *tmp = (char *)std::malloc(it->first.length() + it->second.length() + 2); /* 1 for =, 1 for \0 */
            LOG_E(__LINE__);
            tmp[it->first.length() + it->second.length() + 1] = '\0';
            LOG_E(__LINE__);
            memcpy(tmp, it->first.c_str(), it->first.length());
            LOG_E(__LINE__);
            tmp[it->first.length()] = '=';
            LOG_E(__LINE__);
            memcpy(tmp + it->first.length() + 1, it->second.c_str(), it->second.length());
            LOG_E(__LINE__);
            env.push_back(tmp);
            LOG_E(__LINE__);
        }
        LOG_E(__LINE__);
        env.push_back(NULL);
        LOG_E(__LINE__);
        if (dup2(m_pipe[WRITE_END], STDOUT_FILENO) == -1)
            TERMINATE("dup2 failed");
        LOG_E(__LINE__);
        close(m_pipe[WRITE_END]);
        LOG_E(__LINE__);
        if (execve(&meta_variables["PATH_TRANSLATED"].at(0), args.data(), env.data()) == -1)
            TERMINATE("execve failed");
    }
}
