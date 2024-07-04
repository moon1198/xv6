#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{

    int p[2];
    pipe(p);
    int pid = fork();
    if (pid > 0) {
        //parents
        
        char buf[1] = "0";
        write(p[1], buf, 1);
        close(p[1]);
        
        read(p[0], buf, 1);
        close(p[0]);
        printf("%d: received pong\n", getpid());
        wait(&pid);
        exit(0);
    } else {
        //son
        char buf[1];
        read(p[0], buf, 1);
        close(p[0]);
        printf("%d: received ping\n", getpid());

        write(p[1], buf, 1);
        close(p[1]);
        exit(0);
    }
}
