#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>

void error(char *);
int create_socket(int,struct hostent *);
int open_data_socket(int,struct hostent *);
void list_files(int,struct hostent *);
void get_file(int,struct hostent *,char *);
void create_file(char *,char *,int);
void enter_file(char *,int);

int main(int argc, char *argv[])
{

	int socket_file_descriptor;
	int port = 21;

	if(argc==2){
		struct hostent *server = gethostbyname(argv[1]);

		if ((socket_file_descriptor = create_socket(port,server)) < 0)
			error("could not create socket");

		printf("Connected to %s:%d\n", argv[1], port);

		//LOG IN TO THE FTP SERVER
		//THE PROGRAM LOGS YOU IN WITH THE 'ANONYMOUS' USERNAME
		//AND A BLANK PASSWORD

		//n is the number of characters read/written
		//by a read/write command
		int n;

		char response_220[60];
		char response_331[60];
		char response_230[60];
		char response_227[60];

		bzero(response_220,sizeof(response_220));
		bzero(response_331,sizeof(response_331));
		bzero(response_230,sizeof(response_230));
		bzero(response_227,sizeof(response_227));

		if ((n = read(socket_file_descriptor, response_220, 60)) < 0)
			error("did not receive 220");
		printf("%s\n",response_220);
		

		if (strncmp(response_220, "220", 3) == 0)
		{
			//send username to the password
			char username[] = "USER anonymous\r\n";
			if ((n = write(socket_file_descriptor, username, strlen(username))) < 0)
				error("did not send username");

			if ((n = read(socket_file_descriptor, response_331, 60)) < 0)
				error("did not receive 331");
			printf("%s\n", response_331);

			// send password to server
			if (strncmp(response_331, "331", 3) == 0)
			{
				//send password to the server
				char password[] = "PASS \r\n";
				if ((n = write(socket_file_descriptor, password, strlen(password))) < 0)
					error("did not send password");

				if ((n = read(socket_file_descriptor, response_230, 60)) < 0)
					error("did not receive 230");
				printf("%s\n", response_230);

				if(strncmp(response_230,"230",3)==0)
				{
					list_files(socket_file_descriptor,server);

					printf("Enter file to retrieve\n");
					char filename[100];
					enter_file(filename,100);

					get_file(socket_file_descriptor,server,filename);

				}else{
					error("code 230 not received");
				}
			}else{
				error("code 331 not received");
			}
		}else{
			error("code 220 not received");
		}
	
	}else{
			error("USAGE : ./object_file your_ip_address");
	}
	close(socket_file_descriptor);
	return 0;
}

