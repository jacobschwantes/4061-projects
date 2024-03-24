#include "utils.h"

// Run the (executable, parameter) pairs in batches of 8 to avoid timeouts due to 
// having too many child processes running at once
#define PAIRS_BATCH_SIZE 8

typedef struct {
    char *executable_path;
    int parameter;
    int status;
} pairs_t;

// Store the pairs tested by this worker and the results
pairs_t *pairs;

// Information about the child processes and their results
pid_t *pids;
int *child_status;     // Contains status of child processes (-1 for done, 1 for still running)

int curr_batch_size;   // At most PAIRS_BATCH_SIZE (executable, parameter) pairs will be run at once
long worker_id;        // Used for sending/receiving messages from the message queue


// * Timeout handler for alarm signal - should be the same as the one in autograder.c
void timeout_handler(int signum) {
    for(int i = 0; i < curr_batch_size; i++){
        if(child_status[i] == 1){
            kill(pids[i], SIGKILL);
            child_status[i] = -1;
        }
    }
}


// Execute the student's executable using exec()
void execute_solution(char *executable_path, int param, int batch_idx) {
 
    pid_t pid = fork();

    // Child process
    if (pid == 0) {
        char *executable_name = get_exe_name(executable_path);

        // * Redirect STDOUT to output/<executable>.<input> file

        // * Input to child program can be handled as in the EXEC case (see template.c)
        char output_file_name[256];
        snprintf(output_file_name, sizeof(output_file_name), "output/%s.%d", executable_name, param);
        int fd = open(output_file_name, O_WRONLY | O_CREAT, 0666);
        if(fd == -1){
            perror("Failed to open output file");
            // fprintf(stderr, "Failed to open output file: %s\n", output_file_name);
            exit(1);
        }
        if(dup2(fd, 1) == -1){
            perror("Failed to redirect");
            exit(1);
        }
        close(fd);
        char param_s[10];
        sprintf(param_s, "%d", param);
        execl(executable_path, executable_name, param_s, NULL);

        perror("Failed to execute program in worker");
        exit(1);
    }
    // Parent process
    else if (pid > 0) {
        pids[batch_idx] = pid;
    }
    // Fork failed
    else {
        perror("Failed to fork");
        exit(1);
    }
}


// Wait for the batch to finish and check results
void monitor_and_evaluate_solutions(int finished) {
    // Keep track of finished processes for alarm handler
    child_status = malloc(curr_batch_size * sizeof(int));
    for (int j = 0; j < curr_batch_size; j++) {
        child_status[j] = 1;
    }

    // MAIN EVALUATION LOOP: Wait until each process has finished or timed out
    for (int j = 0; j < curr_batch_size; j++) {
        char *current_exe_path = pairs[finished + j].executable_path;
        int current_param = pairs[finished + j].parameter;

        int status;
        pid_t pid = waitpid(pids[j], &status, 0);

        // * What if waitpid is interrupted by a signal?
        while(pid == -1 && errno == EINTR){
            pid = waitpid(pids[j], &status, 0);
        }

        int exit_status = WEXITSTATUS(status);
        int exited = WIFEXITED(status);
        int signaled = WIFSIGNALED(status);

        // TODO: Check if the process finished normally, segfaulted, or timed out and update the 
        //       pairs array with the results. Use the macros defined in the enum in utils.h for 
        //       the status field of the pairs_t struct (e.g. CORRECT, INCORRECT, SEGFAULT, etc.)
        //       This should be the same as the evaluation in autograder.c, just updating `pairs` 
        //       instead of `results`.
        if(exited){
            char output_file_name[256];
            char *exec_name = get_exe_name(current_exe_path);
            snprintf(output_file_name, sizeof(output_file_name), "output/%s.%d", exec_name, current_param);

            FILE *fd = fopen(output_file_name, "r");
            char in_file = fgetc(fd);
            if(in_file == '0'){
                pairs[finished + j].status = CORRECT;
            }
            else{
                pairs[finished + j].status = INCORRECT;
            }
        }
        else if(signaled){
            int sig = WTERMSIG(status);
            if(sig == SIGKILL || sig == SIGALRM){
                pairs[finished + j].status = STUCK_OR_INFINITE;
            }
            else{
                pairs[finished + j].status = SEGFAULT;
            }
        }
        // printf("finished eval: %s %d\n", pairs[finished + j].executable_path, pairs[finished + j].parameter);
        // Mark the process as finished
        child_status[j] = -1;
    }

    free(child_status);
}


