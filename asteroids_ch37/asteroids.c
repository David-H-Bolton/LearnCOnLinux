// Chapter 37 Linux Add Game levels
#include "hr_time.h"
#include <time.h>
#include "SDL2/SDL.h"   // All SDL Applications need this 
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_mixer.h"
#include "SDL2/SDL_gamecontroller.h"
#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>
#include "lib.h"
#include "levels.h"

// Debugging 
//#define SHOWOVERLAP    // Leave Comment edthis out for normal play, uncomment to show collisions 

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
#define SCREENWIDTH 1024
#define SCREENHEIGHT 768
#define SHIPHEIGHT 64
#define SHIPWIDTH 64
#define EXPLOSIONSIZE 128
#define CELLSIZE 64
#define CHARWIDTH 13

// Define x and y restricted area
#define ZONESIZE 150
#define ZONELEFT (SCREENWIDTH-ZONESIZE)/2
#define ZONERIGHT ZONELEFT+ZONESIZE
#define ZONETOP (SCREENHEIGHT-ZONESIZE)/2
#define ZONEBOTTOM ZONETOP+ZONESIZE

// numbers of things
#define MAXASTEROIDS 32
#define MAXBULLETS 16
#define MAXPLAYERBULLETS 16
#define MAXEXPLOSIONS 256
#define NUMSOUNDS 4
#define NUMTEXTURES 14
#define NUMEXPLOSIONSOUNDS 2
#define NUMPOINTERS 273
#define CELLY SCREENHEIGHT/CELLSIZE
#define CELLX SCREENWIDTH/CELLSIZE
#define MAXTEXTSPRITES 10
#define MAXLEVELS 50


// Texture indices
#define PLBACKDROP 0
#define TEXTTEXTURE 1
#define TEXTUREPLAYERSHIP 2
#define TEXTUREDEBUG 3
#define TEXTUREASTEROID1 4  // 4,5,6,7
#define TEXTUREBULLET 8
#define TEXTURESMALLSHIP 9
#define TEXTUREEXPLOSION 10 // 10,11,12,13

// sound indices
#define THRUSTSOUND 0
#define CANNONSOUND 1
#define EXPLOSIONSOUND 2

// typedefs
typedef struct firstpart * pfirstpart;
typedef pfirstpart ptrarray[100]; // 100 pointers to a firstpart
typedef char * pchar;

// enums
enum ObjectType {tAsteroid,tBullet,tPlayer};

// Use this as a type- matches asteroid, bullet and playership
struct firstpart {
	SDL_Rect r;
	int type; 	
	int active;
	int rotdir;
};

struct pair { int x, y; };

// structs
struct asteroid {	
	SDL_Rect r;
	int type; // 0		
	int active;
	int rotdir;
	float x, y;
	float xvel, yvel;
	int xtime, ytime;
	int xcount, ycount;
	int rotclockwise;
	int rottime;
	int rotcount;
	int size;
};

struct bullet {	
	SDL_Rect r;
	int type; // 1
	int active;
	float x, y;
	int timer;
	float vx, vy;
	int ttl; // time to live
	int countdown;
	int playerbullet; // 0 = player, 1-3 = alien saucer
};

struct explosion {
	int x, y;
	int countdown;
	int frame;
	int dimension;
};

struct player {	
	SDL_Rect r;
	int type; // 2
	int active;	
	int dir; // 0-23 
	float x, y;
	float vx, vy;
	int lives;
} Player;

struct Cell {
	ptrarray ptrs;
	int numobjects;
};

struct TextSprite {
	int x, y;
	int active;
	char message[130];
	int textFactor;
};

// SDL variables
SDL_Window* screen = NULL;
SDL_Renderer *renderer;
SDL_Event event;
SDL_Rect source, destination, dst,sourceRect,destRect;
SDL_Texture* textures[NUMTEXTURES];
SDL_GameController * controller=NULL;

// int variables
int keypressed;
int rectCount = 0;
int frameCount,tickCount,lastTick, showFPS;
int score, paused,gameLevel,CloseFlag;
int debugFlag,fireFlag;
int numsprites;
int rotTimer,speedTimer, fireTimer, jumpTimer,pauseTimer, tempCount;
int rotateFlag, thrustFlag, jumpFlag, pauseFlag, controllerFlag;
int playerBulletCount,alienBulletCount;
int numCells,coolDown;
int rotateFlag; // 0 do nowt, 1 counter clockwise, 2 clockwise 
int numAsteroids = 0;
float speedfactor = 1;
int piFlag;
float lastTemp;
char * maskErrorFile;

// array variables
char timebuff[20];
char buffer[100],buffer2[100];
float thrustx[24];
float thrusty[24];
char bulletmask[1][3][3];
char plmask[24][64][64];
char a1mask[24][280][280];
char a2mask[24][140][140]; 
char a3mask[24][70][70];
char a4mask[24][35][35];


struct Cell cells[CELLX][CELLY];
struct pair cellList[CELLX*CELLY];

// struct variables
struct TextSprite sprites[MAXTEXTSPRITES];
struct asteroid asteroids[MAXASTEROIDS];
struct bullet bullets[MAXBULLETS];
struct explosion explosions[MAXEXPLOSIONS];
struct Mix_Chunk * sounds[NUMSOUNDS];
extern struct level levels[MAXLEVELS];

stopWatch s;

// external variables
extern int errorCount;
extern FILE * dbf;

// consts
const char * texturenames[NUMTEXTURES] = { "images/starfield.png","images/text.png","images/playership.png","images/debug.png",
"images/a1.png","images/a2.png","images/a3.png","images/a4.png","images/bullet.png","images/smallship.png","images/explosion0.png",
"images/explosion1.png","images/explosion2.png","images/explosion3.png" };

const char * soundnames[NUMSOUNDS] = { "sounds/thrust.wav","sounds/cannon.wav","sounds/explosion1.wav","sounds/explosion2.wav" };

const int explosionSizes[4] = { 128,128,128,64 };
const int sizes[4] = { 280,140,70,35 };
const int debugOffs[4] = { 0,280,420,490 };
const int bulletx[24] = { 32,38,50,64,66,67,68,67,66,64,50,38,32,26,16, 0,-2,-3,-4,-3,-2,0,16,24 };
const int bullety[24] = { -4,-3,-2, 0,16,26,32,38,50,64,66,67,71,70,69,64 ,50,38,32,26,16,0,-2,-3 };
const int xdir[8] = { 0,1,1,1,0,-1,-1,-1 };
const int ydir[8] = { -1,-1,0,1,1,1,0,-1 };

// external variables
extern int errorCount;

