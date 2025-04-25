// Chapter 39 - Final version of asteroids
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
//#define SHOWOVERLAP    // Comment this out for normal play, uncomment to show collisions 
#define TIMEGAMELOOP     // uncomment to show time in game loop

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
#define SHIELDKEY SDLK_s
#define THRUSTKEY SDLK_LCTRL
#define TEMPKEY   SDLK_t 

// sizes
#define ALIENSHIPHEIGHT 64
#define ALIENSHIPWIDTH 64
#define SCREENWIDTH 1024
#define SCREENHEIGHT 768
#define SHIPHEIGHT 64
#define SHIPWIDTH 64
#define EXPLOSIONSIZE 128
#define CELLSIZE 64
#define CHARWIDTH 13

// Define x and y restricted area
#define ZONESIZE 200
#define ZONELEFT (SCREENWIDTH-ZONESIZE)/2
#define ZONERIGHT ZONELEFT+ZONESIZE
#define ZONETOP (SCREENHEIGHT-ZONESIZE)/2
#define ZONEBOTTOM ZONETOP+ZONESIZE

// numbers of things
#define ALIENFLASHFRAMES 15
#define MAXALIENS 3
#define MAXBULLETSPERALIEN 8
#define MAXASTEROIDS 32
#define MAXPLAYERBULLETS 16
#define MAXEXPLOSIONS 256
#define NUMSCORES 10
#define NUMSOUNDS 4
#define NUMTEXTURES 15
#define NUMEXPLOSIONSOUNDS 2
#define NUMPOINTERS 273
#define CELLY SCREENHEIGHT/CELLSIZE
#define CELLX SCREENWIDTH/CELLSIZE
#define MAXTEXTSPRITES 10
#define MAXLEVELS 50
#define MAXBULLETS MAXPLAYERBULLETS + (MAXBULLETSPERALIEN * MAXALIENS)

// Texture indices
#define PLBACKDROP 0
#define TEXTTEXTURE 1
#define TEXTUREPLAYERSHIP 2
#define TEXTUREDEBUG 3
#define TEXTUREASTEROID1 4  // 4,5,6,7
#define TEXTUREBULLET 8
#define TEXTURESMALLSHIP 9
#define TEXTUREEXPLOSION 10 // 10,11,12,13
#define TEXTUREALIENSHIP 14

// sound indices
#define THRUSTSOUND 0
#define CANNONSOUND 1
#define EXPLOSIONSOUND 2

// functions 
#define sgn(x) (x < 0) ? -1 : (x > 0)

// typedefs
typedef struct firstpart * pfirstpart;
typedef pfirstpart ptrarray[100]; // 100 pointers to a firstpart
typedef char * pchar;

// enums
enum ObjectType {tAsteroid,tBullet,tPlayer,tAlien};
enum BulletOwner {bNone=-1,bPlayer,bAlien1,bAlien2,bAlien3};

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
	int sizeindex;
	int toDie;
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
	int playerbullet; // -1 = none,0= player, 1.. MAXALIENS = alien saucer
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

struct alien {
	SDL_Rect r;
	int type; // 3
	int active;
	int moveDir; 
	int moveTimer;
	float x, y;
	float xvel, yvel;
	int xcount, ycount;
	int xtime, ytime;
	int ttl;
	int flTimer;
	int flShow;
};

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

struct HighScoreEntry {
	char initials[4];
	int score;
	int d, m, y;
	int level;
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
int score, paused,gameLevel,CloseFlag,playerDead,shieldRadius;
int debugFlag,fireFlag,flashFlag,shieldFlag,shieldStrength,shieldTimer;
int numsprites,playerBulletCount,alienBulletCount, tempCount;
int rotTimer,speedTimer, fireTimer, jumpTimer,pauseTimer,flashTimer,controlTimer;
int rotateFlag, thrustFlag, jumpFlag, pauseFlag, tempFlag,controllerFlag;
int numcells,coolDown,numHighScores;
int rotateFlag; // 0 do nowt, 1 counter clockwise, 2 clockwise 
int piFlag=0;
int numAsteroids = 0;
float speedFactor = 1;
float lastTemp;
char * maskErrorFile;

// array variables
char timebuff[50]; // used for timing
char buffer[100],buffer2[100];
float thrustx[24];
float thrusty[24];
char bulletmask[1][3][3];
char plmask[24][64][64];
char buff1[10000];
char a1mask[24][280][280];
char buff2[10000];
char a2mask[24][140][140]; 
char buff3[10000];
char a3mask[24][70][70];
char buff4[10000];
char a4mask[24][35][35];
char buff5[10000];
char alienmask[64][64];

// struct variables
struct Cell cells[CELLX][CELLY];
struct pair cellList[CELLX*CELLY];
struct TextSprite sprites[MAXTEXTSPRITES];
struct alien aliens[MAXALIENS];
struct asteroid asteroids[MAXASTEROIDS];
struct bullet bullets[MAXBULLETS];
struct explosion explosions[MAXEXPLOSIONS];
struct Mix_Chunk * sounds[NUMSOUNDS];
extern struct level levels[MAXLEVELS];
struct HighScoreEntry highscores[NUMSCORES];

stopWatch s;

// external variables
extern int errorCount;
extern FILE * dbf;  // for logging

// consts
const char * texturenames[NUMTEXTURES] = { "images/starfield.png","images/text.png","images/playership.png","images/debug.png",
"images/a1.png","images/a2.png","images/a3.png","images/a4.png","images/bullet.png","images/smallship.png","images/explosion0.png",
"images/explosion1.png","images/explosion2.png","images/explosion3.png","images/alien.png" };
const char * highscoreFN = { "highscores.txt" };
const char * soundnames[NUMSOUNDS] = { "sounds/thrust.wav","sounds/cannon.wav","sounds/explosion1.wav","sounds/explosion2.wav" };

const int explosionSizes[4] = { 128,128,128,64 };
const int sizes[4] = { 280,140,70,35 };
const int debugOffs[4] = { 0,280,420,490 };
const int bulletx[24] = { 32,38,50,64,66,67,68,67,66,64,50,38,32,26,16, 0,-2,-3,-4,-3,-2,0,16,24 };
const int bullety[24] = { -4,-3,-2, 0,16,26,32,38,50,64,66,67,71,70,69,64 ,50,38,32,26,16,0,-2,-3 };
const int xdir[8] = { 0,1,1,1,0,-1,-1,-1 };
const int ydir[8] = { -1,-1,0,1,1,1,0,-1 };

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

// Load texture from a file- return SDL_Texture*
SDL_Texture* LoadTexture(const char * afile, SDL_Renderer *ren) {
	SDL_Texture *texture = IMG_LoadTexture(ren, afile);
	if (texture == 0) {
		LogError2("Failed to load texture from ", afile);
		LogError2("Error is ",SDL_GetError());
	}
	return texture;
}

// Loads Textures by calling LoadTexture 
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

void SetPiFlag() {
	struct utsname buffer;
	tempFlag = 0;
    if (uname(&buffer) != 0) return;
	LogError2("nodename=%s",buffer.nodename);
	LogError2("machine=%s",buffer.machine);	
    if (strcmp(buffer.nodename,"raspberrypi") !=0) return;
	if (strcmp(buffer.machine,"aarch64") != 0) return; // armv7l = 32-bit aarch64 = 64-bit
	piFlag=1;
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

// Not used
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
	if (!fmask){
		LogError2("Missing mask file=",filename);
	}
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
	if (!LoadMask("masks/alien.msk", 64, 1, &alienmask[0][0])) return 0;
	return LoadMask("masks/am4.msk", 35, 24, &a4mask[0][0][0]);
}

// check if gamecontroller present
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

// clear celllist and cells
void ClearCellList() {
	memset(cellList, 0, sizeof(cellList));
	numcells = 0;
	memset(cells, 0, sizeof(cells));
	//l("ClearCellList");
}

// adds a pointer to an object in cellList
void AddPointerToCell(int x, int y, pfirstpart objectptr) {
	int numobjects = cells[x][y].numobjects;
	if (!numobjects) { // 1st time cell added to so add to cellList
		cellList[numcells].x = x;
		cellList[numcells++].y = y;	
	}
	cells[x][y].ptrs[numobjects] = objectptr;
	cells[x][y].numobjects++;
	//ln("AddPointerToCell", "", numobjects);
}

// Adds an object to every cell it covers. For a large asteroid 280 x 280
// This can occupy between 25, 30 or 36 cells.
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

// returns index of frst free sprite, otherwise if non are free return -1
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
	strncpy(psprite->message,value, sizeof(psprite->message));
	numsprites++;
}

