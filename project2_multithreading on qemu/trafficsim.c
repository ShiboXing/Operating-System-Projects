#include <sys/mman.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <stdio.h>
#include <time.h>

struct car
{
	//id info
	int cid;
	//the next car
	struct car *next;
};

struct cs1550_sem
{
	//the count of cars
	int val;
	char dir;
	//the queue of cars
	struct car *head;
	struct car *end;
};

void down(struct cs1550_sem *sem)
{
	syscall(__NR_cs1550_down, sem);
}
void up(struct cs1550_sem *sem)
{
	syscall(__NR_cs1550_up, sem);
}

int main()
{

	struct cs1550_sem *sems=mmap(NULL, 3*sizeof(struct cs1550_sem), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	struct cs1550_sem *lSem=sems;
	struct cs1550_sem *rSem=sems+sizeof(struct cs1550_sem);

	lSem->val=0;
	lSem->dir='l';
	lSem->head=NULL;
	lSem->end=NULL;

	rSem->val=0;
	rSem->dir='r';
	rSem->head=NULL;
	rSem->end=NULL;
	
		
	//fork one producer from each direction
	if (fork()==0)
	{
		//use nano-second time stamp as the seed of rand() method, otherwise two producers would produce with the same sequence of random number
		time_t seed;
		time(&seed);
		
		while(1)
		{
			//increase randomness even more
			rand();
			rand();
			if(rand(&seed)%10<8){
				up(lSem);
			}
			else{
				printf("no more cars from left, sleeping for 20 secs\n");								sleep(20);
				sleep(20);
			}
		}
	}

	if (fork()==0)
	{	
		//use nano-second time stamp as the seed of rand() method, otherwise two producers would produce with the same sequence of random number
		time_t seed;
		time(&seed);
		
		while(1)
		{
			if(rand(&seed)%10<8){
				up(rSem);
			}
			else
			{
				printf("no more cars from right, sleeping for 20 secs\n"); 
				sleep(20);
			}
		}
	}
	
	//one consumer for each direction
	int right=0;
	while(1)
	{
		
		if(!right)
		{
			down(lSem);
		}
		else 
		{
			down(rSem);
		}	
			
		//passing through the construction zone costs 2 secs
		sleep(2);	

		//if the ten or more cars have piled at the other direction then switch
		//don't need mutex because it's reading
		if (lSem->val>=10 || rSem->val==0)
		{
			right=0;
		}	
		else if(rSem->val>=10 || lSem->val==0)
		{
			right=1;
		}
	}
	
	return 0;
}
