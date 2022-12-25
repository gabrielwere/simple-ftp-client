#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>

void error(char *);
int main(int argc, char *argv[])
{

	int socket_file_descriptor;

	if ((socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		error("could not create socket");
	else
	{

		if (argc == 2)
		{
			// change the port number if you wish to
			int port_number = 21;

			struct hostent *server = gethostbyname(argv[1]);
			if (server == NULL)
				error("no such server");

			struct sockaddr_in server_address;
			server_address.sin_family = AF_INET;
			server_address.sin_port = htons(port_number);
			bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);

			if ((connect(socket_file_descriptor, (struct sockaddr *)&server_address, sizeof(server_address))) < 0)
				error("could not connect");

			printf("Connected to %s:%d\n", argv[1], port_number);

			char response[100];
			int n;

			bzero(response,sizeof(response));
			if ((n = read(socket_file_descriptor, response, 100)) < 0)
				error("did not receive any data");

			// server is ready for new user
			if (strncmp(response, "220", 3) == 0)
			{

				char username[] = "USER anonymous\r\n";
				if ((n = write(socket_file_descriptor, username, strlen(username))) < 0)
					error("did not send any data");

				char enter_password[100];
				bzero(enter_password,sizeof(enter_password));
				if ((n = read(socket_file_descriptor, enter_password, 100)) < 0)
					error("did not receive any data");
				printf("%s\n", enter_password);

				// send password to server
				if (strncmp(enter_password, "331", 3) == 0)
				{
					char password[] = "PASS \r\n";
					if ((n = write(socket_file_descriptor, password, strlen(password))) < 0)
						error("did not send any data");

					char login_status[100];
					bzero(response,sizeof(login_status));
					if ((n = read(socket_file_descriptor, login_status, 100)) < 0)
						error("did not receive any data");

					printf("%s\n", login_status);
					
					char pasv[10] = "PASV \r\n";
					if ((n = write(socket_file_descriptor, pasv, strlen(pasv))) < 0)
						error("did not send any data");

					char server_addr[1000];
					bzero(server_addr,sizeof(server_address));
					if ((n = read(socket_file_descriptor, server_addr, 1000)) < 0)
						error("did not receive any data");

					printf("%s\n", server_addr);
					int code;
					int num1;
					int num2;
					int num3;
					int num4;
					int num5;
					int num6;
					char s1[20];
					char s2[20];
					char s3[20];

					sscanf(server_addr,"%d %s %s %s (%d,%d,%d,%d,%d,%d)",&code,s1,s2,s3,&num1,&num2,&num3,&num4,&num5,&num6);
					int extracted_port = (num5 * 256)+num6;

					int data_socket;

					if ((data_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
						error("could not create socket");

					struct hostent *server2 = gethostbyname(argv[1]);
					if (server2 == NULL)
						error("no such server");

					struct sockaddr_in server2_address;
					server2_address.sin_family = AF_INET;
					server2_address.sin_port = htons(extracted_port);
					bcopy((char *)server2->h_addr, (char *)&server2_address.sin_addr.s_addr, server2->h_length);

					if ((connect(data_socket, (struct sockaddr *)&server2_address, sizeof(server2_address))) < 0)
						error("could not connect");


					char ls[]="LIST \r\n";
					if((n = write(socket_file_descriptor,ls,strlen(ls))) < 0)
						error("did send any data");

					char connection2[1000];
					bzero(connection2,sizeof(connection2));
					if ((n = read(data_socket, connection2, 1000)) < 0)
						error("did not receive any data");

					printf("%s\n", connection2);
					close(data_socket);
				}
			}

			close(socket_file_descriptor);
		}
		else
			error("must provide host name");
	}
}

void error(char *error_message)
{
	perror(error_message);
	exit(1);
}
