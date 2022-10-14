#include <unistd.h> // / header for unix specic functions declarations : fork(), getpid(), getppid(), sleep()
#include <stdio.h>  // header for input and output from console : printf, perror
#include <stdlib.h> // header for general functions declarations: exit() and also has exit macros such as EXIT_FAILURE - unsuccessful execution of a program
#include <sys/socket.h> // header for socket specific functions and macros declarations
#include <netinet/in.h> //header for MACROS and structures related to addresses "sockaddr_in", INADDR_ANY 
#include <string.h> // header for string functions declarations: strlen()
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/mman.h>
#include <dirent.h>


#define LIGHT_BLUE "\033[1;34m" // imitate shell
#define LIGHT_GREEN "\033[1;32m"
#define NONE "\033[m"
#define clear() printf("\033[H\033[J")

#define normal 0 // no pipe
#define pipe1 1  // a | b
#define pipe2 2  // a | b | c
#define pipe3 3  // a | b | c | d
#define pipeError 4 // incorrect pipe 

#define PORT 5564 //or 5554 tcp client server port

void get_param(char *param);                          // get paramaners, a string
int find(char *param);                                // find if the command exists
void cd(char *path);                                  // cd
void anal(char *param, int *num, char arr[100][256]); // analyze command line
void exec(int param_num, char para[100][256], char return_message[4096]);        // execute command
void print_dir();                                     // imitate shell format


int main()// main function
{
  int sock1, sock2, valread;
  struct sockaddr_in address; // structure for storing addres; local interface and port 
  int addrlen = sizeof(address);

  // Creating socket file descriptor with communication: domain of internet protocol version 4, type of SOCK_STREAM for reliable/conneciton oriented communication, protocol of internet  
  if ((sock1 = socket(AF_INET, SOCK_STREAM, 0)) == 0) // checking if socket creation fail
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // setting the address to be bind to socket
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(PORT);

  // attaching socket to addresses (any/all local ip with port 5564)
  if (bind(sock1, (struct sockaddr *)&address,
   sizeof(address)) < 0) // checking if bind fails
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(sock1, 10) < 0) // defining for socket length of queue for pending client connections
  {
    perror("Listen Failed");
    exit(EXIT_FAILURE);
  }

  while (1) // to keep server alive for infintiy
  {
      printf("Server running..\n");
    if ((sock2 = accept(sock1, (struct sockaddr *)&address,
     (socklen_t *)&addrlen)) < 0) // accepting the client connection with creation/return of a new socket for the established connection to enable dedicated communication (active communication on a new socket) with the client
    {
      perror("accept");
      exit(EXIT_FAILURE);
    }

    
    printf("Listening to client..\n"); // while printing make sure to end your strings with \n or \0 to flush the stream, other wise if in anyother concurent process is reading from socket/pipe-end with standard input/output redirection, it will keep on waiting for stream to end. 
    
    while(1){ // keep doing things for this client

        // send the dir to client
        char cwd[1024]={0};
        getcwd(cwd, sizeof(cwd));
        send(sock2,cwd,strlen(cwd),0);

        char message[1024] = {0};
        char return_message[4096];

        int param_num = 0;                 // e.g. ls -l = 2
        char param_arr[100][256];          // formatted command string

        // recieve message
        recv(sock2, message, sizeof(message), 0);
        printf("String recieved: %s\n", message);

        if ((strcmp("exit", message) == 0)) // user want to exit
        {
            // close(sock2);
            break;
        }
        anal(message, &param_num, param_arr);  // split the commend into a 2D array
        if (strcmp(param_arr[0], "cd") == 0) // cd to the place // TODO
        {
            cd(param_arr[1]);
            send(sock2,"already cd to the file\n\0",strlen("already cd to the file\n\0"),0);
            // char cwd[1024];
            // getcwd(cwd, sizeof(cwd));
            // send(sock2,cwd,strlen(cwd),0);
            printf("already cd\n");
            recv(sock2, cwd, sizeof(cwd), 0);
            printf("client got the result");
            continue;
        }
        // clear the global return_message
        for(int qingkong = 0;qingkong<4096;qingkong++){
            return_message[qingkong]=0;
        }

        exec(param_num, param_arr, return_message);
        printf("return message processed!\n");

        if((strcmp("", return_message) == 0)){
            send(sock2,"Nothing returned!\n\0",strlen("Nothing returned!\n\0"),0);
            // char cwd[1024];
            // getcwd(cwd, sizeof(cwd));
            // send(sock2,cwd,strlen(cwd),0);
            recv(sock2, cwd, sizeof(cwd), 0);
            printf("client got the result");
            continue;
        }
        // convert the result to string and sent to client
        send(sock2,return_message,strlen(return_message),0); // send message to client
        // recieve ack
        recv(sock2, cwd, sizeof(cwd), 0);
        printf("client got the result");
    }
    close(sock2); // close the conneciton with client

  }

  close(sock1);
  
  return 0;
}


