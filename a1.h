
#define CHILD_PROCESS 0
#define OUTPUT_PROC 0
#define TRANSLATE_PROC 1
#define BUFFSIZE 81
#define TRUE 1
#define FALSE 0
#define KILL 11

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main();
void input_proc(int[2], int[2], int[2]);
void output_proc(int[2], int[2]);
void translate_proc(int[2], int[2]);
int read_pipe(int, char*, size_t);
void fatal(char *);