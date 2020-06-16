#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <unistd.h> // write
#include <string.h> // memset
#include <stdlib.h> // atoi
#include <stdbool.h> // true, false
#include <ctype.h> //isascii, isalnum
#include <err.h> // error 
#include <errno.h> 
#include <pthread.h> // thread
#include <semaphore.h>
#include <getopt.h> // get option
#include <math.h> //ceil

#include "myQueue.h" // simple queue Implementation from csdn

// global variable
#define BUFFER_SIZE 16384
//pthread_cond_t dispatch_var = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char end_log[10] = "\n========\n";
int lflag = 0;
int log_file;
ssize_t offset = 0;
int entries=0, errors=0;
int Q_size = 0;

struct httpObject {
    char method[100];         // PUT, HEAD, GET
    char filename[BUFFER_SIZE];      // what is the file we are worried about
    char httpversion[20];    // HTTP/1.1
    ssize_t content_length; // example: 13
    char status_code[30];
    int st_code;
    char buffer[BUFFER_SIZE];

    ssize_t msg_offset;
};

typedef struct thread_arg_t {
    int NUM_THREADS;
    int num;
    int cli_sokd;
    int d_ready;
    pthread_cond_t c_var;
    Queue *cli_q;
    //pthread_mutex_t pmutex;
} ThreadArg;

// deal with bad request ex >27 other than("_-")
void bad_request(struct httpObject* message, char* fname);
void read_http_response(ssize_t client_sockd, struct httpObject* message);
// read from input and write to output, and construct message object
void fcat(struct httpObject* message, ssize_t input, ssize_t output);
//if error happen during put, recv data and do nothing
void throw_shit(struct httpObject* message, ssize_t inputt);
int permission_check(struct httpObject* message);
void update_offset(struct httpObject* message);
void response(ssize_t client_sockd, struct httpObject* message);
void process(ssize_t client_sockd, struct httpObject* message);
void* worker(void* obj);
void* dispatch(void* obj);
void log_ging(int log_fd, char *msg, ssize_t *poffset, size_t rc, int *count);

