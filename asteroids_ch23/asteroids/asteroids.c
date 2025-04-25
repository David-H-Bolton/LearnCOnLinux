// Chapter 23
#include "hr_time.h"
#include <time.h>
#include "SDL2/SDL.h"   /* All SDL App's need this */
#include "SDL2/SDL_image.h"
#include <stdio.h>
#include <stdlib.h>


#define QUITKEY SDLK_ESCAPE
#define DEBUGKEY SDLK_TAB
#define COUNTERCLOCKWISE SDLK_q
#define CLOCKWISE SDLK_w
#define THRUSTKEY SDLK_LCTRL

#define WIDTH 1024
#define HEIGHT 768
#define SHIPHEIGHT 64
#define SHIPWIDTH 64

#define NUMTEXTURES 4
// Texture indices
#define PLBACKDROP 0
#define TEXTTEXTURE 1
#define TEXTUREPLAYERSHIP 2
#define TEXTUREDEBUG 3

SDL_Window* screen = NULL;
SDL_Renderer *renderer;
SDL_Event event;
SDL_Rect source, destination, dst,sourceRect,destRect;
SDL_Texture* textures[NUMTEXTURES];

int errorCount = 0;
int keypressed;
int rectCount = 0;
int frameCount,tickCount,lastTick, showfps;
int debugFlag;
int rotTimer,speedTimer;
int rotateFlag, thrustFlag;
int rotateFlag; /* 0 do mowt, 1 counter clockwise, 2 clockwise */
char buffer[100],buffer2[100];
float thrustx[24];
float thrusty[24];
stopWatch s;

struct player {
	float x, y;
	int dir; /* 0-23 */
	float vx, vy;
	int lives;
} Player;


const char * texturenames[NUMTEXTURES] = { "images/starfield.png","images/text.png","images/playership.png","images/debug.png" };

/* returns a number between 1 and max */
int Random(int max) {
	return (rand() % max) + 1;
}

void LogError(char * msg) {
	FILE * ferr= fopen("errorlog.txt", "a");
	printf("%s\n", msg);
	fclose(ferr);
	errorCount++;
}

void LogError2(const char * msg1, const char * msg2) {
	FILE * ferr;
	ferr = fopen("errorlog.txt", "a");
	fprintf(ferr,"%s %s\n", msg1,msg2);
	fclose(ferr);
	errorCount++;
}

/* Sets Window caption according to state - eg in debug mode or showing fps */
void SetCaption(char * msg) {
	if (showfps) {
		snprintf(buffer,sizeof(buffer), "Fps = %d %s", frameCount,msg);
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

/* Loads Textures */
void LoadTextures() {
	for (int i = 0; i<NUMTEXTURES; i++) {
		textures[i] = LoadTexture(texturenames[i], renderer);
	}
}

/* Init thrustx and thrusty[] */
void InitThrustAngles() {
	const float pi = 3.14159265f;
	float degree;
	degree = 0.0f;
	for (int i = 0; i<24; i++) {
		thrustx[i] = (float)sin(degree*pi / 180.0f);
		thrusty[i] = (float)-cos(degree*pi / 180.0f);
		degree += 15;
	}
}

/* Initialize all setup, set screen mode, load images etc */
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
	InitThrustAngles();
}

/* Cleans up after game over */
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


/* print char at rect target */
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

// print string text at x,y pixel coords */
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

void RenderEveryThing() {	
	startTimer(&s);
	renderTexture(textures[PLBACKDROP], 0, 0);
	DrawPlayerShip();
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

// Called to initialise each game
void InitGame() {
	rotTimer = SDL_GetTicks();
	speedTimer = rotTimer;
	InitPlayer();
}

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

// move the player ship 
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
				} // switch keypressed
				break;
			case SDL_QUIT: /* if mouse click to close window */
			{
				return 0;
			}

			case SDL_KEYUP: {
				rotateFlag = 0;
				thrustFlag = 0;
				break;
			}
		} // switch event.type	

	} // while

	return 1; 
}

/* main game loop handles game play */
void GameLoop() {
	tickCount = SDL_GetTicks();

	while (ProcessEvents())
	{	
		RotatePlayerShip();
		ApplyThrust();
		MovePlayerShip();
		RenderEveryThing();
		while (SDL_GetTicks() - tickCount < 17); // delay it to ~60 fps
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
