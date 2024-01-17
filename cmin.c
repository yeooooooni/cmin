#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <sys/stat.h>

#define FILE_SIZE 4096
char * carsh_inputF;
char * error_keyword = NULL;
char * reduced_inputF;
char * target_exeF;

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
                reduced_inputF = optarg;
                break;
            default:
                fprintf(stderr, "Invalid Command \n");
                exit(1);
        }
    }
    // target_exeF = optarg;
    target_exeF = strdup(argv[argc-1]);
    fprintf(stderr, "target: %s\n", target_exeF);
}

int run_target(char * input){
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

    pid_t pid;

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

        char cmd [256];
        // sprintf(cmd, "%s --recover --postvalid -", target_exeF);
        // execlp(cmd, cmd, NULL);
        execlp(target_exeF, target_exeF, NULL);
        // char * const args [3] = {"./target","hi", NULL};

        // execv(target_exeF, args);
    }

    close(parent_w_pipe[0]);
    close(child_w_pipe[1]);
    int bytes_written = write(parent_w_pipe[1], input, strlen(input) + 1);
    if (bytes_written == -1) {
        fprintf(stderr, "hksdfjskldfjklhere?\n");
        perror("write");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "write fread content : %d\n", bytes_written);
    close(parent_w_pipe[1]);
    // while(byte)
    char result[FILE_SIZE];
    int bytes_read;
    int error_flag = 0;
    while((bytes_read = read(child_w_pipe[0], result, sizeof(result) - 1)) > 0){ 
        fprintf(stderr, "%s", result);
        char * is_error = strstr(result, error_keyword);
        if(is_error != NULL){
            error_flag = 1;
            break;
        }
    }

    close(parent_w_pipe[1]);
    close(child_w_pipe[0]);
    int status;
    waitpid(pid, &status, 0);   

    if(error_flag == 1)
        return 1;
    return 0;
}

char * Reduce(char * new_input){


}

int main(int argc, char ** argv){
    fprintf(stderr, "stat start !\n");
    parse_cmd(argc, argv);

    
    struct stat file_info;

    if (stat(carsh_inputF, &file_info) != 0) {
        perror("stat");
        return 1;
    }

    // 파일의 크기 출력
    fprintf(stderr, "stat : File Size: %lld bytes\n", (long long)file_info.st_size);

    FILE *file;
    char input_buffer[FILE_SIZE];  // 읽어온 데이터를 저장할 버퍼

    // 파일 열기
    file = fopen(carsh_inputF, "rb");  // "rb"는 바이너리 모드로 파일 열기

    
    if (file == NULL) {
        perror("파일 열기 실패");
        return 1;
    }

    // 파일에서 데이터 읽기

    int read_byte = 0;
    size_t bytesRead;
    while((bytesRead = fread((input_buffer+read_byte), 1, FILE_SIZE - read_byte, file)) > 0){
        read_byte += bytesRead;
    }
    fprintf(stderr, "file size : %d\n", read_byte);
    
    // fprintf(stderr, "fread : %s\n", input_buffer);
    fclose(file);
    // return 0;

    if(run_target(input_buffer) == 1){
        fprintf(stderr, "ERROR DETECT\n");
    }
    return 0;
}