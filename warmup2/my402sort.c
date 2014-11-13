#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include"my402list.h"
#include"cs402.h"
#include<ctype.h>
#include <errno.h>
#include <sys/stat.h>
FILE *file=NULL;
char* removespace(char*);
typedef struct banktransaction
{
	char type;
	time_t time;
	unsigned long amount;
	char* description;
}transaction;


void PrintList(My402List* mylist)
{

	My402ListElem* elem=NULL;
	fprintf(stdout,"+-----------------+--------------------------+----------------+----------------+\n");
	fprintf(stdout,"|       Date      | Description              |         Amount |        Balance |\n");
	fprintf(stdout,"+-----------------+--------------------------+----------------+----------------+\n");
	
	long int balanceMain = 0;
	for(elem=My402ListFirst(mylist);elem!=NULL;elem=My402ListNext(mylist,elem))
	{

		time_t time=((transaction*)elem->obj)->time;
		
		char transtime[26];
		strncpy(transtime, ctime(&time), sizeof(transtime));
		char tt[16];
		int i = 0; int j=0;
		for(i=0; i<15; i++){
			if(i > 10){ j = i+9; }
			tt[i] = transtime[j];
			j++;
		}
		tt[15] = '\0';
		fprintf(stdout,"| %s | ",tt);

		char desc[25];
		if(strlen(desc)>25)
		{
			strncpy(desc,((transaction*)elem->obj)->description,24);

		}
		else
		{
			strncpy(desc,((transaction*)elem->obj)->description,24);
		}
		i =0 ;
		while(desc[i] != '\0')
		{
			i++;
			if(i>24) break;
			
		}
		for(j = i; j<24; j++) {
			desc[j] = ' ';
		}
		desc[24]='\0';
		fprintf(stdout,"%s | ",desc);

		char type=((transaction*)elem->obj)->type;
		long int amount=((transaction*)elem->obj)->amount;
		char amt[15];
		if(type == '+'){
			if (amount >= 1000000000)
				fprintf(stdout," \?,\?\?\?,\?\?\?.\?\? | ");
			else {
				amt[0] = ' '; amt[13] = ' ';
				for(i = 12; i > 0; i--) {
					if(i == 10){
						amt[i]='.'; continue;
					}
					if(i == 6 || i == 2){
						amt[i]=','; continue;
					}
					amt[i] = (char)(((int)'0')+(amount%10));
					amount= amount/10;
					if(amount == 0) {i--;break;}
				}
				if(i == 10){
					amt[i]='.'; i--;
					amt[i]='0'; i--;
				}
				while(i>0) {
					amt[i] = ' '; 
					i--;
				}
				amt[14]='\0';
				fprintf(stdout,"%s | ",amt);
			}
		} else {
			if (amount >= 1000000000)
				fprintf(stdout,"(\?,\?\?\?,\?\?\?.\?\?) |");
			else {
				amt[0] = '('; amt[13] = ')';
				for(i = 12; i > 0; i--) {
					if(i == 10){
						amt[i]='.'; continue;
					}
					if(i == 6 || i == 2){
						amt[i]=','; continue;
					}
					amt[i] = (char)(((int)'0')+(amount%10));
					amount= amount/10;
					if(amount == 0) {i--;break;}
				}
				if(i == 10){
					amt[i]='.'; i--;
					amt[i]='0'; i--;
				}
				while(i>0) {
					amt[i] = ' '; 
					i--;
				}
				amt[14]='\0';
				fprintf(stdout,"%s | ",amt);
			}
		}

		amount=((transaction*)elem->obj)->amount;
		char balance[15];
		if(type == '+')
			balanceMain = balanceMain + amount;
		else
			balanceMain = balanceMain - amount;
		long int bal = balanceMain;
		if(bal >= 0 ){
			 
			if (bal >= 1000000000)
				fprintf(stdout," \?,\?\?\?,\?\?\?.\?\? |\n");
			else {
				balance[0] = ' '; balance[13] = ' ';
				for(i = 12; i > 0; i--) {
					if(i == 10){
						balance[i]='.'; continue;
					}
					if(i == 6 || i == 2){
						balance[i]=','; continue;
					}
					balance[i] = (char)(((int)'0')+(bal%10));
					bal= bal/10;
					if(bal == 0) {i--;break;}
				}
				if(i == 10){
					balance[i]='.'; i--;
					balance[i]='0'; i--;
				}
				while(i>0) {
					balance[i] = ' '; 
					i--;
				}
				balance[14]='\0';
				fprintf(stdout,"%s |\n",balance);
			}
		} else {
			bal = bal * (-1);
			if (bal >= 1000000000)
				fprintf(stdout,"(\?,\?\?\?,\?\?\?.\?\?) |\n");
			else {
				balance[0] = '('; balance[13] = ')';
				for(i = 12; i > 0; i--) {
					if(i == 10){
						balance[i]='.'; continue;
					}
					if(i == 6 || i == 2){
						balance[i]=','; continue;
					}
					balance[i] = (char)(((int)'0')+(bal%10));
					bal= bal/10;
					if(bal == 0) {i--;break;}
				}
				if(i == 10){
					balance[i]='.'; i--;
					balance[i]='0'; i--;
				}
				while(i>0) {
					balance[i] = ' '; 
					i--;
				}
				balance[14]='\0';
				fprintf(stdout,"%s |\n",balance);
			}
		}
		
		
		
	}
	fprintf(stdout,"+-----------------+--------------------------+----------------+----------------+\n");
}


