#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#define KEYDOWN 1
#define KEYUP 0

int main(int argc, char *argv[])
{
    int status = 0;
    int fd;
    int ret;
    int data;
    struct pollfd fds;
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


    fds.fd = fd;
    fds.events = POLLIN;
    while(1){
        ret = poll(&fds, 1, 1000);
        if(ret){
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
        }else if(ret == 0){
            printf("time_out\r\n");
        }else if(ret <0){
            printf("error\r\n");
        }
    }

    close(fd);
    return 0;
}
