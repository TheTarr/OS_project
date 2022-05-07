#include <unistd.h>     // / header for unix specic functions declarations : fork(), getpid(), getppid(), sleep()
#include <stdio.h>      // header for input and output from console : printf, perror
#include <stdlib.h>     // header for general functions declarations: exit() and also has exit macros such as EXIT_FAILURE - unsuccessful execution of a program
#include <sys/socket.h> // header for socket specific functions and macros declarations
#include <netinet/in.h> // header for MACROS and structures related to addresses "sockaddr_in", INADDR_ANY
#include <string.h>     // header for string functions declarations: strlen()
#include <pthread.h>    // header for thread functions declarations: pthread_create, pthread_join, pthread_exit
#include <signal.h>     // header for signal related functions and macros declarations
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/mman.h>
#include<semaphore.h>
// compile your code with: gcc -o output code.c -lpthread

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
#define NUM_CLIENTS 10

sem_t client[10]; // 10个client，十个锁
int client_flag = 0; // indicate 这个client用的时哪个锁

// …………………………………………………………………………………链表结构…………………………………………………………………………………………………
typedef struct slist // single list 单链表
{
    pthread_t client_id; // 用户线程 id
    int the_flag; // 这个用户用的哪个锁
    char message[1024]; // 用户输入的命令
    int remaining_time; // 这个命令还需要执行多久，系统命令是1，./looping 要变
    struct slist *next;
} L;

// …………………………………………………………………………………handle_client线程参数…………………………………………………………………………………………………
struct client_arg{
   int new_socket; //回消息要用
   L* header; // 单链表
   int my_flag; // 我用的哪个锁
} *args;

// …………………………………………………………………………………链表…………………………………………………………………………………………………
//创建一个节点
L *create_node(pthread_t client_id, char message[1024], int the_flag) // sem_t client_semaphor
{
    //给每个节点分配结构体一样的空间大小
    L *node = (L *)malloc(sizeof(L));
    if (NULL == node)
    {
        printf("malloc error!\n");
        return NULL;
    }
    //由于结构体在未初始化的时候一样是脏数据，所以要清
    memset(node, 0, sizeof(L));
    //初始化第一个节点
    node->client_id = client_id;
    node->the_flag = the_flag;
    strncpy(node->message, message, strlen(message));
    char *p = "./looping"; // 检测命令是否为大循环
    if (strstr(message, p))
    { // 如果是大循环，取出循环时间
        int len = strlen(message);
        int num_len = len - 10;
        char *pointer = message;
        char num_str[num_len];
        for (int i = 0; i < 10; i++)
        {
            pointer++;
        }
        strncpy(num_str, pointer, num_len);
        node->remaining_time = atoi(num_str);
    }
    else
    { // 如果是系统命令统一都是1
        node->remaining_time = 1;
    }
    //将节点的后继指针设置为NULL
    node->next = NULL;
}

//创建一个确定时长的节点
L *strict_create_node(pthread_t client_id, char message[1024], int the_flag, int remaining_time) // sem_t client_semaphor
{
    //给每个节点分配结构体一样的空间大小
    L *node = (L *)malloc(sizeof(L));
    if (NULL == node)
    {
        printf("malloc error!\n");
        return NULL;
    }
    //由于结构体在未初始化的时候一样是脏数据，所以要清
    memset(node, 0, sizeof(L));
    //初始化第一个节点
    node->client_id = client_id;
    node->the_flag = the_flag;
    strncpy(node->message, message, strlen(message));
    node->remaining_time = remaining_time;
    //将节点的后继指针设置为NULL
    node->next = NULL;
}

//链表的尾插
void tail_insert(L *pH, L *new)
{
    //获取当前的位置
    L *p = pH;
    //如果当前位置的下一个节点不为空
    while (NULL != p->next)
    {
        //移动到下一个节点
        p = p->next;
    }
    //如果跳出以上循环，所以已经到了NULL的这个位置
    //此时直接把新插入的节点赋值给NULL这个位置
    p->next = new;
}

//链表的头插
void top_insert(L *pH, L *new)
{
    L *p = pH;
    new->next = p->next;
    p->next = new;
}

