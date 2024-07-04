#include "kernel/types.h"
#include "user/user.h"

void primes(int fd1[], int fd2[]) {
    //printf("%d, %d, %d, %d\n", fd1[0], fd1[1], fd2[0], fd2[1]);
    close(fd1[1]);
    int num = 0;

    int ret = read(fd1[0], &num, sizeof(num));
    if (ret == 0) {
        //递归边界，父进程此向的输入端口已关闭； 
        close(fd1[0]);
        close(fd2[1]);
        close(fd2[0]);
        exit(0);
    }
    printf("prime %d\n", num);
    ret = fork();
    if (ret > 0) {
        //parent
        close(fd2[0]);
        int tmp = 0;
        while (read(fd1[0], &tmp, sizeof(num))) {
            if (tmp % num != 0) {
                write(fd2[1], &tmp, sizeof(num));
            }
        }
        close(fd1[0]);
        close(fd2[1]);
        wait(&ret);
        exit(0);
    } else {
        //son
        //和下一子进程进行通信的端口
        int fd[2];
        ret = pipe(fd);
        close(fd2[1]);
        close(fd1[0]);
        //进入下一层fork前，关闭所有不必要的pipe

        primes(fd2, fd);
        //exit(0);
    }
}

int
main(int argc, char *argv[])
{
    int fd1[2]; int fd2[2];
    pipe(fd1);
    pipe(fd2);

    for (int i = 2; i <= 35; ++ i) {
        write(fd1[1], &i, sizeof(i));
    }
    //此处关闭为返回递归最底层阻塞read的关键
    close(fd1[1]);
    primes(fd1, fd2);
    exit(0);
}
