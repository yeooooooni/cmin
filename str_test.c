#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <sys/stat.h>

#define FILE_SIZE 4096

int run_target(char * input, int input_size){
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

        while((bytes_read = read(child_w_pipe[0], result, sizeof(result) - 1)) > 0){ 
            fprintf(stderr, "result : %s", result);
            char * is_error = strstr(result, error_keyword);
            if(is_error != NULL){
                error_flag = 1;
                break;
            }
        }

        close(parent_w_pipe[1]);
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


int make_substr(char* src, char* dest, int start, int end){
    if(start > end){
        return 0;
    }
    dest = malloc((end-start +1));

    int index = 0;
    for(int i = start; i <= end; i++){
        dest[index++] = src[i];
    }
    return index + 1;
}

int make_strcat(char * reduced_input,char * head, char * tail, int head_size, int taril_size){
    reduced_input = malloc(head_size + taril_size);

    for(int i=0; i<head_size; i++){
        reduced_input[i] = head[i];
    }

    for (int j = 0; j < taril_size; j++){
        reduced_input[head_size + j] = tail[j];
    }

    return (head_size + taril_size);
}

char * Reduce(char * new_input, int input_size){ //png file has NULL so we have to remember input file size becasuse we can't use strlen
    int s = input_size - 1;
    char * head = NULL;
    char * tail = NULL;
    int head_size = 0;
    int taril_size = 0;
    char * reduced_input;
    int reduced_input_size = 0;

    while(s > 0){
        for (int i=0; i<= input_size - s; i++){
            head_size = make_substr(new_input, head, 0, i-1);
            fprintf(stderr, "hew head : %s, size : %d\n", head, head_size);

            taril_size = make_substr(reduced_input, new_input, tail, i+s, input_size -1);
            fprintf(stderr, "hew tail : %s, size : %d\n", tail, tail_size);
            
            reduced_input_size = make_strcat(head, tail, head_size, taril_size);
            fprintf(stderr, "hew reduced_input : %s, size : %d\n", reduced_input, reduced_input_size);
            if(run_target(reduced_input, reduced_input_size) == 1){
                free(head);
                free(tail);
                free(reduced_input);
                return Reduce(reduced_input, reduced_input_size);
            }
        }
        s = s-1;
    }
   
    return new_input;
}

int main(int argc,char ** argv){
    char test_str [24] = "hello";
    Reduce(test_str, strlen(test_str));
}