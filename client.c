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

int main(int argc, char *argv[])
{

	if(argc==2){
		int socket_file_descriptor;
		int port = 21;
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

		char response_220[50];
		char response_331[50];
		char response_230[50];
		char response_227[50];

		bzero(response_220,sizeof(response_220));
		bzero(response_331,sizeof(response_331));
		bzero(response_230,sizeof(response_230));
		bzero(response_227,sizeof(response_227));

		if ((n = read(socket_file_descriptor, response_220, 50)) < 0)
			error("did not receive any data");
		printf("%s\n",response_220);
		

		if (strncmp(response_220, "220", 3) == 0)
		{
			//send username to the password
			char username[] = "USER anonymous\r\n";
			if ((n = write(socket_file_descriptor, username, strlen(username))) < 0)
				error("did not send any data");

			if ((n = read(socket_file_descriptor, response_331, 50)) < 0)
				error("did not receive any data");
			printf("%s\n", response_331);

			// send password to server
			if (strncmp(response_331, "331", 3) == 0)
			{
				//send password to the server
				char password[] = "PASS \r\n";
				if ((n = write(socket_file_descriptor, password, strlen(password))) < 0)
					error("did not send any data");

				if ((n = read(socket_file_descriptor, response_230, 50)) < 0)
					error("did not receive any data");
				printf("%s\n", response_230);

				if(strncmp(response_230,"230",3)==0)
				{
					char pasv[] = "PASV \r\n";
					if ((n = write(socket_file_descriptor, pasv, strlen(pasv))) < 0)
						error("did not send any data");

					
					if ((n = read(socket_file_descriptor,response_227, 50)) < 0)
						error("did not receive any data");
					printf("%s\n", response_227);


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

					int data_socket;
					if ((data_socket = create_socket(extracted_port,server)) < 0)
						error("could not create socket");

					char ls[]="RETR hello.txt\r\n";
					// char ls[]="LIST \r\n";
					if((n = write(socket_file_descriptor,ls,strlen(ls))) < 0)
						error("did send any data");
					
					char connection2[1000];
					bzero(connection2,sizeof(connection2));
					if ((n = read(socket_file_descriptor, connection2, 1000)) < 0)
						error("did not receive any data");
					printf("%s\n", connection2);

					char connection3[10000];
					bzero(connection3,sizeof(connection3));
					if ((n = read(data_socket, connection3, 10000)) < 0)
						error("did not receive any data");

					FILE *fp;
					if((fp = fopen("hello.txt","w"))!=NULL){
						fprintf(fp,"%s",connection3);
					}
					fclose(fp);

					char connection24[1000];
					bzero(connection24,sizeof(connection24));
					if ((n = read(socket_file_descriptor, connection24, 1000)) < 0)
						error("did not receive any data");
					printf("%s\n", connection24);


					close(data_socket);

				}else{
					error("code 230 not received");
				}
			}else{
				error("code 331 not received");
			}
		}else{
			error("code 220 not received");
		}
		close(socket_file_descriptor);
	
	}else{
			error("must provide host name");
	}
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
