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
			count_of_token=0;
			/* check for 1024 transaction length */
			if(strlen(buf)>1024)
			{
				fprintf(stderr,"\nTransaction length cannot be more than 1024\n");
				exit(0);
			}

			char* tempstr = strdup(buf);
    tempstr = trimTransaction(tempstr);
    if(tempstr == NULL)
    {
       return FALSE; 
    }
    //printf("\nTrimmed string : *****%s*****\n", tempstr);
    strcpy(buf,tempstr);
    free(tempstr);
			/*Tokenize the string*/	
			tok=strtok(buf,delimiter);
			while(tok!=NULL && count_of_token<=4)
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
				fprintf(stderr,"Transaction type should be 1 character only");
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

char *trimTransaction(char *str)
{
  if(str == NULL)
     return NULL;
  char *end;
  /*
    Trim leading space
  */
  while(isspace(*str)) str++;
  /*
    All Spaces
  */
  if(*str == 0)  
    return str;
  /*
    Trim trailing space
  */
  end = str + strlen(str) - 1;
  while(end > str && (isspace(*end) || (*end == '\n'))) end--;
  /*
    Write new null terminator
  */
  *(end+1) = 0;

  return str;
}

void sorting(My402List* mylist)
{

	My402ListElem* elem=NULL;
	My402ListElem* nextelem=NULL;
	void* temp;
	int num=0;
	for(elem=My402ListFirst(mylist);num<=My402ListLength(mylist)-1;elem=My402ListNext(mylist,elem))
	{
		for(nextelem=My402ListNext(mylist,elem);nextelem!=NULL;nextelem=My402ListNext(mylist,nextelem))
		{

			if(((transaction*)elem->obj)->time > ((transaction*)nextelem->obj)->time)
			{

				temp=elem->obj;
				elem->obj=nextelem->obj;
				nextelem->obj=temp;
			}

		}
	}

}




int numberOfDigits(long int number)
{
    int digits = 0;
    while (number) {
        number /= 10;
        digits++;
    }
    return digits;
}



