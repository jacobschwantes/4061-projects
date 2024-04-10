#include "../include/client.h"

int port = 0;

pthread_t worker_thread[100];
int worker_thread_id = 0;
char output_path[1028];
char dir_path[1028];

processing_args_t req_entries[100];

void *request_handle(void *args)
{
    int index = *((int *)args);
    char input_path[BUFF_SIZE+1];
    snprintf(input_path, sizeof(input_path), "%s/%s", dir_path, req_entries[index].file_name); // Format filepath using image input directory and filename
    FILE *file = fopen(input_path, "rb");
    if (!file)
    {
        perror("Failed to open file");
        return NULL;
    }

    // Get file length
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
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

    char output_dir[BUFFER_SIZE+1];
    snprintf(output_dir, sizeof(output_dir), "%s/%s", output_path, req_entries[index].file_name);
    if (receive_file_from_server(socket, output_dir) < 0)
    {
        printf("made it here 5\n");
        perror("Failed to recieve file from server");
    }

    // Cleanup
    fclose(file);
    close(socket);

    return NULL;
}

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

    strcpy(dir_path, argv[1]);
    strcpy(output_path, argv[3]);
    port = atoi(argv[2]);

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