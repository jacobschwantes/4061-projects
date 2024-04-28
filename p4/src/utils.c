#include "../include/utils.h"
#include "../include/server.h"
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <stdint.h>

//? Declare a global variable to hold the file descriptor for the server socket
int master_fd;
//? Declare a global variable to hold the mutex lock for the server socket
static pthread_mutex_t master_fd_mutex = PTHREAD_MUTEX_INITIALIZER;
//? Declare a gloabl socket address struct to hold the address of the server
struct sockaddr_in server_addr;
/*
################################################
##############Server Functions##################
################################################
*/

/**********************************************
 * init
   - port is the number of the port you want the server to be
     started on
   - initializes the connection acception/handling system
   - if init encounters any errors, it will call exit().
************************************************/
void init(int port)
{
  // ? create an int to hold the socket file descriptor
  int sd;
  //?create a sockaddr_in struct to hold the address of the server
  struct sockaddr_in addr;

  /**********************************************
   * IMPORTANT!
   * ALL TODOS FOR THIS FUNCTION MUST BE COMPLETED FOR THE INTERIM SUBMISSION!!!!
   **********************************************/

  // TODO: Create a socket and save the file descriptor to sd (declared above)
  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd == -1)
  {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }
  // TODO: Change the socket options to be reusable using setsockopt().
  int opt = 1;
  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
  {
    perror("setsockopt(SO_REUSEADDR) failed");
    close(sd);
    exit(EXIT_FAILURE);
  }

  // TODO: Bind the socket to the provided port.
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY; // Bind to any address
  addr.sin_port = htons(port);       // Port number
  if (bind(sd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("Bind failed");
    close(sd);
    exit(EXIT_FAILURE);
  }

  // TODO: Mark the socket as a pasive socket. (ie: a socket that will be used to receive connections)
  if (listen(sd, SOMAXCONN) < 0)
  { // SOMAXCONN defines the maximum number of pending connections
    perror("Listen failed");
    close(sd);
    exit(EXIT_FAILURE);
  }

  // We save the file descriptor to a global variable so that we can use it in accept_connection().
  // TODO: Save the file descriptor to the global variable master_fd
  pthread_mutex_lock(&master_fd_mutex);
  master_fd = sd;
  pthread_mutex_unlock(&master_fd_mutex);

  printf("UTILS.O: Server Started on Port %d\n", port);
  fflush(stdout);
}

/**********************************************
 * accept_connection - takes no parameters
   - returns a file descriptor for further request processing.
   - if the return value is negative, the thread calling
     accept_connection must should ignore request.
***********************************************/
int accept_connection(void)
{

  // TODO: create a sockaddr_in struct to hold the address of the new connection
  struct sockaddr_in addr;
  socklen_t client_len = sizeof(addr);

  int newsock; // File descriptor for the new socket

  /**********************************************
   * IMPORTANT!
   * ALL TODOS FOR THIS FUNCTION MUST BE COMPLETED FOR THE INTERIM SUBMISSION!!!!
   **********************************************/

  // TODO: Aquire the mutex lock
  pthread_mutex_lock(&master_fd_mutex);

  // TODO: Accept a new connection on the passive socket and save the fd to newsock
  newsock = accept(master_fd, (struct sockaddr *)&addr, &client_len);
  if (newsock < 0)
  {
    perror("Accept failed");
    newsock = -1; // Ensure that a negative value is returned on failure
  }
  // TODO: Release the mutex lock
  pthread_mutex_unlock(&master_fd_mutex);
  // TODO: Return the file descriptor for the new client connection
  return newsock;
}

/**********************************************
 * send_file_to_client
   - socket is the file descriptor for the socket
   - buffer is the file data you want to send
   - size is the size of the file you want to send
   - returns 0 on success, -1 on failure
************************************************/
int send_file_to_client(int socket, char *buffer, int size)
{
  // TODO: create a packet_t to hold the packet data
  packet_t packet;
  packet.size = size;

  // TODO: send the file size packet
  if (send(socket, &packet, sizeof(packet), 0) < 0)
  {
    perror("Failed to send packet");
    return -1;
  }

  // TODO: send the file data
  int bytes_sent = 0;
  while (bytes_sent < size)
  {
    int n = send(socket, buffer + bytes_sent, size - bytes_sent, 0);
    if (n < 0)
    {
      perror("Failed to send file data");
      return -1;
    }
    bytes_sent += n;
  }
  // TODO: return 0 on success, -1 on failure

  return 0; // Success
}

