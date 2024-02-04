#include "include/utility.h"

int main(int argc, char *argv[])
{
    int batch_size = atoi(argv[1]);
    pid_t pids[batch_size];
    char exec_path[100];
    FILE *f;

    f = fopen("submission.txt", "r");

    if (f == NULL)
    {
        perror("Error opening file");
        return -1;
    };

    int i;
    for (i = 0; i < batch_size; i++)
    {
        if (fgets(exec_path, 100, f) != NULL) {
            exec_path[strcspn(exec_path, "\n")] = 0;
        }
        if ((pids[i] = fork()) < 0)
        {
            perror("failed to fork");
            exit(-1);
        }
        else if (pids[i] == 0)
        {
            char *args[] = {exec_path, "3", (char *)0};
            printf("spawning child with pid:%d\n", getpid());
            execv(args[0], args);
            perror("failed to execute");
            exit(1);
        }
    }
    int fclose(FILE * f);
    /* Parent code */
    int status;
    pid_t pid;

    while (batch_size > 0)
    { 
        pid = wait(&status);

        if ((pid == -1))
        {
            /* an error other than an interrupted system call */
            perror("waitpid");
            return;
        };
        if (WIFEXITED(status)) /* process exited normally */
            printf("child process %d exited with value %d\n",pid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status)) /* child exited on a signal */
            printf("child process %d exited due to signal %d\n", pid, WTERMSIG(status));
        else if (WIFSTOPPED(status)) /* child was stopped */
            printf("child process %d was stopped by signal %d\n", pid, WIFSTOPPED(status));

        --batch_size;
    }

    return 0;
}
