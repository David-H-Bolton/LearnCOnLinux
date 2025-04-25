// Chapter 30 Linux
#include "hr_time.h"
#include <time.h>
#include "SDL2/SDL.h"   // All SDL Applications need this 
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_mixer.h"
#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>
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
#define NUMTEXTURES 14
#define NUMEXPLOSIONSOUNDS 2


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

// structs
struct asteroid {
	float x, y;
	int active;
	float xvel, yvel;
	int xtime, ytime;
	int xcount, ycount;
	int rotclockwise;
	int rottime;
	int rotcount;
	int rotdir;
	int size;
};

struct bullet {
	float x, y;
	int timer;
	float vx, vy;
	int ttl; // time to live
	int countdown;
};

struct explosion {
	int x, y;
	int countdown;
	int frame;
	int dimension;
};

struct player {
	float x, y;
	int dir; // 0-23 
	float vx, vy;
	int lives;
} Player;

// SDL variables
SDL_Window* screen = NULL;
SDL_Renderer *renderer;
SDL_Event event;
SDL_Rect source, destination, dst,sourceRect,destRect;
SDL_Texture* textures[NUMTEXTURES];

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
int piFlag;
long tempCount;
float lastTemp;

// array variables
char buffer[100],buffer2[100];
float thrustx[24];
float thrusty[24];

// struct variables
struct asteroid asteroids[MAXASTEROIDS];
struct bullet bullets[MAXBULLETS];
struct explosion explosions[MAXEXPLOSIONS];
struct Mix_Chunk * sounds[NUMSOUNDS];
stopWatch s;

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
void SetCaption(char *msg)
{
	if (showfps)
	{
		snprintf(buffer, sizeof(buffer), "Fps = %d %s", frameCount, msg);
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

// Loads all Textures 
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
	LoadTextures();
	int success=Mix_OpenAudio(44000, AUDIO_F32LSB, 2, 8192);
	if (success==-1 ) {
		LogError2("InitSetup failed to init audio because %s",Mix_GetError());
	}
	LoadSoundFiles();
	InitThrustAngles();

}

void DestroySounds() {
	for (int i = NUMSOUNDS - 1; i >= 0; i--) {
		Mix_FreeChunk(sounds[i]);
	}
}

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
	SDL_Quit();
	exit(0);
}

// Plays a sound from sounds[]
void PlayASound(int soundindex) {
	int success=Mix_PlayChannel(-1, sounds[soundindex], 0);
	if (success == -1) {
		LogError2("InitSetup failed to init audio because %s",Mix_GetError());
	}

}

void renderTexture(SDL_Texture *tex, int x, int y) {
	//Setup the destination rectangle to be at the position we want
	dst.x = x;
	dst.y = y;
	//Query the texture to get its width and height to use
	SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
	SDL_RenderCopy(renderer, tex, NULL, &dst);
}


// print char at rect target 
void printch(char c, SDL_Rect * target) {
	int start = (c - '!');
	if (c != ' ') {
		sourceRect.h = 23;
		sourceRect.w = 12;
		sourceRect.x = start * 12;
		sourceRect.y = 0;
		SDL_RenderCopy(renderer, textures[TEXTTEXTURE], &sourceRect, target);
	}
	(*target).x += 13;
}