void error(char *error_message)
{
	perror(error_message);
	exit(1);
}
int create_socket(int port,struct hostent *server){
	int socket_file_descriptor;
	if((socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error("could not create socket");

	if (server == NULL)
		error("no such server");

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(port);
	bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);

	if ((connect(socket_file_descriptor, (struct sockaddr *)&server_address, sizeof(server_address))) < 0)
		error("could not connect");
	
	return socket_file_descriptor;
}
int open_data_socket(int command_socket,struct hostent *server){
	int data_socket;
	char pasv[] = "PASV \r\n";
	if (write(command_socket, pasv, strlen(pasv)) < 0)
		error("did not send any data");

	char response_227[60];
	bzero(response_227,sizeof(response_227));

	if (read(command_socket,response_227, 60) < 0)
		error("did not receive any data");
	printf("%s\n", response_227);

	if(strncmp(response_227,"227",3)==0){
		//227 Entering passive mode (192,168,1,66,237,109).
		//above is a sample 227 response
		//we are only interested in the last two numbers which give us the port to connect to
		//in this case 237 and 109(stored in num5 and num6 respectively)
		int num5;
		int num6;
		char s1[20];
		int ptr;

		sscanf(response_227,"%d %s %s %s (%d,%d,%d,%d,%d,%d)",&ptr,s1,s1,s1,&ptr,&ptr,&ptr,&ptr,&num5,&num6);
		int extracted_port = (num5 * 256)+num6;

		if ((data_socket = create_socket(extracted_port,server)) < 0)
			error("could not create data socket");
		return data_socket;
	}

	return -1;
}
void list_files(int command_socket,struct hostent *server){

	//open data socket first
	int data_socket;
	if((data_socket = open_data_socket(command_socket,server)) < 0)
		error("could not open data socket");

	char response_125[70];
	bzero(response_125,sizeof(response_125));
	char response_226[70];
	bzero(response_226,sizeof(response_226));

	char ls[]="LIST \r\n";
	if(write(command_socket,ls,strlen(ls)) < 0)
		error("could send list command");

	
	if(read(command_socket,response_125,70) < 0)
		error("error in listing files");
	printf("%s\n",response_125);


	//we should keep reading until bytes read are 0
	//i.e all bytes are read transfer is completed
	//we keep adding storage as needed 
	int BUFSIZE = 1024;
	char *folder_list = (char *)malloc(BUFSIZE);
	int bytes_read;
	int total_bytes=0;
	int buffer_step = 1;

	while(1){
		if((bytes_read = read(data_socket,(folder_list + total_bytes),(BUFSIZE-total_bytes))) < 0)
			error("error in listing files");
		total_bytes += bytes_read;

		if(bytes_read==0)
			break;

		//add memory if it was exceeded
		if(total_bytes >= BUFSIZE){
			buffer_step++;
			BUFSIZE *= buffer_step;
			folder_list = (char *)realloc(folder_list,BUFSIZE);
		}

	}

	if(read(command_socket,response_226,70) < 0)
		error("did not get 226 transfer completed");
	printf("%s\n",response_226);

	printf("%s\n",folder_list);
	

	close(data_socket);
	free(folder_list);
	folder_list=NULL;
	
}

void get_file(int command_socket,struct hostent *server,char *filename){

	//open data socket first
	int data_socket;
	if((data_socket = open_data_socket(command_socket,server)) < 0)
		error("could not open data socket");

	char retr[200]="RETR ";
	strcat(retr,filename);
	strcat(retr,"\r\n");

	char response_125[100];
	char response_226[100];
	char response_200[100];

	bzero(response_125,sizeof(response_125));
	bzero(response_226,sizeof(response_226));
	bzero(response_200,sizeof(response_200));


	//set transfer type to binary
	char binary[] = "TYPE I\r\n";
	if(write(command_socket,binary,strlen(binary)) < 0)
		error("could not send type command");

	if(read(command_socket,response_200,100) < 0)
		error("did not get 125 connection");
	printf("%s\n",response_200);
	

	if(write(command_socket,retr,strlen(retr)) < 0)
		error("could not send retr command");

	//code could be 125 for data connection already open
	//code could also be 500 if the file entered does not exist
	if(read(command_socket,response_125,100) < 0)
		error("did not get 125 connection");
	printf("%s\n",response_125);

	
	//we should keep reading until bytes read are 0
	//i.e all bytes are read meaning transfer is completed
	//we keep adding storage as needed 
	int BUFSIZE = 1024;
	char *filedata = (char *)malloc(BUFSIZE);
	int total_bytes = 0;
	int buffer_step = 1;
	int bytes_read;

	while(1){
		if((bytes_read = read(data_socket,(filedata+total_bytes),BUFSIZE-total_bytes)) < 0)
			error("did not read file data");
		
		if(bytes_read==0)
			break;
		total_bytes += bytes_read;
		
		//keep adding memory if overflowed
		if(total_bytes >= BUFSIZE){
			buffer_step++;
			BUFSIZE *= buffer_step;
			filedata=(char *)realloc(filedata,BUFSIZE);
		}
	}
	if(read(command_socket,response_226,100) < 0)
		error("did not get 226 transfer completed");
	
	printf("%s\n",response_226);
	
	create_file(filename,filedata,total_bytes);

	free(filedata);
	filedata=NULL;
	close(data_socket);
}
void create_file(char *filename,char *filedata,int filesize){
	FILE *fp;
	
	if((fp = fopen(filename,"wb"))!=NULL){
		fwrite(filedata,1,filesize,fp);
		
		fclose(fp);
	}

}

void enter_file(char *filename,int max){
	char *start = filename;
	for(;(filename < (start+max)) && ((*filename = getchar()) != EOF) && *filename!='\n';filename++)
		;

	*filename = '\0';
}