// Send results for the current batch back to the autograder
void send_results(int msqid, long mtype, int finished) {
    // Format of message should be ("%s %d %d", executable_path, parameter, status)
    msgbuf_t message;
    message.mtype = mtype;
    for(int i = 0; i < curr_batch_size; i++){
        snprintf(message.mtext, sizeof(message.mtext), "%s %d %d", pairs[finished + i].executable_path, pairs[finished + i].parameter, pairs[finished + i].status);
        if(msgsnd(msqid,  &message, sizeof(message.mtext), 0) == -1){
            perror("failed to send results");
            exit(1);
        }
    }
    // printf("results sent\n");
}


// Send DONE message to autograder to indicate that the worker has finished testing
void send_done_msg(int msqid, long mtype) {
    msgbuf_t message;
    message.mtype = mtype;
    strncpy(message.mtext, "DONE", sizeof(message.mtext));
    if(msgsnd(msqid,  &message, sizeof(message.mtext), 0) == -1){
        perror("failed to send DONE message");
        exit(1);
    }
    // printf("DONE sent\n");
}


int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <msqid> <worker_id>\n", argv[0]);
        return 1;
    }

    int msqid = atoi(argv[1]);
    worker_id = atoi(argv[2]);

    // * Receive initial message from autograder specifying the number of (executable, parameter) 
    // pairs that the worker will test (should just be an integer in the message body). (mtype = worker_id)
    msgbuf_t message;
    if(msgrcv(msqid, &message, sizeof(message.mtext), worker_id, 0) == -1){
        perror("Failed to receive initial message");
        exit(1);
    }
    // * Parse message and set up pairs_t array
    int pairs_to_test;
    pairs_to_test = atoi(message.mtext);
    pairs = malloc(pairs_to_test * sizeof(pairs_t));

    // * Receive (executable, parameter) pairs from autograder and store them in pairs_t array.
    //       Messages will have the format ("%s %d", executable_path, parameter). (mtype = worker_id)
    for(int i = 0; i < pairs_to_test; i++){
        if(msgrcv(msqid,  &message, sizeof(message.mtext), worker_id, 0) == -1){
            perror("failed to receive (exec, param) pair");
            exit(1);
        }
        char exec_path[256];
        int param;
        sscanf(message.mtext, "%s %d", exec_path, &param);
        pairs[i].executable_path = strdup(exec_path);
        pairs[i].parameter = param;
        pairs[i].status = 0;
    }
    // * Send ACK message to mq_autograder after all pairs received (mtype = BROADCAST_MTYPE)
    message.mtype = BROADCAST_MTYPE;
    sprintf(message.mtext, "%s", "ACK");
    if(msgsnd(msqid, &message, sizeof(message.mtext), 0) == -1){
        perror("Failed to send ACK msg");
        exit(1);
    }
    // * Wait for SYNACK from autograder to start testing (mtype = BROADCAST_MTYPE).
    //       Be careful to account for the possibility of receiving ACK messages just sent.
    while(1){
        if(msgrcv(msqid, &message, sizeof(message.mtext), BROADCAST_MTYPE, 0) == -1){
            perror("Failed to receive SYNACK");
            exit(1);
        }
        if(strcmp(message.mtext, "SYNACK") == 0){
            // printf("SYNACK received\n");
            break;
        }
        else{
            // if not a SYNACK message, then it's an ACK. Resend the message until autograder receives it
            if(msgsnd(msqid, &message, sizeof(message.mtext), 0) == -1){
                perror("Failed to resend ACK");
                exit(1);
            }
        }
    }


    // Run the pairs in batches of 8 and send results back to autograder
    for (int i = 0; i < pairs_to_test; i+= PAIRS_BATCH_SIZE) {
        int remaining = pairs_to_test - i;
        curr_batch_size = remaining < PAIRS_BATCH_SIZE ? remaining : PAIRS_BATCH_SIZE;
        pids = malloc(curr_batch_size * sizeof(pid_t));

        for (int j = 0; j < curr_batch_size; j++) {
            // * Execute the student executable
            execute_solution(pairs[i + j].executable_path, pairs[i + j].parameter, j);
        }

        // * Setup timer to determine if child process is stuck
            start_timer(TIMEOUT_SECS, timeout_handler);  // Implement this function (src/utils.c)

        // * Wait for the batch to finish and check results
        monitor_and_evaluate_solutions(i);

        // * Cancel the timer if all child processes have finished
        if (child_status == NULL) {
            cancel_timer();
        }

        // * Send batch results (intermediate results) back to autograder
        send_results(msqid, worker_id, i);

        free(pids);
    }

    // * Send DONE message to autograder to indicate that the worker has finished testing
    send_done_msg(msqid, worker_id);

    // Free the pairs_t array
    for (int i = 0; i < pairs_to_test; i++) {
        free(pairs[i].executable_path);
    }
    free(pairs);

    // free(pids);
}