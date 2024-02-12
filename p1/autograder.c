#include "include/utility.h"
#include <libgen.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        perror(RED "Incorrect arguments. Usage is ./autograder <P1> <P2> ...\n" RESET);
        exit(EXIT_FAILURE);
    }

    // populate submissions.txt with executable paths
    int SUBMISSION_COUNT = write_filepath_to_submissions("solutions", "submissions.txt");

    char exec_paths[SUBMISSION_COUNT][MAX_LENGTH];
    int outputs[SUBMISSION_COUNT][MAX_LENGTH];

    const int CORE_COUNT = get_core_count();
    const int PARAMS = argc - 1;
    int params_offset = 1;
    int BATCH_SIZE = 0; // variable set after checking whether BATCH_SIZE is bigger than SUBMISSION_COUNT; lines 98-103

    // read in submissions from submission.txt and store in exec_paths
    read_submissions(exec_paths, "submissions.txt", SUBMISSION_COUNT);

    // initialize outputs array with empty strings
    for (int i = 0; i < SUBMISSION_COUNT; ++i)
    {
        outputs[i][0] = '\0';
    }

    while (params_offset < argc)
    {
        printf("Starting execution for test input " YELLOW "%s" RESET "\n", argv[params_offset]);
        int processed_count = 0;

        // need to run another batch with same test input parameter if submission count exceeds batch size
        while (SUBMISSION_COUNT > processed_count)
        {

            // checks to see if we have more cores available than test executables
            // so we dont create more processes than we need
            if (CORE_COUNT > SUBMISSION_COUNT)
            {
                BATCH_SIZE = SUBMISSION_COUNT;
            }
            else
            {
                BATCH_SIZE = CORE_COUNT;
            }

            pid_t pids[BATCH_SIZE];
            struct timeval start_times[BATCH_SIZE];

            // if BATCH_SIZE is greater than remaining executables,
            // set BATCH_SIZE to the remaining number of executables
            if (BATCH_SIZE > (SUBMISSION_COUNT - processed_count))
            {
                int remaining_submissions = (SUBMISSION_COUNT - processed_count) % BATCH_SIZE;
                printf("Only" YELLOW " %d " RESET "submissions remaining for test input" YELLOW " %s" RESET ", setting batch size to " YELLOW "%d" RESET ".\n", remaining_submissions, argv[params_offset], remaining_submissions);
                BATCH_SIZE = remaining_submissions;
            }
            printf("Creating" YELLOW " %d " RESET "processes\n", BATCH_SIZE);
            for (int i = 0; i < BATCH_SIZE; i++)
            {
                if ((pids[i] = fork()) < 0)
                {
                    perror(RED "failed to fork\n" RESET);
                    exit(EXIT_FAILURE);
                }
                else if (pids[i] == 0)
                {

                    execl(exec_paths[i], basename(exec_paths[i + processed_count]), argv[params_offset], (char *)0);
                    perror(RED "failed to execute\n" RESET);
                    exit(EXIT_FAILURE);
                }
                start_timer(&start_times[i]);
            }

            /* Parent Code */
            struct timeval wait_start;
            int status;
            pid_t pid;

            // start timer to track how long we have been waiting for children to exit
            start_timer(&wait_start);

            // waiting for children that will exit
            while (stop_timer(&wait_start) < S + 1)
            {
                pid = waitpid(-1, &status, WNOHANG);

                if (pid == -1)
                {
                    //  no processes left to wait for so we move to next batch
                    break;
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
                                    // (incorrect)
                                    outputs[i + processed_count][params_offset - 1] = INCORRECT;
                                    break;
                                case 0:
                                    if (stop_timer(&start_times[i]) >= S)
                                    {
                                        // (slow)
                                        outputs[i + processed_count][params_offset - 1] = SLOW;
                                    }
                                    else
                                    {
                                        // (correct)
                                        outputs[i + processed_count][params_offset - 1] = CORRECT;
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
                                    // (crashed)
                                    outputs[i + processed_count][params_offset - 1] = SEGFAULT;
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
            }

            // check for blocked or infinite children and kill them
            for (int i = 0; i < BATCH_SIZE; i++)
            {

                if (pids[i] != 0)
                {
                    usleep(1000); // sleep for 1ms before opening status file to allow process state to settle

                    // construct the path to the status file
                    char path[50];
                    snprintf(path, sizeof(path), "/proc/%d/status", pids[i]);

                    FILE *file = fopen(path, "r");

                    if (file == NULL)
                    {
                        perror(RED "Error opening file\n" RESET);
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
                    {
                        // (blocked)
                        outputs[i + processed_count][params_offset - 1] = STUCK_BLOCK;
                    }
                    // R means process is running, so its marked as in an infinite loop
                    if (buffer[0] == 'R')
                    {
                        // (infinite loop)
                        outputs[i + processed_count][params_offset - 1] = INFLOOP;
                    }
                    // kill process after appending status
                    kill(pids[i], SIGKILL);
                }
            }
            // move onto next batch of submissions
            processed_count += BATCH_SIZE;
        }
        // move onto next input parameter
        params_offset++;
    }

    // write scores to output.txt file and print to console
    FILE *output;
    output = fopen("output.txt", "w");
    if (output == NULL)
    {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    };

    for (int i = 0; i < SUBMISSION_COUNT; i++)
    {
        // write solution name to output file
        fprintf(output, "%s ", basename(exec_paths[i]));
        // print solution name to console
        printf("%s: ", basename(exec_paths[i]));
        for (int j = 0; j < PARAMS; j++)
        {
            // write score to file
            fprintf(output, "%c%s%s", *argv[1 + j], get_status_message(outputs[i][j], 0), j != (PARAMS - 1) ? " " : "\n");
            // print score to console
            printf(CYAN "%c" RESET " %s%s", *argv[1 + j], get_status_message(outputs[i][j], 1), j != (PARAMS - 1) ? ", " : "\n");
        }
    }
    fclose(output);
    return 0;
}
