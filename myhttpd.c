#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>

#include "mtqueue.h"

size_t getFileSize(const char * filename){
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}


char* get_path(char* http_req){
    char* i = strchr(http_req, ' ');
    char* start = i+1;
    char* k = strchr(start, ' ');
    *k = '\0';
    return start;
}


pthread_mutex_t mtx;
int sinolikes_selides=0;
long int sinolika_bytes=0;


char* root_dir;

void serve_file(int client_socket, char* file_path){
    char resp200[]="HTTP/1.1 200 OK\r\n"
        "Date: %s\r\n"
        "Server: myhttpd/1.0.0 (Ubuntu64)\r\n"
        "Content-Length: %d\r\n"
        "Content-Type: text/html\r\n"
        "Connection: Closed\r\n\r\n" ;
    char resp403[]="HTTP/1.1 403 Not Found\r\n"
        "Date: %s\r\n"
        "Server: myhttpd/1.0.0 (Ubuntu64)\r\n"
        "Content-Length: 70\r\n"
        "Content-Type: text/html\r\n"
        "Connection: Closed\r\n\r\n"
        "<html>Trying to access this file but don’t think I can make it.</html>";
    char resp404[]="HTTP/1.1 404 Not Found\r\n"
        "Date: %s\r\n"
        "Server: myhttpd/1.0.0 (Ubuntu64)\r\n"
        "Content-Length: 49\r\n"
        "Content-Type: text/html\r\n"
        "Connection: Closed\r\n\r\n"
        "<html>Sorry dude, couldn't find this file.</html>";

    char current_time[100];
    time_t now = time(0);
    struct tm tm;
    gmtime_r(&now, &tm);
    strftime(current_time, sizeof current_time, "%a, %d %b %Y %H:%M:%S %Z", &tm);

    char headers[1024];
    //for prints
    if(access(file_path, F_OK) != -1 ) {
        //file_exist
        if(access(file_path, R_OK)!= -1){
            //we have permissions to read
            char *buffer = NULL;
            int read_size;
            FILE *fp = NULL;

            int bytes=(int )getFileSize(file_path);

            snprintf(headers, sizeof(headers), resp200, current_time, bytes);
            int len = strlen(headers);
            int s = send(client_socket, headers, len, 0);
            if (s < 0) {
                printf("Error during sending headers for file '%s'\n", file_path);
                return;
            }

            fp = fopen(file_path, "rb");
            //
            // Allocate a string that can hold it all
            buffer = (char*)calloc(sizeof(char), bytes);
            // Read it all in one operation
            read_size = fread(buffer, sizeof(char), bytes, fp);
            s = send(client_socket, buffer, read_size, 0);
            free(buffer);
            fclose(fp);

            pthread_mutex_lock (&mtx);
            sinolikes_selides++;
            sinolika_bytes+=bytes;
            pthread_mutex_unlock (&mtx);
        }else{
            //we don't have permissions
            snprintf(headers, sizeof(headers), resp403, current_time);
            int len = strlen(headers);
            send(client_socket, headers, len, 0);
        }
    }else{
        //file does not exist
        snprintf(headers, sizeof(headers), resp404, current_time);
        int len = strlen(headers);
        send(client_socket, headers, len, 0);
    }
    close(client_socket);
}
void* worker(void* ptr){
    mtqueue_t* q = (mtqueue_t*) ptr;
    char receive_data[1024];

    while (1) {
        int* client_socket = NULL;
        int ok = mtq_get(q, (void **) &client_socket);
        if (ok != MTQ_OK) {
            break;
        }
        memset(receive_data, 0, sizeof receive_data);
        recv(*client_socket, &receive_data, sizeof(receive_data),0);

        char file_path[1024];//full path
        char* path = get_path(receive_data);
        snprintf(file_path, sizeof file_path, "%s%s", root_dir, path);

        printf("GET %s --> '%s'\n", path, file_path);

        serve_file(*client_socket, file_path);
        free(client_socket);
    }
    printf("Worker terminated...\n");
    return NULL;
}


