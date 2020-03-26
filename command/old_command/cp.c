#include "stdio.h"
int main(void){
    printf("prog_no_arg from disk\n");
    while(1);
    return 0;
}


#include "stdio.h"
#include "syscall.h"
#include "string.h"
int main(int argc,char **argv){
    int arg_idx=0;
   // printf("start prog_arg\n");
    while(arg_idx<argc){
        printf("argv[%d] is %s \n",arg_idx,argv[arg_idx]);
        arg_idx++;
    }
    int pid=fork();
    if(pid){
        int delay=900000;
        while(delay--)
        printf("\n    I'm father prog, my pid:%d, I will show process list\n",getpid());
        ps();
        
    }
    else{
        char abs_path[512]={0};
        printf("\n    I'm child prog, my pid:%d, I will exec %s right now\n",getpid(),argv[1]);
        if(argv[1][0]!='/'){
            getcwd(abs_path,512);
            strcat(abs_path,"/");
            strcat(abs_path,argv[1]);
            execv(abs_path,argv);
        }
        else{
            printf("execv prog_arg\n");
            execv(argv[1],argv);
        }
    }
    while(1);
    return 0;
}