// This centres the text horizontally 
void AddTextSpriteString(char * value, int y, int textfactor) {
	int textlen = strlen(value) * CHARWIDTH * textfactor; // 13 = width of chars in set
	int x = (SCREENWIDTH - textlen) / 2;
	AddTextSpriteAt(value, x, y, textfactor);
}

// This centres it but also converts int to str
void AddTextSpriteInt(int value, int y, int textfactor) {
	AddTextSpriteString(sltoa(value), y, textfactor);
}

// initialises data but possibly gets overwritten when highscores.txt file is read in in ReadhighScores()
void InitHighScores() {
	memset(highscores, 0, sizeof(highscores));
	strncpy(highscores[0].initials,"DHB",4);
	highscores[0].score = 500;
	highscores[0].d = 1;
	highscores[0].m = 1;
	highscores[0].y = 2018;
	highscores[0].level = 1;
	numHighScores = 1;
}

// copies len chars from line starting from start then return numeric value of copied chars
int StrConv(char * line, int start, int len) {
	char buffer[20];
	//l2("line=",line);
	//l2("start=",sltoa(start));
	//l2("Len=",sltoa(len));
	if (len > sizeof(buffer))
		len = sizeof(buffer) - 1; // error check length
	for (int i = 0; i < len; i++) {
		buffer[i] = line[start++];
		//l2("i=",sltoa(i));
		//lc("buffer[i]=",buffer[i]);
	}
	buffer[start] = 0;
	return atoi(buffer);
}

// copies len chars from line into starting from start
// There's no terminating 0 so can't use stnrcpy
void StrCopyTo(char * dest,char * line, int start, int len) {
	int i=0;
	while (i < len) {
		dest[i++] = line[start++];
	}
	dest[i] = 0;
	return;
}

// Reads highscores from highScoreFile. Each row has this
// ddmmyyyyIIILLSSSSSS - 19 chars long
// where dd= day 01-31 mm = month 01-12 yy = year e.g. 2018 III = Initials e.g. DH, Level LLB and SSSSSS = 6 digit score with leading zeroes
// example 01012012DHB10000500 - 1st Jan 2018 DHB beat level 10 with 500 score
void ReadHighScores() {
	InitHighScores();

	char line[25]; // long enough for 19 char string plus trailing 0
	FILE * fscores = fopen(highscoreFN, "rt");
	if (!fscores){
		LogError2("Missing highscore file=",highscoreFN);
	}	
	numHighScores = 0;
	while (fgets(line, sizeof(line), fscores)) {
		int len = strlen(line);
		//l2("line =",line);
		//l2("Len(line)=",sltoa(len));
		if (len==20) {
			highscores[numHighScores].d = StrConv(line, 0, 2);
			highscores[numHighScores].m = StrConv(line, 2, 2);
			highscores[numHighScores].y = StrConv(line, 4, 4);
			StrCopyTo((char *)&highscores[numHighScores].initials, line, 8, 3);
			highscores[numHighScores].level = (line[11]-'0')*10 + (line[12]-'0');
			//li("Level = ",highscores[numHighScores].level);
			highscores[numHighScores++].score = StrConv(line, 13, 6);
			if (numHighScores == NUMSCORES) break;
		}
		else break;
	} 
	fclose(fscores);
	return;
}

