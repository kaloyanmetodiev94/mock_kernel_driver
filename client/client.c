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
int fd,rc,rcr,rcw;
int integer_input; //integer_input holds the inputted integers
int state=0; //state holds the current state (waiting for command=0; entering first integer=1; entering second integer=2; quit=3)
int send_buffer=0;
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

int input_checker(char *arr, int arr_len){ //last char or arr_len is \n (ASCII:10)
	int i;
	for (i = 0 ; i<arr_len-1 ; ++i){
		if( i==0 && arr[i]=='-' )continue; // handle the signed integer cases
		if( (arr[i]-'0' < 0) || (arr[i]-'0')>9) return 0;
	}
	return 1;
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
	send_buffer=0;
	if (input_checker(buf,rc)){
		integer_input=charArrayToInt(buf,rc-1);
	}else{
		printf("Only integer inputs allowed \n");
		continue;
	}
	if (rc-1==1 && integer_input<=5 && integer_input>=0 && state==0){ //if to handle commands
		server_buf[0]=(unsigned char)integer_input;
		if (integer_input==5||integer_input==0){
			send_buffer=1; //send the buffer if (5)Exit or (0)Menu is selected
			if (integer_input==5) state=3;
		}else{
			state=1;
			printf("Enter operand 1: ");
			fflush(stdout);
		}
	}else if(state==1){
		server_buf[4] = (integer_input >> 24) & 0xFF;
		server_buf[3] = (integer_input >> 16) & 0xFF;
		server_buf[2] = (integer_input >> 8) & 0xFF;
		server_buf[1] = integer_input & 0xFF;
		state=2;
		printf("Enter operand 2: ");
		fflush(stdout);
	}else if(state==2){
		server_buf[8] = (integer_input >> 24) & 0xFF;
		server_buf[7] = (integer_input >> 16) & 0xFF;
		server_buf[6] = (integer_input >> 8) & 0xFF;
		server_buf[5] = integer_input & 0xFF;
		state=0;
		send_buffer=1;
	}
	if (send_buffer) communicate_buffer();
	if (state==3) return 0; //exit the client as well when getting disconnected
  }

  return 0;
}
