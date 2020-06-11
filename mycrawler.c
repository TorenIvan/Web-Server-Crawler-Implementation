#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <regex.h>

#include "mtqueue.h"

#define MAX_BUF  4096

pthread_mutex_t mtx;
int sinolikes_selides=0;
long int sinolika_bytes=0;

char* urlpath_to_filepath(char* save_dir, char* url_path){
	char* file_path;

	int sd = strlen(save_dir);
	int up = strlen(url_path);
	file_path = calloc(sd+up+1, sizeof(char));
	strncpy(file_path, save_dir, sd);
	strncpy(file_path+sd, url_path, up);
	return file_path;
}

int file_exists(char* filepath){
	if( access(filepath, F_OK ) != -1 )
		return 1;
	return 0;
}

int url_is_saved(char* save_dir, char* url_path){
	char* filepath = urlpath_to_filepath(save_dir, url_path);
	int exists = file_exists(filepath);
	free(filepath);
	return exists;
}


char** find_links(char* html, int* links_count){
	int links_size = 0;
	char** links = NULL;
	*links_count = 0;

	regex_t r;

	/* Search for urls in strings that look like: `href = "...."` */
	char* pattern = "href[[:space:]]*=[[:space:]]*\"([^\"]+)\"";

	if (regcomp(&r, pattern, REG_EXTENDED)){
      printf("Could not compile regular expression.\n");
      return links;
    };


    const int n_matches = 5; /* the maximum number of matches allowed. */
    regmatch_t groups[n_matches];/* contains the matches found. */

	char* cursor = html;
	char url[1024];

	for (int m = 0; ; m++){
		if (regexec(&r, cursor, n_matches, groups, 0) != 0)
			break;  // No more matches

		unsigned int offset = groups[0].rm_eo;
		cursor[offset] = 0;

		regmatch_t group = groups[1];
		unsigned int start = group.rm_so;
		unsigned int end = group.rm_eo;

		strncpy(url, cursor + start, end - start);
		url[end-start] = 0;

		/* printf("Found URL '%s' in '%s'\n", url, cursor+groups[0].rm_so); */
		/*
		char* k = urlpath_to_filepath("/tmp/save_dir", url);
		printf("%s [%s]%d\n", url, k, file_exists(k));
		free(k);
		*/
		cursor += offset+1;

		if (*links_count == links_size) {
			int new_size = links_size + 10;
			links = realloc(links, new_size * sizeof(char*));
			links_size = new_size;
		}
		links[*links_count] = strdup(url);
		++*links_count;
	}

	regfree(&r);
	return links;
}


