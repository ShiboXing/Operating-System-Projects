#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <linux/fb.h>
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>
#include "graphics.h"

int rnd(float);

static color_t *buffer;
static int size;
static int line_depth;
static int yres;
static int xres;

int get_xres()
{
	return xres;
}
int get_yres()
{
	return yres;
}

void clear_screen(void* img)
{
//	printf("clear_screen started");
	int tmp=0;
	while (tmp<size)
	{
		*((color_t *)img+tmp)=(color_t)0;
		tmp++;
	}
//	printf("clear_screen completed");
}

void draw_line(void *img,int x1,int y1,int x2,int y2,color_t c)
{
//using Digital Differential Analyzer algorithm
	
	int dx=x2-x1;
	int dy=y2-y1;
	int steps;
	if (abs(dx) > abs(dy))
		steps=abs(dx);
	else
		steps=abs(dy);

	float deltax=dx/(float)steps;
	float deltay=dy/(float)steps;
	
	int v=0;
	float x=x1;
	float y=y1;
	while (v < steps)
	{
		x+=deltax;
		y+=deltay;
		draw_pixel(img,rnd(x),rnd(y),c);
		v++;
	}
}

int rnd(float num)
{
	return (num >=0) ? (int)(num+0.5) : (int)(num-0.5);
}

void draw_pixel(void *img,int x,int y,color_t c)
{	
	*((color_t *)img+y*xres+x)=c;
}

void blit(void *src)
{
	int tmp=0;
	while(tmp<size)
	{
		*(buffer+tmp)=*((color_t *)src + tmp);
		tmp++;
	}
}

void* new_offscreen_buffer()
{
	void* res=mmap(0,line_depth*yres,
	PROT_WRITE|PROT_READ,MAP_PRIVATE | MAP_ANONYMOUS ,-1,0);
	return res;
}

void sleep_ms(long ms)
{
	struct timespec tim;
	tim.tv_sec=ms/1000;
	tim.tv_nsec=(ms%1000)*1000000L;
	
	nanosleep(&tim,NULL);	
}

char getkey()
{
	fd_set read_fds;
	struct timeval tv;
	
	tv.tv_sec=tv.tv_usec=0;
	FD_ZERO(&read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	
	int select_res=select(STDIN_FILENO+1,&read_fds,NULL,
	NULL,&tv);
	
	char ans='\0';

	if (select_res>0)
	{
		read(0,&ans,sizeof(ans));
	}
	
	return ans;
	
}

void init_graphics()
{
	//open fb0
	char gPath[]="/dev/fb0";
	int fd=open(gPath,O_RDWR);
	
	//get screen size (x*y)
	struct fb_var_screeninfo fb_var;
	struct fb_fix_screeninfo fb_fix;
	ioctl(fd,FBIOGET_VSCREENINFO,&fb_var);
	ioctl(fd,FBIOGET_FSCREENINFO,&fb_fix);
	xres=fb_var.xres_virtual;
	yres=fb_var.yres_virtual;
	line_depth=fb_fix.line_length;
	size=xres*yres;
	//disable the keyboard echo
	struct termios term;
	int get_status=ioctl(0,TCGETS,&term);
	term.c_lflag &= ~(ECHO | ICANON);
	int set_status=ioctl(0,TCSETS,&term);

	//make mmap sys call
	buffer=(color_t*)mmap(NULL,line_depth*yres,
	PROT_WRITE|PROT_READ,MAP_SHARED,fd,0);	
	

	printf("\033[2J \n");
}

void exit_graphics()
{
	//open fb0
	char gPath[]="/dev/fb0";
	int fd=open(gPath,O_RDWR);

	//set ECHO and ICANON back to 1
	struct termios term;
	int get_status=ioctl(0,TCGETS,&term);
	term.c_lflag |= (ECHO|ICANON);
	int set_status=ioctl(0,TCSETS,&term);	

}


