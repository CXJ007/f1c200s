#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char *argv[])
{

    int fd;
    char *filename;
    char data;
    if(argc != 2){
        printf("err\r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR);
    if(fd < 0){
        printf("fail to open file\r\n");
        return -1;
    }
    while(1){
        sleep(1);
        read(fd,&data,sizeof(data));
        printf("mpu6050 ID 0x%x\r\n",data);
    }
    close(fd);

    return 0;
}