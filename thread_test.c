#define _GNU_SOURCE
#include<stdio.h>
#include<math.h>
#include<pthread.h>

cpu_set_t cpuset, cpuget;

double waste_time(long n)
{
    double res = 0;
    long i = 0;
    while(i < n * 20000000000)
    {
        i++;
        res += i * i;
    }
    return res;
}


void *thread_fun(void *paras)
{
    CPU_ZERO(&cpuset);
    CPU_SET(0,&cpuset);
    /*bind to cpu 0*/
    if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0)
    {
        perror("pthread_setaffinity_np failed!");
    }

    printf("res is %f\n", waste_time(5));
    pthread_exit(NULL);
}

int main()
{
    pthread_t test_thread;
    time_t starttime, endtime;
    starttime = time(NULL);
    if(pthread_create(&test_thread,NULL,thread_fun,NULL) != 0)
    {
        perror("pthread_create failed!");
    }
    pthread_join(test_thread, NULL);
    endtime = time(NULL);
    printf("wall clock time is %ld\n", endtime - starttime);
    return 0;
}