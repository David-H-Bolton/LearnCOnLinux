#pragma once

int Random(int max);
void LogError(char * msg);
void LogError2(const char * msg1, const char * msg2);
char * sltoa(int n);

// Logging functions 
void l(char * loc);
void l2(char * loc, char * msg);
void ln(char * loc, char * msg, int n);
void InitLogging(char * filename);
void CloseLogging();

