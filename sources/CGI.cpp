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
            close(m_pipe[READ_END]);
            int tmp_cgi_file_in = open("temp/temp_cgi_file_in", O_WRONLY | O_CREAT | O_TRUNC, 0777);
            if (tmp_cgi_file_in == -1)
                TERMINATE("failed to open temp/temp_cgi_file_in");
            /*
             * Experimentation on sending the request header field to the CGI as well
             */
            write(tmp_cgi_file_in, request->payload.data(), request->payload.length());
            close(tmp_cgi_file_in);
            tmp_cgi_file_in = open("temp/temp_cgi_file_in", O_RDONLY);
            if (dup2(tmp_cgi_file_in, STDIN_FILENO) == -1)
                TERMINATE("dup2 failed");
            close(tmp_cgi_file_in);
            char **args = (char **)malloc(3 * sizeof(char *));
            char *arg1 = strdup(request->extension.c_str());
            char *arg2 = strdup(meta_variables["PATH_INFO"].c_str());
            args[0] = arg1;
            args[1] = arg2; // cwd + request.target
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
            int tmp_cgi_file_out = open(request->underLocation.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
            if (tmp_cgi_file_out == -1)
                TERMINATE(("failed to open " + request->underLocation).c_str());
            if (dup2(tmp_cgi_file_out, STDOUT_FILENO) == -1)
                TERMINATE("dup2 failed");
            close(tmp_cgi_file_out);
            if (execve(args[0], args, env) == -1)
                TERMINATE("execve failed");
        }
        wait(NULL);
        struct stat fileInfo;
        if (stat(request->underLocation.c_str(), &fileInfo) == -1)
            TERMINATE(("stat failed on file " + request->underLocation).c_str());
        std::string cgiResponse = "HTTP/1.1 200 OK \nContent-Location: " + request->target + "\n";
        cgiResponse += "Content-Type: text/html\n";
        cgiResponse += "Content-Length: " + std::to_string(request->payload.size()) + "\n";
        cgiResponse += "Connection: close\n";
        cgiResponse += "\n";
        /*
         * store the output of the cgi to temp/temp_cgi_file_out as well
         */
        int cgiNetworkTempFile = open("temp/temp_cgi_file_out", O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (cgiNetworkTempFile == -1)
            TERMINATE("failed to open for writing: temp/temp_cgi_file_out");
        // WARN("request->underLocation: " << request->underLocation);
        int requestUpload = open(request->underLocation.c_str(), O_RDONLY);
        if (requestUpload == -1)
            TERMINATE(("failed to open for reading: " + request->underLocation).c_str());
        // WARN("skipping header");
        char buffer[2];
        std::string requestUploadHeader(100000, '\0');
        while (1)
        {
            int tmp = read(requestUpload, buffer, 1);
            if (tmp == -1)
            {
                TERMINATE("read failed");
            }
            if (tmp == 0)
                break ;
            buffer[tmp] = '\0';
            requestUploadHeader += buffer;
            // WARN(requestUploadHeader);
            if (requestUploadHeader.size() > 3 && (requestUploadHeader.substr(requestUploadHeader.size() - 4) == "\r\n\r\n"
                || requestUploadHeader.substr(requestUploadHeader.size() - 2) == "\n\n"))
                break ;
        }
        write(cgiNetworkTempFile, cgiResponse.data(), cgiResponse.size());
        char buffer2[4096];
        while (1)
        {
            int readRet = read(requestUpload, buffer2, 4095);
            if (readRet == -1)
            {
                PRINT_HERE();
                WARN("read failed");
                break ;
            }
            if (readRet == 0)
                break ;
            write(cgiNetworkTempFile, buffer2, readRet);
            buffer2[readRet] = '\0';
            // WARN(buffer2);
        }
        close(cgiNetworkTempFile);
        close(requestUpload);
        write(m_pipe[WRITE_END], "\n", 1);
        close(m_pipe[WRITE_END]);
        exit(0);
    }
}
