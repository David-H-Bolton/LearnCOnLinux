// Chapter 22
#include "hr_time.h"
#include <time.h>
#include "SDL2/SDL.h"   /* All SDL App's need this */
#include "SDL2/SDL_image.h"
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>


#define QUITKEY SDLK_ESCAPE
#define DEBUGKEY SDLK_TAB
#define COUNTERCLOCKWISE SDLK_q
#define CLOCKWISE SDLK_w
#define THRUSTKEY SDLK_LCTRL

#define WIDTH 1024
#define HEIGHT 768

#define NUMTEXTURES 2
// Texture indices
#define PLBACKDROP 0
#define TEXTTEXTURE 1

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
int rotateFlag, thrustFlag;
char buffer[100],buffer2[100];
stopWatch s;

const char * texturenames[NUMTEXTURES] = { "images/starfield.png","images/text.png" };

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
	FILE * ferr= fopen("errorlog.txt", "a");
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

/* Initialize all setup, set screen mode, load images etc */
void InitSetup() {
	srand((int)time(NULL));
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, SDL_WINDOW_SHOWN, &screen, &renderer);
	if (!screen) {
		LogError("InitSetup failed to create window");
	}
	SetCaption("Example Two");
	LoadTextures();
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
	if (tickCount - lastTick >= 1000) { // Only update caption once per second
		lastTick = tickCount;
		if (showfps) {
			snprintf(buffer2, sizeof(buffer2), "%10.6f %d", diff(&s),frameCount);			
			SetCaption(buffer2);
			frameCount = 0;
		}
	}
	else if (!showfps) {
		SetCaption(buffer2);
	}
}

void RenderEveryThing() {
	int atX, atY;

	renderTexture(textures[PLBACKDROP], 0, 0);
	startTimer(&s);
	for (int i=0;i<500;i++) {
		atX = Random(WIDTH-50) + 1;
		atY = Random(HEIGHT) + 20;
		snprintf(buffer, sizeof(buffer), "%d", i); // comment this and the next line
		TextAt(atX, atY, buffer);		
		//TextAt(atX, atY, SDL_ltoa(i,buffer,10)); // uncomment this
	}	
	stopTimer(&s);
	SDL_RenderPresent(renderer);
	frameCount++;
	UpdateCaption();
}

/* Handle all key presses except esc */
int ProcessEvents() {
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_KEYDOWN:
				keypressed = event.key.keysym.sym;
				syslog(LOG_INFO,"keypressed = %d",keypressed);
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
						return 1;
						break;
					case THRUSTKEY:
						thrustFlag = 1;
						break;
				} // switch
			case SDL_QUIT: /* if mouse click to close window */
			{
				return 0;
			}

			case SDL_KEYUP: {
				break;
			}
		} // switch 
	} // while
	return 1; 
}

/* main game loop handles game play */
void GameLoop() {
	int retval;
	tickCount = SDL_GetTicks();

	while (1)
	{		
		retval = ProcessEvents();
		syslog(LOG_INFO,"Retval = %d",retval);
		RenderEveryThing();
		if (!retval) break;
		//while (SDL_GetTicks() - tickCount < 17); // delay it to ~60 fps
	}
}

int main(int argc,char * args[])
{
	openlog("asteroids",LOG_CONS | LOG_PID | LOG_NDELAY, LOG_USER);
	InitSetup();
	GameLoop();
	FinishOff();
	closelog();
    return 0;
}
