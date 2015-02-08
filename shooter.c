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

#define min(X,Y) (((X) < (Y)) ? (X) : (Y))
#define max(X,Y) (((X) > (Y)) ? (X) : (Y))


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

void showCanvas(Frame* frm, Frame* cnvs, int canvasWidth, int canvasHeight, Coord loc, RGB borderColor, int isBorder) {
	int x, y;
	for (y=0; y<canvasHeight;y++) {
		for (x=0; x<canvasWidth; x++) {
			insertPixel(frm, coord(loc.x - canvasWidth/2 + x, loc.y - canvasHeight/2 + y), cnvs->px[x][y]);
		}
	}
	
	//show border
	if(isBorder){
		for (y=0; y<canvasHeight; y++) {
			insertPixel(frm, coord(loc.x - canvasWidth/2 - 1, loc.y - canvasHeight/2 + y), borderColor);
			insertPixel(frm, coord(loc.x  - canvasWidth/2 + canvasWidth, loc.y - canvasHeight/2 + y), borderColor);
		}
		for (x=0; x<canvasWidth; x++) {
			insertPixel(frm, coord(loc.x - canvasWidth/2 + x, loc.y - canvasHeight/2 - 1), borderColor);
			insertPixel(frm, coord(loc.x - canvasWidth/2 + x, loc.y - canvasHeight/2 + canvasHeight), borderColor);
		}
	}
}

void addBlob(Frame* cnvs, Coord loc, RGB color) {
	int x,y;
	for (y=-2; y<3;y++) {
		for (x=-2; x<3; x++) {
			if (!(abs(x)==2 && abs(y)==2)) {
				insertPixel(cnvs, coord(loc.x+x, loc.y+y), color);
			}
		}
	}
}

	
void plotCircle(Frame* frm,int xm, int ym, int r,RGB col)
{
   int x = -r, y = 0, err = 2-2*r; /* II. Quadrant */ 
   do {
      insertPixel(frm,coord(xm-x, ym+y),col); /*   I. Quadrant */
      insertPixel(frm,coord(xm-y, ym-x),col); /*  II. Quadrant */
      insertPixel(frm,coord(xm+x, ym-y),col); /* III. Quadrant */
      insertPixel(frm,coord(xm+y, ym+x),col); /*  IV. Quadrant */
      r = err;
      if (r <= y) err += ++y*2+1;           /* e_xy+e_y < 0 */
      if (r > x || err > y) err += ++x*2+1; /* e_xy+e_x > 0 or no 2nd y-step */
   } while (x < 0);
}

void plotHalfCircle(Frame *frm,int xm, int ym, int r,RGB col)
{
   int x = -r, y = 0, err = 2-2*r; /* II. Quadrant */ 
   do {
      insertPixel(frm,coord(xm+x, ym-y),col); /* III. Quadrant */
      insertPixel(frm,coord(xm+y, ym+x),col); /*  IV. Quadrant */
      r = err;
      if (r <= y) err += ++y*2+1;           /* e_xy+e_y < 0 */
      if (r > x || err > y) err += ++x*2+1; /* e_xy+e_x > 0 or no 2nd y-step */
   } while (x < 0);
}

/* Fungsi membuat garis */
void plotLine(Frame* frm, int x0, int y0, int x1, int y1, RGB lineColor)
{
	int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1; 
	int err = dx+dy, e2; /* error value e_xy */
	int loop = 1;
	while(loop){  /* loop */
		insertPixel(frm, coord(x0, y0), rgb(lineColor.r, lineColor.g, lineColor.b));
		if (x0==x1 && y0==y1) loop = 0;
		e2 = 2*err;
		if (e2 >= dy) { err += dy; x0 += sx; } /* e_xy+e_x > 0 */
		if (e2 <= dx) { err += dx; y0 += sy; } /* e_xy+e_y < 0 */
	}
}

