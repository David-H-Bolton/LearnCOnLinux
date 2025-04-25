#include <stdio.h>
#include <stdlib.h>
#include "lib.h"

int errorCount = 0;
FILE * dbf;
char fbuff[100];
char ibuff[20];
char convbuff[20];
char ltoabuff[10];

// returns a number between 1 and max 
int Random(int max) {
	return (rand() % max) + 1;
}

char * sltoa(int n) {
	snprintf(ltoabuff,sizeof(ltoabuff), "%d", n);
	return ltoabuff;
}

// Log Single errors
void LogError(char * msg) {
	FILE * err= fopen( "errorlog.txt", "a");
	fprintf(err,"%s\n", msg);
	fclose(err);
	errorCount++;
}

// Log Errors with two parameters
void LogError2(const char * msg1, const char * msg2) {
	FILE * err= fopen( "errorlog.txt", "a");
	fprintf(err, "%s %s\n", msg1, msg2);
	fclose(err);
	errorCount++;
}

void l2(char * loc, char * msg) {
	fprintf(dbf, "%s = %s\n", loc, msg);
}

void lc(char * loc, char msg) {
	fprintf(dbf, "%s = %c\n", loc, msg);
}

void l(char * loc) {
	fprintf(dbf, "%s\n", loc);
}

void li(char * loc,int n){
	fprintf(dbf, "%s = %d\n", loc,n);
}

void ln(char * loc, char * msg, int n) {
	fprintf(dbf, "%s = %s %s\n", loc, msg, sltoa(n));
}

void InitLogging(char * filename) {
	dbf = fopen( filename, "wt");
}

void CloseLogging() {
	fclose(dbf);
}

