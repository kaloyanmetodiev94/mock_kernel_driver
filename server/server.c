#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h> 
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdint.h>

#define DEVICE "/dev/chardev_enduro"

int debug = 1, fd = 0, opt=1;
int ppos = 0;
//char *socket_path = "./socket"; //real socket presented as file
char *socket_path = "\0hidden_socket"; // \0 removes the socket from the fs (Linux Abstract Socket Namespace) and client can be anywhere	
struct sockaddr_un addr;
int incoming_buffer_length=9;
int device_buffer_length=8;
int fd_socket,rc,rcw;
ssize_t device_write_ret;
uint8_t case_value;
int32_t ioctl_value;

//handle multiple clients
int addrlen , new_socket , client_socket[10] , 
  max_clients = 10 , activity, i , valread , sd, max_sd;  
fd_set readfds_socket;


// UI COMMANDS definitions
const char *initial_screen="Client n:\r\n\
(1) Add 2 numbers\r\n\
(2) Multiply 2 numbers\r\n\
(3) Subtract 2 numbers\r\n\
(4) Divide 2 numbers\r\n\
(5) Exit\r\n\
Enter Command:\0";

const char *wrong_command="Wrong command. Value must be [0-5] (0 for initial menu)\r\n: ";

const char *disconnect="Disconnected\r\n";

char return_value[24]="Result is "; //"Result is "+ largest integer is 10 characters + !\n

const char *action_names[] = {"ADD","MULTIPLY","SUBTRACT","DIVIDE"};
//end of UI definitions

int setup_socket(int argc, char *argv[]){

	if (argc > 1) socket_path=argv[1];

	//initialise all client_socket[] to 0 so not checked 
	for (i = 0; i < max_clients; i++)  
	{  
		client_socket[i] = 0;  
	} 

	if ( (fd_socket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		return -1;
	}

	//reuse socket (only for the NON-abstract linux socket case) 
	if( setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )  
	{  
		perror("setsockopt");  
		exit(EXIT_FAILURE);  
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
	for(i=0;i<length;i++){
	    printf("%x ",buffer[i]);
	}
	printf("\n");
}

int write_device(int fd, char *buf)
{
	device_write_ret = write(fd, buf+1, device_buffer_length, &ppos);
	if(debug){
		printf("DATA_TO_BE_WRITTEN: ");
		print_custom_buffer(buf+1,8);   
		if (device_write_ret == -1)
		        printf("writting failed\n");
		else
		        printf("writting success\n");
	}
        return 0;
}

int read_device() //Used for debugging. Not used in final version
{
	char *data = (char *)malloc(device_buffer_length * sizeof(char));
        ssize_t ret;

        memset(data, 0, sizeof(data));
        ret = read(fd, data, device_buffer_length, &ppos);
	if (debug){
		printf("DEVICE_BUFFER: ");
		print_custom_buffer(data,8);
		if (ret == -1)
		        printf("reading failed\n");
		else
		        printf("reading success\n");
	}
        free(data);
        return 0;
}


int ioctl_device(int fd, int case_value)
{

        return ioctl(fd, _IOR('a',case_value-1,int32_t*), &ioctl_value);
}

int main(int argc, char *argv[])
{

        int value = 0;
	char *buf = (char *)malloc(incoming_buffer_length * sizeof(char)); // initialize the read-in buffer for the incoming requests in main scope
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
		//clear socket set
		FD_ZERO(&readfds_socket);
		
		//add the server master socket to the set
		FD_SET(fd_socket,&readfds_socket);
		max_sd=fd_socket;
		//add child sockets to set 
		for ( i = 0 ; i < max_clients ; i++)  
		{  
		    //socket descriptor 
		    sd = client_socket[i];  
			 
		    //if valid socket descriptor then add to read list 
		    if(sd > 0)  
			FD_SET( sd , &readfds_socket);  
			 
		    //highest file descriptor number (for select)
		    if(sd > max_sd)  
			max_sd = sd;  
		} 
		
		activity = select( max_sd+1, &readfds_socket, NULL , NULL , NULL); //select is blocking and waiting for activity on socket, no timeout
		if ((activity < 0) && (errno!=EINTR))  
		{  
		    printf("select error");  
		}

		//if activity happens on the fd_socket --> should be an incoming connection
		if (FD_ISSET(fd_socket, &readfds_socket))  
			{  
				if ( (new_socket = accept(fd_socket, NULL, NULL)) == -1) { //waiting to accept clients ( "accept" is blocking )
					perror("accept error");
					exit(-1);
				}
			     
				printf("Client connected!\r\n");  
			   
				 
				//add new socket to array of sockets 
				for (i = 0; i < max_clients; i++)  
				{  
					//if position is empty 
					if( client_socket[i] == 0 )  
					{  
					    client_socket[i] = new_socket;  
					    if (debug) printf("Adding to list of sockets as %d\n" , i);  
					 
					    break;  
					}  
				}  
			}  
		//else it is an IO from a client socket. Loop through them to see what's going on.
		for (i = 0; i < max_clients; i++)  
		{  
			sd = client_socket[i];  
				 
			if (FD_ISSET( sd , &readfds_socket))  
			{  
				
				rc=read(sd,&buf[0],incoming_buffer_length);
				if (rc>0)  
				{ // implement the server logic  
					if(debug) print_custom_buffer(buf,incoming_buffer_length);
					case_value=(uint8_t) buf[0];
					switch (case_value){
						case 0:
							rcw=write(sd,initial_screen,strlen(initial_screen));
							break;
						case 1: case 2: case 3: case 4:
							printf("Received request %s(%d,%d)\n",action_names[case_value-1],(int32_t) buf[1],(int32_t) buf[5]);
							fd = open(DEVICE, O_RDWR); //open device for read/write
							write_device(fd,buf); //write the buffer
							if(ioctl_device(fd,case_value)==0){
								sprintf(return_value+10,"%d!\n",ioctl_value); //execute the command in the sprintf. +10 is "Result is "
							}else{
								sprintf(return_value+10,"weird!\n");
							}
							close(fd); //close device
							rcw=write(sd,return_value,strlen(return_value));
							printf("Sending result!\n");
							break;
						case 5:
							rcw=write(sd,disconnect,strlen(disconnect));
							rc=0; // mimic a Ctrl+C event when 0 bytes are incoming
							goto disconnect_line;
						default:
							rcw=write(sd,wrong_command,strlen(wrong_command));
							break;	
					}  
				} 
				//If incoming message is 0 bytes, then socket is closing.
				else if (rc==0)
				{
					//Close the socket and set as 0
					disconnect_line:
					printf("Client disconnected!\n");
					close( sd );  
					client_socket[i] = 0;  

				}else{
					perror("read error");
				}
			}  
		}  

        }
        return 0;
}

