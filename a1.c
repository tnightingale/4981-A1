/** 
 * @package a1
 * @file    a1.c
 * @author  Tom Nightingale
 *
 * @brief   This program demonstrates simple Linux IPC using pipes and signals.
 * @details 3 processes are created:
 *  1. Input (parent)
 *      This process manages all keyboard input. It passes characters to the
 *      output process for display while also storing them in a buffer. When
 *      the special <ENTER> key is pushed, the buffer is then sent to the
 *      translate process for parsing.
 *  2. Output
 *      This process outputs all characters sent to it.
 *  3. Translate
 *      This process parses any string sent to it according to the rules laid 
 *      out in the assignment brief. Once finished, it passes the resulting
 *      string onto the output process for display.
 */

#include "a1.h"


/**
 * @author  Tom Nightingale
 *
 * @brief   The program's entry point.
 * @details Initializes the processes and ports for IPC.
 */
int main() {  
  int pfd[3][2];
  int i = 0;
  int pid[2];

  // Put keyboard into raw mode
  system("stty raw -icanon igncr -echo");
  
  // Open the pipes.
  for (i = 0; i < 3; i++) {
    if (pipe(pfd[i]) < 0) {
      fatal("pipe call");
    }
  }
  
  // Create 2 child processes; OUTPUT_PROC & TRANSLATE_PROC.
  for (i = 0; i < 2; i++) {
    if ((pid[i] = fork()) <= 0) {
      switch (i) {
        // Output process.
        case OUTPUT_PROC:
          
          // Closing unused pipe descriptors.
          close(pfd[PIPE_INPUT_OUTPUT][WRITE]);
          close(pfd[PIPE_TRANS_OUTPUT][WRITE]);
          close(pfd[PIPE_INPUT_TRANSLATE][READ]);
          close(pfd[PIPE_INPUT_TRANSLATE][WRITE]);
          
          output_proc(pfd[PIPE_INPUT_OUTPUT][READ], pfd[PIPE_TRANS_OUTPUT][READ]);
          return 0;

        // Translate process.  
        case TRANSLATE_PROC:
          
          // Closing unused pipe descriptors.
          close(pfd[PIPE_INPUT_TRANSLATE][WRITE]);
          close(pfd[PIPE_TRANS_OUTPUT][READ]);
          close(pfd[PIPE_INPUT_OUTPUT][READ]);
          close(pfd[PIPE_INPUT_OUTPUT][WRITE]);
          
          translate_proc(pfd[PIPE_INPUT_TRANSLATE][READ], pfd[PIPE_TRANS_OUTPUT][WRITE]);
          return 0;
      }
    }
  }
  

  // Input process (parent).

  // Closing unused pipe descriptors.
  close(pfd[PIPE_INPUT_OUTPUT][READ]);
  close(pfd[PIPE_INPUT_TRANSLATE][READ]);
  close(pfd[PIPE_TRANS_OUTPUT][READ]);
  close(pfd[PIPE_TRANS_OUTPUT][WRITE]);
  input_proc(pid, pfd[PIPE_INPUT_OUTPUT][WRITE], pfd[PIPE_INPUT_TRANSLATE][WRITE]);
  
  // Return keyboard state.
  system("stty -raw icanon -igncr echo");
  
  return 0;
}


/**
 * @author  Tom Nightingale
 *
 * @brief   The input process.
 * @details This is the program's main process. It monitors all keyboard input
 *          and passes characters on to the output process for echoing back.
 *          It also stores the characters within a buffer until the special
 *          <ENTER> character is received. When this occurs, the buffer is sent
 *          to the translate function for processing.
 *
 * @param   pid 
 *              An array containing all child process id's.
 * @param   txpipe_output
 *              A pipe write file descriptor for sending data to the output process.
 * @param   txpipe_translate
 *              A pipe write file descriptor for sending data to the translate process.
 */
void input_proc(int pid[2], int txpipe_output, int txpipe_translate) {
  int c = 0;
  int count = 0;
  char buffer[BUFFSIZE];

  while (c != 'T' && (c = getchar()) != EOF) {
    write_pipe(txpipe_output, &c, 1);
    buffer[count++] = c;
    
    switch (c) {
      case KILL:
        kill(pid[TRANSLATE_PROC], SIGTERM);
        kill(pid[OUTPUT_PROC], SIGTERM);
        return;
        
      case 'T':
        // fall through.
      case 'E':
        write_pipe(txpipe_translate, buffer, count);
        // fall through.
        
      case 'K':
        count = 0;
        buffer[count] = '\0';
        break;
    }
  }
}