// Write high scores to a file but only those with a score
// Each row is like this  ddmmyyyyIIISSSSSS - 17 chars long
void WriteHighScores() {
	char line[20];
	FILE * fscores = fopen(highscoreFN, "wt");
    if (!fscores) {
		LogError2("Unable to create or write to highscore file",highscoreFN);
	}
	numHighScores = 0;
	for (int i = 0; i < NUMSCORES; i++) {
		if (highscores[i].score) {
			numHighScores++;
			snprintf(line, sizeof(line), "%02d%02d%04d", highscores[i].d, highscores[i].m, highscores[i].y);
			StrCopyTo((char *)&line[8], highscores[i].initials, 0, 3);
			snprintf(&line[11], sizeof(line), "%02d", highscores[i].level);
			snprintf(&line[13], sizeof(line), "%06d", highscores[i].score);
			fputs(line, fscores);
			fputs("\n", fscores);
		}
	}
	fclose(fscores);
}

// Initialize all setup, set screen mode, load images etc 
void InitSetup() {
	errorCount = 0;
	CloseFlag = 0;
	flashFlag = 0;
	flashTimer = 0;
	controllerFlag =0;
	srand((int)time(NULL));
	SDL_Init(SDL_INIT_EVERYTHING );
	screen= SDL_CreateWindow("Asteroids", 100, 100,SCREENWIDTH, SCREENHEIGHT, 0);
	if (!screen) {
		LogError("InitSetup failed to create window");
	}
	renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	LoadTextures();
	int success=Mix_OpenAudio(22050, AUDIO_S16LSB, 2, 8192);
	if (success==-1 ) {
		LogError("InitSetup failed to init audio");
	}
	LoadSoundFiles();
	if (!LoadMasks()) {
      LogError2("InitSetup failed to load mask number:",maskErrorFile);
	}
	ReadHighScores();
	if (SDL_GameControllerAddMapping("030000001008000001e5000010010000,NEXT SNES Controller2,platform:Linux,a:b2,b:b1,x:b3,y:b0,back:b8,start:b9,leftshoulder:b4,rightshoulder:b5,dpup:-a1,dpdown:+a1,dpleft:-a0,dpright:+a0")==-1){
		LogError("Unable to load gamepad mappings from gamepad.txt");
	}	
	controllerFlag = HasGameController();
	InitThrustAngles();
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
		char buffer[10];
		LogError2("Mix_PlayChannel failed to play audio sound #",SDL_ltoa(soundindex,buffer,10));
	} */
}

// Used for displaying Starfield
void RenderTexture(SDL_Texture *tex, int x, int y) {
	//Setup the destination rectangle to be at the position we want
	dst.x = x;
	dst.y = y;
	//Query the texture to get its width and height to use
	SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
	SDL_RenderCopy(renderer, tex, NULL, &dst);
}

// print single char c at rect target 
void printch(char c, SDL_Rect * target, float textFactor) {
	int start = (c - '!');
	if (c != ' ') {
		sourceRect.h = 23;
		sourceRect.w = 12;
		sourceRect.x = start * 12;
		sourceRect.y = 0;
		SDL_RenderCopy(renderer, textures[TEXTTEXTURE], &sourceRect, target);
	}
	(*target).x += (int)(CHARWIDTH*textFactor); // in this bitmap font, chars are CHARWIDTH pixels width
}

// print string text at x,y pixel coords 
void TextAt(int atX, int atY, char * msg,float textFactor) {
	destRect.h = (int)(23*textFactor);
	destRect.w = (int)(12*textFactor);
	destRect.x = atX;
	destRect.y = atY;
	for (int i = 0; i < (int)strlen(msg); i++) {
		printch(msg[i], &destRect,textFactor);
	}
}

// Updates caption
void UpdateCaption() {
#ifdef TIMEGAMELOOP
		SetCaption(timebuff);
#else
    if (tempFlag) {
		snprintf(buffer2, sizeof(buffer2), "%10.6f temp = %5.2f C", diff(&s),ReadPiTemperature());		
	}
	else
	{
		snprintf(buffer2, sizeof(buffer2), "%d %10.6f",controllerFlag,diff(&s));
		}
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
#endif
}

// from https://stackoverflow.com/questions/38334081/howto-draw-circles-arcs-and-vector-graphics-in-sdl
void DrawCircle(SDL_Renderer *Renderer, int _x, int _y, int radius)
{
	int x = radius - 1;
	int y = 0;
	int tx = 1;
	int ty = 1;
	int err = tx - (radius << 1); // shifting bits left by 1 effectively
								  // doubles the value. == tx - diameter
	while (x >= y)
	{
		//  Each of the following renders an octant (1/8th) of the circle
		SDL_RenderDrawPoint(Renderer, _x + x, _y - y);
		SDL_RenderDrawPoint(Renderer, _x + x, _y + y);
		SDL_RenderDrawPoint(Renderer, _x - x, _y - y);
		SDL_RenderDrawPoint(Renderer, _x - x, _y + y);
		SDL_RenderDrawPoint(Renderer, _x + y, _y - x);
		SDL_RenderDrawPoint(Renderer, _x + y, _y + x);
		SDL_RenderDrawPoint(Renderer, _x - y, _y - x);
		SDL_RenderDrawPoint(Renderer, _x - y, _y + x);

		if (err <= 0)
		{
			y++;
			err += ty;
			ty += 2;
		}
		else 
		{
			x--;
			tx += 2;
			err += tx - (radius << 1);
		}
	}
}

