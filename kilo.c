/*
ANSI Escape - פקודות למסוף
*/

/*** includes ***/

#include <termios.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>

/*** defines ***/
#define KILO_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct editorConfig {
  int screenrows;
  int screencols;
  struct termios orig_termios; //will hold a copy of the original state of the terminal atts
};

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

/*gets cursor position and storest the row and col values*/
int getCursorPosition(int *rows, int *cols) {
  char buf[32]; //stores the response from the terminal
  unsigned int i = 0; // counter to track the position in the buffer

  /* 
  \x1b[6n - escape sequence that requests the cursor position
  \x1b (Escape ESC character) signals the start of an escape sequence
  [6n is the command that asks the terminal: "Where is the cursor?
  */
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1; 

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1; //validate response format
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1; // extract rows and cols

  return 0;
}

/* places the number of cols and rows into the pointer vals*/
int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1; //C command - cursor forward, B command - cursor down ->the cursor reaches the right and bottom edges of the screen
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** append buffer ***/
struct abuf {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0};

void abAppend(struct abuf *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) {
  free(ab->b);
}


/*** output ***/

void editorDrawRows(struct abuf *ab) {
  int y;
  for (y = 0; y < E.screenrows; y++) {
    if (y == E.screencols / 3) {
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome), 
          "Kilo editor -- version %s", KILO_VERSION);
      if (welcomelen > E.screencols) welcomelen = E.screencols;
      int padding = (E.screencols - welcomelen) / 2;
      //first char is always a tlide
      if (padding) {
        abAppend(ab, "~", 1);
        padding--;
      }
      //
      while (padding--) abAppend(ab, " ", 1);
      abAppend(ab, welcome, welcomelen);
    } else {
      abAppend(ab, "~", 1);
    }
    
    //K - erase in line//
    abAppend(ab, "\x1b[K", 3); //erases part of the current line
    if (y < E.screenrows -1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

/*clear screen
  \1xb - esc
  [ - start of esc sequence which instructs the terminal to do various text formatting tasks*/
void editorRefreshScreen() {
  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6); //hide cursor
  abAppend(&ab, "\x1b[H", 3); //position the cursor 

  editorDrawRows(&ab);

  abAppend(&ab,"\x1b[2J", 4);
  abAppend(&ab, "\x1b[?25h", 6); //show cursor

  write(STDOUT_FILENO, ab.b, ab.len); //write the buffers content 
  abFree(&ab);
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
  initEditor();

  while(1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}