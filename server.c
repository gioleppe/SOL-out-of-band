#define _POSIX_C_SOURCE 200809L

#include<stdlib.h>
#include<stdio.h>
#include<sys/time.h>
#include<time.h>
#include<signal.h>
#include<errno.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<pthread.h> 
#include<limits.h>
#include <sys/select.h>
#include<arpa/inet.h>


typedef struct {
    int pipefd;
    int clientfd;
    int server_num;
} client_t;

static volatile int running = 1;

int send_estimate(int pipefd, int server_num, int estimate, char id[]){
    char message[64] = {0};
    uint32_t id_hbo = ntohl(strtol(id, NULL, 16));
    //checking if the client actually sent the id.
    if (id_hbo == 0)
        return 0;
    snprintf(message, 64, "ID %X ESTIMATE %d from server %d", id_hbo, estimate, server_num);
    //printf("writing on pipe: %s\n", message);
    ssize_t bytes = write(pipefd, message, sizeof(message));
    if (bytes == -1)
        return -1;
    //printf("Written %zd bytes on pipe at %d\n", bytes, pipefd);
    fflush(stdout);
    return 0;
}

int timeval_subtract (result, x, y)
     struct timeval *result, *x, *y;{

  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
  }

void* manage_client(void* client){
    //printf("Managing client from sock %d\n", client_fd);
    client_t *client_struct = (client_t*) client;
    int sock = client_struct->clientfd;
    int pipefd = client_struct->pipefd;
    int server_num = client_struct->server_num;
    free(client_struct);

    int valread;
    char id[16] = {0};
    char buffer[16] = {0};
    struct timeval lst_message, prec_message, estimate;
    prec_message.tv_usec = INT_MAX;
    prec_message.tv_sec = LONG_MAX;
    int minimum = INT_MAX;
    while(running){

        valread = recv(sock, buffer, 8, 0);
        if (valread == -1){
            perror("reading from socket");
            exit(EXIT_FAILURE);
        }
        else if (valread == 0) break;

        gettimeofday(&lst_message, NULL);

        uint32_t hbID = ntohl(strtol(buffer, NULL, 16));
        printf("SERVER %d INCOMING FROM %X @ %ld : %d\n",
        server_num, hbID, lst_message.tv_sec, lst_message.tv_usec/1000);
        timeval_subtract(&estimate, &lst_message, &prec_message);
        //printf("received second message, estimate %ld %d\n", estimate.tv_sec, estimate.tv_usec);
        char* est = calloc(1, 32);
        snprintf(est, 32, "%ld%03d", labs(estimate.tv_sec), abs(estimate.tv_usec/1000));
        //printf("%s\n", est);
        long min_est;
        if ((min_est = strtoll(est, NULL, 10)) == 0){
            perror("strtol");
            exit(EXIT_FAILURE);
        };

        if (min_est == -1 || minimum == -1){
            perror("problems with the strtoll?");
        }

        if (minimum > min_est){
            //printf("found new minimum\n");
            minimum = min_est;
        }
        //printf("estimate: %ld\n", min_est);
        prec_message = lst_message;
        memcpy(&id, &buffer, 8);
        free(est);
        fflush(stdout);

    }
    
    //printf("Closing socket for client\n");
    close(sock);
    //printf("sending estimate: %ld\n", minimum);
    //printf("minimum: %d\n", minimum);

    //this is because the client might connect/disconnect without actually sending the id, resulting in id = 0
    send_estimate(pipefd, server_num, minimum, id);

    fflush(stdout);

    return 0;

}

static void term_handler(int signum) {
    running = 0;
}

int main(int argc, char* argv[]){
    struct sigaction sa;

    sa.sa_handler = term_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* Restart functions if interrupted by handler */

    if (sigaction(SIGTERM, &sa, NULL) == -1) printf("error\n");
    //f (sigaction(SIGINT, SIG_IGN, NULL) == -1) printf("error\n");

    int server_num = atoi(argv[1]);
    int pipefd = atoi(argv[2]);
    printf("SERVER %d ACTIVE\n", server_num);
    //printf("Anon pipe at fd #%d\n", pipefd);

    fd_set set, rdset;

    //thread to manage clients
    pthread_t thread_id;

   //prepare string for socket binding

    char* server_name = calloc(1, 16);
    snprintf(server_name, 16, "OOB-server-%d", server_num);
    //printf("This is %s\n", server_name);

    //just in case
    unlink(server_name);

    //preparing sock and starting to listen
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, server_name, sizeof(addr.sun_path) - 1);
    int master_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    bind(master_socket, (struct sockaddr*)&addr, sizeof(addr));
    listen(master_socket, SOMAXCONN);

    int addrlen = sizeof(addr);
    //clearing set and adding master socket to set
    FD_ZERO (&set);
    FD_SET (master_socket, &set);

    

    while(running){
        fflush(stdout);
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
                    if (fd == master_socket){
                        int fd_c;
                        if ((fd_c = accept(master_socket, (struct sockaddr*)&addr, (socklen_t*) &addrlen)) == -1){
                            perror("accepting new connections");
                            exit(EXIT_FAILURE);
                        }
                        //puts("Accepted new connection from client");
                        //setting up client helper struct
                        client_t *client = malloc(sizeof(client_t));
                        //memset(&client, 0, sizeof(client));
                        client->clientfd = fd_c;
                        client->pipefd = pipefd;
                        client->server_num = server_num;
                        if( pthread_create( &thread_id , NULL ,  manage_client , client) < 0){
                            perror("creating handler thread");
                        }
                    }
                }
            }
        }
    }

    //closing main socket
    close(master_socket);

    fflush(stdout);

    /*//sending estimate to supervisor
   if (send_estimate(pipefd, server_num, 100, 12345) == -1)
    perror("Problems sending estimate");*/


    
    //unlinking socket
    unlink(server_name);
    free(server_name);

    sleep(3);
    return 0;
}