#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int shm_id = allocshm();
    int pid = fork();
    
    
    if (pid == 0) {
        chshm(shm_id,1);
        uint64* p = (uint64*)bindshm(shm_id);
        *p = 114514;
        // unbindshm();
        // if we unbind proc 0, k will be 0 at last,that is,there will be 2 "Senpai,daisuki!" instead of 3
    } else {
        printf("shm_id:%d\n",shm_id);
        printf("Have permission? %d\n",shmpm(shm_id));
        chshm(shm_id,1);
        printf("Now have permission? %d\n",shmpm(shm_id));
        uint64* q = (uint64*)bindshm(shm_id);
        printf("%d\n",*q);
        if (*q == 114514) printf("Senpai,daisuki!\n");
        int k = freeshm(shm_id);//k == -1
        if (k == -1) printf("Senpai,daisuki!\n");
        unbindshm();
        k = freeshm(shm_id);//k == -1 because of proc 0
        if (k == -1) printf("Senpai,daisuki!\n");
    }
    exit(0);
}