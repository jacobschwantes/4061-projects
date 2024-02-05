#include "include/utility.h"

#define MAX_LENGTH 500

void appendstr(char outputs[][MAX_LENGTH], char string[], int index)
{
    if (strlen(outputs[index]) + strlen(string) < MAX_LENGTH)
    {
        strncat(outputs[index], string, MAX_LENGTH - strlen(outputs[index]) - 1);
    }
}

int main(int argc, char *argv[])
{
    const int B = atoi(argv[1]);
    char outputs[B][MAX_LENGTH];
    int params = argc - 2;
    int params_offset = 2;

    for (int i = 0; i < B; ++i)
    {
        outputs[i][0] = '\0';
    }

    while (params_offset < argc)
    {
        char exec_path[100];
        FILE *f;
        f = fopen("submission.txt", "r");

        if (f == NULL)
        {
            perror("Error opening file");
            return -1;
        };
        int batch_size = B;

        printf("starting param: %s\n", argv[params_offset]);
        // while (fgets(exec_path, 100, f) != NULL)
        // {

        if (params < batch_size)
        {
            batch_size = params;
        }
        pid_t pids[batch_size];

        for (int i = 0; i < batch_size; i++)
        {
            if (fgets(exec_path, 100, f) != NULL)
            {
                exec_path[strcspn(exec_path, "\n")] = 0;
                printf("starting exec: %s\n", exec_path);
                if ((pids[i] = fork()) < 0)
                {
                    perror("failed to fork");
                    exit(-1);
                }
                else if (pids[i] == 0)
                {
                    char *args[] = {exec_path, argv[params_offset], (char *)0};
                    printf("spawning child with pid:%d\n", getpid());
                    execv(args[0], args);
                    perror("failed to execute");
                    exit(1);
                }
            }
        }

        /* Parent code */
        int status;
        pid_t pid;

        int count = 0;
        // check every second for child status updates until slow child finishes
        while (count <= 11)
        {
            pid = waitpid(-1, &status, WNOHANG);

            if (pid == -1)
            {
                /* an error other than an interrupted system call */
                perror("waitpid");
                return 0;
            }
            else if (pid != 0)
            {
                if (WIFEXITED(status)) /* process exited normally */
                {
                    for (int i = 0; i < batch_size; i++)
                    {
                        if (pids[i] == pid)
                        {
                            switch (WEXITSTATUS(status))
                            {
                            case 1:
                                appendstr(outputs, "I", i);
                                break;
                            case 0:
                                if (count > (L * 2))
                                {
                                    appendstr(outputs, "S", i);
                                }
                                else
                                {
                                    appendstr(outputs, "C", i);
                                }
                                break;
                            default:
                                break;
                            }
                        }
                    }
                    printf("child process %d exited with value %d\n", pid, WEXITSTATUS(status));
                }
                else if (WIFSIGNALED(status))
                {
                    for (int i = 0; i < batch_size; i++)
                    {
                        if (pids[i] == pid)
                        {
                            switch (WTERMSIG(status))
                            {
                            case 11:
                                appendstr(outputs, "F", i);
                                break;
                            default:
                                break;
                            }
                        }
                    }
                    printf("child process %d exited due to signal %d\n", pid, WTERMSIG(status));
                }
                for (int i = 0; i < batch_size; i++)
                {
                    if (pids[i] == pid)
                    {
                        pids[i] = 0;
                    }
                }
            }
            else
            {
                printf("still waiting\n");
            }

            // int j = 0;
            // while (pids[j] == 0 && j < batch_size)
            // {
            //     if (j == (batch_size - 1))
            //     {
            //         count = 11;
            //     }
            //     j++;
            // }

            ++count;
            sleep(1);
        }
        // check for blocked or infinite children and kill them
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
                        // printf("Process %d is in state: %c\n", pids[i], buffer[0]);
                        break; // Stop reading after finding the state information
                    }
                }
                fclose(file);

                if (buffer[0] == 'S')
                {
                    appendstr(outputs, "B", i);
                    printf("state is S so killing blocked child: %d\n", pids[i]);
                }
                if (buffer[0] == 'R')
                {
                    appendstr(outputs, "L", i);
                    printf("state is R so killing infinite loop child: %d\n", pids[i]);
                }

                kill(pids[i], SIGKILL);

                // printf("stale process: %d\n", pids[i]);
            }
        }
        params_offset++;
        // }
        fclose(f);
    }

    FILE *f;

    f = fopen("submission.txt", "r");
    char exec_name[100];
    for (int i = 0; i < B; i++)
    {
        if (fgets(exec_name, 100, f) != NULL)
        {
            exec_name[strcspn(exec_name, "\n")] = 0;
            const char *lastSeparator = strrchr(exec_name, '/');

            if (lastSeparator != NULL)
            {
                // The last segment is everything after the last separator
                const char *lastSegment = lastSeparator + 1;

                strcpy(exec_name, lastSegment);
            }

            printf("%s: ", exec_name);
            for (int j = 0; j < argc - 2; j++)
            {

                char status_code = outputs[i][j];
                char code[25];
                switch (status_code)
                {
                case 'C':
                    strcpy(code, "(correct)");
                    break;
                case 'I':
                    strcpy(code, "(incorrect)");
                    break;
                case 'L':
                    strcpy(code, "(infinite)");
                    break;
                case 'B':
                    strcpy(code, "(blocked)");
                    break;
                case 'S':
                    strcpy(code, "(slow)");
                    break;
                case 'F':
                    strcpy(code, "(crash)");
                    break;
                default:
                    break;
                }
                printf("%c %s", *argv[2 + j], code);
                if (j != (params - 1))
                {
                    printf(", ");
                }
            }
            printf("\n");
        }
    }

    return 0;
}
