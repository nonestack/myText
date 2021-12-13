#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/
struct editorConfig{
  int sceenrows;
  int sceencols;
  struct termios orig_termios;
};

struct editorConfig E;

/*** terminal ***/
void die(const char *s){
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
}

void disableRawMode(){
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) die("disableRawMode");
}

void enableRawMode(){
  if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");

  atexit(disableRawMode);

  struct termios raw = E.orig_termios;

  raw.c_iflag &= ~(IXON);
  //IXON Enable XON/XOFF flow control on output   XOFF is C-s, XON is C-q
  //raw.c_oflag &= ~(OPOST);
  //OPOST Enable implementation-defined output processing -> \n translate to \r\n
  raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
  //ICANON is canonical mode
  //ISIG is When any of  the characters INTR, QUIT,  SUSP, or DSUSP are received, generate the corresponding signal.
  //IEXTEN ?
  raw.c_cc[VMIN] = 3;
  raw.c_cc[VTIME] = 1;

  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey(){
  int nread;
  char c;
  while((nread = read(STDIN_FILENO, &c, 1)) != 1){
    if(nread == -1 && errno == EAGAIN)
      die("read");
  }
  return c;
}

int getWindowSize(int *rows, int *cols){
  struct winsize ws;

  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
    if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;
    editorReadKey();
    return -1;
  }
  else{
    *rows = ws.ws_row;
    *cols = ws.ws_col;
    return 0;
  }
}

/*** input ***/
void editorProcessKeypress(){
  char c = editorReadKey();

  switch(c){
  case CTRL_KEY('q'):
   write(STDOUT_FILENO, "\x1b[2J", 4);
   write(STDOUT_FILENO, "\x1b[H", 3);
   exit(0);
   break;
  }
}

/*** output ***/
void editorDrawRow(){
  int y;
  for(y = 0; y < E.sceenrows; ++y){
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

void editorRefreshScreen(){
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  editorDrawRow();

  write(STDOUT_FILENO, "\x1b[H", 3);
}


/*** init ***/
void initEditor(){
  if(getWindowSize(&E.sceenrows, &E.sceencols) == -1)
    die("getWindowSize");
}

int main(){
  enableRawMode();
  initEditor();

  //while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q'){
  while(1){
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
