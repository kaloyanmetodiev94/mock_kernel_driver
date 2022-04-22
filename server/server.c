#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>

#define DEVICE "/dev/chardev_enduro"
#define PLUS_VALUE _IOR('a',0,int32_t*)
#define MINUS_VALUE _IOR('a',1,int32_t*)
#define MULTIPLY_VALUE _IOR('a',2,int32_t*)
#define DIVIDE_VALUE _IOR('a',3,int32_t*)

int debug = 1, fd = 0;
int ppos = 0;
//char *socket_path = "./socket"; //real socket presented as file
char *socket_path = "\0hidden_socket"; // \0 removes the socket from the fs (Linux Abstract Socket Namespace) and client can be anywhere	
struct sockaddr_un addr;
char buf[100]; //buffer used for client-server comms
char incoming_buffer_length=9;
int fd_socket,cl,rc,rcw;

char *initial_screen="Client n:\n\
(1) Add 2 numbers\n\
(2) Multiply 2 numbers\n\
(3) Subtract 2 numbers\n\
(4) Divide 2 numbers\n\
(5) Exit\n\
Enter Command:\0";

char *wrong_command="Wrong command. Value must be [0-5] (0 for initial menu)\n: ";

char *disconnect="Disconnected\n";

int setup_socket(int argc, char *argv[]){

	if (argc > 1) socket_path=argv[1];

	if ( (fd_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX; //using unix domain socket addresses as instructed
	if (*socket_path == '\0') {
		*addr.sun_path = '\0';
		strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
		printf("Opening socket %s\n",socket_path+1);
	} else {
		strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
		printf("Opening socket %s\n",socket_path);
		unlink(socket_path); //hidden socket gets handled automatically in the Linux Abstract Socket Namespace
	}

	if (bind(fd_socket, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("bind error");
		return -1;
	}

	if (listen(fd_socket, 5) == -1) { //backlog=5 --> up to 5 incoming connection requests queued
		perror("listen error");
		return -1;
	}
	return 0;
}
void print_custom_buffer(char* buffer, int length){
	int i=0;
	for(i;i<length;i++){
	    printf("%x ",buffer[i]);
	}
	printf("\n");
}

int write_device()
{
        int write_length = 8;
	int val1=10;
	int val2=-3;
	char data[8]="";
        ssize_t ret;
 
	memcpy(data,&val1, sizeof(val1));
	memcpy(data+4,&val2, sizeof(val2));
	printf("DATA_TO_BE_WRITTEN: ");
	print_custom_buffer(data,8);
        ret = write(fd, data, write_length, &ppos);
        if (ret == -1)
                printf("writting failed\n");
        else
                printf("writting success\n");
        return 0;
}

int read_device()
{
        int read_length = 8;
        ssize_t ret;
        char *data = (char *)malloc(1024 * sizeof(char));

        memset(data, 0, sizeof(data));
        ret = read(fd, data, read_length, &ppos);
	printf("DEVICE_BUFFER: ");
	print_custom_buffer(data,8);
        if (ret == -1)
                printf("reading failed\n");
        else
                printf("reading success\n");

        free(data);
        return 0;
}


int ioctl_device(int fd, int case_value)
{
	int32_t value;
        ioctl(fd, _IOR('a',case_value-1,int32_t*), &value);
        printf("Result: %d\n", value);
	return 0;
}

void throw_old_stuff(){
		int value=0;
                printf("please enter:\n\
                    \t 1 to Add\n\
                    \t 2 to Subtract\n\
                    \t 3 to Multiply\n\
                    \t 4 to Divide\n\
                    \t 5 to write buffer\n\
                    \t 6 to read buffer\n\
                    \t 0 to exit\n");
                scanf("%d", &value);
		fd = open(DEVICE, O_RDWR);
                switch (value) {
                case 5 : printf("Writing buffer...\n");
                        write_device();
                        break;
                case 6 : printf("read option selected\n");
                        read_device();
                        break;
		case 1: case 2: case 3: case 4:
                        ioctl_device(fd,value);
			break;
		case 0 :
			printf("exiting loop\n");
			break;
                default : 
			printf("unknown  option selected, please enter right one\n");
                        break;
                }
		close(fd); /*closing the device*/
}



int main(int argc, char *argv[])
{
        int value = 0;

	//opening device
        if (access(DEVICE, W_OK) == -1) {
                printf("module %s not loaded. Maybe permissions issue?\n", DEVICE);
                return 0;
        } else
                printf("module %s loaded.\n", DEVICE);

	//opening socket
	if(setup_socket(argc, argv)<0){
		perror("could not setup socket");
		return 0;
	}

        while (1) {
		if ( (cl = accept(fd_socket, NULL, NULL)) == -1) { //waiting to accept clients ( "accept" is blocking )
			perror("accept error");
                	continue;
                }
		if (cl>=0){
			printf("Client connected\n");
		}
		while ( (rc=read(cl,buf,sizeof(buf))) > 0) { //in this loop while waiting for commands from the client
			if(debug) print_custom_buffer(buf,9);
			switch ((int) buf[0]){
				case 0:
					rcw=write(cl,initial_screen,strlen(initial_screen));
					break;
				case 5:
					rcw=write(cl,disconnect,strlen(disconnect));
					goto disconnect_line;
				default:
					rcw=write(cl,wrong_command,strlen(wrong_command));
					break;	
		
			}
			
		}
		if (rc == -1) {
			perror("read");
			return -1;
		}
		disconnect_line:
		if (rc == 0) {
			printf("Client disconnected\n");
			close(cl);
		}

        }
        return 0;
}

