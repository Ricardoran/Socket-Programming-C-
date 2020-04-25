/*
** client.cpp
    created by Haoran Zhang
    2088-8017-38
* Ref: The setup code structure is mainly copied from beej's pdf
	https://beej.us/guide/bgnet/examples/server.c
	https://beej.us/guide/bgnet/examples/client.c
	https://beej.us/guide/bgnet/examples/listener.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define AWS_TCPPORT "34738" // the port client will be connecting to
#define MAXDATASIZE 1000	// max number of bytes we can get at once
#define localhost "127.0.0.1"

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char const *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	// check if input arguments are valid for this program
	// ./client <Map ID> <Source Vertex Index> <Destination Vertex Index> <File Size>
	// EX: ./ client A 1 3 1024
	if (argc != 5)
	{
		fprintf(stderr, "usage: Input invalid\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(localhost, AWS_TCPPORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
	printf("Booting Up: The client is up and running\n");
	//printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure


	/**************************************************************/
	/********		send mapID and INFO to aws				*******/
	/**************************************************************/

	// send
	if (send(sockfd, argv[1], MAXDATASIZE - 1, 0) == -1)
	{
		perror("send Map ID error");
	}
	if (send(sockfd, argv[2], MAXDATASIZE - 1, 0) == -1)
	{
		perror("send src vertex ID error");
		usleep(1000);
	}
	if (send(sockfd, argv[3], MAXDATASIZE - 1, 0) == -1)
	{
		perror("send dest vertex ID error");
		usleep(1000);
	}
	if (send(sockfd, argv[4], MAXDATASIZE - 1, 0) == -1)
	{
		perror("send file size error");
		usleep(1000);
	}


	printf("\nThe client has sent query to AWS using TCP: ");
	printf("start vertex <%s>, destination vertex <%s>, map <%s>; file size <%s> \n", argv[2], argv[3], argv[1], argv[4]);



	/**************************************************************/
	/********		recv result from aws					*******/
	/**************************************************************/
	
	// set a flag
	// 1. if flag = '0', means receive is error, print error message
	char flag[MAXDATASIZE];
	if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
	{
		perror("recv");
		exit(1);
	}
	buf[numbytes] = '\0';
	strcpy(flag, buf);

	if (flag[0] == '0')		// means err msg
	{
		// print err msg
		printf("\nNo map id <%s> found or No vertex id <%s/%s> found\n", argv[1], argv[2], argv[3]);
		//printf("\nprogram ends!\n");
	}
	else
	{
		// receive calculation result
		// 2. recv path
		char path[MAXDATASIZE];
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
		{
			perror("recv");
			exit(1);
		}
		buf[numbytes] = '\0';
		strcpy(path, buf);


		// 3. recv distance
		char DISTANCE[MAXDATASIZE];
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
		{
			perror("recv");
			exit(1);
		}
		buf[numbytes] = '\0';
		strcpy(DISTANCE, buf);

		// 4. recv T_delay
		char T_dealy[MAXDATASIZE];
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
		{
			perror("recv");
			exit(1);
		}
		buf[numbytes] = '\0';
		strcpy(T_dealy, buf);

		// 5. recv P_delay
		char P_dealy[MAXDATASIZE];
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
		{
			perror("recv");
			exit(1);
		}
		buf[numbytes] = '\0';
		strcpy(P_dealy, buf);

		// make sum
		float delay;
		float fT_delay = atof(T_dealy);
		float fP_delay = atof(P_dealy);
		float distance = atof(DISTANCE);
		delay = fT_delay + fP_delay;

		// display
		printf("\nThe client has received results from AWS:\n");
		printf("--------------------------------------------\n");
		printf("Source\t Destination\t Min Length\t Tt\t Tp\t Delay\n");
		printf("%s\t %s\t \t %0.2f\t %0.2f\t %0.2f\t %0.2f\t\n", argv[2], argv[3], distance, fT_delay, fP_delay, delay);
		printf("--------------------------------------------\n");
		// print path
		printf("Shortest path: ");
		printf("%s\n",path);
		//printf("\nprogram ends\n");
	}

	close(sockfd);

	return 0;
}

