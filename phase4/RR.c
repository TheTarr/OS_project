#include<stdio.h>

int  queue[10],front=-1,rear=-1;
/*here queue variable use for
 enqueue , dqueue of process*/

void enqueue(int i) /*function use for enter value in queue*/
{
    if(rear==10)
        printf("overflow");
    rear++;
    queue[rear]=i;
    if(front==-1)
        front=0;

}
int dequeue()/*function use for enter value in queue*/
{
    if(front==-1)
        printf("underflow");
    int temp=queue[front];
    if(front==rear)
        front=rear=-1;
    else
        front++;
    return temp;
}
/* check whether process in queue or not*/
int isRunningQueue(int i)
{
    int k;
    for(k=front; k<=rear; k++)
    {
        if(queue[k]==i)
            return 1;
    }
    return 0;

}
int main()
{
    /*
    Here n=numner of process,sum_bt=total Brust time,tq=time quantum.
    atat=average turn arounnt time,awt=average waiting time,c=process name
    */
    int n,i,j,time=0,sum_bt=0,tq;
    float atat=0,awt=0;
    char process;
    printf("Enter no of processes:");
    scanf("%d",&n);

    char name[n*2];
    /*
    at=arival time, bt=brust time,rt=remain time, ft=completion time
    */
    int at[n],bt[n],rt[n],ft[n];
    int completed[n];//completed=1 means process complete otherwise not
    for(i=0,process='A'; i<n; i++,process++)
    {
        name[i]=process;
        printf("\nEnter the arrival time and burst time of process %c: ",name[i]);
        scanf("%d%d",&at[i],&bt[i]);
        rt[i]=bt[i];
        completed[i]=0;
        ft[i]=0;
        sum_bt+=bt[i];

    }
    printf("\nEnter the time quantum:");
    scanf("%d",&tq);
    printf("\n--------------------------------------------------------------------------------------------------------");

    /*sort*/
    int t;
    for(i=0; i<n-1; i++)
    {
        for(j=i+1; j<n; j++)
        {
            if(at[i] > at[j])
            {
                t=name[i];
                name[i]= name[j];
                name[j]=t;

                t=at[i];
                at[i]= at[j];
                at[j]=t;

                t=bt[i];
                bt[i]=bt[j];
                bt[j]=t;

                t=rt[i];
                rt[i]=rt[j];
                rt[j]=t;

            }
        }
    }

    if(at[0]!=0)
    {
        time=at[0];
        sum_bt=sum_bt+at[0];
    }
    enqueue(0);/*enter value in queue*/
    printf("\nProcess Gantt Chart: |");
    /* run until the total burst time reached*/
    for(time=at[0]; time<sum_bt;)
    {
        i=dequeue();/*dqueue value from queue*/

        if(rt[i]<=tq)/* for processes having remaining time with -
less than or  equal  to time quantum  */
        {
            time+=rt[i];
/*printf("\ntime %d ",time);*/
            rt[i]=0;/*rt=0 means remain time is finished*/
            completed[i]=1;
            printf(" %c |",name[i]);
            ft[i]=time;
            for(j=0; j<n; j++)/*enqueue the processes
                        which have come  while scheduling */
            {
                if(at[j]<=time && completed[j]!=1 && isRunningQueue(j)!=1)
                {
                    enqueue(j);

                }

            }
        }
        else               /*more than time quantum*/
        {

            time+=tq;
            rt[i]-=tq;
            printf(" %c |",name[i]);
            for(j=0; j<n; j++)  /*first enqueue the processes
                which have come while scheduling */
            {
                if(at[j]<=time && completed[j]!=1&& i!=j&& isRunningQueue(j)!=1)
                {
                    enqueue(j);/*access process when remaining
                     time with less than or  equal  to time quantum */
                }
            }
            enqueue(i);   /*then enqueue the uncompleted process*/
        }
    }
    printf("\n--------------------------------------------------------------------------------------------------------\n");
    printf("\n\nName\tArrival Time\tBurst Time\tComplication Time\tTurnAround Time\tWaiting Time");
    for(i=0; i<n; i++)
    {
        printf("\n%c\t\t%d\t\t%d\t\t%d\t\t     %d\t\t   %d",name[i],at[i],bt[i],ft[i],ft[i]-at[i],(ft[i]-at[i])-bt[i]);
        atat=atat+ft[i]-at[i];
        awt+=((ft[i]-at[i])-bt[i]);
    }
    printf("\nAverage TurnAround Time :->%f",atat/n);
    printf("\nAverage Waiting Time :->%f ",awt/n);
    printf("\n Thanks for visiting my site");
    return 0;
}



// 一个线程终止另一个线程示例代码
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>     // sleep() 函数
//线程执行的函数
void * thread_Fun(void * arg) {
    printf("新建线程开始执行\n");
    sleep(10);
}
int main()
{
    pthread_t myThread;
    void * mess;
    int value;
    int res;
    //创建 myThread 线程
    res = pthread_create(&myThread, NULL, thread_Fun, NULL);
    if (res != 0) {
        printf("线程创建失败\n");
        return 0;
    }
    sleep(1);
    //向 myThread 线程发送 Cancel 信号
    res = pthread_cancel(myThread);
    if (res != 0) {
        printf("终止 myThread 线程失败\n");
        return 0;
    }
    //获取已终止线程的返回值
    res = pthread_join(myThread, &mess);
    if (res != 0) {
        printf("等待线程失败\n");
        return 0;
    }
    //如果线程被强制终止，其返回值为 PTHREAD_CANCELED
    if (mess == PTHREAD_CANCELED) {
        printf("myThread 线程被强制终止\n");
    }
    else {
        printf("error\n");
    }
    return 0;
}

// 传输多参数示例代码

#include <stdio.h>
 2 #include <pthread.h>
 3 #include <stdlib.h>
 4 #include <string.h>
 5 
 6 typedef struct Student
 7 {
 8    int num;
 9    char name[10];
10 }info;
11 
12 void *message(void *arg)
13 {
14   info *p = (info*)arg;
15   printf("num:%d name:%s\n",p->num,p->name);
16 
17 }
18 
19 void *read_routine1(void *arg)
20 {    int *fd;
21     fd = (int*)arg;
22     // fd[0] = ((int *)arg)[0];
23     // fd[1] = ((int *)arg)[1];
24     printf("fd[0]:%d  fd[1]:%d\n",fd[0],fd[1]);
25 }
26 
27 
28 int main(int argc,char *argv[])
29 {    
30     info *st = (info*)malloc(sizeof(info));
31     st->num = 10;
32     strcpy(st->name,"xiaoming");
33     int fd[2];
34     fd[0] = 12;
35     fd[1] = 32;
36     pthread_t tid1,tid2;
37     /*创建两条线程，第一条传送的是一个结构体，第二条是数组*/
38     pthread_create(&tid2,NULL,message,(void*)st);
39     pthread_create(&tid1,NULL,read_routine1,(void*)fd);
40     while(1);
41     free(st);
42     return 0;
43 }