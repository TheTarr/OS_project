#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

typedef struct slist // 单链表
{
	int client_id; // 哪一个用户
    char message[1024]; // 用户输入的命令
    int remaining_time; // 这个命令还需要执行多久，系统命令是1，./looping要变
    struct slist *next;
}L;
 
//创建一个节点 
L *create_node(int client_id, char message[1024])
{
	//给每个节点分配结构体一样的空间大小 
	L *node = malloc(sizeof(L));
	if(NULL == node)
	{
		printf("malloc error!\n");
		return NULL ;
	}
	//由于结构体在未初始化的时候一样是脏数据，所以要清 
	memset(node,0,sizeof(L));
	//初始化第一个节点
	node->client_id = client_id;
    strncpy(node->message, message, strlen(message));
    char *p = "./looping"; // 检测命令是否为大循环
    if(strstr(message,p)){ // 如果是大循环，取出循环时间
        int len = strlen(message);
        int num_len = len - 10;
        char *pointer = message;
        char num_str[num_len];
        for(int i=0;i<10;i++){
            pointer++;
        }
        strncpy(num_str, pointer, num_len);
        node->remaining_time = atoi(num_str);
    }else{ // 如果是系统命令统一都是1
        node->remaining_time = 1;
    }
	//将节点的后继指针设置为NULL 
	node->next = NULL ;
}

//链表的尾插 
void tail_insert(L *pH , L *new)
{
	//获取当前的位置 
	L *p = pH ; 
	//如果当前位置的下一个节点不为空 
	while(NULL != p->next)
	{
		//移动到下一个节点 
		p = p->next ;
	}
	//如果跳出以上循环，所以已经到了NULL的这个位置
	//此时直接把新插入的节点赋值给NULL这个位置 
	p->next = new ;
}
 
//链表的头插 
void top_insert(L *pH , L *new)
{
	L *p = pH ;
	new->next = p->next ;
	p->next = new ;
}

//链表的遍历 
void Print_node(L *pH)
{
	//获取当前的位置 
	L *p = pH ;
	//获取第一个节点的位置 
	p = p->next ;
	//如果当前位置的下一个节点不为空 
	while(NULL != p->next)
	{
		//(1)打印节点的数据 
		printf("id:%d\n",p->client_id);
        printf("message=%s\n",p->message);
        printf("remaining_time=%d\n",p->remaining_time);
		//(2)移动到下一个节点,如果条件仍为真，则重复(1)，再(2) 
		p = p->next;
	}
	//如果当前位置的下一个节点为空，则打印数据
	//说明只有一个节点 
	printf("id:%d\n",p->client_id);
    printf("message=%s\n",p->message);
    printf("remaining_time=%d\n",p->remaining_time);
}
 
//删除链表中的节点 
int detele_list_node(L * pH , int id)
{
	//获取当前头节点的位置 
	L *p = pH ;
	L *prev = NULL;
	while(NULL != p->next)
	{
		//保存当前节点的前一个节点的指针 
		prev = p ;
		//然后让当前的指针继续往后移动 
		p = p->next ; 	
		//判断，找到了要删除的数据  
		if(p->client_id == id)
		{
			//两种情况，一种是普通节点，还有一种是尾节点
			if(p->next != NULL)  //普通节点的情况 
			{
				prev->next = p->next ;
				free(p);
			}
			else //尾节点的情况 
			{
				prev->next = NULL ; //将这个尾节点的上一个节点的指针域指向空 
				free(p); 
			}
			return 0  ;
		}
	}
	printf("没有要删除的节点\n");
	return -1 ;
}
 
int main(int argc , char **argv) 
{
	//创建第一个节点 
	int i ;
	L *header = create_node(0,"header"); 
	for(i = 1 ; i < 10 ; i++)
	{
		tail_insert(header,create_node(i,"./looping 12"));
	}
	Print_node(header);
	detele_list_node(header,5);
	putchar('\n');
	Print_node(header);
	return 0 ;
}