/**********************************************
 * get_request_server
   - fd is the file descriptor for the socket
   - filelength is a pointer to a size_t variable that will be set to the length of the file
   - returns a pointer to the file data
************************************************/
char *get_request_server(int fd, size_t *filelength)
{
  // TODO: create a packet_t to hold the packet data

  // TODO: receive the response packet

  // TODO: get the size of the image from the packet

  // TODO: recieve the file data and save into a buffer variable.

  // TODO: return the buffer

  // Create a packet_t to hold the packet data
  packet_t packet;

  // Receive the response packet
  int n = recv(fd, &packet, sizeof(packet), 0);
  if (n <= 0)
  {
    if (n == 0)
    {
      printf("Connection closed by peer\n");
    }
    else
    {
      perror("Failed to receive packet");
    }
    return NULL;
  }

  printf("recv packet in get request server with size: %d\n", packet.size);

    // Get the size of the file from the packet
  *filelength = packet.size;

  // Allocate memory for the file data
  char *buffer = malloc(packet.size);
  if (buffer == NULL)
  {
    perror("Failed to allocate memory for file data");
    return NULL;
  }

  // Receive the file data and save into buffer
  int bytes_received = 0;

    n = recv(fd, buffer, sizeof(packet.size), 0);
    if (n <= 0)
    {
      if (n == 0)
      {
        printf("Connection closed by peer during file reception\n");
      }
      else
      {
        perror("Failed to receive file data");
      }
      printf("failed in get req server\n");
      free(buffer);
      return NULL;
    }
  
  

  return buffer;
}

/*
################################################
##############Client Functions##################
################################################
*/

/**********************************************
 * setup_connection
   - port is the number of the port you want the client to connect to
   - initializes the connection to the server
   - if setup_connection encounters any errors, it will call exit().
************************************************/
int setup_connection(int port)
{
  // TODO: create a sockaddr_in struct to hold the address of the server

  // TODO: create a socket and save the file descriptor to sockfd

  // TODO: assign IP, PORT to the sockaddr_in struct

  // TODO: connect to the server

  // TODO: return the file descriptor for the socket
  // Create a sockaddr_in struct to hold the address of the server
  struct sockaddr_in server_addr;

  // Create a socket and save the file descriptor to sockfd
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // Zero out the structure
  memset(&server_addr, 0, sizeof(server_addr));

  // Assign IP, PORT to the sockaddr_in struct
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Assuming the server is on localhost

  // Connect to the server
  if (connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
  {
    perror("Connection to server failed");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  // Return the file descriptor for the socket
  return sockfd;
}

/**********************************************
 * send_file_to_server
   - socket is the file descriptor for the socket
   - file is the file pointer to the file you want to send
   - size is the size of the file you want to send
   - returns 0 on success, -1 on failure
************************************************/
int send_file_to_server(int socket, FILE *file, int size)
{
  // TODO: send the file size packet

  // TODO: send the file data

  // TODO: return 0 on success, -1 on failure
  // Send the file size packet
  packet_t packet;
  packet.size = size;
  printf("size of packet in send file to server: %d\n", packet.size);
  printf("size in send file to server: %d\n", size);
  if (send(socket, &packet, sizeof(packet), 0) < 0)
  {
    perror("Failed to send file size packet");
    return -1;
  }

  // Allocate buffer to read file data
  char *buffer = (char *)malloc(size);
  if (!buffer)
  {
    perror("Failed to allocate buffer for file data");
    return -1;
  }

  // Read file data into buffer
  if (fread(buffer, 1, size, file) != size)
  {
    perror("Failed to read file data");
    free(buffer);
    return -1;
  }

  // Send the file data

    int n = send(socket, buffer, size, 0);
    if (n < 0)
    {
      perror("Failed to send file data");
      free(buffer);
      return -1;
    }
 

  // printf("bytes sent: %d\n", bytes_sent);

  // free(buffer);
  return 0; // Success
}

/**********************************************
 * receive_file_from_server
   - socket is the file descriptor for the socket
   - filename is the name of the file you want to save
   - returns 0 on success, -1 on failure
************************************************/
int receive_file_from_server(int socket, const char *filename)
{
  // TODO: create a buffer to hold the file data

  // TODO: open the file for writing binary data

  // TODO: create a packet_t to hold the packet data

  // TODO: receive the response packet

  // TODO: get the size of the image from the packet

  // TODO: recieve the file data and write it to the file

  // TODO: return 0 on success, -1 on failure
  // Create a packet_t to hold the packet data
  packet_t packet;

  // Receive the response packet
  if (recv(socket, &packet, sizeof(packet), 0) <= 0)
  {
    perror("Failed to receive file size packet");
    return -1;
  }

  // Get the size of the file from the packet
  unsigned int size = packet.size;

  // Open the file for writing binary data
  FILE *file = fopen(filename, "wb");
  if (!file)
  {
    perror("Failed to open file");
    return -1;
  }

  // Create a buffer to hold the file data
  char *buffer = malloc(size);
  if (!buffer)
  {
    perror("Failed to allocate memory for file data");
    fclose(file);
    return -1;
  }

  // Receive the file data and write it to the file
  int total_received = 0;
  while (total_received < size)
  {
    int bytes_received = recv(socket, buffer + total_received, size - total_received, 0);
    if (bytes_received <= 0)
    {
      perror("Failed to receive file data");
      free(buffer);
      fclose(file);
      return -1;
    }
    total_received += bytes_received;
  }

  // Write to file
  if (fwrite(buffer, 1, size, file) != size)
  {
    perror("Failed to write file data to disk");
    free(buffer);
    fclose(file);
    return -1;
  }

  // Cleanup
  free(buffer);
  fclose(file);

  return 0; // Success
}