void plotLineWidth(Frame* frm, int x0, int y0, int x1, int y1, float wd, RGB lineColor) { 
	int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1; 
	int dy = abs(y1-y0), sy = y0 < y1 ? 1 : -1; 
	int err = dx-dy, e2, x2, y2;                          /* error value e_xy */

	float ed = dx+dy == 0 ? 1 : sqrt((float)dx*dx+(float)dy*dy);

	for (wd = (wd+1)/2; ; ) {                                   /* pixel loop */
		insertPixel(frm, coord(x0, y0), rgb(max(0,lineColor.r*(abs(err-dx+dy)/ed-wd+1)), 
											max(0,lineColor.g*(abs(err-dx+dy)/ed-wd+1)), 
											max(0,lineColor.b*(abs(err-dx+dy)/ed-wd+1))));

		e2 = err; x2 = x0;
		if (2*e2 >= -dx) {                                           /* x step */
			for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx)
				y2 += sy;
				insertPixel(frm, coord(x0, y2), rgb(max(0,lineColor.r*(abs(e2)/ed-wd+1)), 
															max(0,lineColor.g*(abs(e2)/ed-wd+1)), 
															max(0,lineColor.b*(abs(e2)/ed-wd+1)))); 
			if (x0 == x1) break;
			e2 = err; err -= dy; x0 += sx; 
		} 
		
		if (2*e2 <= dy) {                                            /* y step */
			for (e2 = dx-e2; e2 < ed*wd && (x1 != x2 || dx < dy); e2 += dy)
				x2 += sx;
				insertPixel(frm, coord(x2, y0), rgb(max(0,lineColor.r*(abs(e2)/ed-wd+1)), 
															max(0,lineColor.g*(abs(e2)/ed-wd+1)), 
															max(0,lineColor.b*(abs(e2)/ed-wd+1)))); 
			if (y0 == y1) break;
			err += dx; y0 += sy; 
		}
	}
}

void drawCannon(Frame* frm,Coord loc,RGB color){
	plotLine(frm,loc.x-10,loc.y-10,loc.x-10,loc.y+30,color);
	plotLine(frm,loc.x-10,loc.y+30,loc.x+10,loc.y+30,color);
	plotLine(frm,loc.x+10,loc.y+30,loc.x+10,loc.y-10,color);
	plotLine(frm,loc.x+10,loc.y-10,loc.x-10,loc.y-10,color);	
	plotHalfCircle(frm,loc.x,loc.y-10,10,color);
	loc.y=loc.y-20;
	plotLine(frm,loc.x-5,loc.y-5,loc.x-5,loc.y+2,color);
	//plotLine(loc.x-5,loc.y+5,loc.x+5,loc.y+5);
	plotLine(frm,loc.x+5,loc.y+2,loc.x+5,loc.y-5,color);
	plotLine(frm,loc.x+5,loc.y-5,loc.x-5,loc.y-5,color);	
}


void drawStickman(Frame* frm,Coord loc,int sel,RGB color,int counter){
	plotCircle(frm,loc.x,loc.y,15,color);
	plotLine(frm,loc.x,loc.y+15,loc.x,loc.y+50,color);
	
	if(counter % 2 == 0){
		plotLine(frm,loc.x,loc.y+30,loc.x+20,loc.y+sel-3,color);
		plotLine(frm,loc.x,loc.y+30,loc.x+25,loc.y+(sel+10)-3,color);
	}else{
		plotLine(frm,loc.x,loc.y+30,loc.x+20,loc.y+sel,color);
		plotLine(frm,loc.x,loc.y+30,loc.x+25,loc.y+(sel+10),color);
	}
	
	
}

void drawStickmanAndCannon(Frame *frame, Coord shipPosition, RGB color, int counter){
	
	if(counter % 2 == 0){
		//Draw cannon
		drawCannon(frame, coord(shipPosition.x, shipPosition.y - 70 - 3), rgb(99,99,99));
	}else{
		//Draw cannon
		drawCannon(frame, coord(shipPosition.x, shipPosition.y - 70), rgb(99,99,99));
	}
		
	//Draw stickman
	drawStickman(frame, coord(shipPosition.x - 30, shipPosition.y - 80), 10, rgb(99,99,99),counter);
}

/* Fungsi membuat kapal */
void drawShip(Frame *frame, Coord center, RGB color)
{
	int panjangDekBawah = 100;
	int deltaDekAtasBawah = 60;
	int height = 30;
	int jarakKeUjung = panjangDekBawah / 2 + deltaDekAtasBawah / 2;
	
	//Draw Bawah Kapal
	plotLine(frame, center.x - panjangDekBawah / 2, center.y, center.x + panjangDekBawah / 2, center.y , color);
	
	//Draw Deck Kapal
	plotLine(frame, center.x - jarakKeUjung, center.y - height, center.x + jarakKeUjung, center.y - height, color);
	
	//Draw garis mengikuti kedua ujung garis di atas
	plotLine(frame, center.x - panjangDekBawah / 2, center.y , center.x - jarakKeUjung, center.y - height, color);
	plotLine(frame, center.x + panjangDekBawah / 2, center.y , center.x + jarakKeUjung, center.y - height, color);
	
}