int main(int argc, char** argv) {
	
	int nflag = 0;
    char* log_name = NULL;
    int nthreads=0, c;
    //opterr = 0;

    while ((c = getopt(argc, argv, "N:l:")) != -1) {
        switch (c) {
            case 'N':
                nflag = 1;
                nthreads = atoi(optarg);
                break;
            case 'l':
                lflag = 1;
                log_name = optarg;
                break;
            case '?':
                if ((optopt == 'N') || (optopt == 'l')){
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else if (isprint (optopt)) {
                    fprintf(stderr, "Unknown option -%c.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character \\x%x'.\n", optopt);
                }
            
                return EXIT_FAILURE;
            default:
                abort();
        }
    }

    if((nflag==1) && (nthreads<2)) {
    	printf("#Thread should be an int greater than 2\n");
    	return EXIT_FAILURE;
    }
    else if( (optind+1) < argc ) {
    	printf("More than one port value\n");
    	return EXIT_FAILURE;
    }

    printf("\n #Configuration: \n | nflag: %d  nthreads: %d\n | lflag: %d  log_file: %s\n",
                                             nflag, nthreads, lflag, log_name);
    if(lflag){
        log_file = open(log_name, O_CREAT|O_WRONLY|O_TRUNC, 0644);  
    }
    // initialize the server
    // Create sockaddr_in with server information

    int port = 8080;
    if (optind != argc){
        if (atoi(argv[optind])){
        	port = atoi(argv[optind]);
            
        }else{
        	printf("Port must be a valid non-zero int\n");
            return EXIT_FAILURE;
            
        }
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socklen_t addrlen = sizeof(server_addr);
    //Create server socket
    int server_sockd = socket(AF_INET, SOCK_STREAM, 0);
    // Need to check if server_sockd < 0, meaning an error
    if (server_sockd < 0) {
        perror("socket");
    }
 
    //    Configure server socket
    int enable = 1;
    //This allows you to avoid: 'Bind: Address Already in Use' error    
    int ret = setsockopt(server_sockd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    //Bind server address to socket that is open
    ret = bind(server_sockd, (struct sockaddr *) &server_addr, addrlen);
    //Listen for incoming connections   
    ret = listen(server_sockd, SOMAXCONN); // 5 should be enough, if not use SOMAXCONN
    if (ret < 0) {
        return EXIT_FAILURE;
    }

    //Connecting with a client
    struct sockaddr client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    
    //set up thread
    int NUMTHREADS = nflag ? nthreads : 4;	
    printf(" |\n | Port: %d   #Thread: %d\n", port, NUMTHREADS);
    printf(" #Server starting...\n\n");
    pthread_t thread[NUMTHREADS];
    // queue of client socket
    Queue* q = CreateQueue(100);

    ThreadArg args[NUMTHREADS];
    // setup thread arguments
    for (int i = 0; i < NUMTHREADS; ++i)
    {
    	args[i].num = i;
    	args[i].cli_sokd = 0;
    	args[i].d_ready = 0;
        args[i].NUM_THREADS = NUMTHREADS;
        args[i].cli_q = q;
    	pthread_cond_init(&args[i].c_var, NULL);
        //pthread_mutex_init(&args[i].pmutex, NULL);
    }
    


    // worker threads
    for (int i = 1; i < NUMTHREADS; ++i)
    	pthread_create(&thread[i], NULL, &worker, &args[i]);
    // dispatch thread
    pthread_create(&thread[0], NULL, &dispatch, &args);
    
    while(true){
        printf("----------server is waiting----------\n");
        int client_sockd = accept(server_sockd, &client_addr, &client_addrlen);
        printf("  --------Request accepted--------\n");
        if (client_sockd < 0){
        	perror("client_sockd");
        }else if(IsFullQ(q)){
            printf("Queue of client_sockd is full.\n");

        }else{
            pthread_mutex_lock(&mutex);
            AddQ(q, client_sockd);
             ++Q_size;
            pthread_mutex_unlock(&mutex);
           
        }
    }
    
    free(q);
	for(int i = 0; i< NUMTHREADS ; i++) {
		pthread_join(thread[i], NULL);
    }

    return EXIT_SUCCESS;

}

void* worker(void* obj){
	ThreadArg* argp = (ThreadArg*) obj;
	int num_t = argp->num;
	struct httpObject message;
	printf("\033[0;32m[+] In Worker%d thread...\n\033[0m",num_t);

	while(true){
        pthread_mutex_lock(&mutex);
        printf("\033[0;32m[+] Worker%d has the lock\n\033[0m",num_t);

		while (!argp->d_ready){
			//sleeping
			printf("\033[0;32m[+] Worker%d release the lock & Sleep\n\033[0m",num_t);

            //pthread_mutex_lock(&mutex);
           	pthread_cond_wait(&argp->c_var, &mutex);
            // pthread_mutex_unlock(&mutex);

            printf("\033[0;32m[+] Worker%d Signaled to WAKE UP\n\033[0m",num_t);

		}
        
        argp->cli_sokd = DeleteQ(argp->cli_q);
        sleep(3);

		read_http_response(argp->cli_sokd, &message);
        printf("%s~%s~%s~%ld\n\n", message.method, message.filename, message.httpversion, message.content_length);

		// check permissions & update all status code
        permission_check(&message);
        printf("%s\n\n", message.status_code);
 		
        //update offset 
        if(lflag){
            update_offset(&message);
            printf("Offset updated: %ld\n", offset);
        }      
        pthread_mutex_unlock(&mutex);
        response(argp->cli_sokd, &message);

        pthread_mutex_lock(&mutex);
        process(argp->cli_sokd, &message);

        // if log is on, end loging
        if(lflag) pwrite(log_file, end_log, strlen(end_log), message.msg_offset);

        
        printf("\033[0;32m[+] Worker%d release the lock\n\033[0m",num_t);
        printf("\033[0;32m[+] Worker%d Done processing...\n\033[0m",num_t);

        memset(&message, 0, sizeof(message));
        
		argp->d_ready = 0;
        pthread_mutex_unlock(&mutex);

	}
}

void* dispatch(void* obj){
	printf("\033[0;33m[-] In Dispatch thread...\n\033[0m");
	ThreadArg* argl = (ThreadArg*) obj;
	int dt, NTH = argl->NUM_THREADS;//
   

	while(true){
		dt = 0;
		for (int i = 1; i < NTH; ++i){
			dt = argl[i].d_ready + dt;
		}

        // wait if all workers are busy / no request in cli_sockd queue
		while( dt==(NTH-1) ){
            dt = 0;
            for (int i = 1; i < NTH; ++i)
                dt = argl[i].d_ready + dt;
		}
        

        if(Q_size != 0){ //!IsEmptyQ(argl->cli_q)
            for (int i = 1; i < NTH; ++i)
            {
                if (argl[i].d_ready==0)
                {
                    printf("\033[0;33mWorker%d got work!\n\033[0m", i);
                    //argl[i].d_ready = 1;
                    
                    
                    pthread_mutex_lock(&mutex);
                    argl[i].d_ready = 1;
                    --Q_size;
                    pthread_cond_signal(&argl[i].c_var);
                    pthread_mutex_unlock(&mutex);

                    

                    break;
                }
            }
        }

	}
}

void bad_request(struct httpObject* message, char* fname){  
	bool bad_hver = strcmp(message->httpversion, "HTTP/1.1")!=0;
    if(strlen(fname) > 27 || strlen(fname) == 0 || bad_hver){        
        message->st_code = 400;        
        return;
    } 
    for (size_t i = 0; i < strlen(fname); ++i)
    {
        if(!isascii(fname[i])){            
            message->st_code = 400;            
            return;

         }else{
            if(!isalnum(fname[i])){
                if (!(fname[i] == '-' || fname[i] == '_')){                   
                    message->st_code = 400;
                    return;
                }
            }
        }
            
    }
}
// Want to read in the HTTP message/ data coming in from socket
// \param client_sockd - socket file descriptor
// \param message - object we want to 'fill in' as we read in the HTTP message
void read_http_response(ssize_t client_sockd, struct httpObject* message) {

    char buff[4096];
    ssize_t bytes = recv(client_sockd, buff, 4096, MSG_PEEK);
    buff[bytes] = 0; // null terminate
    printf("[+] received %ld bytes from client\n", bytes);
    //write(1, buff, strlen(buff));
       
    char m[100], f[BUFFER_SIZE], ff[BUFFER_SIZE], ver[9];
    ssize_t cc = 0;

    char *cl = strstr(buff, "Content-Length:");
    if( cl != NULL){

	    sscanf(cl, "Content-Length: %zd", &cc);
	    //printf("%d\n", cc);
	    message->content_length = cc;	
    }
    
    sscanf(buff, "%s %s %s\r\n%*s", m,f,ver);    
    sscanf(f, "/%s", ff);
	// initial message
    strcpy(message->method, m);
    strcpy(message->filename, ff);
    strcpy(message->httpversion, ver);

    // deal with split request
    char header[BUFFER_SIZE];
    if ((strcmp(message->method, "PUT")==0) && (cc!=0)) {
	    
	    while(true){
	    	
	    	memset(&header,0,sizeof(header));

	    	sscanf(buff, "%[^\n]", header);
	    	memset(&buff,0,sizeof(buff));

	    	//printf("l:%ld:%s\n", strlen(header),header);

	    	recv(client_sockd, buff, (strlen(header)+1), 0);
	    	memset(&buff,0,sizeof(buff));

	    	recv(client_sockd, buff, 4096, MSG_PEEK);

	    	//printf("done PEEk recv");
	    	if ( strlen(header)==1 ) break;
	    	
	    }
	}
	
    bad_request(message, ff);
    //printf("%d\n", message->st_code);

	memset(&header,0,sizeof(header));
    memset(&buff,0,sizeof(buff));
    memset(&m,0,sizeof(m));
    memset(&f,0,sizeof(f));
    memset(&ff,0,sizeof(ff));
    memset(&ver,0,sizeof(ver));
    return;
}

// read from input and write to output, and construct message object
// with buffer < 32768
void fcat(struct httpObject* message, ssize_t input, ssize_t output){
	// file is larger than buffer size or not
	int datalength = (message->content_length>BUFFER_SIZE) ? BUFFER_SIZE:message->content_length;
	int rec, wrid, lft = message->content_length;
    int c = 1;
	
    //printf("start fcat\n");
	while(datalength > 0){
        //if(fcatdone) break;
        //printf("in fcat: %d\n", datalength);
    	rec = read(input, message->buffer, datalength);
    	if(rec==-1) break;
    	//wrid = write(STDOUT_FILENO, message->buffer, rec);
    	wrid = write(output, message->buffer, rec);
        if(wrid == -1) break;
        if(lflag){
            log_ging(log_file, message->buffer, &message->msg_offset, rec, &c);
            //printf("!content logged!\n");
        }
        
    	memset(&message->buffer,0,sizeof(message->buffer));
    	
    	lft = lft - rec;
    	datalength = lft>BUFFER_SIZE ? BUFFER_SIZE:lft;
    }
    if(rec==-1 || wrid==-1){
    	warn("%s",message->filename);
    	strcpy(message->status_code, "500 Internal Server Error");
    	message->st_code = 500;
    }else{
	    strcpy(message->status_code, "200 OK");
	    message->st_code = 200;
	}
	//printf("done fcat\n");
    return;
}
// if error happen during put, recv data and do nothing
void throw_shit(struct httpObject* message, ssize_t inputt){
    int ts = (message->content_length>BUFFER_SIZE) ? BUFFER_SIZE:message->content_length;
    int rd, lf = message->content_length;

    while(ts > 0){
        //if(fcatdone) break;
        rd = read(inputt, message->buffer, ts);
       
        if(rd==-1) break;        
        memset(&message->buffer,0,sizeof(message->buffer));
        lf = lf - rd;
       
        ts = lf>BUFFER_SIZE ? BUFFER_SIZE:lf;
       
    }
    
    if(rd==-1){
        warn("%s",message->filename);
        strcpy(message->status_code, "500 Internal Server Error");
        message->st_code = 500;
    }
   
    return;
}

//check file permission
//    error: -1 | (-2 for PUT404) | (-3 for Forbidden) | (-4 for bad request)
//    -r:1 -w:2 -rw:3
//    also set status code
int permission_check(struct httpObject* message){
    struct stat statbuf;
    int fileMode, rtcode;
    int W=0, R=0;

    bool isPut = (strcmp(message->method, "PUT")==0);
    bool isGet = (strcmp(message->method, "GET")==0);
    bool isHead = (strcmp(message->method, "HEAD")==0);
    
    if ( !(isPut || isGet || isHead) || message->st_code == 400 ){
        //memset(message->status_code, 0, sizeof(message->status_code));
        strcpy(message->status_code, "400 Bad Request");
        message->content_length = 0;
        message->st_code = 400;
        return -4;
    }

    if(stat(message->filename, &statbuf)==-1){
        warn("%s", message->filename);

        if(errno == ENOENT){
            strcpy(message->status_code, "404 File not found");
            message->st_code = 404;
            rtcode = strcmp(message->method, "PUT")? -1:-2;
        }else{
            strcpy(message->status_code, "500 Internal Server Error");
            message->st_code = 500;
            rtcode = -1;
        }       

    }else{
        fileMode = statbuf.st_mode;
        if(S_ISREG(fileMode)) {
            if((fileMode & S_IWUSR) && (fileMode & S_IWRITE)) W=2;                

            if((fileMode & S_IRUSR) && (fileMode & S_IREAD)) R=1;   
            rtcode = W+R;
        }
        else if(S_ISDIR(fileMode)) {
            // this is a dir
            strcpy(message->status_code, "403 Forbidden");
            message->st_code = 403;
            rtcode = -3;
        }
        else{
            strcpy(message->status_code, "500 Internal Server Error");
            message->st_code = 500;
            rtcode = -1;
        }
    }
    
    // update status code
    
    bool GorH403 = (isGet || isHead)  && (rtcode==0||rtcode==2);
    bool GorH_read = (isGet || isHead) && (rtcode == 1||rtcode==3);
    bool Put403 = isPut && (rtcode==0||rtcode==1);
    bool Put_create = isPut && (rtcode==2||rtcode==3||rtcode==-2);

    bool PorH_health = (isPut || isHead) && (strcmp(message->filename, "healthcheck")==0);

    

    if (GorH403 || Put403 || PorH_health){
		//message->content_length = 0;
		memset(&message->status_code, 0, sizeof(message->status_code));
		strcpy(message->status_code, "403 Forbidden");
		message->st_code = 403;
		rtcode = -3;
	}else

    if (GorH_read){
    	message->content_length = statbuf.st_size;
        memset(&message->status_code, 0, sizeof(message->status_code));
    	strcpy(message->status_code, "200 OK");
    	message->st_code = 200;
    }else

    if (Put_create){	
    	memset(&message->status_code, 0, sizeof(message->status_code));
    	strcpy(message->status_code, "201 Created");
    	message->st_code = 201;
    }


	memset(&statbuf, 0, sizeof(statbuf));

    return rtcode;
}

void update_offset(struct httpObject* message){
    // mnt current offset
    message->msg_offset = offset;
    printf("Current offset: %ld\n", offset);

    char tmpbuff[BUFFER_SIZE];
    ssize_t log_header = 0;
    double lines = 0;
    int log_content = 0;

    bool isHealthC = (strcmp(message->filename, "healthcheck")==0);
    bool Get = (strcmp(message->method, "GET")==0);

    if (lflag && isHealthC && Get && message->st_code!=400) {

        memset(&message->status_code, 0, sizeof(message->status_code));
        strcpy(message->status_code, "200 OK");
        message->st_code = 200;

        message->content_length = snprintf(message->buffer, BUFFER_SIZE, "%d\n%d", errors, entries);
        //printf("\n");
        printf("Healthcheck:%zd\n%s\n", message->content_length, message->buffer);

        log_header = snprintf(tmpbuff, 200, "GET /healthcheck length %ld", message->content_length);
        offset += (log_header + 1 + 9 + (message->content_length*3) + 9);

    }else

    if (message->st_code==200 || message->st_code==201)
    {
        log_header = snprintf(tmpbuff, sizeof(tmpbuff), "%s /%s length %zd",
                    message->method, message->filename, message->content_length);

        if(strcmp(message->method, "HEAD")!=0){
            lines = message->content_length /20.0;
            log_content = (ceil(lines)*9)+(message->content_length*3);
        }

        offset += (log_header + 1 + log_content + 9);

    }else{
        ++errors;
        log_header = snprintf(tmpbuff, sizeof(tmpbuff), "FAIL: %s /%s %s --- response %d",
                    message->method, message->filename, message->httpversion, message->st_code);
        offset += (log_header + 1 + 9);
    }
    printf("header: %ld content: %d\n", log_header, log_content);
    pwrite(log_file, tmpbuff, log_header, message->msg_offset);
    message->msg_offset += log_header;
    ++entries;
}

//void process_request(ssize_t client_sockd, struct httpObject* message) {}

void response(ssize_t client_sockd, struct httpObject* message) {
    printf("\nConstructing Response\n");
    
    char dst[80];
    ssize_t cl = strcmp(message->method, "PUT") ? message->content_length:0;

    snprintf(dst, 80, "%s %s\r\nContent-Length: %zd\r\n\r\n", 
    					message->httpversion, message->status_code, cl);

    printf("%s", dst);
    write(client_sockd, dst, strlen(dst));
    printf("Response Sent\n\n");

    memset(&dst, 0, sizeof(dst));

}

void process(ssize_t client_sockd, struct httpObject* message) {
    // get/put file
    if(strcmp(message->method, "GET")==0 && strcmp(message->status_code, "200 OK")==0){

        bool isHealthC = (strcmp(message->filename, "healthcheck")==0);

        if (lflag && isHealthC) {
            int c = 1;
            write(client_sockd, message->buffer, message->content_length);
            log_ging(log_file, message->buffer, &message->msg_offset, message->content_length, &c);
            //printf("!content logged!\n");
        
        }else{
            ssize_t fdd = open(message->filename, O_RDONLY);
            fcat(message, fdd, client_sockd);
            close(fdd);   
        }
    	
    }else
    if (strcmp(message->method, "PUT")==0) {
    	ssize_t fd;

        if (message->st_code == 201){
            fd = open(message->filename, O_CREAT|O_WRONLY|O_TRUNC, 0644);
            fcat(message, client_sockd, fd);
            close(fd);
            
        }else{
            // no permission to write
            if(message->st_code == 403){
	            // only recv data
	            //printf("here\n");
	            throw_shit(message, client_sockd);	            
	        }
        }
    	message->content_length = 0;   
    }
    return;
}

void log_ging(int log_fd, char *msg, ssize_t *poffset, size_t rc, int *count){

    char buff[BUFFER_SIZE];
    //printf("%d\n", count);

    for (size_t idx = 0; idx < rc; ++idx) {
        
        if (*(count)%20 == 1){
            snprintf(buff, sizeof(buff), "\n%08d", *(count)-1);
            pwrite(log_fd, buff, strlen(buff), *(poffset));
            *(poffset) += strlen(buff);
        }

        memset(&buff, 0, sizeof(buff));
        snprintf(buff, sizeof(buff), " %02x", (unsigned char)msg[idx]);            
        pwrite(log_fd, buff, strlen(buff), *(poffset));

        *(poffset) += strlen(buff);
        ++*(count);
    }   
}