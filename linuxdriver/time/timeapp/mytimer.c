#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define CLOSE_CMD 		(_IO(0XEF, 0x1))	/* 关闭定时器 */
#define OPEN_CMD		(_IO(0XEF, 0x2))	/* 打开定时器 */
#define SETPERIOD_CMD	(_IO(0XEF, 0x3))	/* 设置定时器周期命令 */

int main(int argc, char *argv[])
{
    int fd,arg,cmd;
    char *filename;

    if(argc != 2){
        printf("error_usage\r\n");
        return -1;
    }
    filename = argv[1];
    fd = open(filename, O_RDWR);
    if(fd < 0){
        printf("Can't open file %s\r\n", filename);
		return -1;
    }

    while (1) {
        printf("Input CMD:");
        scanf("%d", &cmd);
        getchar();
        if(cmd == 1)				
            cmd = CLOSE_CMD;
        else if(cmd == 2)
            cmd = OPEN_CMD;			
        else if(cmd == 3) {
            cmd = SETPERIOD_CMD;	
            printf("Input Timer Period:");
            scanf("%d", &arg);
            getchar();		
        }
        ioctl(fd, cmd, arg);			
	}
	close(fd);

    return 0;
}