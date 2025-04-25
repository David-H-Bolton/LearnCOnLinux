// Genmasks.c from chapter 31
#include "hr_time.h"
#include <time.h>
#include "SDL2/SDL.h"   // All SDL Applications need this 
#include "SDL2/SDL_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lib.h"

// key definitions
#define ADDASTEROIDKEY SDLK_a
#define COUNTERCLOCKWISEKEY SDLK_q
#define BOMBKEY SDLK_b
#define CLOCKWISEKEY SDLK_w
#define DEBUGKEY SDLK_TAB
#define FIREKEY SDLK_SPACE
#define JUMPKEY SDLK_j
#define PAUSEKEY SDLK_PAUSE
#define QUITKEY SDLK_ESCAPE
#define THRUSTKEY SDLK_LCTRL

// sizes
#define WIDTH 1024
#define HEIGHT 768
#define SHIPHEIGHT 64
#define SHIPWIDTH 64
#define EXPLOSIONSIZE 128

// numbers of things
#define MAXASTEROIDS 256
#define MAXBULLETS 16
#define MAXEXPLOSIONS 256
#define NUMSOUNDS 4
#define NUMsurfaceS 15
#define NUMEXPLOSIONSOUNDS 2


// surface indices
#define PLBACKDROP 0
#define TEXTsurface 1
#define surfacePLAYERSHIP 2
#define surfaceDEBUG 3
#define surfaceASTEROID1 4  // 4,5,6,7
#define surfaceBULLET 8
#define surfaceSMALLSHIP 9
#define surfaceEXPLOSION 10 // 10,11,12,13
#define surfaceAlien 14

// SDL variables
SDL_Window* screen = NULL;
SDL_Renderer *renderer;
SDL_Event event;
SDL_Rect source, destination, dst,sourceRect,destRect;
SDL_Surface* surfaces[NUMsurfaceS];

// int variables
int keypressed;
int rectCount = 0;
int frameCount,tickCount,lastTick, showfps;
int score,paused;
int debugFlag;
int fireFlag;
int rotTimer,speedTimer, fireTimer, jumpTimer,pauseTimer;
int rotateFlag, thrustFlag, jumpFlag, pauseFlag;
int rotateFlag; // 0 do nowt, 1 counter clockwise, 2 clockwise 
int numAsteroids = 0;

// array variables
char buffer[100],buffer2[100];
char plmask[24][64][64]; // y,x,i
char asteroidmask[24][280][280];
char alienmask[64][64];
char data[280][280];
stopWatch stw;

// consts
const char * surfacenames[NUMsurfaceS] = { "images/starfield.png","images/text.png","images/playership.png","images/debug.png",
"images/a1.png","images/a2.png","images/a3.png","images/a4.png","images/bullet.png","images/smallship.png","images/explosion0.png",
"images/explosion1.png","images/explosion2.png","images/explosion3.png","images/alien.png" };


// external variables
extern int errorCount;


SDL_Surface* Loadsurface(const char * afile) {
	SDL_Surface *surface = IMG_Load(afile);
	if (surface == 0) {
		LogError2("Failed to load surface from ", afile);
	}
	return surface;
}

// Loads surfaces 
void Loadsurfaces() {
	for (int i = 0; i<NUMsurfaceS; i++) {
		surfaces[i] = Loadsurface(surfacenames[i]);
	}
}

