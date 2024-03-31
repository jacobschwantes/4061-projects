#include "../include/server.h"

// /********************* [ Helpful Global Variables ] **********************/
int num_dispatcher = 0; // Global integer to indicate the number of dispatcher threads
int num_worker = 0;     // Global integer to indicate the number of worker threads
FILE *logfile;          // Global file pointer to the log file
int queue_len = 0;      // Global integer to indicate the length of the queue

database_entry_t database[100];
int image_count = 0;

/* TODO: Intermediate Submission
  TODO: Add any global variables that you may need to track the requests and threads
  [multiple funct]  --> How will you track the p_thread's that you create for workers?
  [multiple funct]  --> How will you track the p_thread's that you create for dispatchers?
  [multiple funct]  --> Might be helpful to track the ID's of your threads in a global array
  What kind of locks will you need to make everything thread safe? [Hint you need multiple]
  What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
  How will you track the number of images in the database?
  How will you track the requests globally between threads? How will you ensure this is thread safe? Example: request_t req_entries[MAX_QUEUE_LEN];
  [multiple funct]  --> How will you update and utilize the current number of requests in the request queue?
  [worker()]        --> How will you track which index in the request queue to remove next?
  [dispatcher()]    --> How will you know where to insert the next request received into the request queue?
  [multiple funct]  --> How will you track the p_thread's that you create for workers? TODO
  How will you store the database of images? What data structure will you use? Example: database_entry_t database[100];
*/

// TODO: Implement this function
/**********************************************
 * image_match
   - parameters:
      - input_image is the image data to compare
      - size is the size of the image data
   - returns:
       - database_entry_t that is the closest match to the input_image
************************************************/
// just uncomment out when you are ready to implement this function
database_entry_t image_match(char *input_image, int size)
{
  // const char *closest_file     = NULL;
  // int         closest_distance = INT_MAX;
  // int closest_index = 0;
  // for(int i = 0; i < 0 /* replace with your database size*/; i++)
  // {
  // 	const char *current_file; /* TODO: assign to the buffer from the database struct*/
  // 	int result = memcmp(input_image, current_file, size);
  // 	if(result == 0)
  // 	{
  // 		return database[i];
  // 	}

  // 	else if(result < closest_distance)
  // 	{
  // 		closest_distance = result;
  // 		closest_file     = current_file;
  //     closest_index = i;
  // 	}
  // }

  // if(closest_file != NULL)
  // {
  //   return database[closest_index];
  // }
  // else
  // {
  //   return database[closest_index];
  // }
}

// TODO: Implement this function
/**********************************************
 * LogPrettyPrint
   - parameters:
      - to_write is expected to be an open file pointer, or it
        can be NULL which means that the output is printed to the terminal
      - All other inputs are self explanatory or specified in the writeup
   - returns:
       - no return value
************************************************/
void LogPrettyPrint(FILE *to_write, int threadId, int requestNumber, char *file_name, int file_size)
{
}
/*
  TODO: Implement this function for Intermediate Submission
  * loadDatabase
    - parameters:
        - path is the path to the directory containing the images
    - returns:
        - no return value
    - Description:
        - Traverse the directory and load all the images into the database
          - Load the images from the directory into the database
          - You will need to read the images into memory
          - You will need to store the image data in the database_entry_t struct
          - You will need to store the file name in the database_entry_t struct
          - You will need to store the file size in the database_entry_t struct
          - You will need to store the image data in the database_entry_t struct
          - You will need to increment the number of images in the database
*/
/***********/

void loadDatabase(char *path)
{
  DIR *dir;
  struct dirent *entry;
  struct stat statbuf;

  dir = opendir(path);
  if (dir == NULL)
  {
    perror("failed to open database directory");
    exit(1);
  }

  while ((entry = readdir(dir)) != NULL)
  {

    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    char filepath[BUFF_SIZE];
    sprintf(filepath, "%s/%s", path, entry->d_name);

    if (stat(filepath, &statbuf) != 0 || !S_ISREG(statbuf.st_mode))
    {
      perror("stat failed");
      continue;
    }
    off_t file_size = statbuf.st_size;
    printf("file_size: %d\n", file_size);
    FILE *file;
    database[image_count].file_name = malloc(BUFF_SIZE);
    database[image_count].buffer = malloc(file_size);

    if (database[image_count].buffer == NULL || database[image_count].file_name == NULL)
    {
      perror("malloc failed");
      fclose(file);
    }
    database[image_count].file_size = file_size;
    strcpy(database[image_count].file_name, entry->d_name);

    // Open and read file contents into a buffer

    printf("filepath: %s\n", filepath);
    file = fopen(filepath, "rb");
    if (!file)
    {
      perror("fopen failed");
      continue;
    }

    size_t bytes_read = fread(database[image_count].buffer, 1, file_size, file);
    if (bytes_read != file_size)
    {
      perror("fread failed");
    }
    else
    {
      // Read the file into the buffer
      printf("File: %s, Size: %ld bytes\n", entry->d_name, file_size);
    }

    fclose(file);
    image_count++;
  }
  closedir(dir);
}

