#ifndef UTILITY_H
#define UTILITY_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h> // For PATH_MAX

#define L 20 // Define timeout for stuck or infinite
#define S 10 // Define timeout for slow
#define MAX_LENGTH 256
#define CYAN "\033[0;36m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define GREEN "\033[0;32m"
#define RESET "\033[0m"

// Define an enum for the program execution outcomes
enum
{
    CORRECT = 1, // Corresponds to case 1: Exit with status 0 (correct answer)
    INCORRECT,   // Corresponds to case 2: Exit with status 1 (incorrect answer)
    SEGFAULT,    // Corresponds to case 3: Triggering a segmentation fault
    INFLOOP,     // Corresponds to case 4: Entering an infinite loop
    STUCK_BLOCK, // Corresponds to case 5: Simulating being stuck/blocked
    SLOW         // Corresponds to case 6: Simulating a slow process
};

// Function to get status message
const char *get_status_message(int status, int enable_color)
{
    switch (status)
    {
    case CORRECT:
        return enable_color ?  GREEN "(correct)" RESET : "(correct)";
    case INCORRECT:
        return enable_color ? RED "(incorrect)" RESET : "(incorrect)";
    case SLOW:
        return enable_color ? YELLOW "(slow)" RESET : "(slow)";
    case SEGFAULT:
        return enable_color ? RED "(segfault)" RESET : "(segfault)";
    case STUCK_BLOCK:
        return enable_color ? RED "(stuck or block)" RESET : "(stuck or block)";
    case INFLOOP:
        return enable_color ? RED "(infloop)" RESET : "(infloop)";
    default:
        return "unknown";
    }
}

// reads executable paths from submission file and writes to memory
int read_submissions(char paths[][MAX_LENGTH], char filename[], int length)
{

    FILE *F = fopen(filename, "r");

    if (F == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
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
    printf("Found " YELLOW "%d" RESET " submissions in %s\n", count, filename);
    return 0;
}

// returns core count of system
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
        exit(EXIT_FAILURE);
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


/**
 * Writes the full paths of all files within a specified directory to a given output file.
 * This function opens the specified directory, iterates over each entry excluding special
 * entries "." and "..", constructs the full path for each file, and writes these paths to
 * the specified output file.
 * @param directoryPath The path to the directory whose file paths are to be written.
 * @param outputFileName The path to the file where the full paths will be saved.
 * @Hint: This function can create submissions.txt if you wish to use it
 */

int write_filepath_to_submissions(const char *directoryPath, const char *outputFileName)
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

    char fullPath[PATH_MAX];
    int file_count = 0;

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
        file_count++;
    }

    fclose(file);
    closedir(dir);
    return file_count;
}

/* The inline keyword is used to suggest that the compiler embeds the function's code directly
at each point of call, potentially reducing function call overhead and improving execution speed.
Note: timer returns second */

static inline void start_timer(struct timeval *start)
{
    gettimeofday(start, NULL);
}

static inline double stop_timer(struct timeval *start)
{
    struct timeval end;
    gettimeofday(&end, NULL);
    double seconds = (double)(end.tv_sec - start->tv_sec);
    double useconds = (double)(end.tv_usec - start->tv_usec) / 1000000.0;
    return seconds + useconds;
}

#endif // UTILITY_H