// Sets Window caption according to state - eg in debug mode or showing fps 
void SetCaption(char * msg) {
		if (showFPS) {
		snprintf(buffer,sizeof(buffer), "Fps = %d #Asteroids =%d %s", frameCount,numAsteroids,msg);
		SDL_SetWindowTitle(screen, buffer);
	}
	else {
		SDL_SetWindowTitle(screen, msg);
	}
}

// Loads an individual texture
SDL_Texture* LoadTexture(const char * afile, SDL_Renderer *ren) {
	SDL_Texture *texture = IMG_LoadTexture(ren, afile);
	if (texture == 0) {
		LogError2("Failed to load texture from ", afile);
	}
	return texture;
}

// Loads Textures 
void LoadTextures() {
	for (int i = 0; i<NUMTEXTURES; i++) {
		textures[i] = LoadTexture(texturenames[i], renderer);
	}
}

// Loads all sound files  
void LoadSoundFiles() {
	for (int i = 0; i<NUMSOUNDS; i++) {
		sounds[i] = Mix_LoadWAV(soundnames[i]);
	}
}

// returns a float with the temperature 
// Only call if running on a RaspI. Checks piFlag
float ReadPiTemperature() {
    char buffer[10]; 
	char * end; 
	if (!piFlag) return 0.0f;
	if (SDL_GetTicks() - tempCount < 1000) {	
		return lastTemp;
	}
	tempCount = SDL_GetTicks() ;
	FILE * temp = fopen("/sys/class/thermal/thermal_zone0/temp","rt"); 
	int numread = fread(buffer,10,1,temp); 
	fclose(temp); 
	lastTemp = strtol(buffer,&end,10)/1000.0f;
	return lastTemp;
}

// Sets piFlag to 1 if running on a Raspberry Pi
void SetPiFlag() {
	struct utsname buffer;
	piFlag=0;
    if (uname(&buffer) != 0) return;
	fprintf(stderr,"uname=%s",buffer.nodename);
    if (strcmp(buffer.nodename,"raspberrypi") !=0) return;
	if (strcmp(buffer.machine,"armv7l") != 0 &&
	   strcmp(buffer.machine,"aarch64") !=0) return;
	piFlag=1;
	tempCount = SDL_GetTicks();	
}

// Init thrustx and thrusty[] 
void InitThrustAngles() {
	const float pi = 3.14159265f;
	const float degreeToRad = pi / 180.0f;
	float degree = 0.0f;
	for (int i = 0; i<24; i++) {
		float radianValue = degree * degreeToRad;
		thrustx[i] = (float)sin(radianValue);
		thrusty[i] = (float)-cos(radianValue);
		degree += 15;
	}
}

// Unused
void processTexture(int texturenum, int size, char * filename) {
	SDL_Texture * targettexture = SDL_CreateTexture(renderer,
		SDL_PIXELFORMAT_ARGB8888, 
		//SDL_TEXTUREACCESS_STREAMING, 
		SDL_TEXTUREACCESS_TARGET,
		size, size);

	SDL_Rect src;
	src.w = size;
	src.h = size;		
	void * pixels;
	src.y = 0;
	int pitch,aw,ah;
	int access;
	for (int i = 0; i < 24; i++) {
		src.x = i * size;	
		SDL_SetRenderTarget(renderer, targettexture);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, textures[texturenum], &src, NULL);
		int success=SDL_QueryTexture(targettexture, NULL,&access, &aw, &ah);
		success=SDL_LockTexture(targettexture, NULL, &pixels, &pitch);
		Uint32 *upixels = (Uint32*)pixels;
		for (int y=0;y<size;y++)
			for (int x = 0; x < aw*ah; x++) {
				Uint32 p = *upixels;
			}
		SDL_SetRenderTarget(renderer, NULL);
		SDL_RenderCopy(renderer, targettexture, NULL, NULL);
		SDL_DestroyTexture(targettexture);
		SDL_RenderPresent(renderer);
		SDL_Delay(500);
	}
}

// Load a mask file containing number images of size * size
int LoadMask(char * filename, int size, int number, char * mask) {
	maskErrorFile = filename;
	int sizeofmask = size * size*number;
	FILE * fmask = fopen(filename, "rb");
	int numread = fread(mask, sizeofmask, 1, fmask);
	fclose(fmask);
	return numread == 1 ? 1 : 0;
}

// Load all masks if any return 0 then failed to load so exit
int LoadMasks() {
	maskErrorFile = NULL;
	if (!LoadMask("masks/bullet.msk", 3, 1, &bulletmask[0][0][0])) return 0; // playership
	if (!LoadMask("masks/playership.msk", 64, 24, &plmask[0][0][0])) return 0; // playership
	if (!LoadMask("masks/am1.msk", 280, 24, &a1mask[0][0][0])) return 0;
	if (!LoadMask("masks/am2.msk", 140, 24, &a2mask[0][0][0])) return 0;
	if (!LoadMask("masks/am3.msk", 70, 24, &a3mask[0][0][0])) return 0;
	return LoadMask("masks/am4.msk", 35, 24, &a4mask[0][0][0]);
}

// clears celllist and cells
void ClearCellList() {
	memset(cellList, 0, sizeof(cellList));
	numCells = 0;
	memset(cells, 0, sizeof(cells));
	//l("ClearCellList");
}

// Adds a pointer into a cell
void AddPointerToCell(int x, int y, pfirstpart objectptr) {
	int numobjects = cells[x][y].numobjects;
	if (!numobjects) { // 1st time cell added to so add to cellList
		cellList[numCells].x = x;
		cellList[numCells++].y = y;	
	}
	cells[x][y].ptrs[numobjects] = objectptr;
	cells[x][y].numobjects++;
	//ln("AddPointerToCell", "", numobjects);
}

// Adds a complete Object (asteroids, playership, bullet) pointer into each cell it covers
void AddObjectToCells(pfirstpart objectptr) {
	int x = objectptr->r.x;
	int y = objectptr->r.y;
	int h = objectptr->r.h;
	int w = objectptr->r.w;
	int cellx = x / CELLSIZE;
	int celly = y / CELLSIZE; 
	int endcellx = (x + w -1) / CELLSIZE;
	int endcelly = (y + h- 1) / CELLSIZE;
	// Range check cellx, endcellx,celly and endcelly
	if (endcellx >= CELLX) endcellx = CELLX-1;
	if (endcelly >= CELLY) endcelly = CELLY-1;
	if (cellx < 0) cellx = 0;
	if (celly < 0) celly = 0;
	for (int ix = cellx; ix <= endcellx; ix++)
		for (int iy = celly; iy <= endcelly; iy++)
			AddPointerToCell(ix, iy, objectptr);
}

// Search sprite list for active==0
int findFreeSprite() {
	for (int i = 0; i < MAXTEXTSPRITES; i++) {
		if (!sprites[i].active)
			return i;
	}
	return -1;
}

