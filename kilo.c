#include <termios.h>
#include <unistd.h>

/*enables raw mode which turns of echoing when the user types
  does this by changing the fourth bit which corresponds to the ECHO bitflag
*/
void enableRawMode() {
    struct termios raw; //terminal attributes will be read into this struct

    tcgetattr(STDIN_FILENO, &raw); //read terminal attributes into the termios struct

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