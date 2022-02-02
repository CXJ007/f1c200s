#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define KEYDOWN 1
#define KEYUP 0

int main(int argc, char *argv[])
{
    int status = 0;
    int fd;
    int ret;
    int data;
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
    while(1){
        ret = read(fd, &data, sizeof(data));
        if(ret < 0){
            printf("read_err\r\n");
        }else{
            if((data==KEYDOWN) && (status==0)){
                printf("KEYDOWN   %d\r\n",data);
                status = 1;
            }else if(data==KEYUP){
                status = 0;
            }
        }
    }

    close(fd);
    return 0;
}
