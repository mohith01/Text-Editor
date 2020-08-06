/*** includes ***/
//Step 32
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdlib.h>
/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)
/*** data ***/

struct editorConfig{
	int screenrows;
	int screencols;
	struct termios orig_termios;
};

struct editorConfig E;
/*** terminal ***/

void die(const char *s) {

  write(STDOUT_FILENO, "\x1b[2J" ,4);
  write(STDOUT_FILENO, "\x1b[H",3);
  perror(s);
  exit(1);
}

void disableRawMode(){
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

void enableRawMode(){
	
	if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");//terminal attributes are read by tcgetattr
	atexit(disableRawMode);

	struct termios raw = E.orig_termios;
	raw.c_iflag &= ~(ICRNL|IXON|BRKINT|INPCK|ISTRIP);//input flags
	raw.c_oflag &= ~(OPOST);//post processing output
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO|ICANON|ISIG|IEXTEN);
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

//ICANON flag turns off terminal mode so byte - byte entry instead of line by line
//c_lflag == local flags ECHO is 000000000000001000 we make it all zero using bitwise and not(ECHO)
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
//term attr are set by tcsetattr and TCSAFLUSH argument specifies when to apply the change: in this case, it waits for all pending output to be written to the terminal, and also discards any input that hasn’t been read.
}

char editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1))!=1 ){
	if(nread == -1 && errno != EAGAIN) die("read");
      }
	return c;
}


int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d",rows, cols) !=2) return -1;
  
  
  return 0;
}


int getWindowSize(int *rows, int *cols){
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
		return getCursorPosition(rows, cols);
	}else {
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

#define ABUF_INIT {NULL, 0}
/*** input ***/

void editorProcessKeypress() {
    char c =  editorReadKey();
    switch (c) {
	case CTRL_KEY('q'):
		write(STDOUT_FILENO, "\x1b[2J" ,4);
		write(STDOUT_FILENO, "\x1b[H",3);
		exit(0);
		break;
   }
}


/*** output ***/
void editorDrawRows() {
  int y;
  for (y=0;y< E.screenrows;y++){
	write(STDOUT_FILENO,"~",1);

	if (y < E.screenrows -1){
		write(STDOUT_FILENO , "\r\n",2);
	}
  }
}


void editorRefreshScreen(){
	write(STDOUT_FILENO, "\x1b[2J" ,4);
	write(STDOUT_FILENO, "\x1b[H",3);// \x1b is an esc sequence for 27 and uses H cmd
 //https://vt100.net/docs/vt100-ug/chapter3.html

	editorDrawRows();

	write(STDOUT_FILENO , "\x1b[H",3);
}

/*** init ***/
void initEditor(){
	if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}

int main(){
enableRawMode();
initEditor();

while (1){
	editorRefreshScreen();
  	editorProcessKeypress();
}

return 0;
}
