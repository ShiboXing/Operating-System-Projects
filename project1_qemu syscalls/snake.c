#include "graphics.h"
#include <stdio.h>

typedef struct Block Block;

color_t *off_buf;
color_t c = RGB(0,28,28);
color_t b = RGB(0,0,0);
static int size=10;
static int len=20;
static int startx=50;
static int starty=50;
static int step=10;
static int xres;
static int yres;

struct Block
{
	int x,y;
	char dir;
};
//if len changed, must change the size of snake as well!!!!!!
static Block snake[20];


void create_snake();
void move();
void draw_block(Block *blk,color_t color)
{
	/*if the index is negative need to plus a standard 
	 resolution
	 else if the index out of the standard resolution bound		  
	 then modulo a standard resolution
	*/

	if (blk->x<0)
		blk->x+=xres;
	else
		blk->x%=xres;
	
	if (blk->y<0)
		blk->y+=yres;
	else
		blk->y%=yres;
	
	int count=0;
//	printf("draw blk->x: %d blk->y: %d\n",blk->x,blk->y);
	while (count<size)
	{
//		printf("draw line");
		draw_line(off_buf,blk->x,blk->y+count,
			blk->x+size,blk->y+count,color);
		count++;
	}
	
}

int main()
{
	init_graphics();	
	//draw snake
	xres=get_xres();
	yres=get_yres();

	off_buf=new_offscreen_buffer();
	create_snake();
	
	while (1)
	{

		char key=getkey();
		if (key=='[')
			key=getkey();
		if (key=='q')
			break;
		else if(key=='A')
			snake[len-1].dir='u';
		else if(key=='B')
			snake[len-1].dir='d';	
		else if(key=='C')
			snake[len-1].dir='r';
		else if(key=='D')
			snake[len-1].dir='l';

//		printf("key is %c\n",key);
		move();
		blit(off_buf);
		sleep_ms(1);
	}
	exit_graphics();	
	return 0;
}
void move()
{
	int count=0;
	while(count<len)
	{
		
		if (count==0)
			draw_block(&snake[count],b);
		char dir=snake[count].dir;
		if (dir=='r')
		{
			snake[count].x+=step;
			draw_block(&snake[count],c);
				
		}
		else if(dir=='l')
		{
			snake[count].x-=step;
			draw_block(&snake[count],c);
		}
		else if(dir=='u')
		{
			snake[count].y-=step;
			draw_block(&snake[count],c);
		}
		else if(dir=='d')
		{
			snake[count].y+=step;
			draw_block(&snake[count],c);
		}
		if (count<len-1)
			snake[count].dir=snake[count+1].dir;
		
		count++;
	}
}

void create_snake()
{
	
	int count=0;
	int blk_size=sizeof(Block);
	while (count<len)
	{
		snake[count].x=startx+count*size;
		snake[count].y=starty;
		snake[count].dir='r';
		count++;
	}	
	
	count--;	
	while (count>=0)
	{
		//printf("snake[%d]: %d\n",count,snake[count].x);
		draw_block(&snake[count],c);
		count--;
	}

	blit(off_buf);	

}
