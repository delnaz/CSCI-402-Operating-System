#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include"th.h"


int main(int argc,char *argv[])
{

pthread_t thread1,thread2;

//char *msg1="Hello";
//char *msg2="World";

//printf("%s\n",msg1);
//printf("%s\n",msg2);

int ret1,ret2;
ret1=pthread_create(&thread1,NULL,cal1,NULL);
ret2=pthread_create(&thread2,NULL,cal2,NULL);

printf("This is\n");

pthread_join(thread1,NULL);
pthread_join(thread2,NULL);


printf("ret1=%d\n",ret1);
printf("ret2=%d\n",ret2);

//pthread_cancel(thread1);
//pthread_cancel(thread2);
return 0;

}