void printlist(My402List *mylist)
{
	My402ListElem* elem = NULL;
	printf("+-----------------+--------------------------+----------------+----------------+");
    	printf("\n|       Date      | Description              |         Amount |        Balance |");
    	printf("\n+-----------------+--------------------------+----------------+----------------+\n");  
	long int sum =0;    	
	for(elem = My402ListFirst(mylist); elem != NULL; elem = My402ListNext(mylist,elem))
	{
		time_t transtime = ((transaction*)elem->obj)->time;
		char* time = ctime(&transtime);
		char subbuff[12];
		memcpy( subbuff, &time[0], 10);
		subbuff[10] = '\0';
	
		char* temp = ((transaction*)elem->obj)->description;
		char desc[25] = {'\0'};
		
		if(strlen(temp)>25)
		{
			memcpy( desc, &temp[0], 25);
			desc[24]='\0';			
		}
		else
		{
			int j=0;
			for(j=0; j<strlen(temp)-1;j++)
			desc[j] = temp[j];
			desc[strlen(temp)]='\0';
		}
		printf("| ");
		printf("%s",subbuff);
		printf(" ");
		printf("%c%c%c%c |",time[20],time[21],time[22],time[23]);
		printf(" %-25s| ",desc);

		char type = ((transaction*)elem->obj)->type;
		long int amount = ((transaction*)elem->obj)->amount;
		int j=0;

		if(type=='+')
		{
			sum =sum+amount;
						
			if(numberOfDigits(amount)<=5)
			{				
				printf("%10ld", amount/100);
				printf(".");
				if(amount%100 <10)
				{
					printf("0");
				}
				printf("%ld", amount%100);
				printf("  ");
				
			}
			else if (numberOfDigits(amount)>=6 && numberOfDigits(amount)<=8)
			{
				for(j=11; j> numberOfDigits(amount);j--)
				{	
					printf(" ");
				}
				printf("%ld",amount/100000);
				amount = amount%100000;
				printf(",");
				for(j=5;j>numberOfDigits(amount);j--)
				{
					printf("0");
				}
				if(amount/100 !=0)
				{
					printf("%ld", amount/100);
				}
				printf(".");
				if(amount%100 <10)
				{
					printf("0");
				}
				printf("%ld", amount%100);
				printf("  ");
				
			}
			else
			{
				printf(" ?,???,???.??  ");
			}			
		}
		else
		{			
			sum = sum-amount;			
			if(numberOfDigits(amount)<=5)
			{				
				printf("(");			
				printf("%9ld", amount/100);
				printf(".");
				if(amount%100 <10)
				{
					printf("0");
				}
				printf("%ld", amount%100);
				printf(") ");
			}
			else if (numberOfDigits(amount)>=6 && numberOfDigits(amount)<=8)
			{
				printf("(");				
				for(j=10; j> numberOfDigits(amount);j--)
				{	
					printf(" ");
				}
				printf("%ld",amount/100000);
				amount = amount%100000;
				printf(",");				
				for(j=5;j>numberOfDigits(amount);j--)
				{
					printf("0");
				}
				if(amount/100 !=0)
				{
					printf("%ld", amount/100);
				}
				printf(".");
				if(amount%100 <10)
				{
					printf("0");
				}
				printf("%ld", amount%100);
				printf(") ");
				
			}
			else
			{
				printf("(?,???,???.??");
				printf(") ");
			}
		}
		printf("| ");
		long int itemp = sum;
		if(itemp>=0)
		{
			if(numberOfDigits(itemp)<=5)
			{				
				printf("%10ld", itemp/100);
				printf(".");
				if(itemp%100 <10)
				{
					printf("0");
				}
				printf("%ld", itemp%100);
				printf("  ");
				
			}
			else if (numberOfDigits(itemp)>=6 && numberOfDigits(itemp)<=8)
			{
				for(j=11; j> numberOfDigits(itemp);j--)
				{	
					printf(" ");
				}
				printf("%ld",itemp/100000);
				itemp = itemp%100000;
				printf(",");				
				for(j=5;j>numberOfDigits(itemp);j--)
				{
					printf("0");
				}
				if(itemp/100!=0)
				{
					printf("%ld", itemp/100);
				}
				printf(".");
				if(itemp%100 <10)
				{
					printf("0");
				}
				printf("%ld", itemp%100);
				printf("  ");
				
			}
			else
			{
				printf(" ?,???,???.??  ");
			}			
		}
		else
		{						
			itemp = abs(itemp);
			if(numberOfDigits(itemp)<=5)
			{				
				printf("(");
				printf("%9ld", itemp/100);
				printf(".");
				if(itemp%100 <10)
				{
					printf("0");
				}
				printf("%ld", itemp%100);
				printf(") ");
			}
			else if (numberOfDigits(itemp)>=6 && numberOfDigits(itemp)<=8)
			{
				printf("(");				
				for(j=10; j> numberOfDigits(itemp);j--)
				{	
					printf(" ");
				}
				printf("%ld",itemp/100000);
				itemp = itemp%100000;
				printf(",");				
				for(j=5;j>numberOfDigits(itemp);j--)
				{
					printf("0");
				}
				if(itemp/100!=0)
				{
					printf("%ld", itemp/100);
				}
				printf(".");
				if(itemp%100 <10)
				{
					printf("0");
				}
				printf("%ld", itemp%100);
				printf(") ");				
			}
			else
			{
				printf("(?,???,???.??");
				printf(") ");
			}
		}

		printf("|\n");
	}
	printf("+-----------------+--------------------------+----------------+----------------+\n"); 
}




int main(int argc,char** argv)
{

	fprintf(stderr,"START");
	int readresult=0;
	My402List mylist;
	printf("b4 init success");	
	My402ListInit(&mylist);	
	printf("init success\n");
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
//		PrintList(&mylist);
		sorting(&mylist);
		printlist(&mylist);		
	}	
	return(0);
}
