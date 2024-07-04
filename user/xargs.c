#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "user/user.h"

int 
main (int argc, char *argv[])
{
    //for (int i = 0; i < argc; ++ i) {
    //    printf("%s\n", argv[i]);
    //}
    //取n
    int n = MAXARG;
    int i = 1;
    if (strcmp(argv[1], "-n") == 0) {
        n = atoi(argv[2]);
        if (n <= 0) {
            printf("-n : too small\n");
            exit(1);
        }
        i = 3;
    }

    //取argv参数
    char args[MAXARG][32];
    int len = 0;
    for (; i < argc; ++ i) {
        strcpy(args[len ++], argv[i]);
    }
    
    //取标准输入里参数
    int idx = 0;
    char ch;
    while (read(0, &ch, 1)) {
        if (ch == '\0') {
            break;
        } else if (ch == ' ' || ch == '\n' || ch == '\t') {
            if (idx >= 0) {
                args[len ++][idx] = '\0';
                idx = 0;
                //printf("%s,read \n", args[i - 1]);
            }
        } else {
            args[len][idx ++] = ch;
        }
    }

    //for (int i = 0; i < len; ++ i) {
    //    printf("%s\n", args[i]);
    //}
    
    //取出参数并执行,共有len个参数，每次取出n个
    i = 1;
    char *new_argv[MAXARG];
    new_argv[0] = args[0];
    while (i < len) {
        int k = 0;
        for (k = 0; i < len && k < n; k ++) {
            //printf("%s, \n", args[i]);
            new_argv[k + 1] = args[i ++];
        }
        new_argv[k + 1] = 0;
        int pid = fork ();
        if (pid == 0) {
            //son
            exec(args[0], new_argv);
            exit(0);
        } else {
            //parent
            wait(&pid);
        }
    }
    exit(0);
}
