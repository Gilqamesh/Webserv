#include <unistd.h>
#include "fcntl.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

int main(int argc, char **argv, char **envp)
{
    // (void)argc;
    // (void)argv;
    // int fd = open("test2.txt", O_RDWR | O_CREAT | O_TRUNC);
    // if (fd == -1)
    //     printf("cant open for writing: test2.txt\n");
    // write(fd, "yo\n", 3);
    // int pid = fork();
    // if (pid == -1)
    //     exit(3);
    // if (pid == 0)
    // {
    //     if (dup2(fd, STDIN_FILENO) == -1)
    //     {
    //         perror("dup2 failed");
    //         exit(4);
    //     }
    //     close(fd);
    //     if (execve("/bin/cat", (char *[]){"/bin/cat", NULL}, envp) == -1)
    //     {
    //         perror("execve failed");
    //         exit(2);
    //     }
    // }
    // int ret;
    // wait(&ret);
    // if (WIFEXITED(ret))
    // {
    //     printf("Exit status: %d\n", WEXITSTATUS(ret));
    // }
    // close(fd);
    // // printf("Response: %s\n", buffer);
    std::cout << "\033[1;31masdf\033[0m" << std::endl;
}