// This creates a Text sprite at the x,y coordinates
void AddTextSpriteAt(char * value, int x, int y, int textfactor) {
	int spriteindex = findFreeSprite();
	if (spriteindex == -1 || y<20) return;
	struct TextSprite * psprite = &sprites[spriteindex];
	psprite->x = x;
	psprite->y = y;
	psprite->active = 60;
	psprite->textFactor = textfactor;
	strncpy(psprite->message, value,sizeof(psprite->message) );
	numsprites++;
}

// returns 1 if gamecontroller attached
int HasGameController(){
	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
		if (SDL_IsGameController(i)) {
			controller = SDL_GameControllerOpen(i);
			if (controller) {
				return 1;
			}
		}
	}	
	return 0;
}
// This centres the text horizontally 
void AddTextSpriteString(char * value, int y, int textfactor) {
	int textlen = strlen(value) * CHARWIDTH * textfactor; // 13 = width of chars in set
	int x = (SCREENWIDTH - textlen) / 2;
	AddTextSpriteAt(value, x, y, textfactor);
}

// This centres it but also converts int to str
void AddTextSpriteInt(int value, int y, int textfactor) {
	AddTextSpriteString(SDL_ltoa(value,buffer,10), y, textfactor);
}

// Initialize all setup, set screen mode, load images etc 
void InitSetup() {
	errorCount = 0;
	CloseFlag = 0;
    SetPiFlag();
	InitThrustAngles();
	controllerFlag =0;
	srand((int)time(NULL));
	SDL_Init(SDL_INIT_EVERYTHING );
	screen= SDL_CreateWindow("Asteroids", 100, 100,SCREENWIDTH, SCREENHEIGHT, 0);
	if (!screen) {
		LogError("InitSetup failed to create window");
	}
	renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	LoadTextures();
	int success=Mix_OpenAudio(44000, AUDIO_F32LSB, 2, 8192);
	if (success==-1 ) {

		LogError2("InitSetup failed to init audio because %s",Mix_GetError());
	}
	LoadSoundFiles();
	if (!LoadMasks()) {
      LogError2("InitSetup failed to load mask number:",maskErrorFile);
	}
	if (SDL_GameControllerAddMapping("030000001008000001e5000010010000,usb gamepad,platform:Linux,a:b2,b:b1,x:b3,y:b0,back:b8,start:b9,leftshoulder:b4,rightshoulder:b5,leftx:a0,lefty:a1,")==-1){
		LogError("Unable to use gamepad mappings from gamepad.txt");
	}	
	InitThrustAngles();	
	controllerFlag = HasGameController();
}

// Free memory allocated for .wav files
void DestroySounds() {
	for (int i = NUMSOUNDS - 1; i >= 0; i--) {
		Mix_FreeChunk(sounds[i]);
	}
}

// release memory for textures
void DestroyTextures() {
	for (int i = NUMTEXTURES - 1; i >= 0; i--) {
		SDL_DestroyTexture(textures[i]);
	}
}

// Cleans up after game over 
void FinishOff() {
	DestroySounds();
	DestroyTextures();
	Mix_CloseAudio();
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(screen);
	if (controllerFlag) {
		SDL_GameControllerClose(controller);
	}
	SDL_Quit();
	exit(0);
}

// Plays a sound from sounds[]
void PlayASound(int soundindex) {
	int success=Mix_PlayChannel(-1, sounds[soundindex], 0);
/* 	if (success == -1) {
		LogError2("InitSetup failed to init audio because %s",Mix_GetError());
	} */
}

void RenderTexture(SDL_Texture *tex, int x, int y) {
	//Setup the destination rectangle to be at the position we want
	dst.x = x;
	dst.y = y;
	//Query the texture to get its SCREENWIDTH and SCREENHEIGHT to use
	SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
	SDL_RenderCopy(renderer, tex, NULL, &dst);
}

// print single char c at rect target 
void printch(char c, SDL_Rect * target, int textFactor) {
	int start = (c - '!');
	if (c != ' ') {
		sourceRect.h = 23;
		sourceRect.w = 12;
		sourceRect.x = start * 12;
		sourceRect.y = 0;
		SDL_RenderCopy(renderer, textures[TEXTTEXTURE], &sourceRect, target);
	}
	(*target).x += (CHARWIDTH*textFactor); // in this bitmap font, chars are CHARWIDTH pixels width
}

// print string text at x,y pixel coords 
void TextAt(int atX, int atY, char * msg,int textFactor) {
	destRect.h = 23*textFactor;
	destRect.w = 12*textFactor;
	destRect.x = atX;
	destRect.y = atY;
	for (int i = 0; i < (int)strlen(msg); i++) {
		printch(msg[i], &destRect,textFactor);
	}
}

void UpdateCaption()
{
	float temp = ReadPiTemperature();
	snprintf(buffer2, sizeof(buffer2), "%10.6f Pi=%d Asteroids =%d temp=%5.2f C", diff(&s),piFlag,numAsteroids,temp);
	tickCount = SDL_GetTicks();
	if (tickCount - lastTick >= 1000) {
		lastTick = tickCount;
		if (showFPS) {
			SetCaption(buffer2);
			frameCount = 0;
		}
	}
	else if (!showFPS) {
		SetCaption(buffer2);
	}
}

// Draw player ship 
void DrawPlayerShip() {
	if (!Player.active) return;

	SDL_Rect spriterect, target;
	char buff[30];

	target.h = SHIPHEIGHT;
	target.w = SHIPWIDTH;	
	target.x = (int)Player.x;
	target.y = (int)Player.y;
	spriterect.h = SHIPHEIGHT;
	spriterect.w = SHIPWIDTH;
	spriterect.x = Player.dir*SHIPWIDTH;
	spriterect.y = 0;

	SDL_RenderCopy(renderer, textures[TEXTUREPLAYERSHIP], &spriterect, &target);
	if (!debugFlag) return;
	// code after here is only run if debug !=0 
	target.w = 64;
	target.h = 64;

	spriterect.x = 280;
	spriterect.y = 140;
	spriterect.w = 66;
	spriterect.h = 66;	
	SDL_RenderCopy(renderer, textures[TEXTUREDEBUG], &spriterect, &target);

	snprintf(buff,sizeof(buff), "(%6.4f,%6.4f) Dir= %i", Player.x, Player.y, Player.dir);
	TextAt((int)Player.x - 50, (int)Player.y + 66, buff,1);
	snprintf(buff, sizeof(buff), "(%6.4f,%6.4f)", Player.vx, Player.vy);
	TextAt((int)Player.x - 50, (int)Player.y + 90, buff,1);
}

