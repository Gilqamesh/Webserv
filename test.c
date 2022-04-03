#include <unistd.h>
#include "fcntl.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv, char **envp)
{
    (void)argc;
    (void)argv;
    int pid = fork();
    if (pid == -1)
        exit(3);
    if (pid == 0)
    {
        int fd = open("test2.txt", O_RDWR);
        if (fd == -1)
            printf("cant open for writing: test2.txt\n");
        write(fd, "yo\n", 3);
        close(fd);
        fd = open("test2.txt", O_RDWR);
        if (dup2(fd, STDIN_FILENO) == -1)
        {
            perror("dup2 failed");
            exit(4);
        }
        close(fd);
        if (execve("/bin/cat", (char *[]){"/bin/cat", NULL}, envp) == -1)
        {
            perror("execve failed");
            exit(2);
        }
    }
    int ret;
    wait(&ret);
    if (WIFEXITED(ret))
    {
        printf("Exit status: %d\n", WEXITSTATUS(ret));
    }
    // close(fd);
    // printf("Response: %s\n", buffer);
}