// Takes a surface of size in pixels from filename and with specified number of rotations
//. It generates a mask file. 
void processsurface(int surfacenum, int size, char * filename,int rotations) {
	Uint32 rmask, gmask, bmask, amask;
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
	SDL_Surface *surface = SDL_CreateRGBSurface(0, size,size, 32, rmask, gmask, bmask, amask);
	SDL_Rect src = {0,0,size,size};
	SDL_Rect dst = src;

	memset(data, 0,sizeof(data));
	FILE * fbin= fopen( filename, "wb");
	for (int i = 0; i < rotations; i++) {
		src.x = i * size;	

		SDL_FillRect(surface, &dst, 0x000000);	
		SDL_BlitSurface(surfaces[surfacenum], &src, surface, &dst);				
		int bpp = surface->format->BytesPerPixel;

		// fetch data
		SDL_LockSurface(surface);
		for(int y=0;y<size;y++)
			for (int x = 0; x < size; x++) {
				Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
				data[y][x] = *p ? 1 : 0;
			}		
		SDL_UnlockSurface(surface);

		char savebuffer[280];

		for (int y = 0; y < size; y++) {		
			memset(savebuffer, 0, sizeof(savebuffer));
			for (int x = 0; x < size; x++) {
				savebuffer[x] = data[y][x];
			}
			fwrite(savebuffer, size, 1, fbin);
		}

		// let's see it		
		SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, &dst);
		SDL_RenderPresent(renderer);
		SDL_DestroyTexture(texture);

	}		
	fclose(fbin);
}

// creates text file with mask and specified number of rotations
void TestMask(char * filename, int size, int rotations, char * mask,char * textfn) {

	int buffersize = size * size*rotations;
	FILE * fmask = fopen(filename, "rb");
	fread(mask, buffersize, 1, fmask);
	fclose(fmask);

	fmask = fopen( textfn, "wt");
	for (int i = 0; i < rotations; i++) {
		fprintf(fmask, "Rotation %d\n", i);
		for (int y = 0; y < size; y++)
		{
			for (int x = 0; x < size; x++) {
				char b = (*mask++);
				fprintf(fmask, "%d", b);
			}
			fprintf(fmask, "\n");
		}
		fprintf(fmask, "\n");
	}
	fclose(fmask);
}

void TestPlayerShip(char * filename,char * textfn) {
	FILE * fmask=fopen( filename, "rb");
	fread(plmask, sizeof(plmask), sizeof(plmask),  fmask);
	fclose(fmask);

	fmask=fopen(textfn, "wt");
	for (int i = 0; i < 24; i++) {
		fprintf(fmask, "Rotation %d\n", i);
		for (int y = 0; y < 64; y++)
		{
			for (int x = 0; x < 64; x++) {
				char b = plmask[i][y][x];
				fprintf(fmask, "%d", b);
			}
			fprintf(fmask, "\n");
		}
		fprintf(fmask, "\n");
	}
	fclose(fmask);
}

void ProcessSurfaces() {
	processsurface(surfaceAlien, 64, "alien.msk",1);
	processsurface(2, 64,"playership.msk",24); // playership
	TestPlayerShip("playership.msk","pl.txt");
	processsurface(4, 280,"am1.msk",24);
	TestMask("am1.msk", 280, 24, (char *)&asteroidmask, "asteroid.txt");
	processsurface(5, 140, "am2.msk",24);
	processsurface(6, 70, "am3.msk",24);
	processsurface(7, 35, "am4.msk",24);
	processsurface(surfaceBULLET, 3, "bullet.msk",1);
}


// Initialize all setup, set screen mode, load images etc 
void InitSetup() {
	errorCount = 0;
	srand((int)time(NULL));
	SDL_Init(SDL_INIT_EVERYTHING );
	screen= SDL_CreateWindow("Asteroids", 100, 100, WIDTH, HEIGHT, 0);
	if (!screen) {
		LogError("InitSetup failed to create window");
	}
	renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	Loadsurfaces();
	ProcessSurfaces();

}

// Cleans up after game over 
void FinishOff() {
	IMG_Quit();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(screen);
	SDL_Quit();
}


int main(int argc,char * args[])
{
	startTimer(&stw);
	InitSetup();
	if (errorCount) {
		return -1;
	}

	FinishOff();
	stopTimer(&stw);

	printf("Elapse time = %4.2f\n", diff(&stw));
    return 0;
}