// Draws throbbing circle round ship and shield strength underneath.
void DisplayShield(SDL_Rect * target) {
	if (shieldFlag && shieldStrength >10) {
		SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
		DrawCircle(renderer, target->x + (SHIPWIDTH/2), target->y + (SHIPHEIGHT/2), shieldRadius);
		shieldRadius += 2;
		if (shieldRadius == 46) {
			shieldRadius = 38;
		}
	}
	if (shieldStrength < 100) {
		TextAt(target->x + 10, target->y + 58, sltoa(shieldStrength), 0.67f);
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
	DisplayShield(&target);

	if (SDL_GetTicks() - jumpTimer < 3000) {
		if (flashFlag)
		  TextAt((int)Player.x - 50, (int)Player.y + 66, "No jump!", 1);
	}

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

// Draw player ship 
void DrawAlienShips() {

	SDL_Rect spriterect, target;
	char buff[30];

	target.h = ALIENSHIPHEIGHT;
	target.w = ALIENSHIPWIDTH;	
	spriterect.h = ALIENSHIPHEIGHT;
	spriterect.w = ALIENSHIPWIDTH;
	spriterect.x = 0;
	spriterect.y = 0;
	for (int i = 0; i < MAXALIENS; i++) {
		if (!aliens[i].active) continue;
		struct alien * palien = &aliens[i];
		target.x = (int)palien->x;
		target.y = (int)palien->y;
		if (palien->flShow) {
			SDL_RenderCopy(renderer, textures[TEXTUREALIENSHIP], &spriterect, &target);
		}
		if (!debugFlag) continue;
		// code after here is only run if debug !=0 
		target.w = ALIENSHIPWIDTH;
		target.h = ALIENSHIPHEIGHT;

		spriterect.x = 280;
		spriterect.y = 140;
		spriterect.w = ALIENSHIPWIDTH;
		spriterect.h = ALIENSHIPHEIGHT;
		SDL_RenderCopy(renderer, textures[TEXTUREDEBUG], &spriterect, &target);

		snprintf(buff, sizeof(buff), "(%6.4f,%6.4f)", palien->x, palien->y);
		TextAt((int)palien->x - 50, (int)palien->y + 66, buff, 1);
		snprintf(buff, sizeof(buff), "(%6.4f,%6.4f)", palien->xvel, palien->yvel);
		TextAt((int)palien->x - 50, (int)palien->y + 90, buff, 1);
	}
}

// Draw All asteroids 
void DrawAsteroids() {
	SDL_Rect asteroidRect, target;
	numAsteroids = 0;
	asteroidRect.y = 0;
	for (int i = 0; i<MAXASTEROIDS; i++) {
		if (asteroids[i].active==1) {
			numAsteroids++;                    // keep track of how many onscreen
			int sizeIndex = asteroids[i].sizeindex; // 0-3
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

// Draw all bullets with countdown > 0 
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

// Draw all explosions with countdown > 0 
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

// Draws all active TextSprites 
void DrawTextSprites() {
	for (int i = 0; i < MAXTEXTSPRITES;i++) {
		struct TextSprite * psprite = &sprites[i];
		if (psprite->active) {
			TextAt(psprite->x, psprite->y--, psprite->message,(float)psprite->textFactor);
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
	DrawAlienShips();
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
	//if (frameCount == 60) frameCount = 0;
}

// Draw Text at start of level showing lives left
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

// Init all Bullets 
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
		pbullet->playerbullet = -1; // set when fired -1 = player=0, 1..3 is alien ship
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

// Init all text sprites
void InitTextSprites() {
	memset(sprites, 0, sizeof(sprites));
	numsprites = 0;
}

// Called to initialise each game
void InitGame() {
	score = 0;
	paused = 0;
	gameLevel = 0;
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

// applies thrust in direction opposite to ship is facing
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
	int size = sizes[past->sizeindex]; // used later so fetch once!

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
			else if (past->y > SCREENHEIGHT) {
				past->y -= SCREENHEIGHT;
			}
		}
	}
	// a check in case the asteroid now has active equals 0
	if (past->active > 0) {
		//l2("Adding pointer for asteroids", sltoa(index));
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

// move all alien ships
void MoveAlienShips() {
	for (int i = 0; i < MAXALIENS; i++) {
		struct alien * palien = &aliens[i];
		if (!palien->active) continue;

		// movement along x axis
		if (palien->xtime > 0) {
			palien->xtime--;
			if (palien->xtime <= 0) {
				palien->x += palien->xvel;
				palien->r.x = (int)palien->x;
				palien->xtime = palien->xcount;
				if (palien->x < -ALIENSHIPWIDTH) { // off left edge, on right
					palien->x += SCREENWIDTH;
				}
				else if (palien->x > SCREENWIDTH) { // off right edge, on left
					palien->x -= SCREENWIDTH;
				}
			}

			// movement along y axis
			if (palien->ytime > 0) {
				palien->ytime--;
				if (palien->ytime == 0) {
					palien->y += palien->yvel;
					palien->r.y = (int)palien->y;
					palien->ytime = palien->ycount;
					if (palien->y < -ALIENSHIPHEIGHT) {
						palien->y += SCREENHEIGHT;
					}
					else if (palien->y > SCREENHEIGHT) {
						palien->y -= SCREENHEIGHT;
					}
				}
			}

			// a check in case the asteroid now has active equals 0
			if (palien->active > 0) {
				// l2("Adding pointer for Aliensd", sltoa(i));
				AddObjectToCells((pfirstpart)palien);
			}
		}
		
	}
}

// return index of slot for asteroid or -1 if not found (table is full in that case)
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

// returns 1 if x,y is in central restricted zone or if a non-zero radius is 
int InCentre(int x, int y,int radius) {
	if (!radius) {
		if ((x < ZONELEFT || x > ZONERIGHT) && (y < ZONETOP || y > ZONEBOTTOM)) return 0;
	}
	else {
		x -= (SCREENWIDTH / 2); // calculate distance from centre
		y -= (SCREENHEIGHT / 2);
		int distance = x * x + y * y;
		if (radius*radius < distance) return 0; // it's outside the distance
	}
	return 1;
}

// adds alien ship to screen
void AddAlienShip() {
	int alienindex = -1;
	for (int index = 0; index < MAXALIENS; index++) {
		if (aliens[index].active == 0) { 
			alienindex = index; 
			break; 
		}
	}
	if (alienindex == -1) return;
	struct alien * palien = &aliens[alienindex];
	palien->active = 1;
	do {
		palien->x = (float)Random(SCREENWIDTH - ALIENSHIPWIDTH) + 2* ALIENSHIPWIDTH;
		palien->y = (float)Random(SCREENHEIGHT - ALIENSHIPHEIGHT) + 2* ALIENSHIPHEIGHT;
		// Prevent asteroids appearing in a 200 x 200 square in the centre. Otherwise player ship can get blown up immediately!
		if (InCentre((int)palien->x, (int)palien->y,200)) continue;
		if (IsEmptySpace((int)palien->x, (int)palien->y)) break;
	} while (1);
	palien->type = tAlien;
	palien->r.y = (int)palien->y;
	palien->r.x = (int)palien->x;
	palien->xvel = (float)3 - Random(6);
	palien->yvel = (float)3 - Random(6);
	palien->r.w = ALIENSHIPWIDTH;
	palien->r.h = ALIENSHIPHEIGHT;
	palien->xtime = 2 + Random(3);
	palien->xcount = palien->xtime;
	palien->ytime = 2 + Random(3);
	palien->ycount = palien->ytime;
	palien->moveTimer = 10 + Random(5);
	palien->flShow = 1;
	palien->flTimer = Random(ALIENFLASHFRAMES)+1;
	palien->ttl = Random(30)+10;
}

// adds asteroid to asteroids table , size = 0..3
void AddAsteroid(int sizeindex) {
	int index = FindFreeAsteroidSlot();
	if (index == -1) // table full so quietly exit 
		return;
	if (sizeindex == -1) {  // Use -1 to sepcify a random size
		sizeindex = Random(4) - 1;
	}
	int asize = sizes[sizeindex];
	struct asteroid * past = &asteroids[index];
	past->active = 1;
	do {
		past->x = (float)Random(SCREENWIDTH - asize) + asize*2;
		past->y = (float)Random(SCREENHEIGHT - asize) + asize*2;
		// Prevent asteroids appearing in a 150 x 150 square in the centre. Otherwise player ship can get blown up immediately!
		if (InCentre((int)past->x, (int)past->y,0)) continue;
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
	past->sizeindex = sizeindex;
	past->r.w = sizes[sizeindex];
	past->r.h = sizes[sizeindex];
}

// add Explosion to explosions table
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
	int sizeindex = a->sizeindex;
	if (sizeindex == 3)
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

		struct asteroid * past = &asteroids[index];
		past->active = 1;
		past->rotdir = 0;
		past->rotclockwise = Random(2) - 1;
		past->xvel = (xdir[i] * Random(3) + xvel)*speedFactor;
		past->yvel = (ydir[i] * Random(3) + yvel)*speedFactor;
		past->xtime = 2 + Random(5);
		past->xcount = past->xtime;
		past->ytime = 2 + Random(5);
		past->ycount = past->ytime;
		past->rottime = 2 + Random(8);
		past->rotcount = past->rottime;

		int  newsizeindex = 3; // smallest
		if (i % 2 == 1) // if i is odd add in next smaller size
			newsizeindex = sizeindex + 1; 
		past->sizeindex = newsizeindex;
		past->r.w = sizes[newsizeindex];
		past->r.h = sizes[newsizeindex];
		past->x = (float)xdir[i] * (sizes[newsizeindex]*6/5) + x;
		past->y = (float)ydir[i] * (sizes[newsizeindex]*6/5) + y;
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

// DestroyAlienShip
void DestroyAlienShip(struct alien * a) {
	a->active = 0;
	AddExplosion(a->r.x + a->r.w / 2, a->r.y + a->r.h / 2);
}

// test code - destroy 40% (roughly) of asteroids
void BlowUpAsteroids() {
	int i;
	for (i = 0; i<MAXASTEROIDS; i++) {
		if (asteroids[i].active && Random(10) <4) {
			DestroyAsteroid(i);
		}
	}
}

// fire a buller- first find a slot and then work out where it should appear relative to player ship then add to bullets
void DoFireBullet() {
	if (playerBulletCount == MAXPLAYERBULLETS) return;
	int index = -1;
	for (int i = 0; i<MAXBULLETS; i++) {
		if (bullets[i].active == 0) { // found a slot 
			index = i;
			break;
		}
	}	
	if (index == -1) return; // no free slots so all bullets must be in play
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
	pbullet->playerbullet = bPlayer;
}

// move bullets  
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
			// l2("Adding pointer bullets", sltoa(i));
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
						shieldFlag =1;
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
						shieldFlag =0;
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
					CloseFlag = 1;
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
				case SHIELDKEY:
					shieldFlag = 1;
					break;
				case TEMPKEY:
					tempFlag = 1-tempFlag;
					break;
				case THRUSTKEY:
					thrustFlag = 1;
					break;
				case ADDASTEROIDKEY:
					for (int i = 0; i < 10; i++) {
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
				shieldFlag = 0;
				shieldRadius = 38;
				break;
			}
		} // switch event.type	
	} // while
	return 1;
}

// Can only fire one bullet every 100 frames
void CheckBulletsFired() {
	if (fireFlag && SDL_GetTicks() - fireTimer > 100) {
		fireTimer = SDL_GetTicks();
		DoFireBullet();
	}
}

// check if pause key pressed
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
	}
}

// Called when player loses a life or clears Level
void ReInitGame() {
	InitPlayer(Player.lives);
	InitAsteroids();
	InitBullets();
	InitExplosions();
	InitTextSprites();
}

// Initialises each level
void InitLevel(int glevel) {
	rotTimer = SDL_GetTicks();
	speedTimer = rotTimer;
	fireTimer = rotTimer;
	jumpTimer = rotTimer;
	pauseTimer = rotTimer;

	coolDown = 0;
	controlTimer = 0;
	shieldStrength = 100;
	shieldRadius = 38;
	playerDead=0;
	ReInitGame(); // Clear BEATT (Bullets Explosions Asteroids Thrust and Text)
	struct level * thislevel = &levels[glevel];
	for (int sizeindex = 0; sizeindex <4; sizeindex++) {
		int numtoadd=thislevel->nums[sizeindex];
		if (numtoadd) {
			for (int aindex = 0; aindex < numtoadd; aindex++) {
				AddAsteroid(sizeindex); 
			}
		}
	}

	for (int alienindex = 0; alienindex < thislevel->aliens; alienindex++) {
		AddAlienShip();
	}
	speedFactor = thislevel->factor;
	DrawLevelStart();
}

// Destroy whatever object is passed in, any of Asteroid, bullet, player ship or alien ship
void DestroyObject(pfirstpart object) {
	switch (object->type) {
	case tAsteroid: // asteroid
		DestroyThisAsteroid((struct asteroid *)object); // cast back from firstpart to...
		break;
	case tBullet: // bullet
		object->active = 0;
		break;
	case tPlayer: 
		if (shieldFlag) return;
		//return; // uncomment to play without player being destroyed
		object->active=0;
		playerDead = 1;
		coolDown = 150;
		InitTextSprites(); // clear so only either of these two messages visible
		if (Player.lives == 1) {
			AddTextSpriteString("Game over!", (SCREENHEIGHT - 20) / 2, 3);
		}
		else {
			AddTextSpriteString("Your ship destroyed!", (SCREENHEIGHT - 20) / 2, 3);
		}
		break;

	case tAlien:
		DestroyAlienShip((struct alien *)object);
		object->active = 0;
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
		case tAlien:
			return (pchar)&alienmask;
	}
	return 0; // null - should never get here!
}

// Checks to see if two objects whose bounding boxes overlap actually touch.
int Overlap(pfirstpart object1, pfirstpart object2, SDL_Rect * rect,int * bangx,int * bangy) {
#ifdef SHOWOVERLAP
	SDL_SetRenderDrawColor(renderer, 0, 0xff, 0, SDL_ALPHA_OPAQUE); // Sets green colour
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
#ifndef SHOWOVERLAP
			if (*pl1++ & *pl2++) {
				return 1;  // hit!
			}
#endif
#ifdef SHOWOVERLAP
			if (*pl1++ & *pl2++) {
				if (object1->type == tPlayer || object2->type == tPlayer) { // comment this line out- it'll show all overlaps but run very very slowly
					SDL_RenderDrawPoint(renderer, *bangx, *bangy);
				}
			}
#endif
		}
		pm1 += size1; // move mask pointers down by one row
		pm2 += size2;
	}
	return 0;  // missed!
}

// Check if player/bullet combination and if hit own bullets
int PlayerHitOwnBullet(pfirstpart object1, pfirstpart object2) {
	struct bullet * pbullet = 0;
	if (object1->type == tBullet && object2->type == tPlayer) {
		pbullet = (struct bullet *)object1;
	}		
	else
		if (object2->type == tBullet && object1->type == tPlayer) {
			pbullet = (struct bullet *)object2;
		}
	if (!pbullet) return 0;
	if (pbullet->playerbullet == 0) return 1; // alien bullet hit
		return 0; // can't hit own bullet
}

// Check if player/bullet combination and if hit own bullets
int AlienHitOwnBullet(pfirstpart object1, pfirstpart object2) {
	struct bullet * pbullet = 0;
	if (object1->type == tBullet && object2->type == tAlien) {
		pbullet = (struct bullet *)object1;
	}
	else
		if (object2->type == tBullet && object1->type == tAlien) {
			pbullet = (struct bullet *)object2;
		}
	if (!pbullet) return 0;
	if (pbullet->playerbullet > 0) return 1; // alien bullet hit alien - no damage
	return 0; // 
}

// Double loop to see if any pairs of objects in this overlap.
void CheckAllObjects(int x, int y) {
	struct Cell * c = &cells[x][y];
	SDL_Rect rect;
	int bangy, bangx;
	int type1, type2;
	for (int index1=0;index1<c->numobjects;index1++)
		for (int index2 = index1 + 1; index2 < c->numobjects; index2++) {
			if (c->ptrs[index1]->active && c->ptrs[index2]->active &&
				SDL_IntersectRect(&c->ptrs[index1]->r, &c->ptrs[index2]->r, &rect)) {
				if (Overlap(c->ptrs[index1], c->ptrs[index2], &rect,&bangx,&bangy)) {
					type1 = c->ptrs[index1]->type;
					type2 = c->ptrs[index2]->type;
					AddExplosion(bangx, bangy);
					if ((type1 == tAsteroid && type2 == tBullet) ||
						(type2 == tAsteroid && type1 == tBullet)) {
						if (!coolDown) { // no score if player ship killed in this frame or after!
							score += 50;
							AddTextSpriteAt("50", bangx, bangy - 20, 2);
						}
					}
						else
					if ((type1 == tAlien && type2 == tBullet) ||
						(type2 == tAlien && type1 == tBullet)) {
						if (!coolDown) { // no score if player ship killed in this frame or after!
							score += 250;
							AddTextSpriteAt("250", bangx, bangy - 20, 2);
						}
					}

					if (!PlayerHitOwnBullet(c->ptrs[index1], c->ptrs[index2])) {
						DestroyObject(c->ptrs[index1]);
						DestroyObject(c->ptrs[index2]);
					}
					if (!AlienHitOwnBullet(c->ptrs[index1], c->ptrs[index2])) {
						DestroyObject(c->ptrs[index1]);
						DestroyObject(c->ptrs[index2]);
					}
				}
			}
		}
}

// Check if any cell (in celllist) has 2 or more objects
void CheckCollisions() {
	for (int i = 0; i < numcells; i++) {
		int x = cellList[i].x;
		int y = cellList[i].y;
		if (cells[x][y].numobjects > 1) {
			CheckAllObjects(x, y);
		}
  }
}

// After playerShip hit, calls this for 150 frames CoolDown 
// it just lets thing run a little rather than abruptly stopping and lets the 
// Your Ship Destroyed/Game over etc messages have time to fully scroll
int DoCoolDown()
{
	if (coolDown == 0) return 0;
	coolDown--;
	if (coolDown > 0) return 0;
	if (playerDead) {
		Player.lives--;
		playerDead = 0;
	}
	return 1;
}

// Debug - shows cells on screen when called from RenderEverything. Aka Tholian web
void ShowCells() {
	SDL_SetRenderDrawColor(renderer, 0,0xff, 0, SDL_ALPHA_OPAQUE);
	for (int y = 0; y < CELLY; y++) {
		int y1 = y * 64;
		int y2 = y1 + 64;
		for (int x = 0; x < CELLX; x++) {
			int x1 = x * 64;
			int x2 = x1 + 64;
			SDL_RenderDrawLine(renderer, x1, y1, (x2-x1)/2, y1); // horizontal
			SDL_RenderDrawLine(renderer, x1, y1, x1, (y2-y1)/2); // horizontal
			if (cells[x][y].numobjects) {
				TextAt(x1 + 10, y1 + 10, sltoa(cells[x][y].numobjects),1);
			}
		}
  }
}

// Are all asteroids and alien ships dead?
int AllObjectsDestroyed() {
	for (int i = 0; i < MAXASTEROIDS; i++) {
		if (asteroids[i].active) return 0;
	}
	for (int i = 0; i < MAXALIENS; i++) {
		if (aliens[i].active) return 0;
	}	
	return 1;
}

// Alien ship uses this to detect nearby asteroids
// Go through all asteroids, identify those within 100 pixels range
// return 1 if so and coordinates of it's centre.
int AnyTargetsNearBy(struct alien * palien, int * x, int * y,int * size) {
	int alx = palien->r.x;
	int aly = palien->r.y;
	int mindist = 1000000;
	int foundone = 0;
	for (int i = 0; i < MAXASTEROIDS; i++) {
		struct asteroid * ast = &asteroids[i];
		if (ast->active) {
			int dist = ((ast->r.x - alx)*(ast->r.x - alx) +
				        (ast->r.y - aly)*(ast->r.y - aly));
			if (dist < 10000 && dist < mindist) {
				mindist = dist;
				*x = ast->r.x;
				*y = ast->r.y;
				*size = ast->r.w;
				foundone = 1;
			}
		}
	}
	return foundone;
}

// calulates direction 0-7 for alien ship to shoot
int GetDir(int fx, int fy, int tx, int ty,float * xvel,float * yvel ) {
	if (fx == tx) {
		*xvel = 0;
		*yvel = 5;
	}
	else if (fy==ty)
	{
		*yvel = 0;
		*xvel = 5;
	}
	else {
		float gr = (float)(tx - fx) / (ty - fy);
		*yvel = (ty-fy)/80 * gr;
		*xvel = (tx - fx)/80 * gr;
	}
	return 1;
}

// Need target size to centre bullet on middle of target
void FireBullet(int fromx, int fromy, int tox, int toy,int targetSize) {
	if (alienBulletCount == MAXALIENS * MAXBULLETSPERALIEN) return;
	tox += targetSize / 2;
	toy += targetSize / 2;
	int index = -1;
	for (int i = 0; i<MAXBULLETS; i++) {
		if (bullets[i].active == 0) { // found a slot 
			index = i;
			break;
		}
	}
	if (index == -1) return; // no free slots as all bullets in play
	//PlayASound(CANNONSOUND);
	// Start the bullet at 25 pixels from the centre of the alien ship towards the target.

	struct bullet * pbullet = &bullets[index];	
	GetDir(fromx, fromy, tox, toy, &pbullet->vx, &pbullet->vy);
	pbullet->active = 1;
	pbullet->type = tBullet;
	pbullet->ttl = 120;
	pbullet->x = fromx + (10 * pbullet->vx);
	pbullet->y = fromy + (10 * pbullet->vy);
	pbullet->r.x = (int)pbullet->x;
	pbullet->r.y = (int)pbullet->y;
	pbullet->r.h = 3;
	pbullet->r.w = 3;
	pbullet->timer = 3;
	pbullet->countdown = 1;
	pbullet->playerbullet = index+1; 
}

// Every few frames, look around, detect any asteroid threats and shoot at them or the player!
void ControlAlienShips() {
	for (int i = 0; i < MAXALIENS; i++) {
		if (!aliens[i].active)continue;
		struct alien * palien = &aliens[i];
		if (palien->ttl-->1) continue;  // subtract 1 after checking
		palien->ttl = (int)((60 + Random(30))/speedFactor);
	
		int ax = Player.r.x;
		int ay = Player.r.y;
		int size;
		if (AnyTargetsNearBy(palien, &ax, &ay, &size)) {
			FireBullet(palien->r.x, palien->r.y, ax, ay, size);
		}
		else {
			FireBullet(palien->r.x, palien->r.y, Player.r.x, Player.r.y, 64);
		}
		if (palien->moveTimer > 0) {
			palien->moveTimer--;
		}
		if (palien->moveTimer == 0) { // Change direction
			palien->moveTimer = 10 + Random(6);
			palien->xvel = (float)3 - Random(6);
			palien->yvel = (float)3 - Random(6);
		}
	}
}

// Use global variables playerbulletcount and alienbulletcount to limit firing
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

// Updates mainFlashFlag and alien ship timers
void UpdateTimers() {
	flashTimer++;
	if (flashTimer == 20)
	{
		flashFlag = 1 - flashFlag;
		flashTimer = 0;
	}
	for (int i = 0; i < MAXALIENS; i++) {
		struct alien * palien = &aliens[i];
		if (palien->active) {
			palien->flTimer++;
			if (palien->flTimer >= ALIENFLASHFRAMES) {
				palien->flTimer = 0;
				palien->flShow = 1 - palien->flShow;
			}
		}
	}
	
}

// if shieldFlag == 0 and the shieldStrength is < 100 then increement it every 3 frames
// if shieldFlag == 1 then decrement it one every 2 frames
void AdjustShield() {
	if (shieldFlag) { // while shield up, drop 1 every frame
		if (shieldStrength > 0) {
			shieldStrength--;
		}
		if (shieldStrength == 0) {
			shieldFlag = 0;
		}
	}
	else { // Shields slowly rebuilding
		shieldTimer++;
		if (shieldTimer == 3) {
			shieldTimer = 0;
			if (shieldStrength < 100)
			{
				shieldStrength++;
			}
		}
	}
}

// Displays highscores
void DisplayHighScores() {
	char buffer[50];
	int flashIndex;
	int hsTimer;
    flashIndex = 1;
	hsTimer = 0;
	while (ProcessEvents()) {
		UpdateTimers(); // needed or flashFlag doesn't work
		int y = 130;  
		if (CloseFlag || fireFlag) return;
		RenderTexture(textures[PLBACKDROP], 0, 0);
		TextAt(330, 30, "High Scores", 3.0f);
		if (flashFlag) {
		  TextAt(400, 650, "Press space to start", 1.0f);
		}
		for (int i = 0; i < NUMSCORES; i++) {
			struct HighScoreEntry * entry = &highscores[i];
			if (entry->score) {
				snprintf(buffer, sizeof(buffer) - 1, "%02d/%02d/%4d    %3s   %02d   %06d", entry->d, entry->m, entry->y, entry->initials,entry->level, entry->score);
			}
		   else {
			   snprintf(buffer, sizeof(buffer) - 1, "--/--/----    ---   --   000000");
			}
		   if (flashIndex != i) {
			   TextAt(345, y, buffer, 1.0f);
		   }		
			y += 50;
		}
		RenderEverything();
		hsTimer++;
		if (hsTimer==18) {
			flashIndex++;
			hsTimer = 0;
			if (flashIndex == 15) flashIndex = 0;
		}
        //while (SDL_GetTicks() - tickCount < 17);		
   }
}


// main game loop handles game play 
void GameLoop() {

	tickCount = SDL_GetTicks();

	InitLevel(gameLevel);
	frameCount = 0;
	while (frameCount < 60) {// One second of text before game
		RenderTexture(textures[PLBACKDROP], 0, 0);
		DrawTextSprites();
		RenderEverything();
	}	
#ifdef TIMEGAMELOOP

	int counter = 0;
	int numTimeSlices= 0;
	double totalTime = 0.0;
#endif
	while (ProcessEvents())
	{
#ifdef TIMEGAMELOOP
		startTimer(&s);
#endif
		UpdateTimers();
		CheckPause();

		if (Player.lives == 0) break;
		ClearCellList();
		if (!paused) {
			DrawEverything();
			ControlAlienShips();
			MoveAsteroids();
			MoveAlienShips();
			RotatePlayerShip();
			CheckBulletsFired();
			MoveBullets();
			CheckJump();
			ApplyThrust();
			MovePlayerShip();

			CheckCollisions();
			CycleExplosions();
			CountBullets();
			AdjustShield();
			if (DoCoolDown()) return;  // exits loop if player destroyed

			// exit if all asteroids etc destroyed
			if (coolDown==0 && AllObjectsDestroyed()) {
				{
					gameLevel++;
					coolDown = 150;
				}
			}
		}
		//ShowCells();

		RenderEverything();

#ifdef TIMEGAMELOOP
		counter++;				
		numTimeSlices++;		
		stopTimer(&s);			
		totalTime += diff(&s);
		if (counter == 60) {		
			snprintf(timebuff, sizeof(timebuff) - 1, "%d %12.8f ",numTimeSlices, totalTime/numTimeSlices/1000000);
			counter = 0;
		}			
		startTimer(&s);
#endif
        //while (SDL_GetTicks() - tickCount < 17);
	}
}

// Sets today's date in Highscore entry
void SetTodaysDate(struct HighScoreEntry * entry) {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	entry->d = tm.tm_mday;
	entry->m = tm.tm_mon + 1;
	entry->y = tm.tm_year + 1900;
}

// Gets initials and puts in high score if score high enough
void CheckHighScore() {
	int Entered = 0;	
	int scoreIndex=-1;
	for (int i = NUMSCORES-1; i >=0; i--) {  // check in reverse order
		if (score > highscores[i].score) {
			scoreIndex = i;
		}
	}
	if (scoreIndex == -1) return;

	// if here, score is good enough to be in high score table
	char newInitials[4] = {'-','-','-',0 };	
	coolDown = 0;
	int letterIndex = -1;
	while (!Entered) {
		RenderTexture(textures[PLBACKDROP], 0, 0);
		UpdateTimers();
		TextAt(350, 30, "New High Score!", 3.0f);
		TextAt(200, 200, "Enter Initials:",1.5F);
		TextAt(500, 200, newInitials,1.5F);
		if (flashFlag) {
			TextAt(350, 5000, "Press three keys", 1.0f);
		}			
		RenderEverything();
		while (SDL_PollEvent(&event)) {
			if (coolDown == 0) {
				switch (event.type) {
				case SDL_KEYDOWN:
					keypressed = event.key.keysym.sym;
					if (keypressed >= SDLK_a && keypressed <= SDLK_z) {
						letterIndex++;
						newInitials[letterIndex] = keypressed - 32; // A--Z
						if (letterIndex == 2) {
							coolDown = 50; // Time for 3rd letter to be seen on screen
						} // if letterIndex
					} // if keypressed
				} // switch
			} // if
		}
		if (coolDown > 0) {
			coolDown--;
			if (coolDown == 0) {
				Entered = 1;
				break;
			}
		}
        while (SDL_GetTicks() - tickCount < 17);
	}
	// Shuffle all highscores after new one, down one
	for (int i = NUMSCORES-1; i > scoreIndex; i--) {
		highscores[i] = highscores[i - 1];
	}
	// Set new scores
	struct HighScoreEntry * entry = &highscores[scoreIndex];
	SetTodaysDate(entry);
	for (int i = 0; i < 4; i++) {
		entry->initials[i] = newInitials[i];
	}
	entry->level = gameLevel;
	entry->score = score;
	WriteHighScores();
}

// Program main entry point
int main(int argc, char * args[])
{
	SetPiFlag();
	InitLogging("biglog.txt");
	InitSetup();
	if (errorCount) {
		return -1;
	}
	gameLevel = 0;
	while(!CloseFlag) {	
		DisplayHighScores(); 			
		if (CloseFlag) break;
		InitGame();
		while (Player.lives){
			{
				GameLoop();
				if (CloseFlag) break;
			}
		};
		// Game over so check if made into high score table
		CheckHighScore();
	};
	CloseLogging();
	FinishOff();
    return 0;
}
