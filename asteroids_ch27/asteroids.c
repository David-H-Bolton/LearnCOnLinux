// Chapter 27 Linux
#include "hr_time.h"
#include <time.h>
#include "SDL2/SDL.h"   // All SDL Applications need this 
#include "SDL2/SDL_image.h"
#include <stdio.h>
#include <stdlib.h>
#include "lib.h"


// key definitions
#define QUITKEY SDLK_ESCAPE
#define DEBUGKEY SDLK_TAB
#define COUNTERCLOCKWISE SDLK_q
#define CLOCKWISE SDLK_w
#define THRUSTKEY SDLK_LCTRL
#define ADDASTEROIDKEY SDLK_a
#define FIREKEY SDLK_SPACE

// sizes
#define WIDTH 1024
#define HEIGHT 768
#define SHIPHEIGHT 64
#define SHIPWIDTH 64

// numbers of things
#define MAXASTEROIDS 256
#define MAXBULLETS 16
#define NUMTEXTURES 9

// Texture indices
#define PLBACKDROP 0
#define TEXTTEXTURE 1
#define TEXTUREPLAYERSHIP 2
#define TEXTUREDEBUG 3
#define TEXTUREASTEROID1 4  // 4,5,6,7
#define TEXTUREBULLET 8

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
int debugFlag;
int fireFlag;
int rotTimer,speedTimer, fireTimer;
int rotateFlag, thrustFlag;
int rotateFlag; // 0 do nowt, 1 counter clockwise, 2 clockwise 
int numAsteroids = 0;

// array variables
char buffer[100],buffer2[100];
float thrustx[24];
float thrusty[24];

// struct variables
struct asteroid asteroids[MAXASTEROIDS];
struct bullet bullets[MAXBULLETS];
stopWatch s;

// consts
const char * texturenames[NUMTEXTURES] = { "images/starfield.png","images/text.png","images/playership.png","images/debug.png",
"images/a1.png","images/a2.png","images/a3.png","images/a4.png","images/bullet.png" };
const int sizes[4] = { 280,140,70,35 };
const int debugOffs[4] = { 0,280,420,490 };
const int bulletx[24] = { 32,38,50,64,66,67,68,67,66,64,50,38,32,26,16, 0,-2,-3,-4,-3,-2,0,16,24 };
const int bullety[24] = { -4,-3,-2, 0,16,26,32,38,50,64,66,67,71,70,69,64 ,50,38,32,26,16,0,-2,-3 };

// external variables
extern int errorCount;


// Sets Window caption according to state - eg in debug mode or showing fps 
void SetCaption(char * msg) {
	if (showfps) {
		snprintf(buffer,sizeof(buffer), "Fps = %d #Asteroids =%d %s", frameCount+1,numAsteroids,msg);
		SDL_SetWindowTitle(screen, buffer);
	}
	else {
		SDL_SetWindowTitle(screen, msg);
	}
}

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

// Init thrustx and thrusty[] 
void InitThrustAngles() {
	const float pi = 3.14159265f;
	float degree = 0.0f;
	for (int i = 0; i<24; i++) {
		thrustx[i] = (float)sin(degree*pi / 180.0f);
		thrusty[i] = (float)-cos(degree*pi / 180.0f);
		degree += 15;
	}
}

// Initialize all setup, set screen mode, load images etc 
void InitSetup() {
	errorCount = 0;
	srand((int)time(NULL));
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, SDL_WINDOW_SHOWN, &screen, &renderer);
	if (!screen) {
		LogError("InitSetup failed to create window");
	}
	SetCaption("Example Two");
	LoadTextures();
}

// Cleans up after game over 
void FinishOff() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(screen);
	//Quit SDL
	SDL_Quit();
	exit(0);
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

void UpdateCaption() {
	snprintf(buffer2, sizeof(buffer2), "%10.6f", diff(&s));
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
	char buff[30];
	SDL_Rect asteroidRect, target;
	numAsteroids = 0;
	asteroidRect.y = 0;
	for (int i = 0; i<MAXASTEROIDS; i++) {
		if (asteroids[i].active) {
			numAsteroids++;
			int size = asteroids[i].size;
			int asize = sizes[size];
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
			SDL_RenderCopy(renderer, textures[TEXTUREASTEROID1 + size], &asteroidRect, &target);
			if (!debugFlag)
				continue; // In a loop so carry on with next asteroid
			asteroidRect.x = debugOffs[size];
			SDL_RenderCopy(renderer, textures[TEXTUREDEBUG], &asteroidRect, &target);
			TextAt(target.x + 25, target.y + 25, SDL_ltoa(i,buff,10));
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
			SDL_RenderCopy(renderer,textures[TEXTUREBULLET], &spriterect, &target);
			if (debugFlag) {
				snprintf(buff, 10,"%i", bullets[i].ttl);
				TextAt((int)bullets[i].x + 5, (int)bullets[i].y, buff);
			}
		}
	}
}

void RenderEveryThing() {	
	startTimer(&s);
	renderTexture(textures[PLBACKDROP], 0, 0);
	DrawAsteroids();
	DrawPlayerShip();
	DrawBullets();
	stopTimer(&s);
	SDL_RenderPresent(renderer);
	frameCount++;
	UpdateCaption();
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

// Called to initialise each game
void InitGame() {
	rotTimer = SDL_GetTicks();
	speedTimer = rotTimer;
	fireTimer = rotTimer;
	InitPlayer();
	InitThrustAngles();
	InitAsteroids();
	InitBullets();
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

// fire a buller- first work out where it should appear relative to player ship then add to bullets
void doFireBullet() {
	int index = -1;
	for (int i = 0; i<MAXBULLETS; i++) {
		if (bullets[i].ttl == 0) { // found a slot 
			index = i;
			break;
		}
	}	
	if (index == -1) return; // no free slots as all bullets in play
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

// Handle all key presses
int ProcessEvents() {
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_KEYDOWN:
				keypressed = event.key.keysym.sym;
				switch (keypressed) {
					case QUITKEY:
						return 0; 
					case COUNTERCLOCKWISE:
						rotateFlag = 1;
						break;
					case CLOCKWISE:
						rotateFlag = 2;
						break;
					case DEBUGKEY:
						debugFlag = 1 - debugFlag;
						showfps = 1 - showfps;
						break;
					case THRUSTKEY:
						thrustFlag = 1;
						break;
					case ADDASTEROIDKEY:
						for (int i = 0; i < 100; i++) {
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
				break;
			}
		} // switch event.type	

	} // while

	return 1; 
}

void CheckBulletsFired() {
	if (fireFlag && SDL_GetTicks() - fireTimer > 100) {
		fireTimer = SDL_GetTicks();
		doFireBullet();
	}
}

// main game loop handles game play 
void GameLoop() {
	tickCount = SDL_GetTicks();

	while (ProcessEvents())
	{	
		MoveAsteroids();
		RotatePlayerShip();
		ApplyThrust();
		MovePlayerShip();
		CheckBulletsFired();
		MoveBullets();
		RenderEveryThing();
		while (SDL_GetTicks() - tickCount < 17); // frame rate cap ~60 fps
	}
}

int main(int argc,char * args[])
{
	InitSetup();
	if (errorCount) {
		return -1;
	}
	InitGame();
	GameLoop();
	FinishOff();
    return 0;
}
