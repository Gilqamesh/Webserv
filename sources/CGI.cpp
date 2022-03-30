#include "header.hpp"
#include "CGI.hpp"
#include <vector>

CGI::CGI(int pipe[2], http_request *httpRequest)
    : request(httpRequest)
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
        pid_t pid2;
        int tmp_pipe2[2];
        if (pipe(tmp_pipe2) == -1)
            TERMINATE("pipe failed");
        if ((pid2 = fork()) == -1)
            TERMINATE("fork failed");
        if (pid2 == 0)
        {
            close(m_pipe[WRITE_END]);
            close(tmp_pipe2[READ_END]);
            int tmp_pipe[2];
            if (pipe(tmp_pipe) == -1)
                TERMINATE("pipe failed");
            write(tmp_pipe[WRITE_END], request->payload.data(), request->payload.length());
            close(tmp_pipe[WRITE_END]);
            if (dup2(tmp_pipe[READ_END], STDIN_FILENO) == -1)
                TERMINATE("dup2 failed");
            close(tmp_pipe[READ_END]);
            char *arg1 = strdup(meta_variables["PATH_TRANSLATED"].c_str());
            char **args = (char **)malloc(3 * sizeof(char *));
            args[0] = strdup("/usr/bin/php");
            args[1] = arg1;
            args[2] = NULL;
            
            char **env = (char **)malloc(sizeof(char *) * (1 + meta_variables.size()));
            env[meta_variables.size()] = NULL;
            size_t index = 0;
            for (std::map<std::string, std::string>::iterator it = meta_variables.begin(); it != meta_variables.end(); ++it)
            {
                if (it->second.back() == '\n')
                    it->second.pop_back();
                env[index++] = strdup(std::string(it->first + "=" + it->second).c_str());
            }
            env[index] = NULL;
            if (dup2(tmp_pipe2[WRITE_END], STDOUT_FILENO) == -1)
                TERMINATE("dup2 failed");
            close(tmp_pipe2[WRITE_END]);
            // for (int i = 0; env[i] != NULL; ++i)
            //     dprintf(STDERR_FILENO, "%s\n", env[i]);
            // dprintf(STDERR_FILENO, "arg0: %s\n", args[0]);
            // dprintf(STDERR_FILENO, "arg1: %s\n", args[1]);
            if (execve(args[0], args, env) == -1)
                TERMINATE("execve failed");
        }
        close(tmp_pipe2[WRITE_END]);
        wait(NULL);
        char buffer[10000];
        buffer[10000 - 1] = '\0';
        read(tmp_pipe2[READ_END], buffer, 10000);
        std::string cgiResponse = "HTTP/1.1 200 OK \nContent-Location: " + std::string("localhost") + request->target + "\n";
        cgiResponse += "Content-Type: text/html\n";
        cgiResponse += "Content-Length: " + std::to_string(strlen(buffer)) + "\n";
        cgiResponse += "Connection: close\n";
        cgiResponse += "\n";
        cgiResponse += buffer;
        write(m_pipe[WRITE_END], cgiResponse.data(), cgiResponse.size());
        close(m_pipe[WRITE_END]);
        exit(0);
    }
}