// Draw All asteroids 
void DrawAsteroids() {
	SDL_Rect asteroidRect, target;
	numAsteroids = 0;
	asteroidRect.y = 0;
	for (int i = 0; i<MAXASTEROIDS; i++) {
		if (asteroids[i].active==1) {
			numAsteroids++;                    // keep track of how many onscreen
			int sizeIndex = asteroids[i].size; // 0-3
			int asize = sizes[sizeIndex];      // asteroid size 280,140, 70,35
			target.h = asize;
			target.w = asize;
			asteroidRect.h = asize;
			asteroidRect.w = asize;
			asteroidRect.x = asize * asteroids[i].rotdir;
			asteroidRect.y = 0;
			if (asteroids[i].rotdir >= 12) { // Adjust for 2nd row
			   asteroidRect.y = asize;
			   asteroidRect.x -= 12*asize;
			}
			target.x = (int)asteroids[i].x;
			target.y = (int)asteroids[i].y;
			SDL_RenderCopy(renderer, textures[TEXTUREASTEROID1 + sizeIndex], &asteroidRect, &target);
			if (!debugFlag)
				continue;
			asteroidRect.x = debugOffs[sizeIndex];
			SDL_RenderCopy(renderer, textures[TEXTUREDEBUG], &asteroidRect, &target);	
			char buff[30];
			TextAt(target.x + 25, target.y + 25, SDL_ltoa(i,buff,10),1);
			TextAt(target.x + 25, target.y + 55, SDL_ltoa(sizeIndex, buff, 10),1);
		}
	}
}

// Draw all active > 1 bullets with ttl > 0 
void DrawBullets() {
	char buff[10];
	SDL_Rect spriterect, target;
	target.h = 3;
	target.w = 3;
	spriterect.h = 3;
	spriterect.w = 3;
	spriterect.x = 0;
	spriterect.y = 0;
	for (int i = 0; i<MAXBULLETS; i++) {
		if (bullets[i].active && bullets[i].ttl >0) {
			target.x = (int)bullets[i].x;
			target.y = (int)bullets[i].y;
			SDL_RenderCopy(renderer,textures[TEXTUREBULLET], &spriterect, &target);
			if (debugFlag) {
				snprintf(buff, 10,"%i", bullets[i].ttl);
				TextAt((int)bullets[i].x + 5, (int)bullets[i].y, buff,1);
			}
		}
	}
}

// Draw all explosions with frame > 0 
void DrawExplosions() {
	SDL_Rect spriterect, target;
	spriterect.y = 0;
	for (int i = 0; i<MAXEXPLOSIONS; i++) {
		if (explosions[i].frame != -1) {
			int dimension = explosions[i].dimension;  // 0..3
			target.x = explosions[i].x- EXPLOSIONSIZE / 2;// adjustment so explosion matches asteroid location
			target.y = explosions[i].y - EXPLOSIONSIZE / 2;
			target.h = EXPLOSIONSIZE;
			target.w = EXPLOSIONSIZE;
			spriterect.h = EXPLOSIONSIZE;
			spriterect.w = EXPLOSIONSIZE;
			int frame = explosions[i].frame;
			spriterect.y = (frame / 8) * EXPLOSIONSIZE;
			spriterect.x = (frame % 8) * EXPLOSIONSIZE;
			SDL_RenderCopy(renderer,textures[TEXTUREEXPLOSION+dimension], &spriterect, &target);
			if (debugFlag) {	
				char buff[15];
				snprintf(buff,sizeof(buff), "X %i", explosions[i].frame);
				TextAt(target.x + 10, target.y+ EXPLOSIONSIZE, buff,1);
			}
		}
	}
}

// Show score and lives on screen 
void DrawScoreAndLives() {
	char scorebuffer[10];
	SDL_Rect destr;
	snprintf(scorebuffer,sizeof(scorebuffer), "%i", score);
	TextAt(25, SCREENHEIGHT-30, scorebuffer,1);
	destr.h = 33;
	destr.w = 24;
	destr.y = SCREENHEIGHT - 70;
	for (int i = 0; i < Player.lives; i++) {
		destr.x = (i * 30) + 50;
		SDL_RenderCopy(renderer, textures[TEXTURESMALLSHIP],NULL, &destr);
	}
}

// Draw Paused if paused == 1 flashing at twice per second
void DrawPauseMessage(char * message) {
	if (paused == 0) return;
	if (frameCount < 16 || frameCount > 45) {
		TextAt((SCREENWIDTH / 2) - 20, (SCREENHEIGHT / 2) - 40, message,2);
	}
}

// Draw any active text sprites
void DrawTextSprites() {
	for (int i = 0; i < MAXTEXTSPRITES;i++) {
		struct TextSprite * psprite = &sprites[i];
		if (psprite->active) {
			TextAt(psprite->x, psprite->y--, psprite->message,psprite->textFactor);
			psprite->active--;
			if (!psprite->y) {
				psprite->active = 0;
			}
		}
	}
}

// Copies Asteroids, playership, bullets etc to offscreen buffer
void DrawEverything() {	
	RenderTexture(textures[PLBACKDROP], 0, 0);
	DrawAsteroids();
	DrawPlayerShip();
	DrawBullets();
	DrawExplosions();
	DrawScoreAndLives();
	DrawTextSprites();
	DrawPauseMessage("Paused");
	stopTimer(&s);	
	UpdateCaption();	
}

// Copies Asteroids, playership, bullets etc to offscreen buffer
void RenderEverything() {
	SDL_RenderPresent(renderer); // make offscreen buffer vsisble
	frameCount++;
	if (frameCount == 60) frameCount = 0;
}

// Draw these at the start of a level
void DrawLevelStart() {
	char buffer[30];
	snprintf(buffer, sizeof(buffer), "Level %d", gameLevel+1);
	AddTextSpriteString(buffer,380,3);
	
	switch (Player.lives) {
		case 1:snprintf(buffer, sizeof(buffer), "Last Life!");
			break;
		case 2:
		case 3:snprintf(buffer, sizeof(buffer), "Lives left: %d", Player.lives);
			break;
		}
	AddTextSpriteString(buffer, 440, 3);
}

// Initialize Player struct 
void InitPlayer(int numlives) {
	Player.type = tPlayer;
	Player.dir = 0;
	Player.vy = 0;
	Player.vy = 0;
	Player.lives = numlives;
	Player.x = 500;
	Player.y = 360;
	Player.r.h = 64;
	Player.r.w = 64;
	Player.vx = 0.0f;
	Player.vy = 0.0f;
	Player.active = 1;
}

// Initialize all asteroid variables 
void InitAsteroids() {
	for (int i = 0; i<MAXASTEROIDS; i++) {
		asteroids[i].active = 0;
	}
}