/**
 * @author  Tom Nightingale
 *
 * @brief   The output process.
 * @details This process handles all output display. It simply reads data off its pipes and
 *          echos it to stdout. If it receives an 'E' (special <ENTER> character), it will
 *          check the translate pipe for any data to echo.
 *
 * @param   rxpipe_input
 *              A pipe read file descriptor for receiving data from the output process.
 * @param   rxpipe_translate
 *              A pipe read file descriptor for receiving data from the translate process.
 */
void output_proc(int rxpipe_input, int rxpipe_translate) {
  char buffer[BUFFSIZE];
  int finished = FALSE;
  
  while (!finished && read_pipe(rxpipe_input, buffer, BUFFSIZE) >= 0) {
    switch (buffer[0]) {
      
      case 'T':
        finished = TRUE;
        // Fall through.
        
      case 'E':
        printf("%s\n\r", buffer);
    
        if (read_pipe(rxpipe_translate, buffer, BUFFSIZE) >= 0) {
          printf("%s\n\r", buffer);
        }
        break;
        
      case 'K':
        printf("%s\n\r", buffer);
        break;

      default:
        printf("%s", buffer);
        fflush(stdout);
        break;
    }
  }
    
  //fprintf(stderr, "END: output_proc.\n\r");
}


/**
 * @author  Tom Nightingale
 *
 * @brief   The translate process.
 * @details This process handles translation of the input buffer. Whenever the input process
 *          writes data onto the pipe, this process will read it off and parse it according 
 *          to the following rules.
 *            * X: Backspace character.
 *            * a: Translated into z.
 *            * T: Terminate character.
 *
 * @param   rxpipe_input
 *              A pipe read file descriptor for receiving data from the output process.
 * @param   txpipe_output
 *              A pipe write file descriptor for sending data to the output process.
 */
void translate_proc(int rxpipe_input, int txpipe_output) {
  int nread;
  int i = 0;
  int j = 0;
  int finished = 0;
  char buffer[BUFFSIZE];
  char translated[BUFFSIZE];
  
  while (!finished && (nread = read_pipe(rxpipe_input, buffer, BUFFSIZE)) >= 0) {
    // Process each character received.
    for (i = 0, j = 0; !finished && i < nread; i++) {
      switch (buffer[i]) {        
        // "Backspace" character.
        case 'X':
          // Don't backspace if nothing there.
          j = (j > 0) ? j - 1 : 0;
          break;
          
        case 'T':
          finished = TRUE;
          translated[j++] = '\0';
          break;
        
        // Replacement.
        case 'a':
          buffer[i] = 'z';
          // fall through.
          
        default:
          translated[j++] = buffer[i];
          break;
      }
    }
    // Terminate translated string.
    translated[j - 1] = '\0';
    
    write_pipe(txpipe_output, translated, j);
  }
}


/**
 * @author  Tom Nightingale
 *
 * @brief   A read() wrapper function.
 * @details Helper function for read(), provides error checking.
 *
 * @param   pipe_rd
 *              A pipe read file descriptor for receiving data.
 * @param   buffer
 *              A buffer to write data into.
 * @param   size
 *              The max amount of data to read.
 */
int read_pipe(int pipe_rd, char * buffer, size_t size) {
  int nread = 0;
  
  if ((nread = read(pipe_rd, buffer, size)) <= 0) {
    fatal("(pipe empty)\n\r");
  } else {
    buffer[nread] = '\0';
  }
  
  return nread;
}


/**
 * @author  Tom Nightingale
 *
 * @brief   A write() wrapper function.
 * @details Helper function for write(), provides error checking.
 *
 * @param   pipe_wr
 *              A pipe write file descriptor for sending data.
 * @param   buffer
 *              A buffer containing data to write.
 * @param   size
 *              The max amount of data to write.
 */
int write_pipe(int pipe_wr, const void * buffer, size_t size) {
  int nwritten = 0;
  
  if (write(pipe_wr, buffer, size) <= 0) {
    fatal("(Error writing to pipe)\n\r");
  }
  
  return nwritten;
}


/**
 * @author  Tom Nightingale
 *
 * @brief   A error helper function. Prints message to stderr and terminates program.
 *
 * @param   s
 *              An error message to print to stderr.
 */
void fatal(char *s) {
  perror(s);    /* print error msg and die */
  system("stty -raw -igncr echo");
  exit(1);
}