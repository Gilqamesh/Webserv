#include <unistd.h>
#include "fcntl.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

int main(int argc, char **argv, char **envp)
{
    {
        char *tmp = getcwd(NULL, 0);
        // free(tmp);
    }
    system("leaks a.out");
}
