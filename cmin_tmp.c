#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <sys/stat.h>

#define FILE_SIZE 4096
#define BUF_SIZE 4096
#define READ 0
#define WRITE 1

//libpng는 sanitizer 붙여줘야 함 

#ifdef DEBUG
    #define debug(fn) fn
#else
    #define debug(fn)
#endif

char * carsh_inputF;
char * error_keyword = NULL;
char * reduced_F;
char * target_exeF;
char * args[16];
int args_size = 0;
int child_timeout = 0;
pid_t pid;
int reduced_input_size = 0;

void timeout(int sig){
    fprintf(stderr, "timeout occured!\n");
    kill(pid, SIGTERM);
}

void parse_cmd(int argc, char ** argv){
    if(argc <8 ){
        fprintf(stderr, "Invalid Command!\n");
        exit(1);
    }

    //command parsing 
    int opt;
    while ((opt = getopt(argc, argv, "i:m:o:")) != -1) {
        switch (opt) {
            case 'i':
                carsh_inputF = optarg;
                break;
            case 'm':
                error_keyword = optarg;
                break;
            case 'o':
                reduced_F = optarg;
                break;
            default:
                fprintf(stderr, "Invalid Command \n");
                exit(1);
        }
    }
    for (int i = optind; i < argc; i++) {
        if(i == optind) target_exeF = argv[i];
        args[args_size++] = argv[i];
    }
    args[args_size] = NULL;

    if(strstr(args[0], " ") != NULL){
        char* tok = (args[args_size++] = strtok(args[0], " "));
        while(tok != NULL) tok = (args[args_size++] = strtok(NULL, " "));
    }

    // target_exeF = strdup(argv[argc-1]);
    // fprintf(stderr, "target :%s\n", target_exeF);
}


