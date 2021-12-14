#include<stdio.h>

int main(int argc,char **argv)
{
    printf("the total argc is %d\n",argc-1);
    for(int i = 1; i < argc; i++)
        printf("the %dth arg is %s\n",i,argv[i]);
}