//链表的遍历打印
void Print_node(L *pH)
{
    //获取当前的位置
    L *p = pH;
    int flag = 1;
    //获取第一个节点的位置
    if (p->next == NULL){
        printf("Nothing in list!\n");
        flag = 0;
    }
    //如果当前位置的下一个节点不为空
    
    if(flag ==1){
        p = p->next;
    while (NULL != p->next)
    {
        //(1)打印节点的数据
        printf("id:%lu\n", p->client_id);
        printf("message=%s\n", p->message);
        printf("remaining_time=%d\n", p->remaining_time);
        //(2)移动到下一个节点,如果条件仍为真，则重复(1)，再(2)
        p = p->next;
    }
    //如果当前位置的下一个节点为空，则打印数据
    //说明只有一个节点
    printf("id:%lu\n", p->client_id);
    printf("message=%s\n", p->message);
    printf("remaining_time=%d\n", p->remaining_time);
    }
}

//查询列表里是否有该id的节点，有回1，没有回0
int check_node_exsist(L *pH, int id)
{
    //获取当前的位置
    L *p = pH;
    //获取第一个节点的位置
    p = p->next;
    //如果当前位置的下一个节点不为空
    while (NULL != p->next)
    {
        if (p->client_id == id)
        {
            return 1;
        }
        p = p->next;
    }
    //如果当前位置的下一个节点为空，则打印数据
    //说明只有一个节点
    if (p->client_id == id)
    {
        return 1;
    }
    return 0;
}

//删除链表中的节点，找到的第一个
int detele_list_node(L *pH, pthread_t id)
{
    //获取当前头节点的位置
    L *p = pH;
    L *prev = NULL;
    while (NULL != p->next)
    {
        //保存当前节点的前一个节点的指针
        prev = p;
        //然后让当前的指针继续往后移动
        p = p->next;
        //判断，找到了要删除的数据
        if (p->client_id == id)
        {
            //两种情况，一种是普通节点，还有一种是尾节点
            if (p->next != NULL) //普通节点的情况
            {
                prev->next = p->next;
                free(p);
            }
            else //尾节点的情况
            {
                prev->next = NULL; //将这个尾节点的上一个节点的指针域指向空
                free(p);
            }
            return 0;
        }
    }
    printf("没有要删除的节点\n");
    return -1;
}

// …………………………………………………………………………………算法管理员…………………………………………………………………………………………………
// 该函数是一个永远在后台跑的线程，用来管理当前的所有线程
void *manage_process(void *header){
    // while (1){
    //     Print_node(header);
    //     putchar('\n');
    //     sleep(2);
    // }
    L*my_list = (L*)header;
    Print_node(my_list);
    putchar('\n');
    printf("hello from manager!\n");
    while(1){
        // get the first node
        // if(my_list->next != NULL){
        //     L *next_node = my_list->next;
        // }
        // else{
        //     printf("Nothing in the queue now, waiting for commands.\n");
        //     sleep(1);
        //     continue;
        // }
        L *next_node = my_list->next;
        if(next_node == NULL){
            printf("Nothing in the queue now, waiting for commands.\n");
            sleep(1);
            continue;
        }
        // 如果list里有东西
        // printf("first going to posted, id: %d!\n", next_node->client_semaphor);
        // 如果是系统命令或loop剩下对后一秒，跑，睡一秒
        if (next_node->remaining_time == 1 && next_node->message[2]!='l' && next_node->message[3]!= 'o'&& next_node->message[4]!= 'o'&& next_node->message[5]!= 'p'){
            // printf("current node: %s\n", next_node->message);
            // int sval;
            // sem_getvalue (&client[next_node->the_flag], &sval);
            // printf("first going to posted, value: %d!\n", sval);
            sem_post(&client[next_node->the_flag]);
            // sem_getvalue (&client[next_node->the_flag], &sval);
            // printf("first posted, now value: %d!\n", sval);
            // sleep(10);
            Print_node(my_list);
            printf("current client id: %lu\n", next_node->client_id);
            detele_list_node(my_list, next_node->client_id);
            // printf("node deleted!!!!!!\n\n\n\n");
            // Print_node(my_list);
            // sleep(1);
            continue;
        // 如果是循环命令
        }else{
            int finish_flag = 0; // 用来判断循环完了没有，完了是1，没完是0
            // sem_post(&client[next_node->the_flag]);
            int i = 0; // 跑了几轮
            for(int x=next_node->remaining_time; x>next_node->remaining_time-3;x--){ // 循环
                if(x<1){
                    finish_flag = 1;
                    break;
                }
                else{
                    printf("from client %lu, round: %d\n",next_node->client_id,x);
                    i += 1;
                    sleep(1);
                    L *first_node = my_list->next;
                    if(first_node->client_id != next_node->client_id){ // 如果当前链表的第一个不是本node
                        break;
                    }
                }
            }
            if(finish_flag == 1){ // 如果循环完了，删掉这个node
                sem_post(&client[next_node->the_flag]);
                detele_list_node(my_list, next_node->client_id);
            }
            else{ // 如果没循环完，把这个更新后的命令插到最后去，删除当前命令
                tail_insert(args -> header, strict_create_node(next_node->client_id, next_node->message, next_node->the_flag, next_node->remaining_time-i));
                detele_list_node(my_list, next_node->client_id);
            }
        }
        // Print_node(my_list);
        // printf("current node: %s\n", next_node->message);
        // int sval;
        // sem_getvalue (&client[next_node->the_flag], &sval);
        // printf("first going to posted, value: %d!\n", sval);
        // sem_post(&client[next_node->the_flag]);
        // sem_getvalue (&client[next_node->the_flag], &sval);
        // printf("first posted, now value: %d!\n", sval);
        // sleep(10);
        
        // // Print_node(my_list);
        // next_node = next_node->next;

        // sem_getvalue (&client[next_node->the_flag], &sval);
        // printf("first going to posted, value: %d!\n", sval);
        // sem_post(&client[next_node->the_flag]);
        // sem_getvalue (&client[next_node->the_flag], &sval);
        // printf("first posted, now value: %d!\n", sval);
        // sleep(10);
    }

}

