#include "header.hpp"
#include "CGI.hpp"
#include <vector>
#include "utils.hpp"

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
            int tmp_cgi_file = open("temp/temp_cgi_file", O_WRONLY | O_CREAT | O_TRUNC, 0777);
            if (tmp_cgi_file == -1)
                TERMINATE("failed to open temp/temp_cgi_file");
            write(tmp_cgi_file, request->payload.data(), request->payload.length());
            if (dup2(tmp_cgi_file, STDIN_FILENO) == -1)
                TERMINATE("dup2 failed");
            close(tmp_cgi_file);
            char *arg1 = strdup(meta_variables["PATH_TRANSLATED"].c_str());
            char **args = (char **)malloc(3 * sizeof(char *));
            args[0] = strdup("/usr/bin/php");
            args[1] = arg1;
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
        std::string cgiResponse = "HTTP/1.1 200 OK \nContent-Location: " + std::string("localhost") + request->target + "\n";
        cgiResponse += "Content-Type: text/html\n";
        // cgiResponse += "Content-Length: " + std::to_string(cgiReturnStr.size()) + "\n";
        cgiResponse += "Transfer-Encoding: chunked\n";
        // cgiResponse += "Content-Length: 0\n";
        cgiResponse += "Connection: close\n";
        cgiResponse += "\n";
        std::string cgiReturnStr;
        while (1)
        {
            // char buffer[10001];
            char *tmp = get_next_line(tmp_pipe2[READ_END]);
            if (tmp == NULL)
            {
                cgiReturnStr += "\r\n0\r\n\r\n";
                break ;
            }
            std::string tmpStr(tmp);
            free(tmp);
            std::stringstream ss;
            ss << std::hex << tmpStr.size();
            cgiReturnStr += ss.str() + "\r\n";
            if (tmpStr.size() == 0)
                break ;
            cgiReturnStr += tmpStr;
            // int readAmount = read(tmp_pipe2[READ_END], buffer, 10000);
            // if (readAmount == -1)
            //     TERMINATE("read failed");
            // buffer[readAmount] = '\0';
            // cgiReturnStr += buffer;
            // if (readAmount == 0)
            //     break ;
        }
        LOG_E("Chunked message: " << cgiReturnStr);
        wait(NULL);
        PRINT_HERE();
        // std::string cgiResponse = "HTTP/1.1 200 OK \nContent-Location: " + std::string("localhost") + request->target + "\n";
        // cgiResponse += "Content-Type: text/html\n";
        // // cgiResponse += "Content-Length: " + std::to_string(cgiReturnStr.size()) + "\n";
        // cgiResponse += "Transfer-Encoding: chunked";
        // // cgiResponse += "Content-Length: 0\n";
        // cgiResponse += "Connection: close\n";
        // cgiResponse += "\n";
        cgiResponse += cgiReturnStr;
        write(m_pipe[WRITE_END], cgiResponse.data(), cgiResponse.size());
        close(m_pipe[WRITE_END]);
        exit(0);
    }
}