void drawAmmunition(Frame *frame, Coord upperBoundPosition, int ammunitionWidth, int ammunitionLength, RGB color){
	plotLine(frame, upperBoundPosition.x, upperBoundPosition.y, upperBoundPosition.x, upperBoundPosition.y + ammunitionLength, color);
	
	int i;
	for(i = 0; i < ammunitionWidth; i++){
		plotLine(frame, upperBoundPosition.x + i, upperBoundPosition.y, upperBoundPosition.x + i, upperBoundPosition.y + ammunitionLength, color);
		plotLine(frame, upperBoundPosition.x - i, upperBoundPosition.y, upperBoundPosition.x - i, upperBoundPosition.y + ammunitionLength, color);
	}	
}

void drawPeluru(Frame *frame, Coord center, RGB color)
{
	int panjangPeluru = 10;
	//DrawKiri
	plotLine(frame, center.x - 3, center.y + panjangPeluru / 2, center.x -3, center.y - panjangPeluru / 2, color); 
	
	//DrawKanan
	plotLine(frame, center.x + 3, center.y + panjangPeluru / 2, center.x + 3, center.y - panjangPeluru / 2, color);
	
	//DrawBawah
	plotLine(frame, center.x - 3, center.y + panjangPeluru / 2, center.x +3, center.y + panjangPeluru / 2, color);
	
	//DrawUjungKiri
	plotLine(frame, center.x - 3, center.y - panjangPeluru / 2, center.x, center.y - (panjangPeluru / 2 + 4), color);
	
	//DrawUjungKanan
	plotLine(frame, center.x + 3, center.y - panjangPeluru / 2, center.x, center.y - (panjangPeluru / 2 + 4), color);
}

void drawPlane(Frame *frame, Coord position, RGB color) {
	int X[19];
	int Y[19];
	X[0] = position.x;
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

	Y[0] = position.y;
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

	plotLine(frame,X[0],Y[0],X[1],Y[1],color);
	plotLine(frame,X[1],Y[1],X[2],Y[2],color);
	plotLine(frame,X[2],Y[2],X[3],Y[3],color);
	plotLine(frame,X[3],Y[3],X[4],Y[4],color);
	plotLine(frame,X[4],Y[4],X[5],Y[5],color);
	plotLine(frame,X[5],Y[5], X[6],Y[6],color);
	plotLine(frame,X[6],Y[6],X[7],Y[7],color);
	plotLine(frame,X[7],Y[7],X[8],Y[8],color);
	plotLine(frame,X[8],Y[8],X[9],Y[9],color);
	plotLine(frame,X[9],Y[9],X[10],Y[10],color);
	plotLine(frame,X[10],Y[10],X[11],Y[11],color);
	plotLine(frame,X[11],Y[11],X[12],Y[12],color);
	plotLine(frame,X[12],Y[12],X[13],Y[13],color);
	plotLine(frame,X[13],Y[13],X[14],Y[14],color);
	plotLine(frame,X[14],Y[14],X[15],Y[15],color);
	plotLine(frame,X[15],Y[15],X[16],Y[16],color);
	plotLine(frame,X[16],Y[16],X[17],Y[17],color);
	plotLine(frame,X[17],Y[17],X[18],Y[18],color);
	plotLine(frame,X[18],Y[18],X[0],Y[0],color);
	//addBlob(frame, coord(position.x+170,position.y+15), rgb(255,0,0));
}

void drawExplosion(Frame *frame, Coord loc, int mult, RGB color){	
	plotLine(frame,loc.x+10*mult,loc.y +10*mult,loc.x+20*mult,loc.y+20*mult,color);
	plotLine(frame,loc.x-10*mult,loc.y -10*mult,loc.x-20*mult,loc.y-20*mult,color);
	plotLine(frame,loc.x+10*mult,loc.y -10*mult,loc.x+20*mult,loc.y-20*mult,color);
	plotLine(frame,loc.x-10*mult,loc.y +10*mult,loc.x-20*mult,loc.y+20*mult,color);
	plotLine(frame,loc.x,loc.y -10*mult,loc.x,loc.y-20*mult,color);
	plotLine(frame,loc.x-10*mult,loc.y,loc.x-20*mult,loc.y,color);
	plotLine(frame,loc.x+10*mult,loc.y ,loc.x+20*mult,loc.y,color);
	plotLine(frame,loc.x,loc.y +10*mult,loc.x,loc.y+20*mult,color);
}

