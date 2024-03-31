# 4061 Project 3 - Multi Threaded Image Matching Server

## Team - 26

- Jacob Schwantes [schw2550]
- Nadir Mustafa [musta099]

## Contributions

- Jacob: Implementation
- Nadir: Research and debugging

## Usage

Before proceeding, ensure the following tools are installed and accessible from your command line or terminal:

- **GNU Make**
- **GCC (GNU Compiler Collection)**

### Compile

- First compile the server and client executables.

```
make all
```

### Run

Run the server

```
./server <Port> <Database path> <num_dispatcher> <num_workers> <queue_length>
```

- `Port`: Socket the server will listen for requests on.
- `Database path`: Directory where database images are located.
- `num_dispatcher`: Number of dispatcher threads to receive incoming requests. Each dispatcher will serve one client request.
- `num_workers`: Number of worker threads to create to process client requests from the queue.
- `queue_length`: Size of the queue buffer for client requests to be inserted from the dispatcher.

Run the client

```
./client <directory path> <server Port> <output directory path>
```

- `directory path`: Directory where input images are located that will be checked by the server against the database images.
- `server Port`: Port that the client will connect to the server on.
- `output directory path`: Directory to output matching images recieved by server in response to requests.

## Assumptions

- Server is meant to be ran with a number of worker and dispatcher threads equal to the number of requests the client will send. Otherwise the server will keep waiting for incoming requests that never come since the client only creates one thread/request for each image in the input directory.

## Notes

Program was tested on machine `csel-kh1250-23.cselabs.umn.edu`.

We plan on using using mutex locks to protect when adding items to the queue and removing them. We also will use mutex conditionals and signals to wake workers to process items in the queue.
