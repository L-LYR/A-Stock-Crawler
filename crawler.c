#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bits/sched.h>

// load the stock code number
const int sh[] = {
#include "./external/sh"
};

const int sz[] = {
#include "./external/sz"
};

#define IPSTR "112.90.6.21" // hq.sinajs.cn
#define PORT 80
#define BUFSIZE 4096
#define MAX_NUMBER_NUM 100
#define MAX_FILE_NUM 100
#define MAX_THREAD_NUM 37 //(stockNum[0] + MAX_NUMBER_NUM) / MAX_NUMBER_NUM + (stockNum[1] + MAX_NUMBER_NUM) / MAX_NUMBER_NUM

const int *stockCode[2] = {sh, sz};
const char *stockCodeHead[2] = {"sh", "sz"};
const int stockNum[2] = {sizeof(sh) / sizeof(*sh), sizeof(sz) / sizeof(*sz)};
const int stockCodeLen = 8;
const int curPriceLoc = 3;
int threadInfoNum[MAX_THREAD_NUM]; // for processing data
int count;                         // for threads and requests

char get_request[MAX_THREAD_NUM][BUFSIZE];

FILE *data = NULL;
int FileNum = 0;
const char RootName[16] = "./data/";
char FileName[64] = "./data/";
char FileNumStr[16] = "0";
struct tm *ptime;

pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

// preprocess the get-request and save
void preprocess_request(int target)
{
    int i, j;
    for (i = 0; i < stockNum[target]; i += MAX_NUMBER_NUM, ++count)
    {
        char temp[16], connectNumber[BUFSIZE];
        memset(connectNumber, '\0', BUFSIZE);
        sprintf(get_request[count], "GET /list=");
        for (j = 0; j < MAX_NUMBER_NUM && i + j < stockNum[target]; ++j)
        {
            strcat(connectNumber, stockCodeHead[target]);
            sprintf(temp, "%06d,", stockCode[target][i + j]);
            strcat(connectNumber, temp);
        }
        threadInfoNum[count] = j;
        connectNumber[strlen(connectNumber) - 1] = '\0';
        strcat(get_request[count], connectNumber);
        strcat(get_request[count],
               " HTTP/1.1\r\nHost: hq.sinajs.cn\r\nContent-Type: application/x-www-form-urlencoded; charset=utf-8\r\nContent-Length: 0\r\n\r\n");
        //printf("%s\n", get_request[count]);
    }
}

// create socket
void connect_server(int *sockfd, struct sockaddr_in *ser_vaddr)
{
    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "socket error!\n");
        exit(0);
    };

    bzero(ser_vaddr, sizeof(*ser_vaddr));
    ser_vaddr->sin_family = AF_INET;
    ser_vaddr->sin_port = htons(PORT);                        // PORT = 80
    if (inet_pton(AF_INET, IPSTR, &ser_vaddr->sin_addr) <= 0) //IPSTR = "221.204.241.179" hq.sinajs.cn
    {
        fprintf(stderr, "inet_pton error!\n");
        exit(0);
    };

    if (connect(*sockfd, (struct sockaddr *)ser_vaddr, sizeof(*ser_vaddr)) < 0)
    {
        fprintf(stderr, "connect error!\n");
        exit(0);
    }
    // printf("Connect successfully!\n");
}

void set_time(void)
{
    time_t setTime;
    time(&setTime);
    ptime = gmtime(&setTime);
}

void open_new_file(void)
{
    strcpy(FileName, RootName);
    sprintf(FileNumStr, "%02d-%02d-%02d", ptime->tm_hour + 8, ptime->tm_min, ptime->tm_sec);
    ++FileNum;
    strcat(FileName, FileNumStr);
    data = fopen(FileName, "w");
}

void *thread_get(void *num)
{
    const int id = *(int *)num;
    // printf("%d thread is called.\n", id);
    int sockfd;
    struct sockaddr_in ser_vaddr;
    connect_server(&sockfd, &ser_vaddr);

    int ret = write(sockfd, get_request[id], strlen(get_request[id]));
    if (ret < 0)
    {
        printf("fail to send, error code: %d, error info: '%s'\n", errno, strerror(errno));
        exit(0);
    }

    usleep(200000); //wait for data

    char buf[BUFSIZE * 10];
    memset(buf, 0, BUFSIZE * 10);

    ret = read(sockfd, buf, BUFSIZE * 10 - 1);
    if (ret < 0)
    {
        close(sockfd);
        fprintf(stderr, "server close!\n");
        exit(0);
    }

    char *var = strstr(buf, "var");
    // str_process_write(var); // still have bugs !!!

    pthread_mutex_lock(&output_mutex);
    fprintf(data, "%s\n", var);
    pthread_mutex_unlock(&output_mutex);

    close(sockfd);
    // printf("%d thread exit.\n", id);
    return NULL;
}

void set_time_interval(int i, struct itimerval *tv)
{
    struct itimerval itv;
    itv.it_interval.tv_sec = i;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = 1;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, tv);
}

// debug
void pth_isalive(pthread_t tid)
{
    int pthread_status;
    pthread_status = pthread_kill(tid, 0);

    if (pthread_status == ESRCH)
        printf("%x thread has exited.\n", (unsigned int)tid);
    else if (pthread_status == EINVAL)
        printf("Illegal signal/n");
    else
        printf("%x thread is still alive\n", (unsigned int)tid);
}

void crawler(int no)
{
    set_time();

    open_new_file();

    pthread_t pids[MAX_THREAD_NUM];
    int i;
    for (i = 0; i < MAX_THREAD_NUM; ++i)
    {
        pthread_create(&pids[i], NULL, thread_get, (void *)(&i));
        usleep(10000); //gap 0.1s
    }
    usleep(10000);                                //wait for the last thread write data.
    pthread_join(pids[MAX_THREAD_NUM - 1], NULL); //all threads have exited.
    fclose(data);
}

int main(void)
{
    preprocess_request(0);
    preprocess_request(1);
    struct itimerval tv;
    signal(SIGALRM, crawler);
    set_time_interval(1, &tv);
    while (1)
        sleep(1);
    return 0;
}