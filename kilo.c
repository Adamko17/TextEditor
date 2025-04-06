#include <termios.h>
#include <unistd.h>
#include <stdlib.h>


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
  raw.c_lflag &= ~(ECHO); //change the fourth bit in the 'local flags' to 0 using NOT, AND.

  //TCSAFLUSH - when to apply the change
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); //apply the modified attributes to the terminal
}

int main() {
  enableRawMode();
  char c;

  //read 1 byte into var c until there are no more bytes to read or 'q' is pressed
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
  return 0;
}