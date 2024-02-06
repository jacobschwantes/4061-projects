#include "include/utility.h"
#include <libgen.h>

#define MAX_LENGTH 256
#define MAX_SUBMISSIONS 25 /* max number of executables (tests) */

// append return status of a process to outputs
void append_status(char outputs[][MAX_LENGTH], char string[], int index)
{
    if (strlen(outputs[index]) + strlen(string) < MAX_LENGTH)
    {
        strncat(outputs[index], string, MAX_LENGTH - strlen(outputs[index]) - 1);
    }
}

// reads list of executables into memory
int read_submissions(char paths[][MAX_LENGTH])
{

    FILE *F = fopen("submission.txt", "r");

    if (F == NULL)
    {
        perror("Error opening file");
        return -1;
    };

    char buffer[MAX_LENGTH];
    int count = 0;

    while (fgets(buffer, MAX_LENGTH, F) != NULL && count < MAX_SUBMISSIONS)
    {
        buffer[strcspn(buffer, "\n")] = 0;
        strncpy(paths[count], buffer, MAX_LENGTH);
        count++;
    }
    fclose(F);
    return count;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        perror("Incorrect arguments. Usage: ./autograder B p1 p2 ... pN\n");
        exit(-1);
    }

    // populate submission.txt with executable paths
    write_filepath_to_submissions("./test", "submission.txt");

    const int BATCH_SIZE = atoi(argv[1]);
    char exec_paths[MAX_SUBMISSIONS][MAX_LENGTH];
    const int submission_count = read_submissions(exec_paths);
    const int PARAMS = argc - 2;
    char outputs[MAX_SUBMISSIONS][MAX_LENGTH];
    int params_offset = 2;

    // initialize outputs array
    for (int i = 0; i < BATCH_SIZE; ++i)
    {
        outputs[i][0] = '\0';
    }

    while (params_offset < argc)
    {
        int processed_count = 0;
        // need to run another batch with same test input if submission count exceeds batch size
        while (submission_count > processed_count)
        {
            pid_t pids[BATCH_SIZE];

            for (int i = 0; i < BATCH_SIZE; i++)
            {
                if ((pids[i] = fork()) < 0)
                {
                    perror("failed to fork");
                    exit(-1);
                }
                else if (pids[i] == 0)
                {
                    execl(exec_paths[i], basename(exec_paths[i + processed_count]), argv[params_offset], (char *)0);
                    perror("failed to execute");
                    exit(-1);
                }
            }

            /* Parent Code */
            int status;
            pid_t pid;

            int count = 0;
            // check every second for child status updates until slow child finishes
            while (count <= (L * 2) + 1)
            {
                pid = waitpid(-1, &status, WNOHANG);

                if (pid == -1)
                {
                    /* an error other than an interrupted system call */
                    perror("waitpid");
                    exit(-1);
                }
                else if (pid != 0)
                {
                    if (WIFEXITED(status)) /* process exited normally */
                    {
                        for (int i = 0; i < BATCH_SIZE; i++)
                        {
                            if (pids[i] == pid)
                            {
                                switch (WEXITSTATUS(status))
                                {
                                case 1:
                                    append_status(outputs, "I", i + processed_count);
                                    break;
                                case 0:
                                    if (count > (L * 2))
                                    {
                                        append_status(outputs, "S", i + processed_count);
                                    }
                                    else
                                    {
                                        append_status(outputs, "C", i + processed_count);
                                    }
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    }
                    else if (WIFSIGNALED(status)) /* process excited by signal */
                    {
                        for (int i = 0; i < BATCH_SIZE; i++)
                        {
                            if (pids[i] == pid)
                            {
                                switch (WTERMSIG(status))
                                {
                                case 11:
                                    append_status(outputs, "F", i + processed_count);
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                    }
                    // zero out pids that have exited
                    for (int i = 0; i < BATCH_SIZE; i++)
                    {
                        if (pids[i] == pid)
                            pids[i] = 0;
                    }
                }

                ++count;
                sleep(1);
            }
            // check for blocked or infinite children and kill them
            for (int i = 0; i < BATCH_SIZE; i++)
            {

                if (pids[i] != 0)
                {

                    // construct the path to the status file
                    char path[50];
                    snprintf(path, sizeof(path), "/proc/%d/status", pids[i]);

                    FILE *file = fopen(path, "r");

                    if (file == NULL)
                    {
                        perror("Error opening file");
                        exit(EXIT_FAILURE);
                    }

                    // read the content of the status file
                    char buffer[256];
                    while (fgets(buffer, sizeof(buffer), file) != NULL)
                    {
                        if (sscanf(buffer, "State:\t%c", &buffer[0]) == 1)
                            break;
                    }

                    fclose(file);

                    // S means process is sleeping, waiting for input, so its marked as blocked
                    if (buffer[0] == 'S')
                        append_status(outputs, "B", i + processed_count);
                    // R means process is running, so its marked as in an infinite loop
                    if (buffer[0] == 'R')
                        append_status(outputs, "L", i + processed_count);
                    // kill process after appending status
                    kill(pids[i], SIGKILL);
                }
            }
            // move onto next batch of submissions if any
            processed_count += BATCH_SIZE;
        }
        // move onto next test input
        params_offset++;
    }

    // print out scores
    for (int i = 0; i < submission_count; i++)
    {
        printf("%s: ", basename(exec_paths[i])); /* submission name */
        for (int j = 0; j < PARAMS; j++)
        {
            char code[25];
            switch (outputs[i][j])
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
            printf("%c %s", *argv[2 + j], code); /* test input, exit status */
            if (j != (PARAMS - 1))
                printf(", ");
        }
        printf("\n");
    }

    return 0;
}
