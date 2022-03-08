#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/mman.h>

#define LIGHT_BLUE "\033[1;34m" // imitate shell
#define LIGHT_GREEN "\033[1;32m"
#define NONE "\033[m"
#define clear() printf("\033[H\033[J")

#define normal 0 // no pipe
#define pipe1 1  // a | b
#define pipe2 2  // a | b | c

void get_param(char *param);                          // get paramaners, a string
int find(char *param);                                // find if the command exists
void cd(char *path);                                  // cd
void anal(char *param, int *num, char arr[100][256]); // analyze command line
void exec(int param_num, char para[100][256]);        // execute command
void print_dir();                                     // imitate shell format

int main(int argc, char *argv[])
{
    // for(int i = 0; i < argc; i++){
    //         param_arr[i] = expand_escapes_alloc(param_arr[i]);
    // }
    clear();
    while (1)
    {
        int param_num = 0;                 // e.g. ls -l = 2
        char *param = (char *)malloc(256); // store the command string user input
        char param_arr[100][256];          // formatted command string
        print_dir();                       // imitate shell
        printf(NONE "$ ");
        get_param(param); // get the input from the user, store in param
        if ((strcmp("exit", param) == 0))
        { // type exit to quit the program
            break;
        }
        anal(param, &param_num, param_arr);  // split the commend into a 2D array
        if (strcmp(param_arr[0], "cd") == 0) // cd to the place // TODO
        {
            cd(param_arr[1]);
            continue;
        }
        exec(param_num, param_arr); // execute the program
        free(param);
    }
    return 0;
}

void get_param(char *param)
{
    char ch;
    int i = 0;
    while (1)
    {
        ch = getchar();
        if (ch == '\n')
            break;
        if (ch == '\'')
            continue; // GET RID OF SPECIAL CHARACTERS
        if (ch == '\"')
            continue;
        param[i++] = ch;
    }
    param[i] = '\0';
    // printf("the string we get from gat_param:%s\n", param);
}

void anal(char *param, int *num, char arr[100][256])
{
    char *p = param;
    char *q = param;
    int number = 0;
    while (1)
    {
        if (p[0] == '\0')
            break;
        if (p[0] == ' ')
            p++;
        else
        {
            q = p;
            number = 0; // "l" "s" "\0"
            while (q[0] != '\0' && q[0] != ' ')
            {             // "-" "l" "\0"
                q++;      // "|" "\0"
                number++; // "d" "a" "t" "e" "\0"
            }
            strncpy(arr[*num], p, number + 1);
            arr[*num][number] = '\0';
            *num = *num + 1;
            p = q;
        }
    }
}

void exec(int param_num, char para[100][256])
{
    pid_t pid1; //        child 1
    pid_t pid2; //         /   |
    pid_t pid3; //  child 2   child 3
    int fd1[2];
    int fd2[2];
    char *arg[param_num + 1];

    int stat_val1;
    int stat_val2;
    int i, j, x = 0, y = 0; // util for split the arg123
    int flag = 0;           // how many "|" in total
    int how = 0;            // 0:normal, 1:a|b, 2:a|b|c
    int bg = 0;
    int position1 = 0; // flag for the first |
    int position2 = 0; // flag for the second |

    for (i = 0; i < param_num; i++)
    {
        arg[i] = (char *)para[i]; // store whatever user input into arg
    }
    arg[param_num] = NULL;

    // if(!find(arg1[0])) return;       // no such command

    for (i = 0; i < param_num; i++) // chechk how many | are there
    {
        if (strcmp(para[i], "|") == 0)
        {
            flag++;
        }
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
    }

    if (how == pipe2) // process command, split it into arg1 and arg2
    {
        for (i = 0; i < param_num; i++)
        {
            if (strcmp(para[i], "|") == 0)
            {
                if (position1 == 0)
                    position1 = i;
                else
                    position2 = i;
            }
        }
    }

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
                execvp(arg[0], arg);
                exit(0);
            }
            break;

        case 1:
            if (pid1 == 0)
            {
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

                // the process can run with these two commands given directly
                // char * arg1[3] = {"ls", "-l", NULL};
                // char * arg2[3] = {"grep", ".c", NULL};

                // printf("child 1 created!\n");
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
                        // printf("child 2 created!");
                        dup2(fd1[1], 1);
                        close(fd1[0]);
                        close(fd1[1]);
                        // printf("child 2 going to execute!\n");
                        if (execvp(arg1[0], arg1) < 0)
                        {
                            printf("cihld 2 error");
                            perror("Execvp failed from child");
                        }
                        // printf("child 2 successful");
                        exit(EXIT_FAILURE);
                    }
                    else // child 1
                    {
                        pid3 = fork();
                        if (pid3 == 0)
                        {
                            // printf("child 3 created!");
                            dup2(fd1[0], 0);
                            close(fd1[1]);
                            close(fd1[0]);
                            // printf("child 3 going to execute!\n");
                            if (execvp(arg2[0], arg2) < 0)
                            {
                                printf("cihld 3 error");
                                perror("Execvp failed from p");
                            }
                            // perror ("Execvp failed while child 2\n");
                            exit(EXIT_FAILURE);
                        }
                        else
                        {
                            close(fd1[0]);
                            close(fd1[1]);
                            wait(NULL);
                        }
                    }
                }
            }
            break;

        case 2:
            if (pid1 == 0)
            {
                // printf("%d\n",param_num);
                // printf("%d\n",position1);
                // printf("%d\n",position2);

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

                // 直接给这三个也是对的
                // char * arg1[3] = {"ls", "-l", NULL};
                // char * arg2[3] = {"grep", ".c", NULL};
                // char * arg3[3] = {"wc", "-c", NULL};

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
                        else
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
            break;
        default:
            break;
        }
    }
    if (bg)
        return;
    waitpid(pid1, &stat_val2, 0);
}

int find(char *param)
{
    DIR *dir;
    struct dirent *ptr;
    int ice = 0;
    dir = opendir("/bin");
    readdir(dir);
    while (ptr = readdir(dir))
    {
        if (strcmp(param, ptr->d_name) == 0)
            return 1;
    }
    closedir(dir);
    printf("Command '%s' not found\n", param);
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