void anal(char *param, int *num, char arr[100][256]) // param: whole string      num: numbers of segment        arr: 2D command array
{
    char *p = param;
    char *q = param;  // all point to first character of the original string
    int number = 0;  // length of current segment
    while (1)
    {
        if (p[0] == '\0'){ // end of string, stop processing
            break;
        }
        if (p[0] == ' '){  // skip space
            p++;
        } 
        if(p[0] == '|'){   // put | in a separate column
            strncpy(arr[*num], p, 2);
            arr[*num][1] = '\0';
            *num = *num + 1;
            p++;
        }
        else // meeting something not space, not end, not |
        {
            q = p;  // q point to p
            number = 0;
            while (q[0] != '\0' && q[0] != ' ' && q[0] != '|') // count how long before next space or |
            {
                q++;
                number++;
            }
            strncpy(arr[*num], p, number + 1);
            arr[*num][number] = '\0';
            *num = *num + 1;
            p = q; // p point to q now, the space or |
        }
    }
}

void exec(int param_num, char para[100][256], char return_message[4096])
{
    pid_t pid1; //        child 1
    pid_t pid2; //         /   |
    pid_t pid3; //  child 2   child 3
    pid_t pid4;
    int fd1[2];
    int fd2[2];
    int fd3[2];
    char *arg[param_num + 1];

    int stat_val;
    int i, j, x = 0, y = 0; // util for split the arg123
    int flag = 0;           // how many "|" in total
    int how = 0;            // 0:normal, 1:a|b, 2:a|b|c
    int bg = 0;
    int position1 = -1; // flag for the first |
    int position2 = -1; // flag for the second |
    int position3 = -1; // flag for the second |

    int link[2];

    for (i = 0; i < param_num; i++)
    {
        arg[i] = (char *)para[i]; // store whatever user input into arg
    }
    arg[param_num] = NULL;

    // if(!find(arg1[0])) return;       // no such command

    for (i = 0; i < param_num; i++) // chechk how many | are there
    {   
        if (strchr(para[i],'|') != NULL){
            if (strcmp(para[i], "|") == 0)
            {
                flag++;
            }
            else{
                printf("wrong!");
                flag = -1;
                break;
            }
        }
    }
    

    if (flag == -1){
        how = pipeError;
    }
    if (flag == 0)
    {
        // printf("--- oh, no pipe ---\n");
        how = normal;
    }

    if (flag == 1)
    {
        // printf("--- this is a pipe b ---\n");
        how = pipe1;
    }

    if (flag == 2)
    {
        // printf("--- this is a pipe b pipe c ---\n");
        how = pipe2;
    }

    if (flag == 3)
    {
        // printf("--- this is a pipe b pipe c pipe d ---\n");
        how = pipe3;
    }

    if (how == pipe1) // process command, split it into arg1 and arg2
    {
        for (i = 0; i < param_num; i++)
        {
            if (strcmp(para[i], "|") == 0)
            {
                position1 = i;
                break;
            }
        }
        if (position1 == 0 || position1 == param_num - 1){
            how = pipeError;
        }
    }

    if (how == pipe2) // process command, split it into arg1 and arg2 and arg3
    {
        for (i = 0; i < param_num; i++)
        {
            if (strcmp(para[i], "|") == 0)
            {
                if (position1 == -1)
                    position1 = i;
                else
                    position2 = i;
            }
        }

        if (position1 == 0 || (position2 - position1 == 1) || (position2 == param_num - 1))
            how = pipeError;
        
    }
    if (how == pipe3) // process command, split it into arg1 and arg2 and arg3 and arg4
    {
        for (i = 0; i < param_num; i++)
        {
            if (strcmp(para[i], "|") == 0)
            {
                if (position1 == -1)
                    position1 = i;
                else if(position2 == -1)
                    position2 = i;
                else
                    position3 = i;
            }
        }
        if (position1 == 0 || position2 - position1 == 1 || position3 - position2 == 1 || position3 == param_num - 1){
            how = pipeError;
        }
    }

    pipe(link);
    pid1 = fork();

    if (pid1 < 0)
    {
        printf("child 1 creation failed\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        switch (how)
        {

        case 0:
            if (pid1 == 0)
            {
                dup2(link[1], STDOUT_FILENO); // redirect the result
                close(link[0]);
                close(link[1]);

                // check if the command exsist
                int flag0 = find(arg[0]);
                if(flag0 == 0){
                    printf("Command '%s' not found.\n", arg[0]);
                    exit(0);
                }
                
                // execute
                execvp(arg[0], arg);
                exit(0);
            }else{
                wait(NULL);
                close(link[1]);
                // get the result and store in global return_message
                read(link[0], return_message, 4096);
                close(link[0]);
                printf("in the child, result be like:\n");
                printf("%s\n", return_message);
            }
            break;

        case 1:
            if (pid1 == 0)
            {
                // handle incorrect pipe position
                // split commands
                char *arg1[position1 + 1];
                for (int i = 0; i < position1; i++)
                {
                    arg1[i] = (char *)para[i];
                    // strcpy(arg1[i], para[i]);
                    // printf("%s\n",arg[i]);
                }
                arg1[position1] = NULL;

                char *arg2[param_num - position1];
                int x = 0;
                for (int j = position1 + 1; j < param_num; j++)
                {
                    arg2[x] = (char *)para[j];
                    // printf("%s\n",arg[j]);
                    x++;
                }
                arg2[param_num - position1 - 1] = NULL;

                dup2(link[1], STDOUT_FILENO);
                close(link[0]);
                close(link[1]);

                // check if the command exsist
                int flag1 = find(arg1[0]);
                int flag2 = find(arg2[0]);

                if(flag1 == 0){
                    printf("Command '%s' not found.\n", arg1[0]);
                    exit(0);
                }
                if(flag2 == 0){
                    printf("Command '%s' not found.\n", arg2[0]);
                    exit(0);
                }

                // execute
                pipe(fd1);
                if (pipe(fd1) < 0)
                {
                    printf("pipe 1 creation failed\n");
                    exit(EXIT_FAILURE);
                }
                pid2 = fork();
                if (pid2 < 0)
                {
                    printf("child 2 creation failed\n");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    if (pid2 == 0) // child 2
                    {
                        dup2(fd1[1], 1);
                        close(fd1[0]);
                        close(fd1[1]);
                        if (execvp(arg1[0], arg1) < 0)
                        {
                            printf("child 2 error");
                            perror("Execvp failed from child");
                        }
                        exit(EXIT_FAILURE);
                    }
                    else // child 1
                    {
                        wait(NULL);
                        dup2(fd1[0], 0);
                        close(fd1[1]);
                        close(fd1[0]);
                        if (execvp(arg2[0], arg2) < 0)
                        {
                            printf("cihld 2 error");
                            perror("Execvp failed from p");
                        }
                        // perror ("Execvp failed while child 2\n");
                        exit(EXIT_FAILURE);
                    }
                }
            }else{
                waitpid(pid1, &stat_val, 0);
                close(link[1]);
                read(link[0], return_message, 4096);
                close(link[0]);
                printf("in the child, result be like:\n");
                printf("%s\n", return_message);
            }
            break;

        case 2:
            if (pid1 == 0)
            {   
                // split commands
                char *arg1[position1 + 1];
                for (int i = 0; i < position1; i++)
                {
                    arg1[i] = arg[i];
                    // printf("%s\n",arg[i]);
                }
                arg1[position1] = NULL;

                int x = 0;
                char *arg2[position2 - position1];
                for (int j = position1 + 1; j < position2; j++)
                {
                    arg2[x] = arg[j];
                    x++;
                    // printf("%s\n",arg[j]);
                }
                arg2[position2 - position1 - 1] = NULL;

                char *arg3[param_num - position2];
                int y = 0;
                for (int q = position2 + 1; q < param_num; q++)
                {
                    arg3[y] = arg[q];
                    y++;
                    // printf("%s\n",arg[j]);
                }
                arg3[param_num - position2 - 1] = NULL;

                dup2(link[1], STDOUT_FILENO);
                close(link[0]);
                close(link[1]);

                // check if the command exsist
                int flag1 = find(arg1[0]);
                int flag2 = find(arg2[0]);
                int flag3 = find(arg3[0]);

                if(flag1 == 0){
                    printf("Command '%s' not found.\n", arg1[0]);
                    exit(0);
                }
                if(flag2 == 0){
                    printf("Command '%s' not found.\n", arg2[0]);
                    exit(0);
                }
                if(flag3 == 0){
                    printf("Command '%s' not found.\n", arg3[0]);
                    exit(0);
                }

                // execute
                if (pipe(fd1) < 0)
                    exit(EXIT_FAILURE);
                if (pipe(fd2) < 0)
                    exit(EXIT_FAILURE);
                pid2 = fork();
                if (pid2 < 0)
                {
                    printf("child 2 creation failed\n");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    if (pid2 == 0) // child 2
                    {
                        dup2(fd1[1], 1); // write by redirecting standard output to pipe 1
                        close(fd1[1]);
                        close(fd1[0]);
                        close(fd2[0]);
                        close(fd2[1]);
                        execvp(arg1[0], arg1);
                        perror("Execvp failed while executing child 2");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        pid3 = fork();
                        if (pid3 == 0)
                        {
                            // printf("%d: child 2 - grep\n", getpid());
                            dup2(fd1[0], 0); // reading redirected ouput of ls through pipe 1
                            dup2(fd2[1], 1); // write by redirecting standard output to pipe 2
                            close(fd1[1]);
                            close(fd1[0]);
                            close(fd2[1]);
                            close(fd2[0]);
                            execvp(arg2[0], arg2);
                            perror("Execvp failed while executing child 3");
                            exit(EXIT_FAILURE);
                        }
                        else  // child 1
                        {
                            dup2(fd2[0], 0);
                            close(fd1[1]);
                            close(fd1[0]);
                            close(fd2[1]);
                            close(fd2[0]);
                            execvp(arg3[0], arg3);
                            perror("Execvp failed while executing main child");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
            else{
                waitpid(pid1, &stat_val, 0);
                close(link[1]);
                read(link[0], return_message, 4096);
                close(link[0]);
                printf("in the child, result be like:\n");
                printf("%s\n", return_message);
            }
            break;
        case 3:
            if (pid1 == 0)
            {
                // split commands
                char *arg1[position1 + 1];
                for (int i = 0; i < position1; i++)
                {
                    arg1[i] = arg[i];
                    // printf("%s\n",arg[i]);
                }
                arg1[position1] = NULL;

                int x = 0;
                char *arg2[position2 - position1];
                for (int j = position1 + 1; j < position2; j++)
                {
                    arg2[x] = arg[j];
                    x++;
                    // printf("%s\n",arg[j]);
                }
                arg2[position2 - position1 - 1] = NULL;

                x = 0;
                char *arg3[position3 - position2];
                for (int j = position2 + 1; j < position3; j++)
                {
                    arg3[x] = arg[j];
                    x++;
                    // printf("%s\n",arg[j]);
                }
                arg3[position3 - position2 - 1] = NULL;

                char *arg4[param_num - position3];
                int y = 0;
                for (int q = position3 + 1; q < param_num; q++)
                {
                    arg4[y] = arg[q];
                    y++;
                    // printf("%s\n",arg[j]);
                }
                arg4[param_num - position3 - 1] = NULL;

                dup2(link[1], STDOUT_FILENO);
                close(link[0]);
                close(link[1]);

                // check if the command exsist
                int flag1 = find(arg1[0]);
                int flag2 = find(arg2[0]);
                int flag3 = find(arg3[0]);
                int flag4 = find(arg4[0]);

                if(flag1 == 0){
                    printf("Command '%s' not found.\n", arg1[0]);
                    exit(0);
                }
                if(flag2 == 0){
                    printf("Command '%s' not found.\n", arg2[0]);
                    exit(0);
                }
                if(flag3 == 0){
                    printf("Command '%s' not found.\n", arg3[0]);
                    exit(0);
                }
                if(flag4 == 0){
                    printf("Command '%s' not found.\n", arg4[0]);
                    exit(0);
                }

                // execute
                if (pipe(fd1) < 0)
                    exit(EXIT_FAILURE);
                if (pipe(fd2) < 0)
                    exit(EXIT_FAILURE);
                if (pipe(fd3) < 0)
                    exit(EXIT_FAILURE);

                pid2 = fork();
                if (pid2 < 0)
                {
                    printf("child 2 creation failed\n");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    if (pid2 == 0) // child 2
                    {
                        dup2(fd1[1], 1); // write by redirecting standard output to pipe 1
                        close(fd1[1]);
                        close(fd1[0]);
                        close(fd2[0]);
                        close(fd2[1]);
                        close(fd3[1]);
                        close(fd3[0]);
                        execvp(arg1[0], arg1);
                        perror("Execvp failed while executing child 2");
                        exit(EXIT_FAILURE);
                    }
                    else
                    {
                        pid3 = fork();
                        if (pid3 == 0)
                        {
                            // printf("%d: child 2 - grep\n", getpid());
                            dup2(fd1[0], 0); // reading redirected ouput of ls through pipe 1
                            dup2(fd2[1], 1); // write by redirecting standard output to pipe 2
                            close(fd1[1]);
                            close(fd1[0]);
                            close(fd2[1]);
                            close(fd2[0]);
                            close(fd3[1]);
                            close(fd3[0]);
                            execvp(arg2[0], arg2);
                            perror("Execvp failed while executing child 3");
                            exit(EXIT_FAILURE);
                        }
                        else
                        {
                            pid4 = fork();
                            if (pid4 == 0){
                                dup2(fd2[0], 0); // reading redirected ouput of ls through pipe 1
                                dup2(fd3[1], 1); // write by redirecting standard output to pipe 2
                                close(fd1[1]);
                                close(fd1[0]);
                                close(fd2[1]);
                                close(fd2[0]);
                                close(fd3[1]);
                                close(fd3[0]);
                                execvp(arg3[0], arg3);
                                perror("Execvp failed while executing child 3");
                                exit(EXIT_FAILURE);
                            }
                            else
                                dup2(fd3[0], 0);
                                close(fd1[1]);
                                close(fd1[0]);
                                close(fd2[1]);
                                close(fd2[0]);
                                close(fd3[1]);
                                close(fd3[0]);
                                execvp(arg4[0], arg4);
                                perror("Execvp failed while executing main child");
                                exit(EXIT_FAILURE);
                        }
                    }
                }
            }else{
                waitpid(pid1, &stat_val, 0);
                close(link[1]);
                read(link[0], return_message, 4096);
                close(link[0]);
                printf("in the child, result be like:\n");
                printf("%s\n", return_message);
            }
            break;
        case 4:
            {
            strcpy(return_message,"ERROR Pipe Input!"); 
            }
            break;
        default:
            break;
        }
    }
    if (bg)
        return;

    waitpid(pid1, &stat_val, 0);
    return;
}

int find(char *param)
{
    DIR *dir;
    struct dirent *ptr;
    dir = opendir("/bin");
    while ((ptr = readdir(dir))!=0)
    {
        if (strcmp(param, ptr->d_name) == 0)
            return 1;
    }
    closedir(dir);
    // printf("Command '%s' not found.\n", param);
    return 0;
}

void cd(char *path)
{
    if (chdir(path) < 0)
        perror("chdir");
}

void print_dir()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf(LIGHT_GREEN "Dir:");
    printf(LIGHT_BLUE "%s", cwd);
}