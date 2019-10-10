#define _POSIX_C_SOURCE 200809L

#include<stdlib.h>
#include<stdio.h>
#include<time.h>
#include<errno.h>
#include<unistd.h>
#include <signal.h>
#include<string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>
#include <linkedlist.h>

static volatile int running = 1;
time_t lastint = 0;
pid_t *pids;

typedef struct client_stats {
    char id[16];
    int best_estimate;
    int n_estimates_received;
} client_stats;

linkedlist_elem *list = NULL;

//spawns n_servers servers
int spawn_servers(int n_servers, int pipefds[n_servers][2]){
    int i = 0;
    pids = (pid_t*)calloc(n_servers, sizeof(pid_t));
    while(i < n_servers){
        pid_t pid = fork();
        if (pid == 0){
            //we're in the child
            //child closing useless pipes before exec
            //sleep(3);
            //printf("Child #%d\n", i + 1);
            for (int j = 0; j < n_servers; j++){
                if (j != i){
                    if (close(pipefds[j][0]) && close(pipefds[j][1]) == -1)
                    perror("Error closing other pipes from child");
                }
                else if (close(pipefds[j][0]) == -1)
                    perror("Error closing reading pipe from child");
            }
            char server_num_arg[4];
            char pipefd_arg[4];
            snprintf(server_num_arg, 4, "%d", i+1);
            snprintf(pipefd_arg, 4, "%d", pipefds[i][1]);
            execl("server", "server", server_num_arg, pipefd_arg, NULL);
            return 1;
        }

        //we're in the father
        pids[i] = pid;
        i++;
    }
    return 0;
}

int iter(const void *ptr, void *arg) {
    client_stats *stat = (client_stats*)ptr;
    char *id = (char*)arg;
    return strcmp(stat->id, id) == 0;
}

void parsebuf(char *buff, int size) {
    printf("%s\n", buff);
	char buf[64] = {0};
	strncpy(buf, buff, 64);
	strtok(buf, " ");
	char *id = strtok(NULL, " ");
	strtok(NULL, " ");
	char *estimate = strtok(NULL, " ");
	strtok(NULL, " ");
	strtok(NULL, " ");
	char *server = strtok(NULL, " ");
	int estim = atoi(estimate);
	int srv = atoi(server);
	printf("SUPERVISOR ESTIMATE %d FOR %s FROM %d\n", estim, id, srv);
    client_stats *elem = (client_stats*)linkedlist_search(list, &iter, id);
    if (elem) {
        if (elem->best_estimate > estim) elem->best_estimate = estim;
        elem->n_estimates_received = elem->n_estimates_received + 1;
    } else {
        client_stats *stat = (client_stats*)calloc(1, sizeof(client_stats));
        list = linkedlist_new(list, stat);
        strncpy(stat->id, id, 8);
        stat->best_estimate = estim;
        stat->n_estimates_received = 1;
    }
}

static int sigiter(const void *ptr, void *arg) {
    client_stats *stat = (client_stats*)ptr;
    int *target = (int*)arg;
    if (target) fprintf(stdout, "SUPERVISOR ESTIMATE %d FOR %s BASED ON %d\n", stat->best_estimate, stat->id, stat->n_estimates_received);
    else fprintf(stderr, "SUPERVISOR ESTIMATE %d FOR %s BASED ON %d\n", stat->best_estimate, stat->id, stat->n_estimates_received);
    return 0;
}

void int_handler(int signum) {
    if (time(NULL) - lastint <= 1) running = 0;
    else {
        int target = 0;
        linkedlist_search(list, &sigiter, &target);
    }
    lastint = time(NULL);
}

int main(int argc, char* argv[]){

    struct sigaction sa;

    sa.sa_handler = int_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* Restart functions if interrupted by handler */

    if (sigaction(SIGINT, &sa, NULL) == -1) printf("error\n");

    int n_servers = atoi(argv[1]);
    fd_set set, rdset;

    //setting up linked list


    //setting up n_servers pipes
    int pipefds[n_servers][2];
    for (int i = 0; i < n_servers; i ++){
        if (pipe(pipefds[i]) == -1){
            perror("Error setting up pipes");
        };
    }
    //printf("MAX PIPE BYTES: %d\n", _PC_PIPE_BUF);

    //writing check on stdout
    printf("SUPERVISOR STARTING %d\n", n_servers);

    //spawns n_servers servers and closes pipes on children
    if(spawn_servers(n_servers, pipefds))
        //this means exec returned != 0, wtf?
        perror("Problems with server spawning");
    
    sleep(1);
    printf("Supervisor successfully started %d servers.\n", n_servers);

    //I have to actively wait for pipe fds to be ready, might be using select here.

    //close all pipes' writing ends from supervisor
    for(int i = 0; i < n_servers; i++){ 
       if (close(pipefds[i][1]) == -1){
           perror("Error closing writing pipes");
       }
    }

    sleep(2);

    //preparing select to check for arriving estimates
    FD_ZERO(&set);
    for (int i = 0; i < n_servers; i++){
        FD_SET(pipefds[i][0], &set);
    }

    while(running){
        rdset = set;
        if (select(FD_SETSIZE, &rdset, NULL, NULL, NULL) < 0){
            if (errno != EINTR) {
                perror("select");
                exit (EXIT_FAILURE);
            }
        }
        else {
            for (int fd = 0; fd < FD_SETSIZE; fd++){
                if (FD_ISSET(fd, &rdset)){
                    char buf[64] = {0};
                    int valread = 0;
                    if ((valread = read(fd, buf, sizeof(char) * 64) == -1)){
                        exit(EXIT_FAILURE);
                    };
					if (strlen(buf) > 0) parsebuf(buf, valread);
                }
            }
        }
    }

    for (int i = 0; i < n_servers; i++) kill(pids[i], SIGTERM);
    free(pids);
    
    int target = 1;
    linkedlist_search(list, &sigiter, &target);
    printf("SUPERVISOR EXITING\n");
    
    fflush(stdout);
    linkedlist_free(list);

    return 0;

}