void animateExplosion(Frame* frame, int explosionMul, Coord loc){
	int explosionR, explosionG, explosionB;
	explosionR = explosionG = explosionB = 255-explosionMul*12;	
	if(explosionR <= 0 || explosionG <= 0 || explosionB <= 0){
		explosionR = explosionG = explosionB = 0;
	}
	drawExplosion(frame, loc, explosionMul, rgb(explosionR, 0, 0));
}

void drawBomb(Frame *frame, Coord center, RGB color)
{
	int panjangBomb = 10;
	//DrawKiri
	plotLine(frame, center.x - 3, center.y + panjangBomb / 2, center.x -3, center.y - panjangBomb / 2, color); 
	
	//DrawKanan
	plotLine(frame, center.x + 3, center.y + panjangBomb / 2, center.x + 3, center.y - panjangBomb / 2, color);
	
	//DrawAtas
	plotLine(frame, center.x - 3, center.y - panjangBomb / 2, center.x +3, center.y - panjangBomb / 2, color);
	
	//DrawUjungKiri
	plotLine(frame, center.x - 3, center.y + panjangBomb / 2, center.x, center.y + (panjangBomb / 2 + 4), color);
	
	//DrawUjungKanan
	plotLine(frame, center.x + 3, center.y + panjangBomb / 2, center.x, center.y + (panjangBomb / 2 + 4), color);
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
		
	// prepare ship
	int shipVelocity = 5; // velocity (pixel/ loop)
	int planeVelocity = 10;
	
	int shipXPosition = canvasWidth - 80;
	int shipYPosition = 490;
	int planeXPosition = canvasWidth;
	int planeYPosition = 50;
	int explosionMul = 0;
	
	// prepare ammunition
	Coord firstAmmunitionCoordinate;
	int isFirstAmmunitionReleased = 1;
	Coord secondAmmunitionCoordinate;
	int isSecondAmmunitionReleased = 0;
	int ammunitionVelocity = 5;
	int ammunitionLength = 20;
	
	firstAmmunitionCoordinate.x = shipXPosition;
	firstAmmunitionCoordinate.y = shipYPosition - 120;
	secondAmmunitionCoordinate.y = shipYPosition - 120;
	
	
	//prepare Bomb
	Coord firstBombCoordinate;
	int isFirstBombReleased = 1;
	Coord secondBombCoordinate;
	int isSecondBombReleased = 0;
	int bombVelocity = 10;
	int bombLength = 20;
	
	firstBombCoordinate.x = planeXPosition;
	firstBombCoordinate.y = planeYPosition + 120;
	secondBombCoordinate.y = planeYPosition - 120;
	
	
	int i; //for drawing.
	int MoveLeft = 1;
	int stickmanCounter = 0;
	
	int isXploded = 0;
	Coord coordXplosion;
	
	/* Main Loop ------------------------------------------------------- */
	
	while (loop) {
		
		// clean composition frame
		flushFrame(&cFrame, rgb(33,33,33));
				
		showCanvas(&cFrame, &canvas, canvasWidth, canvasHeight, canvasPosition, rgb(99,99,99), 1);
		
		// clean canvas
		flushFrame(&canvas, rgb(0,0,0));
		
		// draw ship
		drawShip(&canvas, coord(shipXPosition,shipYPosition), rgb(99,99,99));
		
		// draw stickman and cannon
		drawStickmanAndCannon(&canvas, coord(shipXPosition,shipYPosition), rgb(99,99,99), stickmanCounter++);
				
		// draw plane
		drawPlane(&canvas, coord(planeXPosition -= planeVelocity, planeYPosition), rgb(99, 99, 99));
		
		// Plane Bomb
		if(isFirstBombReleased){
			firstBombCoordinate.y+=bombVelocity;
			
			if(firstBombCoordinate.y >= 2 * canvasHeight/3 && !isSecondBombReleased){
				isSecondBombReleased = 1;
				secondBombCoordinate.x = planeXPosition;
				secondBombCoordinate.y = planeYPosition + 15;
			}
			
			if(firstBombCoordinate.y >= screenY - ((screenY - canvasHeight)/2)){
				isFirstBombReleased = 0;
			}
			
			drawBomb(&canvas, firstBombCoordinate, rgb(99, 99, 99));
			drawAmmunition(&canvas, firstBombCoordinate, 3, ammunitionLength, rgb(99, 99, 99));
		}
		
		if(isSecondBombReleased){
			secondBombCoordinate.y+=bombVelocity;
			
			if(secondBombCoordinate.y >= canvasHeight/3 && !isFirstBombReleased){
				isFirstBombReleased = 1;
				firstBombCoordinate.x = planeXPosition;
				firstBombCoordinate.y = planeYPosition + 15;
			}
			
			if(secondBombCoordinate.y >= screenY - 150){
				isSecondBombReleased = 0;
			}
			
			drawBomb(&canvas, secondBombCoordinate, rgb(99, 99, 99));
			drawAmmunition(&canvas, secondBombCoordinate, 3, ammunitionLength, rgb(99, 99, 99));
		}
		
		// stickman ammunition
		if(isFirstAmmunitionReleased){
			firstAmmunitionCoordinate.y-=ammunitionVelocity;
			
			if(firstAmmunitionCoordinate.y <= canvasHeight/3 && !isSecondAmmunitionReleased){
				isSecondAmmunitionReleased = 1;
				secondAmmunitionCoordinate.x = shipXPosition;
				secondAmmunitionCoordinate.y = shipYPosition - 120;
			}
			
			if(firstAmmunitionCoordinate.y <= -ammunitionLength){
				isFirstAmmunitionReleased = 0;
			}
			
			drawPeluru(&canvas, firstAmmunitionCoordinate, rgb(99, 99, 99));
			drawAmmunition(&canvas, firstAmmunitionCoordinate, 3, ammunitionLength, rgb(99, 99, 99));
		}
		
		if(isSecondAmmunitionReleased){
			secondAmmunitionCoordinate.y-=ammunitionVelocity;
			
			if(secondAmmunitionCoordinate.y <= canvasHeight/3 && !isFirstAmmunitionReleased){
				isFirstAmmunitionReleased = 1;
				firstAmmunitionCoordinate.x = shipXPosition;
				firstAmmunitionCoordinate.y = shipYPosition - 120;
			}
			
			if(secondAmmunitionCoordinate.y <= 0){
				isSecondAmmunitionReleased = 0;
			}
			
			drawPeluru(&canvas, secondAmmunitionCoordinate, rgb(99, 99, 99));
			drawAmmunition(&canvas, secondAmmunitionCoordinate, 3, ammunitionLength, rgb(99, 99, 99));
		}
			
		//explosion
		
		if (isInBound(coord(firstAmmunitionCoordinate.x, firstAmmunitionCoordinate.y), coord(planeXPosition-5, planeYPosition-15), coord(planeXPosition+170, planeYPosition+15))) {
			coordXplosion = firstAmmunitionCoordinate;
			isXploded = 1;
			//printf("boom");
		} else if (isInBound(coord(secondAmmunitionCoordinate.x, secondAmmunitionCoordinate.y), coord(planeXPosition-5, planeYPosition-15), coord(planeXPosition+170, planeYPosition+15))) {
			coordXplosion = secondAmmunitionCoordinate;
			isXploded = 1;
			//printf("boom");
		}
		if (isXploded == 1) {
			animateExplosion(&canvas, explosionMul, coordXplosion);
			explosionMul++;
			if(explosionMul >= 20){
				explosionMul = 0;
				isXploded = 0;
			}
		}
		
		if(planeXPosition <= -170){
			planeXPosition = canvasWidth;
		}
		
		if(planeXPosition == screenX/2 - canvasWidth/2 - 165){
			planeXPosition = screenX/2 + canvasWidth/2;
		}
		
		if(shipXPosition == 80){
			MoveLeft = 0;
		} 
		
		if(shipXPosition == canvasWidth - 80){
			MoveLeft = 1;
		} 
		
		if(MoveLeft){
			shipXPosition -= shipVelocity;
		}else{
			shipXPosition += shipVelocity;
		}
		
		//show frame
		showFrame(&cFrame,&fb);
		
	}

	/* Cleanup --------------------------------------------------------- */
	munmap(fb.ptr, sInfo.smem_len);
	close(fbFile);
	fclose(fmouse);
	return 0;
}