void serverExitHandler(int sig_num);
void get_param(char *param);                          // get paramaners, a string
int find(char *param);                                // find if the command exists
void cd(char *path);                                  // cd
void anal(char *param, int *num, char arr[100][256]); // analyze command line
void exec(int param_num, char para[100][256], char return_message[4096]);        // execute command
void print_dir();                                     // imitate shell format

void serverExitHandler(int sig_num)
{
  printf("\n Exiting server.  \n");
  fflush(stdout); // force to flush any data in buffers to the file descriptor of standard output,, a pretty convinent function
  exit(0);
}

// Function that handle dedicated communication with a client
void *HandleClient(void *arguments)
{
  struct client_arg *args = arguments;
  pthread_detach(pthread_self()); // detach the thread as we don't need to synchronize/join with the other client threads, their execution/code flow does not depend on our termination/completion
  // int socket = *(int *)args->new_socket;
  int socket = args->new_socket;
  int flag = args->my_flag;
  // free(new_socket);
  printf("handling new client in a thread using socket: %d\n", socket);
  printf("Listening to client..\n"); // while printing make sure to end your strings with \n or \0 to flush the stream, other wise if in anyother concurent process is reading from socket/pipe-end with standard input/output redirection, it will keep on waiting for stream to end.

  while(1){ // keep doing things for this client

        // send the dir to client
        char cwd[1024]={0};
        getcwd(cwd, sizeof(cwd));
        send(socket,cwd,strlen(cwd),0);

        char message[1024] = {0};
        char return_message[4096];

        int param_num = 0;                 // e.g. ls -l = 2
        char param_arr[100][256];          // formatted command string

        // recieve message
        recv(socket, message, sizeof(message), 0);
        printf("String recieved: %s\n", message);

        if ((strcmp("exit", message) == 0)) // user want to exit
        {
          printf("Client want to exit.");
            close(socket);
            break;
        }
        if(message[2]=='l' && message[3]== 'o'&& message[4]== 'o'&& message[5]== 'p'){ // 如果是loop命令，插到队尾
            tail_insert(args -> header, create_node(pthread_self(), message,flag));
        }else{
            top_insert(args -> header, create_node(pthread_self(), message,flag)); // 如果是系统命令，插到队首
        }
        // signal插在这里
        int sval;
        sem_getvalue (&client[flag], &sval);
        // printf("now waiting, id: %d\n",args -> client_semaphor);
        printf("bafore waiting, value: %d\n",sval);
        sem_wait(&client[flag]);
        printf("now waking!\n");

        anal(message, &param_num, param_arr);  // split the commend into a 2D array
        if (strcmp(param_arr[0], "cd") == 0) // cd to the place // TODO
        {
            cd(param_arr[1]);
            send(socket,"already cd to the file\n\0",strlen("already cd to the file\n\0"),0);
            // char cwd[1024];
            // getcwd(cwd, sizeof(cwd));
            // send(socket,cwd,strlen(cwd),0);
            printf("already cd\n");
            recv(socket, cwd, sizeof(cwd), 0);
            printf("client got the result\n");
            continue;
        }
        // clear the global return_message
        for(int qingkong = 0;qingkong<4096;qingkong++){
            return_message[qingkong]=0;
        }

        exec(param_num, param_arr, return_message);
        printf("return message processed!\n");

        if((strcmp("", return_message) == 0)){
            if(message[2]=='l' && message[3]== 'o'&& message[4]== 'o'&& message[5]== 'p'){
                send(socket,"Looping finished!\n\0",strlen("Looping finished!\n\0"),0);
            }
            else{
                send(socket,"Nothing returned!\n\0",strlen("Nothing returned!\n\0"),0);
            }
            // char cwd[1024];
            // getcwd(cwd, sizeof(cwd));
            // send(sock2,cwd,strlen(cwd),0);
            recv(socket, cwd, sizeof(cwd), 0);
            printf("client got the result");
            continue;
        }

        // 结束在这里
        // sem_post(&args -> client_semaphor);
        // convert the result to string and sent to client
        send(socket,return_message,strlen(return_message),0); // send message to client
        // recieve ack
        recv(socket, cwd, sizeof(cwd), 0);
        printf("client got the result");
    }

  pthread_exit(NULL); // terminate the thread
}