// Init all MAXBULLETS 
void InitBullets() {
	for (int i = 0; i< MAXBULLETS; i++) {
		struct bullet * pbullet = &bullets[i];
		pbullet->active = 0;
		pbullet->x = 0;
		pbullet->y = 0;
		pbullet->timer = 0;
		pbullet->ttl = 0;
		pbullet->countdown = 0;
		pbullet->r.h = 3;
		pbullet->r.w = 3;
		pbullet->type = tBullet;
		pbullet->playerbullet = -1; // 0= player, 1-3 = alien
	}
}

// Init all Explosions 
void InitExplosions() {
	for (int i = 0; i< MAXEXPLOSIONS; i++) {
		explosions[i].x = 0;
		explosions[i].y = 0;
		explosions[i].countdown = 0;
		explosions[i].frame = -1;
	}
}

// Clear sprites array
void InitTextSprites() {
	memset(sprites, 0, sizeof(sprites));
	numsprites = 0;
}

// Called to initialise each game
void InitGame() {
	rotTimer = SDL_GetTicks();
	speedTimer = rotTimer;
	fireTimer = rotTimer;
	jumpTimer = rotTimer;
	pauseTimer = rotTimer;
	score = 0;
	paused = 0;
	coolDown = 0;
	InitPlayer(3); // pass in number lives
	// All other Initialisation done in ReInitGame called from InitLevel
}

// Alter Player.dir according to rotation key pressed
void RotatePlayerShip() {
	if (rotateFlag && (SDL_GetTicks() - rotTimer > 40)) {
		rotTimer = SDL_GetTicks();
		if (rotateFlag == 1) // CounterClockwise 
		{
			Player.dir += 23;
			Player.dir %= 24;
		}
		else
			if (rotateFlag == 2) // Clockwise
			{
				Player.dir++;
				Player.dir %= 24;
			}
	}
}

//move the player ship 
void MovePlayerShip() {
	if (!Player.active) return;
	Player.x += Player.vx;
	Player.y += Player.vy;
	if (Player.x <= -5)
		Player.x += SCREENWIDTH;
	else
		if (Player.x > SCREENWIDTH-4)
			Player.x = 0;
	if (Player.y <= -5)
		Player.y += SCREENHEIGHT;
	else
		if (Player.y > SCREENHEIGHT-4)
			Player.y = 0;
	Player.r.y = (int)Player.y;
	Player.r.x = (int)Player.x;
	//l("Adding pointer for player");
	AddObjectToCells((pfirstpart)&Player);
}

// applies thrust
void ApplyThrust() {
	if (thrustFlag && (SDL_GetTicks() - speedTimer > 40)) {
		speedTimer = SDL_GetTicks();
		Player.vx += (thrustx[Player.dir] / 3.0f);
		Player.vy += (thrusty[Player.dir] / 3.0f);
		PlayASound(THRUSTSOUND);
	}
}

// Move an Asteroid 
void MoveAsteroid(int index) {
	if (!asteroids[index].active)
		return;
	struct asteroid * past = &asteroids[index];
	int size = sizes[past->size]; // used later so fetch once!

	// do the rotation
	if (past->rottime > 0) {
		past->rottime--;
		if (past->rottime == 0)
		{
			past->rottime = past->rotcount;
			if (past->rotclockwise) {
				past->rotdir = (past->rotdir + 1) % 24;
			}
			else {
				past->rotdir = ((past->rotdir - 1) + 24) % 24;
			}
		}
	}

	// movement along x axis
	if (past->xtime > 0) {
		past->xtime--;
		if (past->xtime <= 0) {
			past->x += past->xvel;
			past->r.x = (int)past->x;
			past->xtime = past->xcount;
			if (past->x < -size) { // off left edge, on right
				past->x += SCREENWIDTH;
			}
			else if (past->x > SCREENWIDTH) { // off right edge, on left
				past->x -= SCREENWIDTH;
			}
		}
	}
	
	// movement along y axis
	if (past->ytime > 0) {
		past->ytime--;
		if (past->ytime == 0) {
			past->y += past->yvel;
			past->r.y = (int)past->y;
			past->ytime = past->ycount;
			if (past->y < -size) {
				past->y += SCREENHEIGHT;
			}
			else {
				asteroids[index].rotdir = ((asteroids[index].rotdir - 1) + 24) % 24;
			}
		}
	}
	// a check in case the asteroid now has active equals 0
	if (past->active > 0) {
		//l2("Adding pointer for asteroids", SDL_ltoa(index,buffer,10));
		AddObjectToCells((pfirstpart)past);
	}
}

// Move All Asteroids
void MoveAsteroids() {
	for (int i = 0; i<MAXASTEROIDS; i++) {
		{
			if (asteroids[i].active) {
				MoveAsteroid(i);
				}
			}
		}
	}

// return index or -1 if not found 
int FindFreeAsteroidSlot() {
	for (int i = 0; i< MAXASTEROIDS; i++) {
		if (!asteroids[i].active) {
			return i;
		}
	}
	return -1;
}

// Checks target x,y coords have nothing in cells below
int IsEmptySpace(int x, int y) {
	if (x<0 || y < 0 || x >SCREENWIDTH || y > SCREENHEIGHT) {
		return 0;
	}
	int celly = y / CELLSIZE;
	int cellx = x / CELLSIZE;
	if (cells[cellx][celly].numobjects == 0 && cells[cellx + 1][celly].numobjects == 0
		&& cells[cellx][celly + 1].numobjects == 0 && cells[cellx + 1][celly + 1].numobjects == 0) return 1;
	return 0;
}

// return 1 if x,y is in a central zone defined by four Zone variables
int InCentre(int x, int y) {
	return ((x < ZONELEFT || x > ZONERIGHT) && (y < ZONETOP || y > ZONEBOTTOM)) ? 0 : 1;
}

// adds to table , size = 0..3
void AddAsteroid(int size) {
	int index = FindFreeAsteroidSlot();
	if (index == -1) // table full so quietly exit 
		return;
	if (size == -1) {  // Use -1 to sepcify a random size
		size = Random(4) - 1;
	}
	int asize = sizes[size];
	struct asteroid * past = &asteroids[index];
	past->active = 1;
	do {
		past->x = (float)Random(SCREENWIDTH - asize) + asize*2;
		past->y = (float)Random(SCREENHEIGHT - asize) + asize*2;
		// Prevent asteroids appearing in a 150 x 150 square in the centre. Otherwise player ship can get blown up immediately!
		if (InCentre((int)past->x, (int)past->y)) continue;
		if (IsEmptySpace((int)past->x, (int)past->y)) break;
	} while (1);
	past->type = tAsteroid;
	past->r.y = (int)past->y;
	past->r.x = (int)past->x;
	past->rotdir = Random(24);
	past->rotclockwise = Random(2) - 1;
	past->xvel = (float)4 - Random(8);
	past->yvel = (float)4 - Random(8);
	past->xtime = 2 + Random(5);
	past->xcount = past->xtime;
	past->ytime = 2 + Random(5);
	past->ycount = past->ytime;
	past->rottime = 2 + Random(8);
	past->rotcount = past->rottime;
	past->size = size;
	past->r.w = sizes[size];
	past->r.h = sizes[size];
}

