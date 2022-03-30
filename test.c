#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv, char **envp)
{
    (void)argc;
    (void)argv;
    int p[2];
    pipe(p);
    int pid = fork();
    if (pid == 0)
    {
        close(p[0]);
        dup2(p[1], STDOUT_FILENO);
        close(p[1]);
        write(STDOUT_FILENO, "yo", 2);
        execve("/usr/bin/php", (char *[]){"/usr/bin/php", "./hello.php", NULL}, envp);
    }
    close(p[1]);
    wait(NULL);
    char buffer[100000];
    buffer[100000 - 1] = '\0';
    read(p[0], buffer, 100000);
    printf("Response: %s\n", buffer);
}