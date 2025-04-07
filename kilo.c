/*** includes ***/

#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <stdio.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct editorConfig {
  int screenrows;
  int screencols;
  struct termios orig_termios; //will hold a copy of the original state of the terminal atts
} ;

struct editorConfig E;

/*** terminal ***/

/*prints an error massage and exits the program
  will be used as error handling*/
void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4); //clears the entire screen
  write(STDOUT_FILENO, "\x1b[H", 3); //position the cursor 

  perror(s); //prints the right error according to the global errno variable
  exit(1); //indicates failure
}

/* resotres the user's original state of the terminal attributes*/
void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) 
    die("tcsetattr");
}

/*enables raw mode which turns off echoing when the user types
  does this by changing the fourth bit which corresponds to the ECHO bitflag
*/
void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr"); //read terminal attributes into the termios struct
  atexit(disableRawMode);

  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); //disables ctrl-s and ctrl-q
  raw.c_oflag &= ~(OPOST); // turn off output processing like carriage return or newline 
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); //change the ECHO, ICANON, IEXTEN(ctrl-o) flags to 0
  raw.c_cc[VMIN] = 0; //min number of bytes of input before read() can return
  raw.c_cc[VTIME] = 1; // max amount of time to wait before read() returns - 1/10 of a second

  //TCSAFLUSH - when to apply the change (discards any unread input before applying the changes to the terminal)
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("setattr"); //apply the modified attributes to the terminal
}

/*wait for a keypress and return it*/
char editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) die("read");
  }
  return c;
}

/* places the number of cols and rows into the pointer vals*/
int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    editorReadKey();
    return -1; //indicating failure
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** output ***/

void editorDrawRows() {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    write(STDOUT_FILENO, "~\r\n", 3);

  }
}

/*clear screen
  \1xb - esc
  [ - start of esc sequence which instructs the terminal to do various text formatting tasks*/
void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4); //clears the entire screen
  write(STDOUT_FILENO, "\x1b[H", 3); //position the cursor 

  editorDrawRows();
  write(STDOUT_FILENO, "\x1b[H", 3);
}


/*** input ***/

/*handles the key press*/
void editorProcessKeypress() {
  char c = editorReadKey();

  switch (c) {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
  }
}
/*** init ***/

/* initialize all the fields in the E struct*/
void initEditor() {
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main() {
  enableRawMode();

  while(1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}