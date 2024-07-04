#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"


void dfs(char *path, char *p, char *name) {
    int fd;
    struct dirent de;
    struct stat st;
    if((fd = open(path, O_RDONLY)) < 0){
      fprintf(2, "ls: cannot open %s\n", path);
      return;
    }

    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if (!(strcmp(de.name, ".") && strcmp(de.name, ".."))) {
            continue;
        }
        //printf("%s%s\n", path, de.name);
        if(de.inum == 0)
            continue;
        strcpy(p, de.name);
        int tmp_fd;
        if((tmp_fd = open(path, O_RDONLY)) < 0){
            fprintf(2, "ls: cannot open %s\n", path);
            return;
        }
        if(fstat(tmp_fd, &st) < 0){
            fprintf(2, "ls: cannot stat %s\n", path);
            close(tmp_fd);
            return;
        }
        close(tmp_fd);
        switch(st.type){
            case T_DEVICE:
            case T_FILE:
                if (!strcmp(de.name, name)) {
                    printf("%s\n", path);
                }
                break;
            case T_DIR:
                *(p + strlen(de.name)) = '/';
                *(p + strlen(de.name) + 1) = '\0';
                dfs(path, p + strlen(de.name) + 1, name);
                break;
        }
        *p = '\0';
    }
    close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc < 3){
      printf("too fewer argument\n");
      exit(0);
  }
  char name[DIRSIZ];
  char dir[DIRSIZ];
  strcpy(name, argv[2]);
  strcpy(dir, argv[1]);

  char path[512];
  char *p = path;
  strcpy(p, dir);
  p += strlen(dir);
  if (dir[strlen(dir) - 1] != '/') {
      *p ++ = '/';
  }
  dfs(path, p, name);

  exit(0);
}