// add Explosion to table
void AddExplosion(int x, int y) {
	int index = -1;
	for (int i = 0; i<MAXEXPLOSIONS; i++) {
		if (explosions[i].frame == -1) {
			index = i;
			break;
		}
	}
	if (index == -1) return;
	PlayASound(EXPLOSIONSOUND + Random(NUMEXPLOSIONSOUNDS-1));
	explosions[index].frame = 0;
	explosions[index].x = x;
	explosions[index].y = y;
	explosions[index].countdown = 1;
	explosions[index].dimension = Random(4) - 1; // 0..3
}

// DestroyAsteroid 2 from pointer, create 4 smaller plus 4 smallest (size 3 )
void DestroyThisAsteroid(struct asteroid * a) {
	a->active = 0;
	AddExplosion((int)a->x, (int)a->y);
	int size = a->size;
	if (size == 3)
		return; // it's the smallest size so just destroy it, and exit 
				// otherwise add in 4 smaller and 4 smallest type asteroids
	float xvel = a->xvel;
	float yvel = a->yvel;
	int x = (int)a->x;
	int y = (int)a->y;
	for (int i = 0; i<8; i++) {
		int index = FindFreeAsteroidSlot();
		if (index == -1)
			return; // no more asteroid slots free so don't add any (rare!)
		int  newsize;
		if (i % 2 == 0) // if i is even add in smallest size 3 
			newsize = 3;
		else
			newsize = size + 1; // next smaller size 

		struct asteroid * past = &asteroids[index];
		past->active = 1;
		past->rotdir = 0;
		past->rotclockwise = Random(2) - 1;
		past->xvel = (xdir[i] * Random(3) + xvel)*speedfactor;
		past->yvel = (ydir[i] * Random(3) + yvel)*speedfactor;
		past->xtime = 2 + Random(5);
		past->xcount = past->xtime;
		past->ytime = 2 + Random(5);
		past->ycount = past->ytime;
		past->rottime = 2 + Random(8);
		past->rotcount = past->rottime;

		newsize = 3; // smallest
		if (i % 2 == 1) // if i is odd add in next smaller size
			newsize = size + 1; 
		past->size = newsize;
		past->r.w = sizes[newsize];
		past->r.h = sizes[newsize];
		past->x = (float)xdir[i] * (sizes[newsize]*2/3) + x;
		past->y = (float)ydir[i] * (sizes[newsize]*2/3) + y;
		past->r.y = (int)past->y;
		past->r.x = (int)past->x;
	}
}

// DestroyAsteroid 1 from index, create 4 smaller plus 4 size 3 
void DestroyAsteroid(int deadIndex) {
	struct asteroid * a = &asteroids[deadIndex];
	AddExplosion(a->r.x + a->r.w / 2, a->r.y + a->r.h / 2); 
	DestroyThisAsteroid(a);
}

// test code 
void BlowUpAsteroids() {
	for (int i = 0; i<MAXASTEROIDS; i++) {
		if (asteroids[i].active && Random(10) <4) {
			DestroyAsteroid(i);
		}
	}
}

// Use global variables playerbulletcount and alienbulletcount to cap firing
void CountBullets() {
	alienBulletCount = 0;
	playerBulletCount = 0;
	for (int i = 0; i < MAXBULLETS; i++) {
		struct bullet * pbullet = &bullets[i];
		if (pbullet->active) {
			switch (pbullet->playerbullet) {
			case -1: continue;
			case 0: playerBulletCount++;
				break;
			case 1:
			case 2:
			case 3:
				alienBulletCount++;
				break;
			}
		}
	}
}

// fire a bullet- first find a slot and then work out where it should appear relative to player ship then add to bullets
void DoFireBullet() {
	if (playerBulletCount == MAXPLAYERBULLETS) return;
	int index = -1;
	for (int i = 0; i<MAXBULLETS; i++) {
		if (bullets[i].active == 0) { // found a slot 
			index = i;
			break;
		}
	}	
	if (index == -1) return; // no free slots as all bullets in play
	PlayASound(CANNONSOUND);

	// This starts the bullet at the nose, irrespective of rotation. 
	int x = (int)round(Player.x + bulletx[Player.dir]);
	int y = (int)round(Player.y + bullety[Player.dir]);

	struct bullet * pbullet = &bullets[index];
	pbullet->active = 1;
	pbullet->type = tBullet;
	pbullet->ttl = 120;
	pbullet->x = x * 1.0f;
	pbullet->y = y * 1.0f;
	pbullet->r.x = (int)x;
	pbullet->r.y = (int)y;
	pbullet->r.h = 3;
	pbullet->r.w = 3;
	pbullet->timer = 3;
	pbullet->countdown = 1;
	pbullet->vx = Player.vx + thrustx[Player.dir] * 5;
	pbullet->vy = Player.vy + thrusty[Player.dir] * 5;
	pbullet->playerbullet = 1;
}

// move bullets * 
void MoveBullets() {
	for (int i = 0; i< MAXBULLETS; i++) {
		struct bullet * pbullet = &bullets[i];
		if (pbullet->active && pbullet->countdown >0) {
			pbullet->countdown--;
			if (pbullet->countdown == 0) {
				if (pbullet->ttl >0)
				{
					pbullet->ttl--;
					if (pbullet->ttl == 0)
					{
						pbullet->active=0;
						continue; // expired, onto next bullet
					}
				}
				// move it 
				pbullet->countdown = pbullet->timer;
				float vx = pbullet->x + pbullet->vx;
				float vy = pbullet->y + pbullet->vy;
				if (vx <= -2) // check for screen edges 
					vx += 1024;
				else
					if (vx > 1022)
						vx = 0;
				if (vy <= -2)
					vy += 768;
				else
					if (vy > 766)
						vy = 0;
				pbullet->x = vx;
				pbullet->y = vy;
				pbullet->r.x = (int)pbullet->x;
				pbullet->r.y = (int)pbullet->y;
			}
			// even though a bullet doesn't move every frame, it still has to be added in to a cell every frame
			//l2("Adding pointer bullets", SDL_ltoa(i,buffer,10));
			AddObjectToCells((pfirstpart)&bullets[i]);
		}
	}
}

