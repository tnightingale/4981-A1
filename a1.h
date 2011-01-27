/** 
 * @package a1
 * @file    a1.h
 * @author  Tom Nightingale
 *
 * @brief   See: a1.c.
 * 
 */
 
 
#define CHILD_PROCESS 0
#define OUTPUT_PROC 0
#define TRANSLATE_PROC 1
#define PIPE_INPUT_OUTPUT 0
#define PIPE_INPUT_TRANSLATE 1
#define PIPE_TRANS_OUTPUT 2
#define BUFFSIZE 81
#define TRUE 1
#define FALSE 0
#define KILL 11
#define READ 0
#define WRITE 1

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main();
void input_proc(int[2], int, int);
void output_proc(int, int);
void translate_proc(int, int);
int read_pipe(int, char*, size_t);
int write_pipe(int, const void*, size_t);
void fatal(char *);