int run_target(unsigned char * input, int input_size){
    int parent_w_pipe[2]; 
    int child_w_pipe[2]; 
 
    if(pipe(parent_w_pipe) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if(pipe(child_w_pipe) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // pid_t pid;

    if((pid = fork()) == -1){
        fprintf(stderr, "fork error\n");
        exit(1);
    }

    if(pid == 0){ // child process
        close(parent_w_pipe[1]);
        close(child_w_pipe[0]);
        
        if (dup2(parent_w_pipe[0], STDIN_FILENO) == -1){
            perror("dup2");
            return 1;
        }

        if (dup2(child_w_pipe[1],STDERR_FILENO) == -1){
            perror("dup2");
            return 1;
        }

        close(STDOUT_FILENO);
        // fprintf(stderr, "target exe : %s, args_size : %d\n", target_exeF, args_size);
        // for(int i=0 ;i<args_size; i++){
        //     fprintf(stderr, "args %s\n", args[i]);
        // }
        if(execv(target_exeF, args) == -1){
            perror("execv");
            exit(EXIT_FAILURE);
        }
    }

    else {
        close(parent_w_pipe[0]);
        close(child_w_pipe[1]);

        alarm(3);
        int bytes_written = write(parent_w_pipe[1], input, input_size);
        // fprintf(stderr, "write : %d write read content\n", bytes_written);
        if (bytes_written == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        
        close(parent_w_pipe[1]);

        char result[FILE_SIZE];
        int bytes_read;
        int error_flag = 0;

        while((bytes_read = read(child_w_pipe[0], result, FILE_SIZE - 1)) > 0){ 
            // fprintf(stderr, "result : %s\n\n", result);
            result [bytes_read] = '\0';
            char * is_error = strstr(result, error_keyword);
            if(is_error != NULL){
                error_flag = 1;
                // break;
            }
        }

        close(child_w_pipe[0]);

        int status;
        pid_t child_pid = wait(&status);
        // waitpid(pid, NULL, 0);   
        alarm(0);

        if(error_flag == 1 || WIFSIGNALED(status))
            return 1;
        return 0;
    }
    
}



// int make_substr(unsigned char* src, unsigned char* dest, int start, int end){
//     if(start > end){
//         int index = 0;
//         dest[index] = '\0';
//         return 0;
//     }
//     // fprintf(stderr, "malloc size : %d\n", end - start +1);
//     // dest = malloc((end-start +1));

//     int index = 0;
//     for(int i = start; i <= end; i++){
//         dest[index++] = src[i];
//     }
//     dest[index] = '\0';
//     //fprintf(stderr, "index : %d, make substr : %s\n", index, dest);
//     return index;
// }

int make_substr(unsigned char* src, unsigned char* dest, int start, int end){

    int index = 0;
    for(int i = start; i < end; i++){
        dest[index++] = src[i];
    }
    // dest[index] = 0;
    return index;

}


int make_strcat(unsigned char * reduced_input, unsigned char * head, unsigned char * tail, int head_size, int tail_size){
    // fprintf(stderr, "malloc size : %d", head_size + tail_size);
    // reduced_input = malloc(head_size + tail_size);

    for(int i=0; i<head_size; i++){
        reduced_input[i] = head[i];
    }

    for (int j = 0; j < tail_size; j++){
        reduced_input[head_size + j] = tail[j];
    }
    // fprintf(stderr, "reduced_input : %s", )
    // reduced_input[head_size + tail_size] = '\0';
    return (head_size + tail_size);
}


unsigned char * Reduce(unsigned char * new_input, int input_size){ //png file has NULL so we have to remember input file size becasuse we can't use strlen
    int s = input_size - 1;
    int head_size = 0;
    int tail_size = 0;
    // unsigned char head [FILE_SIZE];
    // unsigned char tail [FILE_SIZE];
    // unsigned char reduced_input [FILE_SIZE];
    unsigned char * head = malloc (sizeof(unsigned char) * FILE_SIZE);
    unsigned char * tail = malloc (sizeof(unsigned char) * FILE_SIZE);
    unsigned char * reduced_input = malloc (sizeof(unsigned char) * FILE_SIZE);

    

    fprintf(stderr, "new input : %d\n", input_size);
    while(s > 0){
        for (int i=0; i<= (input_size - s); i++){
            head_size = make_substr(new_input, head, 0, i);
            // fprintf(stderr, "new head : %s, size : %d\n", head, head_size);

            tail_size = make_substr(new_input, tail, i+s, input_size);
            // fprintf(stderr, "new tail : %s, size : %d\n", tail, tail_size);

            reduced_input_size = make_strcat(reduced_input, head, tail, head_size, tail_size);
            // if(reduced_input[0] == 0) continue;
            // fprintf(stderr, "new reduced_input : %s, size : %d\n", reduced_input, reduced_input_size);
            if(run_target(reduced_input, reduced_input_size) == 1){
                free(head);
                free(tail);
                free(new_input);
                return Reduce(reduced_input, reduced_input_size);
            }
            
        }
        // fprintf(stderr, "mid start!\n");
        for (int i=0; i<= input_size - s; i++){
            // fprintf(stderr, "s : %d, %d ~ %d\n", s ,i, i+s-1);
            reduced_input_size = make_substr(new_input, reduced_input, i, i+s);
            // if(reduced_input[0] == 0) continue;
            // fprintf(stderr, "new reduced_input : %s, size : %d\n", reduced_input, reduced_input_size);
            if(run_target(reduced_input, reduced_input_size) == 1){
                free(head);
                free(tail);
                free(new_input);
                return Reduce(reduced_input, reduced_input_size);
            }
        }
        s = s-1;
        fprintf(stderr, "s : %d\n", s);
    }
    reduced_input_size = input_size;
    free(head);
    free(tail);
    free(reduced_input);

    int reduced_fd;

    // fprintf(stderr, "reducedF : %s\n", reduced_F);
    if ((reduced_fd = open(reduced_F, O_WRONLY| O_TRUNC| O_CREAT, 0666)) == -1) {
        perror("파일 열기 실패");
    //     return 1;
    }

    int bytes_written = 0;
    while(bytes_written != reduced_input_size){
        int write_byte= write(reduced_fd, new_input +bytes_written, reduced_input_size - bytes_written);
        bytes_written += write_byte;
    }
    // fprintf(stderr, "bytes written : %d, reduced input size : %d", bytes_written, reduced_input_size);
    close(reduced_fd);

    return new_input;
}

int main(int argc, char ** argv){
    signal(SIGALRM, timeout);  // alarm 시그널 핸들러 등록

    parse_cmd(argc, argv);

    // struct stat file_info;

    // if (stat(carsh_inputF, &file_info) != 0) {
    //     perror("stat");
    //     return 1;
    // }

    // // 파일의 크기 출력
    // fprintf(stderr, "stat : File Size: %lld bytes\n", (long long)file_info.st_size);

    int fd = open (carsh_inputF, O_RDONLY);
    int len = 0;
    unsigned char * buf = malloc(sizeof(unsigned char) * FILE_SIZE);
	len = read(fd, buf, BUF_SIZE);
	if(buf[len-2] == '\n') buf[(len--)-2] = 0;

    close(fd);

    // unsigned char * tm = malloc(sizeof(unsigned char) * FILE_SIZE);
    Reduce(buf, len);
	// strcpy(tm, Reduce(buf, len));
    // fprintf(stderr, "CIMIN\n %s\n", tm);

    // int reduced_fd;

    // // fprintf(stderr, "reducedF : %s\n", reduced_F);
    // if ((reduced_fd = open(reduced_F, O_WRONLY| O_TRUNC| O_CREAT, 0666)) == -1) {
    //     perror("파일 열기 실패");
    //     return 1;
    // }

    // int bytes_written = 0;
    // while(bytes_written != reduced_input_size){
    //     int write_byte= write(reduced_fd, tm +bytes_written, reduced_input_size - bytes_written);
    //     bytes_written += write_byte;
    // }
    // // fprintf(stderr, "bytes written : %d, reduced input size : %d", bytes_written, reduced_input_size);
    // close(reduced_fd);

    return 0;
}