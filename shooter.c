/* Raw Graphics Demonstrator Main Program
 * Computer Graphics Group "Chobits"
 * 
 * NOTES:
 * http://www.ummon.eu/Linux/API/Devices/framebuffer.html
 * 
 * TODOS:
 * - make dedicated canvas frame handler (currently the canvas frame is actually screen-sized)
 * 
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

/* SETTINGS ------------------------------------------------------------ */
#define screenXstart 250
#define screenX 1366
#define screenY 768
#define mouseSensitivity 1

/* TYPEDEFS ------------------------------------------------------------ */

//RGB color
typedef struct s_rgb {
	unsigned char r;
	unsigned char g;
	unsigned char b;
} RGB;

//Frame of RGBs
typedef struct s_frame {
	RGB px[screenX][screenY];
} Frame;

//Coordinate System
typedef struct s_coord {
	int x;
	int y;
} Coord;

//The integrated frame buffer plus info struct.
typedef struct s_frameBuffer {
	char* ptr;
	int smemLen;
	int lineLen;
	int bpp;
} FrameBuffer;



/* MATH STUFF ---------------------------------------------------------- */

// construct coord
Coord coord(int x, int y) {
	Coord retval;
	retval.x = x;
	retval.y = y;
	return retval;
}

unsigned char isInBound(Coord position, Coord corner1, Coord corner2) {
	unsigned char xInBound = 0;
	unsigned char yInBound = 0;
	if (corner1.x < corner2.x) {
		xInBound = (position.x>corner1.x) && (position.x<corner2.x);
	} else if (corner1.x > corner2.x) {
		xInBound = (position.x>corner2.x) && (position.x<corner1.x);
	} else {
		return 0;
	}
	if (corner1.y < corner2.y) {
		yInBound = (position.y>corner1.y) && (position.y<corner2.y);
	} else if (corner1.y > corner2.y) {
		yInBound = (position.y>corner2.y) && (position.y<corner1.y);
	} else {
		return 0;
	}
	return xInBound&&yInBound;
}

/* MOUSE OPERATIONS ---------------------------------------------------- */

// get mouse coord, with integrated screen-space bounding
Coord getCursorCoord(Coord* mc) {
	Coord xy;
	if (mc->x < 0) {
		mc->x = 0;
		xy.x = 0;
	} else if (mc->x >= screenX*mouseSensitivity) {
		mc->x = screenX*mouseSensitivity-1;
		xy.x = screenX-1;
	} else {
		xy.x = (int) mc->x / mouseSensitivity;
	}
	if (mc->y < 0) {
		mc->y = 0;
		xy.y = 0;
	} else if (mc->y >= screenY*mouseSensitivity) {
		mc->y = screenY*mouseSensitivity-1;
		xy.y = screenY-1;
	} else {
		xy.y = (int) mc->y / mouseSensitivity;
	}
	return xy;
}

/* VIDEO OPERATIONS ---------------------------------------------------- */

// construct RGB
RGB rgb(unsigned char r, unsigned char g, unsigned char b) {
	RGB retval;
	retval.r = r;
	retval.g = g;
	retval.b = b;
	return retval;
}

// insert pixel to composition frame, with bounds filter
void insertPixel(Frame* frm, Coord loc, RGB col) {
	// do bounding check:
	if (!(loc.x >= screenX || loc.x < 0 || loc.y >= screenY || loc.y < 0)) {
		frm->px[loc.x][loc.y].r = col.r;
		frm->px[loc.x][loc.y].g = col.g;
		frm->px[loc.x][loc.y].b = col.b;
	}
}

// Sprite Controller
void insertSprite(Frame* frm, Coord loc, unsigned short type) {
	switch (type) {
		case 1 : { // the mouse sprite
			insertPixel(frm, loc, rgb(255,255,255));
			int i;
			for (i=5; i<10; i++) {
				insertPixel(frm, coord(loc.x-i, loc.y), rgb(0,0,0));
				insertPixel(frm, coord(loc.x+i, loc.y), rgb(0,0,0));
				insertPixel(frm, coord(loc.x, loc.y-i), rgb(0,0,0));
				insertPixel(frm, coord(loc.x, loc.y+i), rgb(0,0,0));
				
				insertPixel(frm, coord(loc.x-i, loc.y+1), rgb(255,255,255));
				insertPixel(frm, coord(loc.x-i, loc.y-1), rgb(255,255,255));
				insertPixel(frm, coord(loc.x+i, loc.y+1), rgb(255,255,255));
				insertPixel(frm, coord(loc.x+i, loc.y-1), rgb(255,255,255));
				insertPixel(frm, coord(loc.x+1, loc.y-i), rgb(255,255,255));
				insertPixel(frm, coord(loc.x-1, loc.y-i), rgb(255,255,255));
				insertPixel(frm, coord(loc.x+1, loc.y+i), rgb(255,255,255));
				insertPixel(frm, coord(loc.x-1, loc.y+i), rgb(255,255,255));
			}
		} break;
		case 2 : { // something?
			
		} break;
	}	
}

// delete contents of composition frame
void flushFrame (Frame* frm, RGB color) {
	int x;
	int y;
	for (y=0; y<screenY; y++) {
		for (x=0; x<screenX; x++) {
			frm->px[x][y] = color;
		}
	}
}

