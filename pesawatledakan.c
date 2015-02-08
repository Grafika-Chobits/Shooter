#include <unistd.h>
#include <fcntl.h>      /* for fcntl */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>      /* for mmap */
#include <sys/ioctl.h>
#include <linux/fb.h>
 
#include <stdio.h>
#include <stdlib.h>

long int screensize = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
int fbfd;               /* frame buffer file descriptor */
char* fbp;              /* pointer to framebuffer */
int location;              /* iterate to location */

void Initialize() {
   /* open the file for reading and writing */
   fbfd = open("/dev/fb0",O_RDWR);
   if (!fbfd) {
      printf("Error: cannot open framebuffer device.\n");
      exit(1);
   }
   printf ("The framebuffer device was opened successfully.\n");
 
   /* get the fixed screen information */
   if (ioctl (fbfd, FBIOGET_FSCREENINFO, &finfo)) {
      printf("Error reading fixed information.\n");
      exit(2);
   }
 
   /* get variable screen information */
   if (ioctl (fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
      printf("Error reading variable information.\n");
      exit(3);
   }
 
   /* figure out the size of the screen in bytes */
   //screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
 
   /* map the device to memory */
   fbp = (char*)mmap(0, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
 
   if ((int)fbp == -1) {
      printf ("Error: failed to map framebuffer device to memory.\n");
      exit(4);
   }
   printf ("Framebuffer device was mapped to memory successfully.\n");
}

void setPixel(int x, int y) {

   location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;

     if ( vinfo.bits_per_pixel == 32 ) { 
         *(fbp + location) = 0; // Blue 
         *(fbp + location + 1) = 255; // Green 
         *(fbp + location + 2) = 0; // Red
         *(fbp + location + 3) = 0; // No transparency 
     } else { //assume 16bpp 
         int b = 10; int g = (x-100)/6; // A little green 
         int r = 31-(y-100)/16; // A lot of red 
         unsigned short int t = r<<20 | g << 5 | b; 
         *((unsigned short int*)(fbp + location)) = t; 
     }
}

void plotLine(int x0, int y0, int x1, int y1)
{
   // Draw line
   int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
   int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1; 
   int err = dx+dy, e2; /* error value e_Y */
 
   for(;;){  /* loop */
      setPixel(x0,y0);
      if (x0==x1 && y0==y1) break;
      e2 = 2*err;
      if (e2 >= dy) { err += dy; x0 += sx; } /* e_Y+e_x > 0 */
      if (e2 <= dx) { err += dx; y0 += sy; } /* e_Y+e_y < 0 */
   }
}

void gambarPesawat(int xAwal, int yAwal) {
	int X[19];
	int Y[19];
	X[0] = xAwal;
	X[1] = X[0] + 15;
	X[2] = X[1] + 30;
	X[3] = X[2] + 13;
	X[4] = X[3] + 13;
	X[5] = X[4] + 13;
	X[6] = X[5] + 13;
	X[7] = X[6] + 50;
	X[8] = X[7] + 5;
	X[9] = X[8] + 10;
	X[10] = X[9] + 3;
	X[11] = X[10] - 1;
	X[12] = X[11] + 1;
	X[13] = X[12] - 67;
	X[14] = X[13] + 13;
	X[15] = X[14] - 10;
	X[16] = X[15] - 17;
	X[17] = X[16] - 37;
	X[18] = X[17] - 27;

	Y[0] = yAwal;
	Y[1] = Y[0] - 5;
	Y[2] = Y[1] - 3;
	Y[3] = Y[2] - 4;
	Y[4] = Y[3] - 3;
	Y[5] = Y[4] + 3;
	Y[6] = Y[5] + 4;
	Y[7] = Y[6] - 3;
	Y[8] = Y[7] - 18;
	Y[9] = Y[8] - 4;
	Y[10] = Y[9] + 27;
	Y[11] = Y[10] + 5;
	Y[12] = Y[11] + 5;
	Y[13] = Y[12] + 3;
	Y[14] = Y[13] + 25;
	Y[15] = Y[14] - 6;
	Y[16] = Y[15] - 18;
	Y[17] = Y[16] - 1;
	Y[18] = Y[17] - 3;


	plotLine(X[0],Y[0],X[1],Y[1]);
	plotLine(X[1],Y[1],X[2],Y[2]);
	plotLine(X[2],Y[2],X[3],Y[3]);
	plotLine(X[3],Y[3],X[4],Y[4]);
	plotLine(X[4],Y[4],X[5],Y[5]);
	plotLine(X[5],Y[5], X[6],Y[6]);
	plotLine(X[6],Y[6],X[7],Y[7]);
	plotLine(X[7],Y[7],X[8],Y[8]);
	plotLine(X[8],Y[8],X[9],Y[9]);
	plotLine(X[9],Y[9],X[10],Y[10]);
	plotLine(X[10],Y[10],X[11],Y[11]);
	plotLine(X[11],Y[11],X[12],Y[12]);
	plotLine(X[12],Y[12],X[13],Y[13]);
	plotLine(X[13],Y[13],X[14],Y[14]);
	plotLine(X[14],Y[14],X[15],Y[15]);
	plotLine(X[15],Y[15],X[16],Y[16]);
	plotLine(X[16],Y[16],X[17],Y[17]);
	plotLine(X[17],Y[17],X[18],Y[18]);
	plotLine(X[18],Y[18],X[0],Y[0]);
}

void gambarLedakan(int posX, int posY) {
	int X[16];
	int Y[16];

	X[0] = posX;
	X[1] = X[0] - 15;
	X[2] = X[1] - 45;
	X[3] = X[2] + 30;
	X[4] = X[3] - 70;
	X[5] = X[4] + 70;
	X[6] = X[5] - 30;
	X[7] = X[6] + 45;
	X[8] = X[7] + 15;
	X[9] = X[8] + 15;
	X[10] = X[9] + 45;
	X[11] = X[10] - 30;
	X[12] = X[11] + 70;
	X[13] = X[12] - 70;
	X[14] = X[13] + 30;
	X[15] = X[14] - 45;

	Y[0] = posY;
	Y[1] = Y[0] + 70;
	Y[2] = Y[1] - 35;
	Y[3] = Y[2] + 50;
	Y[4] = Y[3] + 15;
	Y[5] = Y[4] + 15;
	Y[6] = Y[5] + 50;
	Y[7] = Y[6] - 35;
	Y[8] = Y[7] + 70;
	Y[9] = Y[8] - 70;
	Y[10] = Y[9] + 35;
	Y[11] = Y[10] - 50;
	Y[12] = Y[11] - 15;
	Y[13] = Y[12] - 15;
	Y[14] = Y[13] - 50;
	Y[15] = Y[14] + 35;


	plotLine(X[0],Y[0],X[1],Y[1]);
	plotLine(X[1],Y[1],X[2],Y[2]);
	plotLine(X[2],Y[2],X[3],Y[3]);
	plotLine(X[3],Y[3],X[4],Y[4]);
	plotLine(X[4],Y[4],X[5],Y[5]);
	plotLine(X[5],Y[5], X[6],Y[6]);
	plotLine(X[6],Y[6],X[7],Y[7]);
	plotLine(X[7],Y[7],X[8],Y[8]);
	plotLine(X[8],Y[8],X[9],Y[9]);
	plotLine(X[9],Y[9],X[10],Y[10]);
	plotLine(X[10],Y[10],X[11],Y[11]);
	plotLine(X[11],Y[11],X[12],Y[12]);
	plotLine(X[12],Y[12],X[13],Y[13]);
	plotLine(X[13],Y[13],X[14],Y[14]);
	plotLine(X[14],Y[14],X[15],Y[15]);
	plotLine(X[15],Y[15],X[0],Y[0]);
}

int main() {
	Initialize();
	int xPesawat=1280;
	int yPesawat=200;
	while(1) {
   		///*
   		// Pesawat bergerak
   		gambarPesawat(xPesawat,yPesawat);
   		system("clear");
   		xPesawat--;
   		//*/

   		// Ledakan
   		gambarLedakan(500,500);
	}
   	// Close framebuffer
   	munmap(fbp, screensize);
   	close(fbfd);
	return 0;
}