char* get_page(char* host, int port, char* path){
	  int sockfd, n;
    struct sockaddr_in dest;
    char buffer[MAX_BUF];

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
        return NULL;

    /* Initialize server address/port struct   */
    bzero(&dest, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
	in_addr_t addr = inet_addr(host);
    if (addr == INADDR_NONE)
		return NULL;
	dest.sin_addr.s_addr = addr;

    /* Connect to server   */
    connect(sockfd, (struct sockaddr*)&dest, sizeof(dest));

    sprintf(buffer, "GET %s HTTP/1.1\n\n", path);
    send(sockfd, buffer, strlen(buffer), 0);

	int reading_headers = 1;
	int body_size = MAX_BUF;
	char* body = malloc(body_size * sizeof(char));
	int body_len = 0;
    while (1){
        memset(buffer, 0, sizeof(buffer));
        n = recv(sockfd, buffer, sizeof(buffer), 0);
				if (n <= 0)
					break;
		if (reading_headers) {

			char* end_of_hdr = strstr(buffer, "\r\n\r\n");

			char* start_of_body = end_of_hdr + 4;
			int k = n - (int)(start_of_body - buffer);
			if (body_len + k > body_size) {
				body = realloc(body, body_len + MAX_BUF);
			}
			memcpy(body+body_len, start_of_body, k);
			body_len += k;
			reading_headers = 0;
		}
		else {
			if (body_len + n > body_size) {
				body = realloc(body, body_len + MAX_BUF);
			}
			memcpy(body+body_len, buffer, n);
			body_len += n;
		}
    }

	body[body_len] = '\0';
	printf("READ %d body bytes from URL %s\n", body_len, path);

    close(sockfd);


    return body;
}

char* server_host;
int server_port;
char* save_dir;

void* worker(void* arg){
    mtqueue_t* q = (mtqueue_t*) arg;
	int terminated = 0;
    while (!terminated) {
        char* urlpath = NULL;
        int ok = mtq_get(q, (void **) &urlpath);
        if (ok != MTQ_OK) {
            break;
        }

		char* data = get_page(server_host, server_port, urlpath);
		if (data == NULL) {
			printf("Worker error: couldn't download page '%s'\n", urlpath);
			continue;
		}

		char* filepath = urlpath_to_filepath(save_dir, urlpath);
		if (file_exists(filepath)) {
			printf("Worker found that file '%s' has been downloaded already..\n",
					filepath);
		}
		else {
			char dir[1024];
			strncpy(dir, filepath, sizeof dir);
			char* s = strrchr(dir, '/');
			*s = 0;
			mkdir(dir, 0700);

			FILE* fp = fopen(filepath, "wb");
			int data_len = strlen(data);
			fwrite(data, sizeof(char), data_len, fp);
			fclose(fp);

			pthread_mutex_lock(&mtx);
			++sinolikes_selides;
			sinolika_bytes += data_len;
			pthread_mutex_unlock(&mtx);


			int no_links;
			char** links = find_links(data, &no_links);
			printf("URL: '%s' %d links found..\n", urlpath, no_links);

			for(int i=0; i<no_links; ++i) {
				if (!terminated && !url_is_saved(save_dir, links[i])) {
					int ok = mtq_put(q, links[i]);
					if (ok == MTQ_OK)
						continue;
					terminated = ok == MTQ_TERMINATED;
				}
				free(links[i]);
			}
			free(links);
			free(data);
		}
		free(filepath);
		free(urlpath);
	}

	printf("Worker terminated...\n");
	return NULL;
}
int main(int argc, char* argv[]){

    if (argc!=12) {
        printf("Not right amount of arguments. Command should be:\n"
				"crawler -h host_or_IP -p port -c command_port -t num_of_threads -d save_dir starting_URL\n");
        exit(-1);
    }

    if (strcmp(argv[1],"-h")!=0 || strcmp(argv[3],"-p")!=0 || strcmp(argv[5],"-c")!=0 || strcmp(argv[7],"-t")!=0 || strcmp(argv[9], "-d")!=0)  {
        printf("Wrong! Check again your %c arguments\n", '-');
        exit(-2);
    }

    if(access(argv[10],F_OK | W_OK)==-1){
        printf("cannot have access to the save_dir\n" );
        exit(-4);
    }

	server_host = argv[2];
	server_port = atoi(argv[4]);
	int port = atoi(argv[6]);
    int threads_no = atoi(argv[8]);
	save_dir = argv[10];
	char* path = argv[11];

    struct timeval tv1;


    gettimeofday(&tv1, NULL);

    pthread_t  *workers = calloc(sizeof(pthread_t), threads_no);
    mtqueue_t* q = mtq_init(1000, threads_no);

	/* Put starting url in queue to be ready for download */
	mtq_put(q, strdup(path));

    pthread_mutex_init(&mtx, NULL);
    for (int i=0; i<threads_no; ++i) {
        pthread_create (workers+i, NULL, worker, q);
    }

    int server_socket;
    server_socket=socket(AF_INET, SOCK_STREAM, 0);

    //define server address
    struct sockaddr_in server_address1;
    server_address1.sin_family = AF_INET;
    server_address1.sin_port = htons(port);
    server_address1.sin_addr.s_addr = INADDR_ANY;

    int ok = bind(server_socket, (struct sockaddr *) &server_address1, sizeof(server_address1));
	if (ok != 0) {
		perror("bind");
		exit(-1);
	}
    //listen to start listening for connection
    ok = listen(server_socket, 5);
	if (ok != 0) {
		perror("listen");
		exit(-1);
	}
	printf("Crawler started listening on port %d\n", port);


	char shutdown_cmd[] = "SHUTDOWN";
	char stats_cmd[] = "STATS";
	char search_cmd[] = "SEARCH";

	char control_cmd[1024];
	while(1){
		int client_socket;
		socklen_t addrlen;

		addrlen = sizeof(server_address1);

		client_socket = accept(server_socket,(struct sockaddr *) &server_address1, &addrlen);
		int b = recv(client_socket, &control_cmd, sizeof(control_cmd),0);
		control_cmd[b] = 0; /* NULL terminate string */
		printf("Received Control command: '%s'\n", control_cmd);
		if (strncmp(control_cmd, shutdown_cmd, sizeof shutdown_cmd - 1) == 0) {
			printf("Got shutdown request, telling workers to commit suicide...\n");
			mtq_terminate(q);
			break;
		}
		else if(strncmp(control_cmd, stats_cmd, sizeof stats_cmd - 1) == 0) {
			char stats_msg[1024];
			struct timeval tv2;
			gettimeofday(&tv2, NULL);
			double seconds = (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec);
			int hours = (int) (seconds / (60 * 60));
			int minutes = (int) (seconds - 60 * 60 * hours) / 60;
			int rest_secs = (int) (seconds - 60 * 60 * hours - 60 * minutes);
			printf ("Total time = %f seconds\n", seconds);
			pthread_mutex_lock (&mtx);
			snprintf(stats_msg, sizeof stats_msg, "Server up for %02d:%02d.%02d, downloaded %d pages, %ld bytes",
					hours, minutes, rest_secs,
					sinolikes_selides,
					sinolika_bytes);
			pthread_mutex_unlock (&mtx);
			send(client_socket, stats_msg, strlen(stats_msg)+1,0);
		}
		else if(strncmp(control_cmd, search_cmd, sizeof search_cmd - 1) == 0) {
			mtq_info_t info = mtq_info(q);
			if (info.count > 0) {
				char msg[] = "Crawling still in progress..";
				send(client_socket, msg, strlen(msg), 0);
			}
			else {
				char* words[100];
				int k = 0;
				char *token = strtok(control_cmd, " ");
				while(token && k<100) {
					token = strtok(NULL, " ");
					if (token)
						words[k++] = token;
				}
				for (int i = 0; i<k; ++i) {
					printf("SEARCH WORD: '%s'\n", words[i]);
				}

				char msg[] = "Not implemented...";
				send(client_socket, msg, strlen(msg), 0);
			}
		}
		close(client_socket);
	}
	for (int i=0; i<threads_no; ++i) {
		pthread_join (workers[i], NULL);
	}
	printf("worker threads joined...\n");
	mtq_destroy(q);
	pthread_mutex_destroy (&mtx);

	return 0;
}