int main() // main function
{
    //创建第一个节点
    L *header = create_node(0, "header", -1); // header就是这个单链表

    // 创建一个thread，专门用来管理当前线程
    pthread_t thread_id;
    int rc;
    rc = pthread_create(&thread_id, NULL, manage_process, header);
    if (rc) // if rc is > 0 imply could not create new thread
    {
      printf("\n ERROR: return code from pthread_create is %d \n", rc);
      exit(EXIT_FAILURE);
    }

  // Set the SIGINT (Ctrl-C) signal handler to serverExitHandler
  signal(SIGINT, serverExitHandler);

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

  if (listen(sock1, NUM_CLIENTS) < 0) // defining for socket length of queue for pending client connections
  {
    perror("Listen Failed");
    exit(EXIT_FAILURE);
  }

  while (1) // to keep server alive for infintiy
  {

    if ((sock2 = accept(sock1, (struct sockaddr *)&address,
                        (socklen_t *)&addrlen)) < 0) // accepting the client connection with creation/return of a new socket for the established connection to enable dedicated communication (active communication on a new socket) with the client
    {
      perror("accept");
      exit(EXIT_FAILURE);
    }

    int rc;                                       // return value from pthread_create to check if new thread is created successfukky                           */
    pthread_t thread_id;                          // thread's ID (just an integer, typedef unsigned long int) to indetify new thread
    sem_init(&client[client_flag],0,0);
    
    // int *new_socket = (int *)malloc(sizeof(int)); // for passing safely the integer socket to the thread
    // if (new_socket == NULL)
    // {
    //   fprintf(stderr, "Couldn't allocate memory for thread new socket argument.\n");
    //   exit(EXIT_FAILURE);
    // }
    // *new_socket = sock2;

    args = malloc(sizeof(struct client_arg) * 1);
    args->new_socket = sock2;
    args->header = header;
    args->my_flag = client_flag;
    // args->client_semaphor = client_semaphor;

    // create a new thread that will handle the communication with the newly accepted client
    rc = pthread_create(&thread_id, NULL, HandleClient, args);
    if (rc) // if rc is > 0 imply could not create new thread
    {
      printf("\n ERROR: return code from pthread_create is %d \n", rc);
      exit(EXIT_FAILURE);
    }
    client_flag++;
  }

  close(sock1);

  pthread_exit(NULL); // terminate the main thread
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
    if(param[0]=='.' && param[1]== '/'){
        return 1;
    }
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