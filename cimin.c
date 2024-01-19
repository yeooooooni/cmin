#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#define BUF_SIZE 4096
#define READ 0
#define WRITE 1
#ifdef DEBUG
    #define debug(fn) fn
#else
    #define debug(fn)
#endif

char *input = NULL;
char *error_keyword = NULL;
char *output_filepath = NULL;
char *target = NULL;
int args_size= 0;
char *args[16];
pid_t pid = -1;

int origin_size = 0;
int tm_size = 0;
char* tm = NULL;

//timeout: signal handler for SIGALARM
void timeout(int sig){
	fprintf(stderr, "timeout occured!\n");
    kill(pid, SIGTERM);
	exit(EXIT_SUCCESS);
}

//quit: signal handler for SIGINT
void quit(int sig){
	fprintf(stderr, "program terminate...\n");
	printf("cimin: %s \n(%d) -> (%d)\n", tm, origin_size, tm_size);
	exit(EXIT_SUCCESS);
}

//printUsage: print usage
void printUsage(char* binary){
	
	fprintf(stderr, "	Usage: %s -i [crash_input] -m [error_keyword] \n		-o [output_filepath] [target_binary] (target_arg) ...\n", binary);
}

//parseArgs: parse args and save it to corresponding variables
void parseArgs(int argc, char *argv[]){
	
	int opt;
    while ((opt = getopt(argc, argv, "i:m:o:")) != -1) {
        switch (opt) {
            case 'i':
                input = optarg;
                break;
            case 'm':
                error_keyword = optarg;
                break;
            case 'o':
                output_filepath = optarg;
                break;
			case '?':	
			default:
				printUsage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

	if (input == NULL || error_keyword == NULL || output_filepath == NULL) {
		printUsage(argv[0]);
	    exit(EXIT_FAILURE);
	}
    for (int i = optind; i < argc; i++) {
		if(i == optind) target = argv[i];
		args[args_size++] = argv[i]; 
    }
	args[args_size] = NULL;

	if(strstr(args[0], " ") != NULL){
		char* tok = (args[args_size++] = strtok(args[0], " "));
		while(tok != NULL) tok = (args[args_size++] = strtok(NULL, " "));
	}

	debug(
		printf("Input: %s\n", input);
    	printf("Error Keyword: %s\n", error_keyword);
    	printf("Output: %s\n", output_filepath);
    	printf("Target: %s\n", target);
		for(int i = 0; i < args_size; i++) printf("args %d: %s\n", i+1, args[i]);
		printf("\n");
	);
	return;

}


//is_crashed: exec target program with input crash as stdin.
//return 0 if theres no crash, return 1 if there was crash
int is_crashed(char* crash, int size){

	int p2c[2], c2p[2];
	if((pipe(p2c) < 0) || (pipe(c2p) < 0)){
		perror("pipe()");
		exit(EXIT_FAILURE);
	}

	pid = fork();
	if(pid < 0){				//error
		perror("fork");
		exit(EXIT_FAILURE);
	}else if(pid > 0){			//parents
		debug(fprintf(stderr, "parent start\n"););
		close(p2c[READ]);
		close(c2p[WRITE]);
		alarm(3);

		//send crash input as stdin of child
		write(p2c[WRITE], crash, size);
		close(p2c[WRITE]);
		waitpid(pid, NULL, 0);
		alarm(0);

		//read output from child stderr
		char buf[BUF_SIZE];
		int len, error;
		while((len = read(c2p[READ], buf, BUF_SIZE-1)) > 0){
			buf[len] = 0;
			debug(printf("\nparents recv: %s\n", buf););
			//determine if error keyword we find is exist
			if(strstr(buf, error_keyword) != NULL) error = 1;
			else error = 0;
		}
		close(c2p[READ]);

		debug(fprintf(stderr, "parent end\n"););
		return error;

	}else{						//child
		debug(fprintf(stderr, "child start\n"););
		close(p2c[WRITE]);
		close(c2p[READ]);

		//redirect some std pipes to our unnamed pipes
		dup2(p2c[READ], STDIN_FILENO);
		dup2(c2p[WRITE], STDERR_FILENO);
		close(STDOUT_FILENO); 

		char buf[BUF_SIZE];
		execv(target, args);

		close(p2c[READ]);
		close(c2p[WRITE]);
		debug(fprintf(stderr, "child end\n"););
		exit(EXIT_SUCCESS);

	}

	return 0;




}


//make_substr: make substring of src to dest, range of start-end index.
//return size of substr
int make_substr(char* src, char* dest, int start, int end){
	
	int index = 0;
	for(int i = start; i < end; i++) dest[index++] = src[i];
	dest[index] = 0;
	return index;

}

int make_strcat(char * reduced_input, char * head, char * tail, int head_size, int tail_size){
    // fprintf(stderr, "malloc size : %d", head_size + tail_size);
    // reduced_input = malloc(head_size + tail_size);

    for(int i=0; i<head_size; i++){
        reduced_input[i] = head[i];
    }

    for (int j = 0; j < tail_size; j++){
        reduced_input[head_size + j] = tail[j];
    }
    // fprintf(stderr, "reduced_input : %s", )
    reduced_input[head_size + tail_size] = '\0';
    return (head_size + tail_size);
}
//reduce: reduce crash input and return it
char* reduce(char* origin, int len){

	//TODO idk if its right...
	char head[BUF_SIZE], tail[BUF_SIZE], mid[BUF_SIZE];
	tm = origin;
	tm_size = len;
	fprintf(stderr, "new input : %d\n", len);
	int s = len - 1;
	char reduced [BUF_SIZE];
	while(s > 0){	
		debug(printf("\n## src: %s (%d)\n", origin, s););
		for(int i = 0; i <= (len - s); i++){

			int headlen = make_substr(origin, head, 0, i);
			int taillen = make_substr(origin, tail, (s+i), len);
			debug(printf("%d)\n	head : %s \n	tail: %s\n", i, head, tail););
			
			debug(printf("	meow: %s\n", head););
			make_strcat(reduced, head, tail, headlen, taillen);
			if(head[0] == 0) continue;
			if(is_crashed(reduced, headlen+taillen)) return reduce(reduced, headlen+taillen);
		}
		for(int i = 0; i <= (len - s); i++){
			int midlen = make_substr(origin, mid, i, (i+s));
			if(mid[0] == 0) continue;
			debug(printf("	mid: %s\n", mid););
			if(is_crashed(mid, midlen)) return reduce(mid, midlen);
		}
		//return origin;
		s--;
		fprintf(stderr, "s : %d\n", s);
	}
	return origin;

}

//this is MAIN
int main(int argc, char *argv[]) {
	
	char buf[BUF_SIZE];

	parseArgs(argc, argv);
	signal(SIGALRM, timeout);
	signal(SIGINT, quit);

	//get initial crash input
	int fd = open(input, O_RDONLY);
	origin_size = read(fd, buf, BUF_SIZE);
	if(buf[origin_size-2] == '\n') buf[(origin_size--)-2] = 0;

	printf("CIMIN: %s \n", reduce(buf, origin_size));
	printf("cimin: %s \n(%d) -> (%d)\n", tm, origin_size, tm_size);

    return 0;
}

