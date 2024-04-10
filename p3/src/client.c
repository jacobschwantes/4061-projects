#include "../include/client.h"

int port = 0;

pthread_t worker_thread[100];
int worker_thread_id = 0;
char output_path[1028];
char dir_path[1028];

processing_args_t req_entries[100];

/* TODO: implement the request_handle function to send the image to the server and recieve the processed image
 * 1. Open the file in the read-binary mode - Intermediate Submission
 * 2. Get the file length using the fseek and ftell functions - Intermediate Submission
 * 3. set up the connection with the server using the setup_connection(int port) function - Intermediate Submission
 * 4. Send the file to the server using the send_file_to_server(int socket, FILE *fd, size_t size) function - Intermediate Submission
 * 5. Receive the processed image from the server using the receive_file_from_server(int socket, char *file_path) function
 * 6. receive_file_from_server saves the processed image in the output directory, so pass in the right directory path
 * 7. Close the file and the socket
 */

void *request_handle(void *args)
{
    int index = *((int *)args);
    // printf("index: %d\n", index);
    char input_path[BUFF_SIZE];
    snprintf(input_path, sizeof(input_path), "%s/%s", dir_path, req_entries[index].file_name);
    FILE *file = fopen(input_path, "rb");
    if (!file)
    {
        perror("Failed to open file");
        return NULL;
    }

    // Get file length
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    printf("file size in client: %ld\n", file_size);
    fseek(file, 0, SEEK_SET);

    // Set up connection
    int socket = setup_connection(port);
    if (socket < 0)
    {
        perror("Failed to set up connection");
        fclose(file);
        return NULL;
    }

    // Send the file
    if (send_file_to_server(socket, file, file_size) != 0)
    {
        perror("Failed to send file to server");
        fclose(file);
        close(socket);
        return NULL;
    }
    // printf("made it here\n");

    char output_dir[BUFFER_SIZE];

    // printf("made it here 2\n");

    snprintf(output_dir, sizeof(output_dir), "%s/%s", output_path, req_entries[index].file_name);

    printf("output dir: %s\n", output_dir);
    if (receive_file_from_server(socket, output_dir) < 0)
    {
        printf("made it here 5\n");
        perror("Failed to recieve file from server");
    } else {
        // printf("made it here 4\n");
    }

    // printf("made it here 3\n");
    // Cleanup
    fclose(file);
    close(socket);

    // printf("client thread %d is about to exit\n", req_entries[index].number_worker);

    return NULL;
}

/* TODO: Intermediate Submission
 * implement the directory_trav function to traverse the directory and send the images to the server
 * 1. Open the directory
 * 2. Read the directory entries
 * 3. If the entry is a file, create a new thread to invoke the request_handle function which takes the file path as an argument
 * 4. Join all the threads
 * Note: Make sure to avoid any race conditions when creating the threads and passing the file path to the request_handle function.
 * use the req_entries array to store the file path and pass the index of the array to the thread.
 */
void directory_trav(char *path)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir(path);
    if (dir == NULL)
    {
        perror("failed to open database directory");
        exit(1);
    }
    int worker_thread_ids[100];
    while ((entry = readdir(dir)) != NULL)
    {

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        req_entries[worker_thread_id].file_name = malloc(BUFF_SIZE);
        sprintf(req_entries[worker_thread_id].file_name, "%s", entry->d_name);
        printf("filepath in dir traversal: %s\n", req_entries[worker_thread_id].file_name);
        worker_thread_ids[worker_thread_id] = worker_thread_id;
        int worker_result;
        worker_result = pthread_create(&worker_thread[worker_thread_id], NULL, request_handle, (void *)&worker_thread_ids[worker_thread_id]);
        if (worker_result != 0)
        {
            perror("failed to create worker thread");
            exit(1);
        }
        worker_thread_id++;
    }
    closedir(dir);
}
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: ./client <directory path> <Server Port> <output path>\n");
        exit(-1);
    }
    /*TODO:  Intermediate Submission
     * 1. Get the input args --> (1) directory path (2) Server Port (3) output path
     */


    strcpy(dir_path, argv[1]);
    strcpy(output_path, argv[3]);
    port = atoi(argv[2]);

    /*TODO: Intermediate Submission
     * Call the directory_trav function to traverse the directory and send the images to the server
     */
    directory_trav(dir_path);
    int i;
    for (i = 0; i < worker_thread_id; i++)
    {
        fprintf(stderr, "JOINING CLIENT WORKER %d \n", i);
        if ((pthread_join(worker_thread[i], NULL)) != 0)
        {
            printf("ERROR : Fail to join worker thread %d.\n", i);
        }
    }

    for (int i = 0; i < worker_thread_id; i++)
    {
        free(req_entries[i].file_name);
    }
    return 0;
}