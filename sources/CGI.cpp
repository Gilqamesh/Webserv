#include "header.hpp"
#include "CGI.hpp"
#include <vector>
#include "utils.hpp"
#include <sys/stat.h>

CGI::CGI(int pipe[2], http_request *httpRequest)
    : request(httpRequest)
{
    m_pipe[0] = pipe[0];
    m_pipe[1] = pipe[1];
    /* Set up meta_variables RFC3875/4.1., some of this comes from the calling server */
    meta_variables["GATEWAY_INTERFACE"] = "CGI/1.1";
    meta_variables.insert(std::pair<std::string, std::string>("GATEWAY_INTERFACE", "CGI/1.1"));
    LOG_E("Meta variables");
    for (std::map<std::string, std::string>::iterator it = meta_variables.begin(); it != meta_variables.end(); ++it)
    {
        LOG_E(it->first << ": " << it->second);
    }
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
        if ((pid2 = fork()) == -1)
            TERMINATE("fork failed");
        if (pid2 == 0)
        {
            close(m_pipe[WRITE_END]);
            int tmp_cgi_file_in = open("temp/temp_cgi_file_in", O_WRONLY | O_CREAT | O_TRUNC, 0777);
            if (tmp_cgi_file_in == -1)
                TERMINATE("failed to open temp/temp_cgi_file_in");
            write(tmp_cgi_file_in, request->payload.data(), request->payload.length());
            if (dup2(tmp_cgi_file_in, STDIN_FILENO) == -1)
                TERMINATE("dup2 failed");
            close(tmp_cgi_file_in);
            char *arg1 = strdup(meta_variables["SCRIPT_NAME"].c_str());
            // char *arg1 = strdup("cgi_tester");
            // char **args = (char **)malloc(3 * sizeof(char *));
            // args[0] = strdup("/usr/bin/php");
            // args[1] = arg1;
            // args[2] = NULL;
            char **args = (char **)malloc(3 * sizeof(char *));
            args[0] = arg1;
            char *arg2 = strdup(meta_variables["PATH_INFO"].c_str());
            args[1] = arg2; // cwd + request.target
            args[2] = NULL;
            char **env = (char **)malloc(sizeof(char *) * (1 + meta_variables.size()));
            env[meta_variables.size()] = NULL;
            size_t index = 0;
            LOG_E("Meta variables");
            for (std::map<std::string, std::string>::iterator it = meta_variables.begin(); it != meta_variables.end(); ++it)
            {
                if (it->second.back() == '\n')
                    it->second.pop_back();
                LOG(it->first << ": " << it->second);
                env[index++] = strdup(std::string(it->first + "=" + it->second).c_str());
            }
            env[index] = NULL;
            int tmp_cgi_file_out = open("temp/temp_cgi_file_out", O_WRONLY | O_CREAT | O_TRUNC, 0777);
            if (tmp_cgi_file_out == -1)
                TERMINATE("failed to opne temp/temp_cgi_file_out");
            if (dup2(tmp_cgi_file_out, STDOUT_FILENO) == -1)
                TERMINATE("dup2 failed");
            close(tmp_cgi_file_out);
            for (int i = 0; env[i] != NULL; ++i)
                dprintf(STDERR_FILENO, "%s\n", env[i]);
            dprintf(STDERR_FILENO, "arg0: %s\n", args[0]);
            dprintf(STDERR_FILENO, "arg1: %s\n", args[1]);
            if (execve(args[0], args, env) == -1)
                TERMINATE("execve failed");
        }
        wait(NULL);
        struct stat fileInfo;
        if (stat("temp/temp_cgi_file_out", &fileInfo) == -1)
            TERMINATE("stat failed on file temp/temp_cgi_file_out");
        std::string cgiResponse = "HTTP/1.1 200 OK \nContent-Location: " + std::string("localhost") + request->target + "\n";
        cgiResponse += "Content-Type: text/html\n";
        cgiResponse += "Content-Length: " + std::to_string(fileInfo.st_size) + "\n";
        cgiResponse += "Connection: close\n";
        cgiResponse += "\n";
        write(m_pipe[WRITE_END], cgiResponse.data(), cgiResponse.size());
        close(m_pipe[WRITE_END]);
        exit(0);
    }
}
