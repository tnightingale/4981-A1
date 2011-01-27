#include "a1.h";

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
          
          // Closing unused pipe.
          close(pfd[1][0]);
          close(pfd[1][1]);
          
          output_proc(pfd[0], pfd[2]);
          return 0;

        // Translate process.  
        case TRANSLATE_PROC:
          
          // Closing unused pipe.
          close(pfd[0][0]);
          close(pfd[0][1]);
          
          translate_proc(pfd[1], pfd[2]);
          return 0;
      }
    }
  }
  

  // Input process (parent).

  // Closing unused pipe.
  close(pfd[2][0]);
  close(pfd[2][1]);
  input_proc(pid, pfd[0], pfd[1]);
  
  // Return keyboard state.
  system("stty -raw icanon -igncr echo");
  
  return 0;
}

// Parent Processes.
void input_proc(int pid[2], int txpipe_output[2], int txpipe_translate[2]) {
  int c = 0;
  int count = 0;
  char buffer[BUFFSIZE];
  
  // Close the read descriptors.
  close(txpipe_output[0]);
  close(txpipe_translate[0]);

  while (c != 'T' && (c = getchar()) != EOF) {
    write(txpipe_output[1], &c, 1);
    buffer[count++] = c;
    
    switch (c) {
      case KILL:
        kill(pid[TRANSLATE_PROC], SIGTERM);
        kill(pid[OUTPUT_PROC], SIGTERM);
        return;
        
      case 'T':
        // fall through.
      case 'E':
        write(txpipe_translate[1], buffer, count);
        // fall through.
        
      case 'K':
        count = 0;
        buffer[count] = '\0';
        break;
    }
  }
  
  //fprintf(stderr, "END: parent.\n\r");
}

// Child Processes.
void output_proc(int rxpipe_input[2], int rxpipe_translate[2]) {
  char buffer[BUFFSIZE];
  int finished = FALSE;

  // Close the write descriptors.
  close(rxpipe_input[1]);
  close(rxpipe_translate[1]);
  
  while (!finished && read_pipe(rxpipe_input[0], buffer, BUFFSIZE) >= 0) {
    switch (buffer[0]) {
      
      case 'T':
        finished = TRUE;
        // Fall through.
        
      case 'E':
        printf("%s\n\r", buffer);
    
        if (read_pipe(rxpipe_translate[0], buffer, BUFFSIZE) >= 0) {
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

void translate_proc(int rxpipe_input[2], int txpipe_output[2]) {
  int nread;
  int i = 0;
  int j = 0;
  int finished = 0;
  char buffer[BUFFSIZE];
  char translated[BUFFSIZE];

  // Close the write descriptors.
  close(rxpipe_input[1]);
  close(txpipe_output[0]);
  
  while (!finished && (nread = read_pipe(rxpipe_input[0], buffer, BUFFSIZE)) >= 0) {
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
    
    write(txpipe_output[1], translated, j);
  }
}

int read_pipe(int pipe_rd, char * buffer, size_t size) {
  int nread = 0;
  
  if ((nread = read(pipe_rd, buffer, size)) <= 0) {
    fatal("(pipe empty)\n\r");
  } else {
    buffer[nread] = '\0';
  }
  
  return nread;
}

// Error Function.
void fatal(char *s) {
  perror(s);    /* print error msg and die */
  system("stty -raw -igncr echo");
  exit(1);
}