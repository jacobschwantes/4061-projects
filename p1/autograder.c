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
        if (fgets(exec_path, 100, f) != NULL)
        {
            exec_path[strcspn(exec_path, "\n")] = 0;
        }
        if ((pids[i] = fork()) < 0)
        {
            perror("failed to fork");
            exit(-1);
        }
        else if (pids[i] == 0)
        {
            char *args[] = {exec_path, "1", (char *)0};
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

    int count = 0;

    while (count <= 11)
    {
        pid = waitpid(-1, &status, WNOHANG);

        if ((pid == -1))
        {
            /* an error other than an interrupted system call */
            perror("waitpid");
            return 0;
        }
        else if (pid != 0)
        {
            for (int i = 0; i < batch_size; i++)
            {
                if (pids[i] == pid)
                {
                    pids[i] = 0;
                }
            }
            if (WIFEXITED(status)) /* process exited normally */
                printf("child process %d exited with value %d\n", pid, WEXITSTATUS(status));
            else if (WIFSIGNALED(status)) /* child exited on a signal */
                printf("child process %d exited due to signal %d\n", pid, WTERMSIG(status));
            else if (WIFSTOPPED(status)) /* child was stopped */
                printf("child process %d was stopped by signal %d\n", pid, WIFSTOPPED(status));
        }
        else
        {
            printf("still waiting\n");
        }

        ++count;
        sleep(1);
    };

    for (int i = 0; i < batch_size; i++)
    {
        if (pids[i] != 0)
        {

           

            // Construct the path to the status file
            char path[50];
            snprintf(path, sizeof(path), "/proc/%d/status", pids[i]);

            // Open the status file for reading
            FILE *file = fopen(path, "r");

            if (file == NULL)
            {
                perror("Error opening file");
                exit(EXIT_FAILURE);
            }

            // Read and print the content of the file
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), file) != NULL)
            {
                if (sscanf(buffer, "State:\t%c", &buffer[0]) == 1)
                {
                    printf("Process %d is in state: %c\n", pids[i], buffer[0]);
                    break; // Stop reading after finding the state information
                }
            }
            fclose(file);

            // printf("stale process: %d\n", pids[i]);
        }
    }

    return 0;
}
