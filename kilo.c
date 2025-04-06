#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>


struct termios orig_termios; //will hold a copy of the original state of the terminal atts

/* resotres the user's original state of the terminal attributes*/
void disableRawMode() {
 tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

/*enables raw mode which turns off echoing when the user types
  does this by changing the fourth bit which corresponds to the ECHO bitflag
*/
void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios); //read terminal attributes into the termios struct
  atexit(disableRawMode);

  struct termios raw = orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); //disables ctrl-s and ctrl-q
  raw.c_oflag &= ~(OPOST); // turn off output processing like carriage return or newline 
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); //change the ECHO, ICANON, IEXTEN(ctrl-o) flags to 0
  raw.c_cc[VMIN] = 0; //min number of bytes of input before read() can return
  raw.c_cc[VTIME] = 1; // max amount of time to wait before read() returns - 1/10 of a second

  //TCSAFLUSH - when to apply the change (discards any unread input before applying the changes to the terminal)
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); //apply the modified attributes to the terminal
}

int main() {
  enableRawMode();
  
  while(1) {
    char c = '\0';
    read(STDIN_FILENO, &c, 1);
    if (iscntrl(c)) {
      printf("%d\r\n", c);
    } else {
      printf("%d ('%c')\r\n", c, c);
    }
    if (c == 'q') break;
  }

  return 0;
}