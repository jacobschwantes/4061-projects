#include "include/utility.h"
#include <libgen.h>

#define MAX_LENGTH 256
#define CYAN "\033[0;36m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define GREEN "\033[0;32m"
#define RESET "\033[0m"

/**
 * Writes the full paths of all files within a specified directory to a given output file.
 * This function opens the specified directory, iterates over each entry excluding special
 * entries "." and "..", constructs the full path for each file, and writes these paths to
 * the specified output file.
 * @param directoryPath The path to the directory whose file paths are to be written.
 * @param outputFileName The path to the file where the full paths will be saved.
 * @returns count The number of files read from the directory
 * @Hint: This function can create submissions.txt if you wish to use it
 */
int write_filepaths_to_submissions(const char *directoryPath, const char *outputFileName)
{
    DIR *dir;
    struct dirent *entry;
    FILE *file;

    // Open the directory
    dir = opendir(directoryPath);
    if (!dir)
    {
        perror("Failed to open directory");
        exit(EXIT_FAILURE);
    }

    // Open or create the output file
    file = fopen(outputFileName, "w");
    if (!file)
    {
        perror("Failed to open output file");
        closedir(dir);
        exit(EXIT_FAILURE);
    }
    int count = 0;
    char fullPath[PATH_MAX];
    while ((entry = readdir(dir)) != NULL)
    {
        // Skip "." and ".." directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // Construct the full path
        snprintf(fullPath, sizeof(fullPath), "%s/%s", directoryPath, entry->d_name);

        // Write the full path to the file
        fprintf(file, "%s\n", fullPath);
        count++;
    }

    fclose(file);
    closedir(dir);
    return count;
}

// appends exit status of a completed process to outputs array
void append_status(char outputs[][MAX_LENGTH], char string[], int index)
{
    if (strlen(outputs[index]) + strlen(string) < MAX_LENGTH)
    {
        strncat(outputs[index], string, MAX_LENGTH - strlen(outputs[index]) - 1);
    }
}

// reads list of executables from submission.txt into memory
int read_submissions(char paths[][MAX_LENGTH], int length)
{

    FILE *F = fopen("submission.txt", "r");

    if (F == NULL)
    {
        perror("Error opening file");
        exit(-1);
    };

    char buffer[MAX_LENGTH];
    int count = 0;

    while (fgets(buffer, MAX_LENGTH, F) != NULL && count < length)
    {
        buffer[strcspn(buffer, "\n")] = 0;
        strncpy(paths[count], buffer, MAX_LENGTH);
        count++;
    }
    fclose(F);
    printf("Found " YELLOW "%d" RESET " submissions in submission.txt\n", count);
    return 0;
}

// reads proc/cpuinfo and returns the core count of the host.
int get_core_count()
{

    FILE *fp;
    int cores = 0;
    char line[256];

    // Open the /proc/cpuinfo file for reading
    fp = fopen("/proc/cpuinfo", "r");
    if (fp == NULL)
    {
        perror(RED "Error opening /proc/cpuinfo\n" RESET);
        exit(-1);
    }

    // Read each line, if line starts with the word "processor", we increment cores by one
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (strstr(line, "processor") == line)
        {
            cores++;
        }
    }
    printf("Core count of host is " YELLOW "%d\n" RESET, cores);
    fclose(fp);
    return cores;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        perror(RED "Incorrect arguments. Usage: ./autograder p1 p2 ... pN\n" RESET);
        exit(-1);
    }
    int slow_count = 0;

    // populate submission.txt with executable paths
    int SUBMISSION_COUNT = write_filepaths_to_submissions("./solutions", "submission.txt");

    char exec_paths[SUBMISSION_COUNT][MAX_LENGTH];
    char outputs[SUBMISSION_COUNT][MAX_LENGTH];

    const int CORE_COUNT = get_core_count();
    const int PARAMS = argc - 1;
    int params_offset = 1;
    int BATCH_SIZE = 0; // variable set after checking whether BATCH_SIZE is bigger than SUBMISSION_COUNT; lines 98-103

    // read in submissions from submission.txt and store in exec_paths
    read_submissions(exec_paths, SUBMISSION_COUNT);

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
                    exit(-1);
                }
                else if (pids[i] == 0)
                {

                    execl(exec_paths[i], basename(exec_paths[i + processed_count]), argv[params_offset], (char *)0);
                    perror(RED "failed to execute\n" RESET);
                    exit(-1);
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
            while (stop_timer(&wait_start) < (S + 2) * 1000)
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
                                    append_status(outputs, "I", i + processed_count);
                                    break;
                                case 0:
                                    if (stop_timer(&start_times[i]) >= (S + 1) * 1000)
                                    {
                                        slow_count++;
                                        // (slow)
                                        append_status(outputs, "S", i + processed_count);
                                    }
                                    else
                                    {
                                        // (correct)
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
                                    // (crashed)
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
                        // (blocked)
                        append_status(outputs, "B", i + processed_count);
                    // R means process is running, so its marked as in an infinite loop
                    if (buffer[0] == 'R')
                        // (infinite loop)
                        append_status(outputs, "L", i + processed_count);
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

    // print out results
    printf("Results from executing " YELLOW "%d" RESET " submissions with test inputs ", SUBMISSION_COUNT);
    for (int i = 0; i < PARAMS; i++)
    {
        printf(YELLOW "%s" RESET, argv[i + 1]);
        if (i != PARAMS - 1)
        {
            printf(", ");
        }
        else
        {
            printf(":\n");
        }
    }
    for (int i = 0; i < SUBMISSION_COUNT; i++)
    {
        printf("%s: ", basename(exec_paths[i])); /* submission name */
        for (int j = 0; j < PARAMS; j++)
        {
            char code[25];
            switch (outputs[i][j])
            {
            case 'C':
                strcpy(code, GREEN "(correct)" RESET);
                break;
            case 'I':
                strcpy(code, RED "(incorrect)" RESET);
                break;
            case 'L':
                strcpy(code, RED "(infinite)" RESET);
                break;
            case 'B':
                strcpy(code, RED "(blocked)" RESET);
                break;
            case 'S':
                strcpy(code, YELLOW "(slow)" RESET);
                break;
            case 'F':
                strcpy(code, RED "(crash)" RESET);
                break;
            default:
                break;
            }
            printf(CYAN "%c" RESET " %s", *argv[1 + j], code); /* test input, exit status */
            if (j != (PARAMS - 1))
                printf(", ");
        }
        printf("\n");
    }
    printf("slow count: %d\n", slow_count);
    return 0;
}
