#include "a1.h";

int main() {
  // Put keyboard into raw mode
  system("stty raw -icanon igncr -echo");
  
  int pfd[3][2];
  int i = 0;
  int pid[2];

  // Open the pipes.
  for (i = 0; i < 3; i++) {
    if (pipe(pfd[i]) < 0) {
      fatal("pipe call");
      exit(1);
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
  
  // Closing unused pipe.
  close(pfd[2][0]);
  close(pfd[2][1]);
  input_proc(pfd[0], pfd[1]);
  
  // Return keyboard state.
  system("stty -raw icanon -igncr echo");
  
  return 0;
}

// Parent Processes.
void input_proc(int txport_output[2], int txport_translate[2]) {
  int c = 0;
  int count = 0;
  char buffer[BUFFSIZE];
  
  // Close the read descriptors.
  close(txport_output[0]);
  close(txport_translate[0]);

  while (c != EOF && (c = getchar()) != EOF) {
    write(txport_output[1], &c, 1);
    buffer[count++] = c;
    
    switch (c) {
      // Vertical Tab
      case 0x0B:
        break;
        
      case 'T':
        c = EOF;
        // fall through.
        
      case 'E':
        write(txport_translate[1], buffer, count);
        // fall through.
        
      case 'K':
        count = 0;
        buffer[count] = '\0';
        break;
    }
  }
  
  fprintf(stderr, "END: parent.\n\r");
}

// Child Processes.
void output_proc(int rxport_input[2], int rxport_translate[2]) {
  int nread;
  char buffer[BUFFSIZE];

  // Close the write descriptors.
  close(rxport_input[1]);
  close(rxport_translate[1]);
  
  while ((nread = read(rxport_input[0], buffer, BUFFSIZE)) != EOF) {    
    switch (nread) {
      case -1:
      case 0:
        fprintf(stderr, "(pipe empty)\n\r");
        return;
      
      default:
        buffer[nread] = '\0';

        if (buffer[0] == 'E') { 
            printf("%s\n\r", buffer);
        } else {
            printf("%s", buffer);
        }
        
        fflush(stdout);
        break;
    }
  }
  fprintf(stderr, "\n\rEND: output_proc.\n\r");
}

void translate_proc(int rxport_input[2], int txport_output[2]) {
  int nread;
  int i = 0;
  int j = 0;
  char buf[BUFFSIZE];
  char translated[BUFFSIZE];

  // Close the write descriptors.
  close(rxport_input[1]);
  close(txport_output[0]);
  
  while ((nread = read(rxport_input[0], buf, BUFFSIZE)) != EOF) {
    switch (nread) {
      case -1:
      case 0:
        fprintf(stderr, "(pipe empty)\n\r");
        return;
      
      default:
        for (i = 0, j = 0; i < nread; i++) {
          
          switch (buf[i]) {
            case 'X':
              j--;
              break;
              
            default:
              translated[j++] = buf[i];
              break;
          }
        }
        translated[j] = '\0';
        
        printf("%s\n\r", translated);
        fflush(stdout);
        break;
    }
  }
  
  fprintf(stderr, "END: translate_proc.\n\r");
}

// Error Function.
void fatal(char *s) {
    perror(s);    /* print error msg and die */
    system("stty -raw -igncr echo");
    exit(1);
}