// Cycle Explosions through all the phases 
void CycleExplosions() {
	for (int i = 0; i<MAXEXPLOSIONS; i++) {
		if (explosions[i].frame >-1) {
			if (explosions[i].countdown>0) {
				explosions[i].countdown--;
				if (explosions[i].countdown == 0) {
					explosions[i].frame++;
					if (explosions[i].frame == 64) {
						explosions[i].frame = -1; // bye bye bang 
						continue;
					}
					explosions[i].countdown = 1;
				}
			}
		}
	}
}

// Handle all key presses
int ProcessEvents() {
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_CONTROLLERBUTTONDOWN:
			if (event.cbutton.state== SDL_PRESSED){
				switch(event.cbutton.button){
					case SDL_CONTROLLER_BUTTON_A:
						fireFlag =1;
						break;
					case SDL_CONTROLLER_BUTTON_B:						
					    jumpFlag =1;
					    break;
					case SDL_CONTROLLER_BUTTON_X:
						//shieldFlag =1;
						break;
					case SDL_CONTROLLER_BUTTON_Y:						
					    thrustFlag =1;
					    break;						
					case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
						rotateFlag = 1;
						break;
					case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
						rotateFlag= 2;				
						break;
					}
				}
			case SDL_CONTROLLERBUTTONUP:
				if (event.cbutton.state== SDL_RELEASED){
				switch(event.cbutton.button){							
					case SDL_CONTROLLER_BUTTON_A:
						fireFlag =0;
						break;
					case SDL_CONTROLLER_BUTTON_B:
						jumpFlag =0;
						break;		
					case SDL_CONTROLLER_BUTTON_X:
						//shieldFlag =0;
						break;
					case SDL_CONTROLLER_BUTTON_Y:						
					    thrustFlag =0;
					    break;											
					case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
					case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
						rotateFlag=0;				
						break;
					}
			}
			case SDL_KEYDOWN:
				keypressed = event.key.keysym.sym;
				switch (keypressed) {
				case QUITKEY:
					return 0;
				case BOMBKEY:
					BlowUpAsteroids();
					break;
				case COUNTERCLOCKWISEKEY:
					rotateFlag = 1;
					break;
				case CLOCKWISEKEY:
					rotateFlag = 2;
					break;
				case DEBUGKEY:
					debugFlag = 1 - debugFlag;
					showFPS = 1 - showFPS;
					break;
				case JUMPKEY:
					jumpFlag = 1;
					break;
				case PAUSEKEY:
					pauseFlag = 1;
					break;
				case THRUSTKEY:
					thrustFlag = 1;
					break;
				case ADDASTEROIDKEY:
					for (int i = 0; i < 1; i++) {
						AddAsteroid(0);
					}
					break;
				case FIREKEY:
					fireFlag = 1;
					break;
				} // switch keypressed
				break;
			case SDL_QUIT: // if mouse click to close window 
			{
				return 0;
			}

			case SDL_KEYUP: {
				rotateFlag = 0;
				thrustFlag = 0;
				fireFlag = 0;
				pauseFlag = 0;
				jumpFlag = 0;
				break;
			}
		} // switch event.type	
	} // while
	return 1;
}

void CheckBulletsFired() {
	if (fireFlag && SDL_GetTicks() - fireTimer > 100) {
		fireTimer = SDL_GetTicks();
		DoFireBullet();
	}
}

void CheckPause() {
	if (pauseFlag && SDL_GetTicks() - pauseTimer > 100) {
		pauseTimer = SDL_GetTicks();
		paused = 1 - paused;
	}
}

// doHyperJump() find empty space on screen fo PlayerShip
void CheckJump() {
	int hx, hy;

	if (jumpFlag && SDL_GetTicks() - jumpTimer > 3000) { 
		jumpTimer = SDL_GetTicks();
		jumpFlag = 0;
		do {
			hx = 65 + Random(SCREENWIDTH - 130);
			hy = 65 + Random(SCREENHEIGHT - 130);
			if (IsEmptySpace(hx, hy)) break;
		} while (1);
		Player.x = (float)hx;
		Player.y = (float)hy;
		Player.vx = 0;
		Player.vy = 0;
		Player.dir = 0;
	}
}

// Called when player loses a life or clears Level
void ReInitGame() {
	InitPlayer(Player.lives);
	InitThrustAngles();
	InitAsteroids();
	InitBullets();
	InitExplosions();
	InitTextSprites();
}

// Initialises each level
void InitLevel(int glevel) {	
	ReInitGame(); // Clear BEATT Bullets Explosions Asteroids Thrust and Text
	struct level * thislevel = &levels[glevel];
	for (int sizeindex = 0; sizeindex <4; sizeindex++) {
		int numtoadd=thislevel->nums[sizeindex];
		if (numtoadd) {
			for (int aindex = 0; aindex < numtoadd; aindex++) {
				AddAsteroid(sizeindex);
			}
		}
	}
	speedfactor = thislevel->factor;
	// for (int alienindex = 0; alienindex < thislevel->aliens; alienindex++) {
	//	AddAlienShip();
	//};
	DrawLevelStart();
}

// Something got destroyed- but what?
void DestroyObject(pfirstpart object) {
	switch (object->type) {
	case tAsteroid: // asteroid
		DestroyThisAsteroid((struct asteroid *)object); // cast back from firstpart to...
		break;
	case tBullet: // bullet
		object->active = 0;
		break;
	case tPlayer: 
		object->active=0;
		coolDown = 150;
		if (Player.lives == 1) {
			AddTextSpriteString("Game over!", (SCREENHEIGHT - 20) / 2, 3);
		}
		else {
			AddTextSpriteString("Your ship destroyed!", (SCREENHEIGHT - 20) / 2, 3);
		}
		break;
	}
}

// Returns a pointer to the top left char of the correct mask depending on object type
pchar GetMask(int type, int rotation, int size) {
	switch (type) {
		case tAsteroid: // asteroid
		{
			switch (size)
			{
			case 280:
				return (pchar)&a1mask[rotation];
			case 140:
				return (pchar)&a2mask[rotation];
			case 70:
				return (pchar)&a3mask[rotation];
			case 35:
				return (pchar)&a4mask[rotation];
			}
		};
		case tBullet: // bullet
			return (pchar)&bulletmask;
		case tPlayer: // player
			return (pchar)&plmask[rotation];
	}
	return 0; // null - should never get here!
}