// copy composition Frame to FrameBuffer
void showFrame (Frame* frm, FrameBuffer* fb) {
	int x;
	int y;
	for (y=0; y<screenY; y++) {
		for (x=0; x<screenX; x++) {
			int location = x * (fb->bpp/8) + y * fb->lineLen;
			*(fb->ptr + location    ) = frm->px[x][y].b; // blue
			*(fb->ptr + location + 1) = frm->px[x][y].g; // green
			*(fb->ptr + location + 2) = frm->px[x][y].r; // red
			*(fb->ptr + location + 3) = 255; // transparency
		}
	}
}

void showCanvas(Frame* frm, Frame* cnvs, int canvasWidth, int canvasHeight, Coord loc, RGB borderColor) {
	int x, y;
	for (y=0; y<canvasHeight;y++) {
		for (x=0; x<canvasWidth; x++) {
			insertPixel(frm, coord(loc.x - canvasWidth/2 + x, loc.y - canvasHeight/2 + y), cnvs->px[x][y]);
		}
	}
	//show border
	for (y=0; y<canvasHeight; y++) {
		insertPixel(frm, coord(loc.x - canvasWidth/2 - 1, loc.y - canvasHeight/2 + y), borderColor);
		insertPixel(frm, coord(loc.x  - canvasWidth/2 + canvasWidth, loc.y - canvasHeight/2 + y), borderColor);
	}
	for (x=0; x<canvasWidth; x++) {
		insertPixel(frm, coord(loc.x - canvasWidth/2 + x, loc.y - canvasHeight/2 - 1), borderColor);
		insertPixel(frm, coord(loc.x - canvasWidth/2 + x, loc.y - canvasHeight/2 + canvasHeight), borderColor);
	}
}

/* MAIN FUNCTION ------------------------------------------------------- */
int main() {	
	/* Preparations ---------------------------------------------------- */
	
	// get fb and screenInfos
	struct fb_var_screeninfo vInfo; // variable screen info
	struct fb_fix_screeninfo sInfo; // static screen info
	int fbFile;	 // frame buffer file descriptor
	fbFile = open("/dev/fb0",O_RDWR);
	if (!fbFile) {
		printf("Error: cannot open framebuffer device.\n");
		exit(1);
	}
	if (ioctl (fbFile, FBIOGET_FSCREENINFO, &sInfo)) {
		printf("Error reading fixed information.\n");
		exit(2);
	}
	if (ioctl (fbFile, FBIOGET_VSCREENINFO, &vInfo)) {
		printf("Error reading variable information.\n");
		exit(3);
	}
	
	// create the FrameBuffer struct with its important infos.
	FrameBuffer fb;
	fb.smemLen = sInfo.smem_len;
	fb.lineLen = sInfo.line_length;
	fb.bpp = vInfo.bits_per_pixel;
	
	// and map the framebuffer to the FB struct.
	fb.ptr = (char*)mmap(0, sInfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbFile, 0);
	if ((long int)fb.ptr == -1) {
		printf ("Error: failed to map framebuffer device to memory.\n");
		exit(4);
	}
	
	// prepare mouse controller
	FILE *fmouse;
	char mouseRaw[3];
	fmouse = fopen("/dev/input/mice","r");
	Coord mouse; // mouse internal counter
	mouse.x = 0;
	mouse.y = 0;
		
	// prepare environment controller
	unsigned char loop = 1; // frame loop controller
	Frame cFrame; // composition frame (Video RAM)
	
	// prepare canvas
	Frame canvas;
	flushFrame(&canvas, rgb(0,0,0));
	int canvasWidth = 1000;
	int canvasHeight = 500;
	Coord canvasPosition = coord(screenX/2,screenY/2);
	
	int i; //for drawing.
	
	/* Main Loop ------------------------------------------------------- */
	while (loop) {
		
		if (mouse.x < screenX/2 - canvasWidth/2){
			mouse.x = screenX/2 - canvasWidth/2;
		} else if (mouse.x >= screenX/2 + canvasWidth/2) {
			mouse.x = screenX/2 + canvasWidth/2;
		}
		
		if (mouse.y < screenY/2 - canvasHeight/2){
			mouse.y = screenY/2 - canvasHeight/2;
		} else if (mouse.y >= screenY/2 + canvasHeight/2) {
			mouse.y = screenY/2 + canvasHeight/2;
		}
		
		//clean
		flushFrame(&cFrame, rgb(33,33,33));
		
		showCanvas(&cFrame, &canvas, canvasWidth, canvasHeight, canvasPosition, rgb(99,99,99));
		
		//fill mouse LAST
		insertSprite(&cFrame, getCursorCoord(&mouse), 1);
		
		//show frame
		showFrame(&cFrame,&fb);
		
		//read next mouse
		fread(mouseRaw,sizeof(char),3,fmouse);
		mouse.x += mouseRaw[1];
		mouse.y -= mouseRaw[2];

	}

	/* Cleanup --------------------------------------------------------- */
	munmap(fb.ptr, sInfo.smem_len);
	close(fbFile);
	fclose(fmouse);
	return 0;
}