// print string text at x,y pixel coords 
void TextAt(int atX, int atY, char * msg) {
	destRect.h = 23;
	destRect.w = 12;
	destRect.x = atX;
	destRect.y = atY;
	for (int i = 0; i < (int)strlen(msg); i++) {
		printch(msg[i], &destRect);
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

void UpdateCaption()
{
	float temp = ReadPiTemperature();
	snprintf(buffer2, sizeof(buffer2), "%10.6f Pi=%d Asteroids =%d temp=%5.2f C", diff(&s),piFlag,numAsteroids,temp);
	tickCount = SDL_GetTicks();
	if (tickCount - lastTick >= 1000) {
		lastTick = tickCount;
		if (showfps) {
			SetCaption(buffer2);
			frameCount = 0;
		}
	}
	else if (!showfps) {
		SetCaption(buffer2);
	}
}

// Draw player ship 
void DrawPlayerShip() {
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
	TextAt((int)Player.x - 50, (int)Player.y + 66, buff);
	snprintf(buff, sizeof(buff), "(%6.4f,%6.4f)", Player.vx, Player.vy);
	TextAt((int)Player.x - 50, (int)Player.y + 90, buff);
}

// Draw All asteroids 
void DrawAsteroids() {
	SDL_Rect asteroidRect, target;
	numAsteroids = 0;
	asteroidRect.y = 0;
	for (int i = 0; i<MAXASTEROIDS; i++) {
		if (asteroids[i].active) {
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
			TextAt(target.x + 25, target.y + 25, SDL_ltoa(i,buff,10));
			TextAt(target.x + 25, target.y + 55, SDL_ltoa(sizeIndex, buff, 10));
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
		if (bullets[i].ttl >0) {
			target.x = (int)bullets[i].x;
			target.y = (int)bullets[i].y;
			SDL_RenderCopy(renderer, textures[TEXTUREBULLET], &spriterect, &target);
			if (debugFlag)
			{
				snprintf(buff, 10, "%i", bullets[i].ttl);
				TextAt((int)bullets[i].x + 5, (int)bullets[i].y, buff);
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
				TextAt(target.x + 10, target.y+ EXPLOSIONSIZE, buff);
			}
		}
	}
}

// Show score and lives on screen 
void DrawScoreAndLives() {
	char scorebuffer[10];
	SDL_Rect destr;
	snprintf(scorebuffer,sizeof(scorebuffer), "%i", score);
	TextAt(25, HEIGHT-30, scorebuffer);
	destr.h = 33;
	destr.w = 24;
	destr.y = HEIGHT - 70;
	for (int i = 0; i < Player.lives; i++) {
		destr.x = (i * 30) + 50;
		SDL_RenderCopy(renderer, textures[TEXTURESMALLSHIP],NULL, &destr);
	}
}

// Draw Paused if paused == 1 flashing at twice per second
void DrawPauseMessage(char * message) {
	if (paused == 0) return;
	if (frameCount < 16 || frameCount > 45) {
		TextAt((WIDTH / 2) - 20, (HEIGHT / 2) - 40, message);
	}
}

// Copies Asteroids, playership, bullets etc to offscreen buffer
void RenderEveryThing() {	
	startTimer(&s);
	renderTexture(textures[PLBACKDROP], 0, 0);
	DrawAsteroids();
	DrawPlayerShip();
	DrawBullets();
	DrawExplosions();
	DrawScoreAndLives();	
	DrawPauseMessage("Paused");
	stopTimer(&s);	
	UpdateCaption();	
	SDL_RenderPresent(renderer); // make offscreen buffer vsisble
	frameCount++;
}

// Initialize Player struct 
void InitPlayer() {
	Player.dir = 0;
	Player.vy = 0;
	Player.vy = 0;
	Player.lives = 3;
	Player.x = 500;
	Player.y = 360;
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
		bullets[i].x = 0;
		bullets[i].y = 0;
		bullets[i].timer = 0;
		bullets[i].ttl = 0;
		bullets[i].countdown = 0;
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

// Called to initialise each game
void InitGame() {
	rotTimer = SDL_GetTicks();
	speedTimer = rotTimer;
	fireTimer = rotTimer;
	jumpTimer = rotTimer;
	pauseTimer = rotTimer;
	score = 0;
	paused = 0;
	InitPlayer();
	InitAsteroids();
	InitBullets();
	InitExplosions();
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
	Player.x += Player.vx;
	Player.y += Player.vy;
	if (Player.x <= -5)
		Player.x += WIDTH;
	else
		if (Player.x > WIDTH-4)
			Player.x = 0;
	if (Player.y <= -5)
		Player.y += HEIGHT;
	else
		if (Player.y > HEIGHT-4)
			Player.y = 0;
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
	if (asteroids[index].xtime >0) {
		asteroids[index].xtime--;
		if (asteroids[index].xtime <= 0) {
			asteroids[index].x += asteroids[index].xvel;
			asteroids[index].xtime = asteroids[index].xcount;
			if (asteroids[index].x < -sizes[asteroids[index].size] || asteroids[index].x > WIDTH)
				asteroids[index].active = 0;
		}
	}
	if (asteroids[index].ytime >0) {
		asteroids[index].ytime--;
		if (asteroids[index].ytime <= 0) {
			asteroids[index].y += asteroids[index].yvel;
			asteroids[index].ytime = asteroids[index].ycount;
			if (asteroids[index].y < -sizes[asteroids[index].size] || asteroids[index].y > HEIGHT)
				asteroids[index].active = 0;
		}
	}

	if (asteroids[index].rottime >0) {
		asteroids[index].rottime--;
		if (asteroids[index].rottime <= 0)
		{
			asteroids[index].rottime = asteroids[index].rotcount;
			if (asteroids[index].rotclockwise) {
				asteroids[index].rotdir = (asteroids[index].rotdir + 1) % 24;
			}
			else {
				asteroids[index].rotdir = ((asteroids[index].rotdir - 1) + 24) % 24;
			}
		}
	}
}

// Move All Asteroids
void MoveAsteroids() {
	for (int i = 0; i<MAXASTEROIDS; i++) {
		{
			if (asteroids[i].active) {
				MoveAsteroid(i);
				//oa = HitAsteroid(i);
				//if (oa != -1) {
				//	DestroyAsteroid(i);
				//	DestroyAsteroid(oa);
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

// adds to table , size = 0..3
void AddAsteroid(int size) {
	int index = FindFreeAsteroidSlot();
	if (index == -1) // table full so quietly exit 
		return;
	if (size == -1) {  // Use -1 to sepcify a random size
		size = Random(4) - 1;
	}

	asteroids[index].active = 1;
	asteroids[index].x =(float) Random(WIDTH - size) + 20;
	asteroids[index].y = (float)Random(HEIGHT - size) + 20;
	asteroids[index].rotdir = Random(24);
	asteroids[index].rotclockwise = Random(2) - 1;
	asteroids[index].xvel = (float)4 - Random(8);
	asteroids[index].yvel = (float)4 - Random(8);
	asteroids[index].xtime = 2 + Random(5);
	asteroids[index].xcount = asteroids[index].xtime;
	asteroids[index].ytime = 2 + Random(5);
	asteroids[index].ycount = asteroids[index].ytime;
	asteroids[index].rottime = 2 + Random(8);
	asteroids[index].rotcount = asteroids[index].rottime;
	asteroids[index].size = size;
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

// DestroyAsteroid, create 4 smaller plus 4 size 3 
void DestroyAsteroid(int deadIndex) {
	asteroids[deadIndex].active = 0;
	AddExplosion((int)asteroids[deadIndex].x, (int)asteroids[deadIndex].y);	
	int size = asteroids[deadIndex].size;
	if (size == 3)
		return; // it's destroyed, not split so just exit 
	// add in 4 smaller and 4 smallest type asteroids
	float xvel = asteroids[deadIndex].xvel;
	float yvel = asteroids[deadIndex].yvel;
	int x = (int)asteroids[deadIndex].x;
	int y = (int)asteroids[deadIndex].y;
	for (int i = 0; i<8; i++) {			
		int index = FindFreeAsteroidSlot();
		if (index == -1)
			return; // no more asteroid slots free so don't add any (rare!)
		int  newsize;
		if (i % 2 == 0) // if i is even add in smallest size 3 
			newsize = 3;
		else
			newsize = size + 1; // next smaller size 

		asteroids[index].active = 1;
		asteroids[index].x = (float)xdir[i] * sizes[size] + x;
		asteroids[index].y = (float)ydir[i] * sizes[size] + y;
		asteroids[index].rotdir = 0;
		asteroids[index].rotclockwise = Random(2) - 1;
		asteroids[index].xvel = xdir[i] * Random(4) + xvel;
		asteroids[index].yvel = ydir[i] * Random(4) + yvel;
		asteroids[index].xtime = 2 + Random(5);
		asteroids[index].xcount = asteroids[index].xtime;
		asteroids[index].ytime = 2 + Random(5);
		asteroids[index].ycount = asteroids[index].ytime;
		asteroids[index].rottime = 2 + Random(8);
		asteroids[index].rotcount = asteroids[index].rottime;
		asteroids[index].size = newsize;
	}
}

// test code 
void BlowUpAsteroids() {
	for (int i = 0; i<MAXASTEROIDS; i++) {
		if (asteroids[i].active && Random(10) <4) {
			DestroyAsteroid(i);
		}
	}
}



// fire a buller- first work out where it should appear relative to player ship then add to bullets
void DoFireBullet() {
	int index = -1;
	for (int i = 0; i<MAXBULLETS; i++) {
		if (bullets[i].ttl == 0) { // found a slot 
			index = i;
			break;
		}
	}	
	if (index == -1) return; // no free slots as all bullets in play
	PlayASound(CANNONSOUND);
	int x = (int)round(Player.x + bulletx[Player.dir]);
	int y = (int)round(Player.y + bullety[Player.dir]);

	bullets[index].ttl = 120;
	bullets[index].x = x * 1.0f;
	bullets[index].y = y * 1.0f;
	bullets[index].timer = 3;
	bullets[index].countdown = 1;
	bullets[index].vx = Player.vx + thrustx[Player.dir] * 5;
	bullets[index].vy = Player.vy + thrusty[Player.dir] * 5;
}

// move bullets * 
void MoveBullets() {
	for (int i = 0; i< MAXBULLETS; i++) {
		if (bullets[i].countdown >0) {
			bullets[i].countdown--;
			if (bullets[i].countdown == 0) {
				if (bullets[i].ttl >0)
				{
					bullets[i].ttl--;
					if (bullets[i].ttl == 0)
						break; // expired 
				}
				// move it 
				bullets[i].countdown = bullets[i].timer;
				float vx = bullets[i].x + bullets[i].vx;
				float vy = bullets[i].y + bullets[i].vy;
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
				bullets[i].x = vx;
				bullets[i].y = vy;
			}
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
						showfps = 1 - showfps;
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
						for (int i = 0; i < 10; i++) {
							AddAsteroid(-1);
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
		paused = 1-paused;
	}
}

// doHyperJump() find empty space on screen
void CheckJump() {
	int hx, hy;

	if (jumpFlag && SDL_GetTicks() - jumpTimer > 3000) {
		jumpTimer = SDL_GetTicks();
		jumpFlag  = 0;
		do {
			hx = 50 + Random(WIDTH - 100);
			hy = 50 + Random(HEIGHT - 100);
		}
		while(0);
		//} while (PlayerHitAsteroid(hx, hy) != -1);
		Player.x = (float)hx;
		Player.y = (float)hy;
		Player.vx = 0;
		Player.vy = 0;
	}
}


// if it detects running on a Raspberry pi - set piFlag to 1
void SetPiFlag() {
	struct utsname buffer;
	piFlag =0;
    if (uname(&buffer) != 0) return;
    if (strcmp(buffer.nodename,"raspberrypi") !=0) return;
	if (strcmp(buffer.machine,"armv7l") != 0 &&
	   strcmp(buffer.machine,"aarch64") !=0) return;
	piFlag=1;
}

// main game loop handles game play
void GameLoop()
{
	tickCount = SDL_GetTicks();

	while (ProcessEvents())
	{	
		CheckPause();
		if (!paused) {
			MoveAsteroids();
			RotatePlayerShip();
			CheckJump();
			ApplyThrust();
			MovePlayerShip();
			CheckBulletsFired();
			MoveBullets();
			CycleExplosions();
		}
		RenderEveryThing();
	}
}

int main(int argc,char * args[])
{
	SetPiFlag();
	InitSetup();
	if (errorCount) {
		return -1;
	}

	InitGame();
	GameLoop();
	FinishOff();
    return 0;
}