// Checks to see if two objects that has SDL_IntersectRected'd actually touch.
int Overlap(pfirstpart object1, pfirstpart object2, SDL_Rect * rect,int * bangx,int * bangy) {
	#ifdef SHOWOVERLAP
	  SDL_SetRenderDrawColor(renderer, 0, 0xff, 0, SDL_ALPHA_OPAQUE);
	#endif	
	int y = rect->y;
	int x = rect->x;
	int w = rect->w;
	int h = rect->h;
	int y1 = object1->r.y;
	int x1 = object1->r.x;
	int y2 = object2->r.y;
	int x2 = object2->r.x;
	int dir1 = object1->type == tBullet ? 0 : object1->rotdir;
	int dir2 = object2->type == tBullet ? 0 : object2->rotdir;
	int size1 = object1->r.h;
	int size2 = object2->r.h;
	pchar pm1 = GetMask(object1->type, object1->rotdir, size1);
	pchar pm2 = GetMask(object2->type, object2->rotdir, size2);

	int oy1 = y - y1; // determines offset into object (and mask) where the bounding boxes touch
	int oy2 = y - y2;
	int ox1 = x - x1;
	int ox2 = x - x2;

	pm1 += (oy1	*size1) + ox1; // adjust mask 1 pointer to point into the mask where the bounding boxes touch top left corner
	pm2 += (oy2	*size2) + ox2; // Same for object 2/mask 2
	for (int iy = 0; iy < h; iy++)
	{
		pchar pl1 = pm1;
		pchar pl2 = pm2;
		*bangy = iy + y;
		for (int ix = 0; ix < w; ix++) {
			*bangx = ix + x; // Sets the location of explosion (if it happens)
			if (*pl1++ & *pl2++) {
				#ifdef SHOWOVERLAP
				if (object1->type == tAsteroid || object2->type == tAsteroid) { // comment this out- it'll show all overlaps but run very very slowly
				  SDL_RenderDrawPoint(renderer, *bangx, *bangy);			  
				}			
				#else
				  return 1; //only if SHOWOVERLAP is not defined
				#endif
			}
		}
		pm1 += size1; // move mask pointers down by one row
		pm2 += size2;
	}
	return 0;  // missed!
}

// Double loop to see if any pairs of objects in this overlap.
void CheckAllObjects(int x, int y) {
	struct Cell * c = &cells[x][y];
	SDL_Rect rect;
	int bangy, bangx;
	for (int index1=0;index1<c->numobjects;index1++)
		for (int index2 = index1 + 1; index2 < c->numobjects; index2++) {
			if (c->ptrs[index1]->active && c->ptrs[index2]->active &&
				SDL_IntersectRect(&c->ptrs[index1]->r, &c->ptrs[index2]->r, &rect)) {
				if (Overlap(c->ptrs[index1], c->ptrs[index2], &rect,&bangx,&bangy)) {
					AddExplosion(bangx, bangy);
					if ((c->ptrs[index1]->type == tAsteroid && c->ptrs[index2]->type == tBullet) ||
						(c->ptrs[index2]->type == tAsteroid && c->ptrs[index1]->type == tBullet)) {
						if (!coolDown) { // no score if player ship killed in this frame or after!
							score += 50;
							AddTextSpriteAt("50", bangx, bangy - 20, 2);
						}
					}
					DestroyObject(c->ptrs[index1]);
					DestroyObject(c->ptrs[index2]);
				}
			}
		}
}

// Look through celllist for 2 or more objects.
void CheckCollisions() {
	for (int i = 0; i < numCells; i++) {
		int x = cellList[i].x;
		int y = cellList[i].y;
		if (cells[x][y].numobjects > 1) {
			CheckAllObjects(x, y);
		}
  }
}

// After playerShip hit, calls this for 150 frames CoolDown 
// it just lets thing run a little rather than abruptly stopping and lets the 
// Your Ship Destroyed/Game over messages have time to fully display
int DoCoolDown()
{
	if (coolDown == 0) return 0;
	coolDown--;
	if (coolDown > 0) return 0;
	Player.lives--;
	return 1;
}

// Shows cells on screen with number of objects - debug only
void ShowCells() {
	SDL_SetRenderDrawColor(renderer, 0,0xff, 0, SDL_ALPHA_OPAQUE);
	for (int y = 0; y <= CELLY; y++) {
		int y1 = y * 64;
		int y2 = y1 + 64;
		for (int x = 0; x <= CELLX; x++) {
			int x1 = x * 64;
			int x2 = x1 + 64;
			SDL_RenderDrawLine(renderer, x1, y1, (x2-x1)/2, y1); // horizontal
			SDL_RenderDrawLine(renderer, x1, y1, x1, (y2-y1)/2); // horizontal
			if (cells[x][y].numobjects) {
				TextAt(x1 + 10, y1 + 10, SDL_ltoa(cells[x][y].numobjects,buffer,10),1);
			}
		}
  }
}

// All asteroids plus alien ships dead?
int AllObjectsDestroyed() {
	for (int i=0;i<MAXASTEROIDS;i++)
		if (asteroids[i].active) return 0;
	return 1;
	// TBD Aliens ships.
}


// main game loop handles game play 
void GameLoop() {
	tickCount = SDL_GetTicks();
	/*
	stopWatch start;
	double totalTime;
	totalTime = 0.0;
	int counter=0;
	*/
	InitLevel(gameLevel);
	frameCount = 0;
	while (frameCount < 59) {
		RenderTexture(textures[PLBACKDROP], 0, 0);
		DrawTextSprites();
		RenderEverything();
	}
	while (ProcessEvents())
	{	
		startTimer(&s);
		CheckPause();

		//startTimer(&start);
		if (Player.lives == 0) break;
		ClearCellList();
		/*
		stopTimer(&start);
		totalTime += diff(&start)*1000000;
		counter++;
		if (counter == 60) {
			snprintf(timebuff, sizeof(timebuff)-1,"%f5.2", totalTime / counter);
			counter = 0;
			totalTime = 0.0;
		}
		*/
		l("GameLoop ");
		if (!paused) {
			MoveAsteroids();
			RotatePlayerShip();
			CheckBulletsFired();
			MoveBullets();
			CheckJump();
			ApplyThrust();
			MovePlayerShip();
			CycleExplosions();
			CountBullets();			
			if (DoCoolDown()) return;

			if (AllObjectsDestroyed()) {
				gameLevel++;
				return;
			}
		}
		//ShowCells();
		DrawEverything();
		if (!paused) {
			CheckCollisions(); // This is put here so SHOWOVERLAP works
		}
		RenderEverything();
		//while (SDL_GetTicks() - tickCount < 17); // delay it to ~60 fps				
	}
}

int main(int argc, char * args[])
{
	InitLogging("biglog.txt");
	InitSetup();
	if (errorCount) {
		return -1;
	}
	gameLevel = 1;
	do {			
		InitGame();
		while (Player.lives){
			GameLoop();
			if (CloseFlag) break;
		};
		if(!Player.lives) break;
	} while (!CloseFlag);
	CloseLogging();
	FinishOff();
    return 0;
}
