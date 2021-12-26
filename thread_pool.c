#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>

#define N 1000
#define true 1
#define producer_num 3
#define consumer_num 1
#define sleeptime 10

typedef int semaphore;
typedef int item;
item buff[N] = {0};
int in = 0;
int out = 0;
int procount = 0;
semaphore mutex = 1, empty = N, full = 0, proCmutex = 1;

void *producer(void *a)
{
    while(true)
    {
        while(proCmutex <= 0);
        proCmutex--;
        procount++;
        printf("生产一个产品ID%d, 缓冲区位置%d\n", procount,in);
        proCmutex++;
        while (empty <= 0)
        {
            /* code */
            printf("缓冲区已满!\n");

        }
        empty--;
        while(mutex <= 0);
        mutex--;
        buff[in] = procount;
        in = (in + 1) % N;
        mutex++;
        full++;
        sleep(sleeptime);
    }

}

void *consumer(void *b)
{
    while(true)
    {
        while(full <= 0)
        {
            printf("缓冲区已满!\n");
        }
        full--;

        while(mutex <= 0);
        mutex--;

        int nextc = buff[out];
        buff[out] = 0;

        out = (out + 1) % N;
        mutex++;
        empty++;

        printf("消费一个产品ID %d, 缓冲区位置%d\n", nextc, out);
        sleep(sleeptime);
    }

}

int main()
{
    pthread_t thread_pool[producer_num + consumer_num];
    int i;
    for(i = 0; i < producer_num; i++)
    {
        pthread_t temp;
        if(pthread_create(&temp, NULL, producer, NULL) == -1)
        {
            printf("can not create pthread\n");
            exit(1);
        }
        thread_pool[i] = temp;
    }
    for(i = 0; i < consumer_num; i++)
    {
        pthread_t temp;
        if(pthread_create(&temp, NULL, producer, NULL) == -1)
        {
            printf("can not create pthread\n");
            exit(1);
        }
        thread_pool[i] = temp;
    }

    void *result;

    for(i = 0; i < producer_num + consumer_num; i++)
    {
        if(pthread_join(thread_pool[i],NULL) == -1)
        {
            printf("fail to recollect!\n");
            exit(1);
        }
    }

    return 0;
}