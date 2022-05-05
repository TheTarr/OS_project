#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

//test.c 编译后产生可执行文件test.exe或test.out，程序来源于C Primer Plus第十一章
int main(int argc,char *argv[]) {
    int count;
	int total = atoi(argv[1]);
    printf("The command line has %d arguments :\n",argc-1);
    for (count = total; count > 0; count--) {
        printf("%d\n", count);
		sleep(1);
    }
    return 0;
}

