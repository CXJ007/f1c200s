#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define KEYDOWN 1
#define KEYUP 0


int fd;
int data;

static void sigio_function(int signum)
{
    int ret;
    ret = read(fd, &data, sizeof(data));
        if(ret < 0){
            printf("read_err\r\n");
        }else{
            if(data == KEYDOWN){
                printf("KEYDOWN   %d\r\n",data);
            }else if(data == KEYUP){
                printf("KEYUP   %d\r\n",data);
            }
        }
} 



int main(int argc, char *argv[])
{
    int ret;
    char *filename = argv[1];

    if(argc != 2){
        printf("argv_err\r\n");
        return -1;
    }
    fd = open(filename, O_RDWR);
    if(fd < 0){
        printf("fail to open file\r\n");
        return -1;
    }
    signal(SIGIO, sigio_function);
    fcntl(fd, F_SETOWN, getpid());
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|FASYNC);

    while(1){
        sleep(1);
    }

    close(fd);
    return 0;
}
