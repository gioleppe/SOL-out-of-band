#define _POSIX_C_SOURCE 200809L

#include<stdlib.h>
#include<stdio.h>
#include<time.h>
#include<errno.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<string.h>
#include <arpa/inet.h>
#include <stdint.h>

int sent_messages;

/*multiplicative congruential generator
long long mcg64(void){
    int i = rand();
    return (i = (164603309694725029ull * i) % 14738995463583502973ull);
}*/

//opens a socket at "OOB-server-server_num"
int open_socket(int server_num){
    char* server_name = calloc(1, 16);
    snprintf(server_name, 16, "OOB-server-%d", server_num);
    //printf("Opening socket on %s\n", server_name);
    fflush(stdout);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, server_name, sizeof(addr.sun_path) - 1);
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("connect");
        exit(EXIT_FAILURE);
    }
    free(server_name);

    return sockfd;
}


int main(int arcg, char* argv[]){

    //p, k e w, with 1 ≤ p <k and w > 3p. else set errno to EINVAL
    //assign args and checking for correctness
    int p = atoi(argv[1]); //5 
    int k = atoi(argv[2]); //8
    int w = atoi(argv[3]); //20
    if (p >= k || p <= 1 || w > 3*k){
        errno = EINVAL;
        perror("you made a mistake with the args"); 
        exit(EXIT_FAILURE);
    }


    //initializing rand and generating secret
    int pid = getpid();
    int seed = (pid * 776531419) ^ time(NULL);
    srand(seed);
    int secret = rand() % 3000 + 1;

    //generating a random ID
    long ID = rand();
    //printf("%lu\n", sizeof(ID));

    //printing on stdout
    printf("CLIENT %lX SECRET %d\n", ID, secret);

    fflush(stdout);


    //choosing p distinct numbers between 1 and k
    int* chosen_servers = calloc(k+1, sizeof(int*));
    int i = 0;
    while (i < p){
        int chosen = rand() % k;
        if (chosen_servers[chosen] == 0){
            chosen_servers[chosen] = 1;
            i++;
        }
    }

    int* sock_fds = calloc(k+1, sizeof(int*));

    //open socket for chosen servers
    for (int i = 0; i < k; i++){
        if (chosen_servers[i] == 1){
        //printf("Chose server #%d\n", i + 1);
        sock_fds[i] = open_socket(i + 1);
        }
    }


    //while we still have messages to send, send them to one of the chosen servers
    //this loop implements the client's main logic
    sent_messages = 0;

    //preparing timespec for nanosleep
    struct timespec tim;
    tim.tv_sec = secret/1000;
    tim.tv_nsec = (secret % 1000) * 1e6;
    //ID in network byte order
    uint32_t nbID = htonl(ID);

    while (sent_messages < w){
        int chosen = abs(rand() * 113 % k);
        //printf("trying to send to server %d\n", chosen + 1);
        if(chosen_servers[chosen] == 1){
            int bytes_sent = 0;
           // printf("Sending id to OOB-server-%d\n", chosen + 1);
            //printf("nboID: %x\n", nbID);
            char buf[16] = {0};
            snprintf(buf, 16, "%x", nbID);
            char to_send[8] = {0};
            memcpy(to_send, buf, 8);
            if ((bytes_sent = send(sock_fds[chosen], to_send, sizeof(to_send), 0)) < 0){
                perror("Probably the sock was closed by server");
            }
            //printf("%x\n", nbID);
            //printf("%s\n", to_send);
            nanosleep(&tim, NULL);
            sent_messages ++;
            //printf("sent %d messages, last message was %d bytes\n", sent_messages, bytes_sent);
            fflush(stdout);

        }   
    }

    for (int i = 0; i < k; i++){
        close(sock_fds[i]);
    }
    
    free(sock_fds);
    free(chosen_servers);

    printf("CLIENT %lX DONE\n", ID);
    return 0;

}