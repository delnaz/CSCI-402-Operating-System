#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include"my402list.h"
#include"cs402.h"
#include<ctype.h>
FILE *file=NULL;

typedef struct banktransaction
{
	char type;
	time_t time;
	unsigned long amount;
	char* description;
}transaction;



int read(My402List* mylist)
{

	char buf[1026];
	char* token[4];
	token[0]=NULL;
	token[1]=NULL;
	token[2]=NULL;
	token[3]=NULL;	

	char* tok=NULL;
	char* delimiter="\t";
	//int i=0;
	int count_of_token=0;
	/*read line by line from the file*/
	while(fgets(buf, sizeof(buf),file)!=NULL)
	{
			/* check for 1024 transaction length */
			if(strlen(buf)>1024)
			{
				fprintf(stderr,"\nTransaction length cannot be more than 1024\n");
				exit(0);
			}
			/*Tokenize the string*/	
			tok=strtok(buf,delimiter);
			while(tok!=NULL)
			{
				token[count_of_token]=strdup(tok);
				tok=strtok(NULL,delimiter);
				count_of_token++;
			}
			if(count_of_token!=4)
			{
				fprintf(stderr,"\nTransaction must have 4 fields\n");
				exit(0);
			}
			
			if(strlen(token[0])>1|| (strcmp(token[0],"+")&&strcmp(token[0],"-")))
			{
				fprintf(stderr,"Transaction type should be 1 character, + or - only");
				exit(0);
			}

			else if(!strcmp(token[0],"+"))
				{
					trans->type='+';
				
				}
				else
				{
					trans->type='-';
				}
			if(strlen(token[1])>10)
			{
				fprintf(stderr,"Transaction timestamp is bad");
				exit(0);
			}

			unsigned long transtime=(unsigned long)strtoul(token[1],NULL,0);
			if(!(0<=transtime && transtime<=(unsigned long)time(NULL)))
			{
				fprintf(stderr,"Timestamp should be between 0 and current time");
				exit(0);
			}
			trans->time=(time_t)transtime;

			char* dollar;
			int cent='.';
			dollar=strchr(token[2],cent);
			*dollar='\0';
			dollar=dollar+1;
			
			if(strlen(token[2])>7||strlen(dollar)!=2)
			{
				fprintf(stderr,"Amount should have 7 digits before decimal and 2 digits after decimal");
				exit(0);
			}
			unsigned long transamount=(unsigned long)(strtoul(token[2],NULL,0)*100)+(unsigned long) (strtoul(dollar,NULL,0));

			trans->amount=transamount;

			if(strlen(token[3])==0)
			{
				fprintf(stderr,"Transaction description cannot be empty");
				exit(0);
			}
			trans->description=token[3];

			My402ListAppend(mylist,(void*) trans);

		}

		
	
return TRUE;
		
}



int main(int argc,char* argv[])
{


//	transaction* trans=(transaction*)malloc(sizeof(transaction));
	int readresult=0;
	My402List mylist;	
	My402ListInit(&mylist);	
	file=fopen("test.tfile","r");
	if(file==NULL)
	{
		fprintf(stderr,"\n Cannot open the file\n");
		exit(0);
	}
	else
	{
		readresult=read(&mylist);
		
		if(readresult==FALSE)
		{
			fprintf(stderr,"\n Cannot read the file\n");
			exit(0);
		}
		//sorting(&mylist);
		
	}	
	return(0);
}