//main
int main(int argc, char** argv){

    struct timeval  tv1, tv2;

    if (argc!=9) {
        printf("Not right amount of arguments\n");
        exit(-1);
    }

    if (strcmp(argv[1],"-p")!=0 || strcmp(argv[3],"-c")!=0 || strcmp(argv[5],"-t")!=0 || strcmp(argv[7],"-d")!=0) {
        printf("Wrong! Check again your %c arguments\n", '-');
        exit(-2);
    }

    if(strcmp(argv[2],argv[4])==0){
        printf("We want different ports here!\n");
        exit(-3);
    }

    if(access(argv[8],F_OK | R_OK)==-1){
        printf("cannot have access to the root_directory\n" );
        exit(-4);
    }

    gettimeofday(&tv1, NULL);

    root_dir = argv[8];
    int server_port = atoi(argv[2]);

    int threads_no = atoi(argv[6]);
    pthread_t  *workers = calloc(sizeof(pthread_t), threads_no);
    mtqueue_t* q = mtq_init(1000, threads_no+1);

  /*  mtq_info_t info = mtq_info(q);
    printf("empty space in q=%d, max workers=%d, no readers=%d, no writers=%d\n",
            info.size-info.count, info.max_workers, info.readers, info.writers);*/

    pthread_mutex_init(&mtx, NULL);

    for (int i=0; i<threads_no; ++i) {
        pthread_create (workers+i, NULL, worker, q);
    }


    int server_socket2;
    int server_socket1;
    server_socket1=socket(AF_INET, SOCK_STREAM, 0);
    server_socket2=socket(AF_INET, SOCK_STREAM, 0);


    //define server address
    struct sockaddr_in server_address1;
    server_address1.sin_family = AF_INET;
    server_address1.sin_port = htons(server_port);
    server_address1.sin_addr.s_addr = INADDR_ANY;
    int opt = 1;
    if( setsockopt(server_socket1, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ){
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address2;
    server_address2.sin_family = AF_INET;
    server_address2.sin_port = htons(atoi(argv[4]));
    server_address2.sin_addr.s_addr = INADDR_ANY;

    //bind the socket to our specify address and port
    if (bind(server_socket1, (struct sockaddr *) &server_address1, sizeof(server_address1)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    bind(server_socket2, (struct sockaddr *) &server_address2, sizeof(server_address2));

    //listen to start listening for connection
    listen(server_socket1, 5);
    listen(server_socket2, 5);



    int client_socket;
    fd_set readfds;
    socklen_t addrlen;


    while(1){
        FD_ZERO(&readfds);          /* initialize the fd set */
        FD_SET(server_socket1, &readfds);
        FD_SET(server_socket2, &readfds); /* add socket fd */
        int fids = server_socket2>server_socket1 ? server_socket2 : server_socket1;

        int result=select(fids+1, &readfds, NULL,NULL,NULL);
        if (result<0) {
            printf("select() failed\n");
            break;
        }
        if (result>0) {
            if(FD_ISSET(server_socket1, &readfds)){ /* serving port */
                addrlen = sizeof(server_address1);

                client_socket = accept(server_socket1, (struct sockaddr *) &server_address1, &addrlen);
                int* infd = malloc(sizeof(int));
                *infd = client_socket;
                mtq_put(q, infd);

            }else if (FD_ISSET(server_socket2, &readfds)){ /* command port */

                char shutdown_cmd[] = "SHUTDOWN";
                char stats_cmd[] = "STATS";

                char control_cmd[1024];
                addrlen = sizeof(server_address2);

                client_socket = accept(server_socket2,(struct sockaddr *) &server_address2, &addrlen);
                int b = recv(client_socket, &control_cmd, sizeof(control_cmd),0);
                control_cmd[b] = 0;
                printf("Received Control command: '%s'\n", control_cmd);
                if (strncmp(control_cmd, shutdown_cmd, sizeof shutdown_cmd - 1) == 0) {
                    printf("Got shutdown request, telling workers to commit suicide...\n");
                    mtq_terminate(q);
                    break;
                }
                else if(strncmp(control_cmd, stats_cmd, sizeof stats_cmd - 1) == 0) {
                    char stats_msg[1024];
                    gettimeofday(&tv2, NULL);
                    double seconds = (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
                    int hours = (int) (seconds / (60 * 60));
                    int minutes = (int) (seconds - 60 * 60 * hours) / 60;
                    int rest_secs = (int) (seconds - 60 * 60 * hours - 60 * minutes);
                    printf ("Total time = %f seconds\n", seconds);
                    pthread_mutex_lock (&mtx);
                    snprintf(stats_msg, sizeof stats_msg, "Server up for %02d:%02d.%02d, served %d pages, %ld bytes",
                            hours, minutes, rest_secs,
                            sinolikes_selides,
                            sinolika_bytes);
                    pthread_mutex_unlock (&mtx);
                    send(client_socket, stats_msg, strlen(stats_msg)+1,0);

                }
                close(client_socket);
            }
        }
    }
    for (int i=0; i<threads_no; ++i) {
        pthread_join (workers[i], NULL);
    }
    printf("worker threads joined...\n");
    mtq_destroy(q);
    pthread_mutex_destroy (&mtx);

    return 0;
}
