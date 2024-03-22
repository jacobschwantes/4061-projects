#include "utils.h"

pid_t *workers;          // Workers determined by batch size
int *worker_done;        // 1 for done, 0 for still running

// Stores the results of the autograder (see utils.h for details)
autograder_results_t *results;

int num_executables;      // Number of executables in test directory
int total_params;         // Total number of parameters to test - (argc - 2)
int num_workers;          // Number of workers to spawn


void launch_worker(int msqid, int pairs_per_worker, int worker_id) {
    
    pid_t pid = fork();

    // Child process
    if (pid == 0) {

        // * exec() the worker program and pass it the message queue id and worker id.
        //       Use ./worker as the path to the worker program.
        char msqid_s[10];
        char worker_id_s[10];
        sprintf(msqid_s, "%d", msqid);
        sprintf(worker_id_s, "%d", worker_id);
        execl("./worker", "./worker", msqid_s, worker_id_s, NULL);

        perror("Failed to spawn worker");
        exit(1);
    } 
    // Parent process
    else if (pid > 0) {
        // TODO: Send the total number of pairs to worker via message queue (mtype = worker_id)
        msgbuf_t message;
        message.mtype = worker_id;
        sprintf(message.mtext, "%d", pairs_per_worker);
        if(msgsnd(msqid,  &message, sizeof(message.mtext), 0) == -1){
            perror("failed to send message");
            exit(1);
        }

        // Store the worker's pid for monitoring
        workers[worker_id - 1] = pid;
    }
    // Fork failed 
    else {
        perror("Failed to fork worker");
        exit(1);
    }
}


// TODO: Receive ACK from all workers using message queue (mtype = BROADCAST_MTYPE)
void receive_ack_from_workers(int msqid, int num_workers) {
    msgbuf_t message;
    for(int i = 0; i < num_workers; i++){
        if(msgrcv(msqid,  &message, sizeof(message.mtext), BROADCAST_MTYPE, 0) == -1){
            perror("failed to receive ACK message");
            exit(1);
        }
    }
}


// TODO: Send SYNACK to all workers using message queue (mtype = BROADCAST_MTYPE)
void send_synack_to_workers(int msqid, int num_workers) {
    msgbuf_t message;
    message.mtype = BROADCAST_MTYPE;
    strcpy(message.mtext, "SYNACK");
    printf("%s\n", message.mtext);
    for(int i = 0; i < num_workers; i++){
        if(msgsnd(msqid,  &message, sizeof(message.mtext), 0) == -1){
            perror("failed to send SYNACK message");
            exit(1);
        }
    }
}


// Wait for all workers to finish and collect their results from message queue
void wait_for_workers(int msqid, int pairs_to_test, char **argv_params) {
    int received = 0;
    worker_done = malloc(num_workers * sizeof(int));
    for (int i = 0; i < num_workers; i++) {
        worker_done[i] = 0;
    }

    while (received < pairs_to_test) {
        for (int i = 0; i < num_workers; i++) {
            if (worker_done[i] == 1) {
                continue;
            }

            // Check if worker has finished
            pid_t retpid = waitpid(workers[i], NULL, WNOHANG);
            
            int msgflg;
            if (retpid > 0)
                // Worker has finished and still has messages to receive
                msgflg = 0;
            else if (retpid == 0)
                // Worker is still running -> receive intermediate results
                msgflg = IPC_NOWAIT;
            else {
                // Error
                perror("Failed to wait for child process");
                exit(1);
            }

            // TODO: Receive results from worker and store them in the results struct.
            //       If message is "DONE", set worker_done[i] to 1 and break out of loop.
            //       Messages will have the format ("%s %d %d", executable_path, parameter, status)
            //       so consider using sscanf() to parse the message.
            char exec_path[100];
            int parameter;
            int status;
            msgbuf_t message;
            while (1) {
                if(msgrcv(msqid, &message, sizeof(message.mtext), workers[i], msgflg) == -1){
                    perror("Failed to receive results from worker");
                    exit(1);
                }
                if(strcmp(message.mtext, "DONE") == 0){
                    worker_done[i] = 1;
                    received++;
                    break;
                }
                break;
            }
            sscanf(message.mtext, "%s %d %d", exec_path, &parameter, &status);
            results[i].exe_path = exec_path;
            results[i].params_tested = &parameter;
            results[i].status = &status;
            
        }
    }

    free(worker_done);
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <testdir> <p1> <p2> ... <pn>\n", argv[0]);
        return 1;
    }

    char *testdir = argv[1];
    total_params = argc - 2;

    char **executable_paths = get_student_executables(testdir, &num_executables);

    // Construct summary struct
    results = malloc(num_executables * sizeof(autograder_results_t));
    for (int i = 0; i < num_executables; i++) {
        results[i].exe_path = executable_paths[i];
        results[i].params_tested = malloc((total_params) * sizeof(int));
        results[i].status = malloc((total_params) * sizeof(int));
    }

    num_workers = get_batch_size();
    // Check if some workers won't be used -> don't spawn them
    if (num_workers > num_executables * total_params) {
        num_workers = num_executables * total_params;
    }
    workers = malloc(num_workers * sizeof(pid_t));

    // Create a unique key for message queue
    key_t key = IPC_PRIVATE;

    // * Create a message queue
    int msqid;
    msqid = msgget(key, IPC_CREAT | 0666);

    int num_pairs_to_test = num_executables * total_params;
    
    // Spawn workers and send them the total number of (executable, parameter) pairs they will test
    for (int i = 0; i < num_workers; i++) {
        int leftover = num_pairs_to_test % num_workers - i > 0 ? 1 : 0;
        int pairs_per_worker = num_pairs_to_test / num_workers + leftover;

        // TODO: Spawn worker and send it the number of pairs it will test via message queue
        launch_worker(msqid, pairs_per_worker, i + 1);
    }

    // Send (executable, parameter) pairs to workers
    int sent = 0;
    for (int i = 0; i < total_params; i++) {
        for (int j = 0; j < num_executables; j++) {
            msgbuf_t msg;
            long worker_id = sent % num_workers + 1;
            // TODO: Send (executable, parameter) pair to worker via message queue (mtype = worker_id)
            msg.mtype = worker_id;
            snprintf(msg.mtext, sizeof(msg.mtext), "%s %d", executable_paths[j], i + 1);
            if(msgsnd(msqid,  &msg, sizeof(msg.mtext), 0) == -1){
                perror("failed to send (executable, parameter) pair");
                exit(1);
            }
            sent++;
        }
    }

    // TODO: Wait for ACK from workers to tell all workers to start testing (synchronization)
    receive_ack_from_workers(msqid, num_workers);

    // TODO: Send message to workers to allow them to start testing
    send_synack_to_workers(msqid, num_workers);

    // TODO: Wait for all workers to finish and collect their results from message queue
    wait_for_workers(msqid, num_pairs_to_test, argv + 2);

    // * Remove ALL output files (output/<executable>.<input>)
    for(int i = 0; i < total_params; i++){
        remove_output_files(results, sent, num_executables, argv[i]);
    }

    write_results_to_file(results, num_executables, total_params);

    // You can use this to debug your scores function
    // get_score("results.txt", results[0].exe_path);

    // Print each score to scores.txt
    write_scores_to_file(results, num_executables, "results.txt");

    // * Remove the message queue
    msgctl(msqid, IPC_RMID, 0);


    // Free the results struct and its fields
    for (int i = 0; i < num_executables; i++) {
        free(results[i].exe_path);
        free(results[i].params_tested);
        free(results[i].status);
    }

    free(results);
    free(executable_paths);
    free(workers);
    
    return 0;
}