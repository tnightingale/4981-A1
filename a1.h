
#define CHILD_PROCESS 0
#define OUTPUT_PROC 0
#define TRANSLATE_PROC 1
#define BUFFSIZE 80

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main();
void input_proc(int[2], int[2]);
void output_proc(int[2], int[2]);
void translate_proc(int[2], int[2]);
void fatal(char *);