int read(My402List* mylist)
{

	char buf[1026];
	char* token[4];
	/*token[0]=NULL;
	token[1]=NULL;
	token[2]=NULL;
	token[3]=NULL;*/	
	char* tok=NULL;
	char* delimiter="\t";
	int count_of_token=0;
	/*read line by line from the file*/
	while(fgets(buf, sizeof(buf),file)!=NULL)
	{
		if(strcmp(buf,"\n")!=0)
	{		
	token[0]=NULL;
	token[1]=NULL;
	token[2]=NULL;
	token[3]=NULL;
				
	transaction* trans=(transaction*)malloc(sizeof(transaction));
			count_of_token=0;
			/* check for 1024 transaction length */
			if(strlen(buf)>1024)
			{
				fprintf(stderr,"\nTransaction length cannot be more than 1024\n");
				exit(0);
			}
	
			char* tempstr = strdup(buf);
    			tempstr = removespace(tempstr);
    			if(tempstr == NULL)
    			{
       				return FALSE; 
    			}
    			strcpy(buf,tempstr);
    			free(tempstr);
			/*Tokenize the string*/	
			char* temp=strdup(buf);
			tok=strtok(temp,delimiter);
			while(tok!=NULL && count_of_token<=4)
			{
				token[count_of_token]=strdup(tok);
				tok=strtok(NULL,delimiter);
				count_of_token++;
			}
			free(temp);
			
	
			if(count_of_token!=4)
			{
				fprintf(stderr,"\nTransaction must have 4 fields\n");
				exit(0);
			}
			
			else if(strlen(token[0])>1|| (strcmp(token[0],"+")&&strcmp(token[0],"-")))
			{
				fprintf(stderr,"Transaction type should be 1 character or + or - only");
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
	
			trans->description=strdup(token[3]);

			My402ListAppend(mylist,(void*) trans);

		}
	}	
	
return TRUE;
		
}

char *removespace(char *str)
{
  if(str == NULL)
     return NULL;
  char *end;
  
  while(isspace(*str)) str++;
  
  if(*str == 0)  
    return str;
  
  end = str + strlen(str) - 1;
  while(end > str && (isspace(*end) || (*end == '\n'))) end--;
  
  *(end+1) = 0;

  return str;
}



void sorting(My402List* mylist)
{

	My402ListElem* elem=NULL;
	My402ListElem* nextelem=NULL;
	void* temp;
	for(elem=My402ListFirst(mylist);My402ListNext(mylist,elem)!=NULL;elem=My402ListNext(mylist,elem))
	{
		for(nextelem=My402ListNext(mylist,elem);nextelem!=NULL;nextelem=My402ListNext(mylist,nextelem))
		{

			if(((transaction*)elem->obj)->time > ((transaction*)nextelem->obj)->time)
			{

				temp=elem->obj;
				elem->obj=nextelem->obj;
				nextelem->obj=temp;
			}
			else if(((transaction*)elem->obj)->time == ((transaction*)nextelem->obj)->time)
			{
				fprintf(stderr,"Duplicate timestamp values");
			}
		}
	}

}










int main(int argc,char** argv)
{
	struct stat s;
	if(argc<2 || argc>3 )
	{
		fprintf(stderr,"Invalid number of command line arguments.\n");
		exit(0);
	}
	else if(argc == 2 && !strcmp(argv[1], "sort"))
	{
		file=stdin;
	}
	else if(strcmp(argv[1], "sort"))
	{

		fprintf(stderr,"Invalid command line argument, expected sort as second argument.\n");
		exit(0);

	}
	else if(argc==3)
	{
  		file=fopen(argv[2],"r");
		if( stat(argv[2],&s)==0)
			if( s.st_mode & S_IFDIR )
			{
			    fprintf(stderr,"Input argument is a Directory\n");
			    return(0);
			}
	}
	
	int readresult=0;
	My402List mylist;	
	My402ListInit(&mylist);	
	if(file==NULL)
	{
		if(argc == 3) 
			fprintf(stderr,"%s\n", strerror(errno));
		else
			fprintf(stderr,"%s\n", strerror(errno));
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

		sorting(&mylist);
		PrintList(&mylist);		
	}	
	return(0);
}
