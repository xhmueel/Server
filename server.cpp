#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <sys/stat.h>
#include <time.h>
#include "server_api.h"

using namespace std;

#define FALSE 0
#define TRUE 1
#define BUFFER_MAX 128



//default port
#define PORT "58016"

//pid for udp_server and tcp_server
pid_t udp_pid;
pid_t tcp_pid;


//create verbose vector string and put "n" as default
string verbose;
//GS port
string port;
//name of the words file
string word_file;
//file descriptor of word_file
FILE *word_file_fd;

int main(int argc, char **argv){
    //put "n" as default in verbose vector
    verbose = "n";
    //put default port in port vector
    port = PORT;
    //flag for child processes
    int n_child_finished = 0;
    //status of child processes
    int child_status;

    //parse initial server arguments
    parse_init_args(argc, argv);

    //treat SIGINT signal and SIGSEGV signal with SIGINT_handler and SIGSEGV_handler
    signal(SIGINT, SIGINT_handler);
    //signal(SIGSEGV, SIGSEGV_handler);

    //check if port is valid
    if(portIsValid(port) == FALSE){
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    

    int fd,errcode;
    ssize_t n;
    socklen_t addrlen;
    struct addrinfo hints,*res;
    struct sockaddr_in addr;
    char buffer_rec[BUFFER_MAX];
    char buffer_send[BUFFER_MAX];

    //pass port to char*
    char port_char[7];
    strcpy(port_char, port.c_str());

    //printf("UDP server started");
    fd = socket(AF_INET,SOCK_DGRAM,0); //UDP socket

    if(fd == -1){
        //error in crceating socket
        printf("Error in creating socket");
        exit(EXIT_FAILURE);
    }

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET; // IPv4
    hints.ai_socktype=SOCK_DGRAM; // UDP socket
    hints.ai_flags=AI_PASSIVE;

    errcode = getaddrinfo(NULL, port_char,&hints,&res);
    if(errcode!=0){
        //error in getaddrinfo
        printf("Error in getaddrinfo");
        exit(EXIT_FAILURE);
    }

    n = bind(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1){
        //error in bind
        printf("Error in bind");
        exit(EXIT_FAILURE);
    }

    //transform word_file from string to char*
    char word_file_char[BUFFER_MAX];
    strcpy(word_file_char, word_file.c_str());
    //strcat(word_file_char,"\0");
    word_file_char[word_file.size()] = '\0';

    //open word file
    word_file_fd = fopen(word_file_char, "r");
    if(word_file_fd == NULL){
        //error
        exit(EXIT_FAILURE);
    }

    while (1){
        
        addrlen = sizeof(addr);
        memset(buffer_rec,'\0', BUFFER_MAX);
        n = recvfrom(fd, buffer_rec, 128, 0, (struct sockaddr*)&addr, &addrlen);
        if(n == -1){
            //error in recvfrom
            printf("Error in recvfrom");
            exit(EXIT_FAILURE);
        }
        
        //treat request
        memset(buffer_send,'\0', BUFFER_MAX);
        string str_receive = buffer_rec;
        udpSwitch(buffer_send, str_receive);

        //change value of n to send
        n = sendto(fd, buffer_send, 20, 0, (struct sockaddr*)&addr, addrlen);
        if(n == -1){
            //error in sendto
            printf("Error in sendto");
            exit(EXIT_FAILURE);
        }
    }

    freeaddrinfo(res);
    close(fd);
  
    //end main server (maybe TO DO)
    printf("Game server ending\n");
    
    exit(EXIT_SUCCESS);

}

//parse command line arguments ./GS word_file [-p GS_port] [-v]
void parse_init_args(int argc, char *argv[]) {

    int opt;

    //store word_file name
    word_file = argv[1];

    //check for flags
    while ((opt = getopt(argc, argv, "vp:")) != -1) {
        switch (opt) {
            case 'p':
                //change port
                port = optarg;
                break;
            case 'v':
                //change verbose to "s"
                verbose = "s";
                break;
            default:
                fprintf(stderr, "./GS word_file: %s [-p %s] [-v %s]\n", word_file.c_str(), port.c_str(), verbose.c_str());
                exit(EXIT_FAILURE);
        }
    }
}

//check if GSport is valid
int portIsValid(string port) {
    //convert string to int
    int port_n = atoi(port.c_str());
    if (port_n < 1024 || port_n > 65535) {
        return FALSE;
    }
    return TRUE;
}

//treat SIGINT signal
void SIGINT_handler(int signum) {
    signal(SIGINT, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);

    //end main server
    printf("\nSIGINT received, do you want the server to quit = y/n?\n");
    char c;
    scanf("%c", &c);
    if (c == 'y') {
        printf("Game server ending\n");
        exit(EXIT_SUCCESS);
    }
    signal(SIGINT, SIGINT_handler);
    signal(SIGSEGV, SIGSEGV_handler);


}

//treat SIGSEGV signal
void SIGSEGV_handler(int signum) {
    signal(SIGINT, SIG_IGN);
    signal(SIGSEGV, SIG_IGN);

    //end main server
    printf("SIGSEGV received, ending server");
    exit(EXIT_FAILURE);
}