void *dispatch(void *arg)
{
  while (1)
  {
    size_t file_size = 0;
    request_detials_t request_details;

    /* TODO: Intermediate Submission
     *    Description:      Accept client connection
     *    Utility Function: int accept_connection(void)
     */

    /* TODO: Intermediate Submission
     *    Description:      Get request from client
     *    Utility Function: char * get_request_server(int fd, size_t *filelength)
     */

    /* TODO
     *    Description:      Add the request into the queue
         //(1) Copy the filename from get_request_server into allocated memory to put on request queue


         //(2) Request thread safe access to the request queue

         //(3) Check for a full queue... wait for an empty one which is signaled from req_queue_notfull

         //(4) Insert the request into the queue

         //(5) Update the queue index in a circular fashion

         //(6) Release the lock on the request queue and signal that the queue is not empty anymore
    */
  }
  return NULL;
}

void *worker(void *arg)
{

  int num_request = 0; // Integer for tracking each request for printing into the log file
  int fileSize = 0;    // Integer to hold the size of the file being requested
  void *memory = NULL; // memory pointer where contents being requested are read and stored
  int fd = INVALID;    // Integer to hold the file descriptor of incoming request
  char *mybuf;         // String to hold the contents of the file being requested

  /* TODO : Intermediate Submission
   *    Description:      Get the id as an input argument from arg, set it to ID
   */

  while (1)
  {
    /* TODO
    *    Description:      Get the request from the queue and do as follows
      //(1) Request thread safe access to the request queue by getting the req_queue_mutex lock
      //(2) While the request queue is empty conditionally wait for the request queue lock once the not empty signal is raised

      //(3) Now that you have the lock AND the queue is not empty, read from the request queue

      //(4) Update the request queue remove index in a circular fashion

      //(5) Fire the request queue not full signal to indicate the queue has a slot opened up and release the request queue lock
      */

    /* TODO
     *    Description:       Call image_match with the request buffer and file size
     *    store the result into a typeof database_entry_t
     *    send the file to the client using send_file_to_client(int socket, char * buffer, int size)
     */
  }
}

int main(int argc, char *argv[])
{
  if (argc != 6)
  {
    printf("usage: %s port path num_dispatcher num_workers queue_length \n", argv[0]);
    return -1;
  }

  int port = -1;
  char path[BUFF_SIZE] = "no path set\0";
  num_dispatcher = -1; // global variable
  num_worker = -1;     // global variable
  queue_len = -1;      // global variable

  /* DONE Intermediate Submission
   *    Description:      Get the input args --> (1) port (2) path (3) num_dispatcher (4) num_workers  (5) queue_length
   */
  port = atoi(argv[1]);
  strcpy(path, argv[2]);
  num_dispatcher = atoi(argv[3]);
  num_worker = atoi(argv[4]);
  queue_len = atoi(argv[5]);

  printf("path: %s\n", path);

  /* DONE Intermediate Submission
   *    Description:      Open log file
   *    Hint:             Use Global "File* logfile", use "server_log" as the name, what open flags do you want?
   */

  logfile = fopen("server_log.txt", "w");
  if (logfile == NULL)
  {
    perror("failed to open server_log");
    exit(1);
  }

  /* DONE: Intermediate Submission
   *    Description:      Start the server
   *    Utility Function: void init(int port); //look in utils.h
   */
  init(port);

  /* DONE: Intermediate Submission
   *    Description:      Load the database
   */
  loadDatabase(path);

  /* TODO: Intermediate Submission
   *    Description:      Create dispatcher and worker threads
   *    Hints:            Use pthread_create, you will want to store pthread's globally
   *                      You will want to initialize some kind of global array to pass in thread ID's
   *                      How should you track this p_thread so you can terminate it later? [global]
   */

  // Wait for each of the threads to complete their work
  // Threads (if created) will not exit (see while loop), but this keeps main from exiting
  // int i;
  // for (i = 0; i < num_dispatcher; i++)
  // {
  //   fprintf(stderr, "JOINING DISPATCHER %d \n", i);
  //   if ((pthread_join(dispatcher_thread[i], NULL)) != 0)
  //   {
  //     printf("ERROR : Fail to join dispatcher thread %d.\n", i);
  //   }
  // }
  // for (i = 0; i < num_worker; i++)
  // {
  //   // fprintf(stderr, "JOINING WORKER %d \n",i);
  //   if ((pthread_join(worker_thread[i], NULL)) != 0)
  //   {
  //     printf("ERROR : Fail to join worker thread %d.\n", i);
  //   }
  // }
  for (int i = 0; i < image_count; i++)
  {
    free(database[i].buffer);
    free(database[i].file_name);
  }
  fprintf(stderr, "SERVER DONE \n"); // will never be reached in SOLUTION
  fclose(logfile);                   // closing the log files
}