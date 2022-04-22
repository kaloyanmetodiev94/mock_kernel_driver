#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>


//char *socket_path = "./socket";
char *socket_path = "\0hidden_socket";

char server_buf[9]=""; //initialized with \x00 to trigger announcement [1] byte command [4] bytes integer [4] bytes integer
int server_buf_len=9; //length of the buffer that the server receives

struct sockaddr_un addr;
char buf[200]; //needs the bytes to accept the initial announcement
int fd,rc,rcr,rcw,integer_input;
char char_int;

void setup_socket(int argc, char *argv[]){
  if (argc > 1) socket_path=argv[1];

  if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  if (*socket_path == '\0') {
    *addr.sun_path = '\0';
    strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
  } else {
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
  }

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("connect error");
    exit(-1);
  }
}

int charArrayToInt(char *arr, int arr_len) { //trying not to use atoi as my characters are not const
    int i, value, r, flag;   
    flag = 1;
    i = value = 0;   
    for( i = 0 ; i<arr_len ; ++i){       
        // if arr contain negative number
        if( i==0 && arr[i]=='-' ){
            flag = -1;
            continue;
        }
        
        r = arr[i] - '0';
        value = value * 10 + r;
    }  
    value = value * flag;
    return value;     
}


void communicate_buffer(){
    if(write(fd, server_buf, server_buf_len)!=server_buf_len){
	perror("writing error\n");
    }
    rcr=read(fd, buf,sizeof(buf));
    printf("%.*s ", rcr, buf);
    fflush(stdout); //this is to make the command entering on the same line
}


int main(int argc, char *argv[]) {
  setup_socket(argc,argv);
  communicate_buffer(); //communicate the empty-initialized buffer and use it as acknowledge request.

  while( (rc=read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
	integer_input=charArrayToInt(buf,rc-1);
	if (rc-1==1 && integer_input<=5){ //if to handle commands
		server_buf[0]=(unsigned char)integer_input;
	}
	communicate_buffer();
	if (integer_input==5) return 0; //exit the client as well when getting disconnected
  }

  return 0;
}
