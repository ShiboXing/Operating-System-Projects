#define RGB(R,G,B) (color_t)(B | G << 5 | R << 11)

typedef unsigned short color_t;

void init_graphics();
void exit_graphics();
char getkey();
void sleep_ms(long ms);
void clear_screen(void *img);
void draw_pixel(void *img,int x,int y, color_t c);
void draw_line(void *img,int x1,int y1,int x2,int y2,color_t c);
void *new_offscreen_buffer();
void blit(void *src);

