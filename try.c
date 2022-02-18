// 网上抄的代码

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#define CMD_LINE 1024
#define PIPE_MAX 16
#define ARG_MAX 10
typedef struct
{
    char *arg[ARG_MAX];
    char *in;
    char *out;
} cmd_t;
extern int parse_token(char *buf, cmd_t cmd[]);
extern int parse(char *buf, cmd_t *cmd);
extern int test_parse(cmd_t cmd[], int len);
int main(int argc, char *argv[])
{
    char buf[CMD_LINE];
    cmd_t cmd[PIPE_MAX + 1];
    int fd[PIPE_MAX][2];
    int j, i;
    int cmd_len, pipe_len;
    pid_t pid;
    while (1)
    {
        printf("my_shell#");
        fgets(buf, CMD_LINE, stdin);
        buf[strlen(buf) - 1] = '\0';
        cmd_len = parse_token(buf, cmd);
        pipe_len = cmd_len - 1;
        if (pipe_len > PIPE_MAX)
            continue;
        for (i = 0; i < pipe_len; ++i)
            pipe(fd[i]);
        for (i = 0; i < cmd_len; ++i)
            if ((pid = fork()) == 0)
                break;
        if (pid == 0)
        {
            if (pipe_len)
            {
                if (i == 0)
                {
                    close(fd[i][0]);
                    dup2(fd[i][1], 1);
                    close(fd[i][1]);
                    for (j = 1; j < pipe_len; ++j)
                        close(fd[j][0]),
                            close(fd[j][1]);
                }
                else if (i == pipe_len)
                {
                    close(fd[i - 1][1]);
                    dup2(fd[i - 1][0], 0);
                    close(fd[i - 1][0]);
                    for (j = 0; j < pipe_len - 1; ++j)
                        close(fd[j][0]),
                            close(fd[j][1]);
                }
                else
                {
                    dup2(fd[i - 1][0], 0);
                    close(fd[i][0]);
                    dup2(fd[i][1], 1);
                    close(fd[i][1]);
                    for (j = 0; j < pipe_len; ++j)
                    {
                        if ((j != i - 1) || (j != i))
                            close(fd[j][0]),
                                close(fd[j]
                                        [1]);
                    }
                }
            }
            if (cmd[i].in)
            {
                int fd = open(cmd[i].in, O_RDONLY);
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (cmd[i].out)
            {
                int fd =
                    open(cmd[i].out,
                         O_RDWR | O_CREAT | O_TRUNC, 0644);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
            execvp(cmd[i].arg[0], cmd[i].arg);
            fprintf(stderr, "Failed exec\n");
            exit(127);
        }
        /* parent */
        for (i = 0; i < pipe_len; ++i)
            close(fd[i][0]), close(fd[i][1]);
        for (i = 0; i < cmd_len; ++i)
            wait(NULL);
    }
    return 0;
}
int parse_token(char *buf, cmd_t cmd[])
{
    int n = 0;
#if 1
    char *save_p;
    char *p = strtok_r(buf, "|", &save_p);
    while (p != NULL)
    {
        parse(p, &cmd[n++]);
        p = strtok_r(NULL, "|", &save_p);
    }
#else
    cmd[n].arg[0] = "ls";
    cmd[n].arg[1] = "-l";
    cmd[n].arg[2] = NULL;
#endif
    return n;
}
int test_parse(cmd_t cmd[], int len)
{
    int i;
    for (i = 0; i < len; ++i)
    {
        printf("cmd[%d]:", i);
        int j = 0;
        while (cmd[i].arg[j])
            printf(" %s", cmd[i].arg[j++]);
        if (cmd[i].in)
            printf("\tin:%s", cmd[i].in);
        if (cmd[i].out)
            printf("\tout:%s", cmd[i].out);
        printf("\n");
    }
    return 0;
}
int parse(char *buf, cmd_t *cmd)
{
    int i = 0;
    cmd->in = NULL;
    cmd->out = NULL;
    char *p = strtok(buf, " ");
    while (p)
    {
        if (*p == '<')
        {
            if (*(p + 1))
                cmd->in = p + 1;
            else
                cmd->in = strtok(NULL, " ");
        }
        else if (*p == '>')
        {
            if (*(p + 1))
                cmd->out = p + 1;
            else
                cmd->out = strtok(NULL, " ");
        }
        else
            cmd->arg[i++] = p;
        p = strtok(NULL, " ");
    }
    cmd->arg[i] = NULL;
    return 0;
}