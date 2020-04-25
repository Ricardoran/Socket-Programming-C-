/*
** aws.cpp
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>

#define AWS_TCPPORT "34738" 	// the port user will be connecting to via TCP
#define AWS_UDPPort 33738		// the port user will be connecting to via UDP
#define ServerA_Port 30738		// UDP port backend server A will be connected
#define ServerB_Port 31738		// UDP port backend server B will be connected
#define ServerC_Port 32738		// UDP port backend server C will be connected
#define localhost "127.0.0.1"

#define MAXDATASIZE 1000		// max size of buffer
#define BACKLOG 10	 			// how many pending connections queue will hold
using namespace std;

char recv_MAPID[MAXDATASIZE];
char recv_MAPINFO[MAXDATASIZE];
char recv_MAPINFO_copy[MAXDATASIZE];
char recv_file_size[MAXDATASIZE];
char* recv_trans;
char* recv_prop;
char* input_src_vex;
char* input_dest_vex;
char* p;
char calculation_result[MAXDATASIZE];
char path[MAXDATASIZE];
char DISTANCE[MAXDATASIZE];
char T_Delay[MAXDATASIZE];
char P_Delay[MAXDATASIZE];
char client_flag[MAXDATASIZE];


void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// send mapid to serverA to check iff mapid exists
bool runUDPtoA(char* mapid){
	int udp_sockfd;
	int numbytes;
	struct sockaddr_in aws_addr;
	struct sockaddr_in A_addr;
	char buf[MAXDATASIZE];
	socklen_t A_addr_len;
	int yes = 1;

	udp_sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &yes, sizeof(yes));

	aws_addr.sin_family = AF_UNSPEC;
	aws_addr.sin_port = htons(AWS_UDPPort);
	aws_addr.sin_addr.s_addr = inet_addr(localhost);
	memset(aws_addr.sin_zero, '\0', sizeof aws_addr.sin_zero);

	// Setup connection with server A
	A_addr.sin_family = AF_UNSPEC;
	A_addr.sin_port = htons(ServerA_Port);
	A_addr.sin_addr.s_addr = inet_addr(localhost);
	memset(A_addr.sin_zero, '\0', sizeof A_addr.sin_zero);


	// bind the port to AWS server
	bind(udp_sockfd, (struct sockaddr *)&aws_addr, sizeof aws_addr);


	/**************************************************************/
	/********			send mapid to server A				*******/
	/**************************************************************/

	// send map_id to backend server A to check if map_id exists
	if ((numbytes = sendto(udp_sockfd, mapid, strlen(mapid), 0, (struct sockaddr *)&A_addr, sizeof A_addr)) == -1){
		perror("AWS: sendto server A error");
		exit(1);
	}
	cout <<"\nThe AWS has sent map ID to server A using UDP over port <" << AWS_UDPPort << ">"<< endl;


	/**************************************************************/
	/********	receive gragh(if exists) from server A		*******/
	/**************************************************************/

	// receive mapid from serverA
	if ((numbytes = recvfrom(udp_sockfd, buf, MAXDATASIZE - 1, 0, (struct sockaddr *)&A_addr, &A_addr_len)) == -1){
		perror("recvfrom serverA mapid check result error");
		exit(1);
	}
	buf[numbytes] = '\0';
	strcpy(recv_MAPID, buf);
	//printf("\nThe AWS has received map ID <%c> from server server <A> \n", recv_MAPID[0]);

	// recv map info from serverA
	if ((numbytes = recvfrom(udp_sockfd, buf, MAXDATASIZE - 1, 0, (struct sockaddr *)&A_addr, &A_addr_len)) == -1)
	{
		perror("recvfrom serverA mapinfo check result error");
		exit(1);
	}
	buf[numbytes] = '\0';
	strcpy(recv_MAPINFO, buf);
	// printf("\nThe AWS has received mapinfo <%s> from server server <A> \n", recv_MAPINFO);

	// signal to end
	printf("\nThe AWS has received map information from server server <A> \n");

	if (recv_MAPID[0] == '0')
	{
		// printf("\n map %s does not exist in server B\n", mapid);
		return false;
	}

	// make a copy for recv_MAPINFO if mapid exists
	strcpy(recv_MAPINFO_copy, recv_MAPINFO);

	return true;	// mapid exists

}

// send mapid to serverB to check if mapid exists
bool runUDPtoB(char *mapid){
	int udp_sockfd;
	int numbytes;
	struct sockaddr_in aws_addr;
	struct sockaddr_in B_addr;
	char buf[MAXDATASIZE];
	socklen_t B_addr_len;
	int yes = 1;

	udp_sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &yes, sizeof(yes));

	aws_addr.sin_family = AF_UNSPEC;
	aws_addr.sin_port = htons(AWS_UDPPort);
	aws_addr.sin_addr.s_addr = inet_addr(localhost);
	memset(aws_addr.sin_zero, '\0', sizeof aws_addr.sin_zero);

	// Setup connection with server B
	B_addr.sin_family = AF_UNSPEC;
	B_addr.sin_port = htons(ServerB_Port);
	B_addr.sin_addr.s_addr = inet_addr(localhost);
	memset(B_addr.sin_zero, '\0', sizeof B_addr.sin_zero);

	// bind the port to AWS server
	bind(udp_sockfd, (struct sockaddr *)&aws_addr, sizeof aws_addr);


	/**************************************************************/
	/********			send mapid to server B				*******/
	/**************************************************************/

	// send map_id to backend server B to check if map_id exists
	if ((numbytes = sendto(udp_sockfd, mapid, strlen(mapid), 0, (struct sockaddr *)&B_addr, sizeof B_addr)) == -1)
	{
		perror("AWS: sendto server B error\n");
		exit(1);
	}
	cout << "\nThe AWS has sent map ID to server B using UDP over port <" << AWS_UDPPort << "> " << endl;



	/**************************************************************/
	/********	receive gragh(if exists) from server B		*******/
	/**************************************************************/

	// receive mapid from serverB(if exits)
	if ((numbytes = recvfrom(udp_sockfd, buf, MAXDATASIZE - 1, 0, (struct sockaddr *)&B_addr, &B_addr_len)) == -1)
	{
		perror("recvfrom serverB mapid check result error\n");
		exit(1);
	}
	buf[numbytes] = '\0';
	strcpy(recv_MAPID, buf);
	//printf("\nThe AWS has received map ID <%c> from server server <B> \n", recv_MAPID[0]);

	// recv map info from serverA
	if ((numbytes = recvfrom(udp_sockfd, buf, MAXDATASIZE - 1, 0, (struct sockaddr *)&B_addr, &B_addr_len)) == -1)
	{
		perror("recvfrom serverB mapinfo check result error");
		exit(1);
	}
	buf[numbytes] = '\0';
	strcpy(recv_MAPINFO, buf);
	printf("\nThe AWS has received map information from server server <B> \n");

	if (recv_MAPID[0] == '0')
	{
		//printf("\n map %s does not exist in server B\n", mapid);
		return false;
	}

	// make a copy for recv_MAPINFO if mapid exists
	strcpy(recv_MAPINFO_copy, recv_MAPINFO);

	return true; // mapid exists
}

// send mapinfo to serverC for final calculation
void runUDPtoC(char *mapid)
{
	int udp_sockfd;
	int numbytes;
	int yes = 1;
	struct sockaddr_in aws_addr;
	struct sockaddr_in C_addr;
	char buf[MAXDATASIZE];
	socklen_t C_addr_len;

	udp_sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	setsockopt(udp_sockfd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, &yes, sizeof(yes));

	aws_addr.sin_family = AF_UNSPEC;
	aws_addr.sin_port = htons(AWS_UDPPort);
	aws_addr.sin_addr.s_addr = inet_addr(localhost);
	memset(aws_addr.sin_zero, '\0', sizeof aws_addr.sin_zero);

	// Setup connection with server C
	C_addr.sin_family = AF_UNSPEC;
	C_addr.sin_port = htons(ServerC_Port);
	C_addr.sin_addr.s_addr = inet_addr(localhost);
	memset(C_addr.sin_zero, '\0', sizeof C_addr.sin_zero);

	// bind the server c port to AWS server
	bind(udp_sockfd, (struct sockaddr *)&aws_addr, sizeof aws_addr);



	/**************************************************************/
	/********		send mapID and INFO to server C			*******/
	/**************************************************************/

	// send mapID to serverC for calculation
	if ((numbytes = sendto(udp_sockfd, mapid, strlen(mapid), 0, (struct sockaddr *)&C_addr, sizeof C_addr)) == -1)
	{
		perror("AWS: sendto server C error\n");
		exit(1);
	}

	// send file size to serverC
	if ((numbytes = sendto(udp_sockfd, recv_file_size, strlen(recv_file_size), 0, (struct sockaddr *)&C_addr, sizeof C_addr)) == -1)
	{
		perror("AWS: sendto server C error\n");
		exit(1);
	}

	// send src idx to serverC for calculation
	if ((numbytes = sendto(udp_sockfd, input_src_vex, strlen(input_src_vex), 0, (struct sockaddr *)&C_addr, sizeof C_addr)) == -1)
	{
		perror("AWS: sendto server C error\n");
		exit(1);
	}

	// send dest idx to serverC for calculation
	if ((numbytes = sendto(udp_sockfd, input_dest_vex, strlen(input_dest_vex), 0, (struct sockaddr *)&C_addr, sizeof C_addr)) == -1)
	{
		perror("AWS: sendto server C error\n");
		exit(1);
	}

	// send mapINFO to serverC for calculation
	if ((numbytes = sendto(udp_sockfd, recv_MAPINFO_copy, strlen(recv_MAPINFO_copy), 0, (struct sockaddr *)&C_addr, sizeof C_addr)) == -1)
	{
		perror("AWS: sendto server C error\n");
		exit(1);
	}
	//printf("\nAWS send mapinfo to ser C is %s", recv_MAPINFO_copy);
	printf("\nThe AWS has sent map, source ID, destination ID, propagation speed and transmission speed to server C using UDP over port <%d>\n", AWS_UDPPort);


	/**************************************************************/
	/********	receive calculation from server C			*******/
	/**************************************************************/

	//1. receive path from serverC
	usleep(1000);
	if ((numbytes = recvfrom(udp_sockfd, buf, MAXDATASIZE - 1, 0, (struct sockaddr *)&C_addr, &C_addr_len)) == -1)
	{
		perror("recvfrom serverC path result error\n");
		exit(1);
	}
	usleep(1000);
	buf[numbytes] = '\0';
	strcpy(path, buf);

	// 2. receive shortest distance from serverC
	if ((numbytes = recvfrom(udp_sockfd, buf, MAXDATASIZE - 1, 0, (struct sockaddr *)&C_addr, &C_addr_len)) == -1)
	{
		perror("recvfrom serverC shortest dist result error\n");
		exit(1);
	}
	buf[numbytes] = '\0';
	strcpy(DISTANCE, buf);
	float calculated_dist = atof(DISTANCE);

	// 3. receive Trans delay from serverC
	if ((numbytes = recvfrom(udp_sockfd, buf, MAXDATASIZE - 1, 0, (struct sockaddr *)&C_addr, &C_addr_len)) == -1)
	{
		perror("recvfrom serverC T delay error\n");
		exit(1);
	}
	buf[numbytes] = '\0';
	strcpy(T_Delay, buf);
	float t_delay = atof(T_Delay);

	// 4. receive Prop delay from serverC
	if ((numbytes = recvfrom(udp_sockfd, buf, MAXDATASIZE - 1, 0, (struct sockaddr *)&C_addr, &C_addr_len)) == -1)
	{
		perror("recvfrom prop delay error\n");
		exit(1);
	}
	buf[numbytes] = '\0';
	strcpy(P_Delay, buf);
	float p_delay = atof(P_Delay);

	printf("\nThe AWS has received results from server C:\n");
	printf("Shortest path: ");
	cout << path <<endl;
	printf("Shortest distance: <%0.2f> km\n", calculated_dist);
	printf("Transmission delay: <%0.2f> s\n", t_delay);
	printf("Propagation delay: <%0.2f> s\n", p_delay);
}

// check nodes if exist in the specific mapid
// ref:http://www.cplusplus.com/reference/cstring/strtok/?kw=strtok
bool checkNodes(char* recv_MAPINFO){

	char* each_edge;
	int src_idx;
	int dest_idx;
	float distance_val;
	int src_arr[40];
	int dest_arr[40];
	float distance_arr[40];
	int i = 1;

	//printf("input_src_vex is %s\n", input_src_vex);
	//printf("input_dest_vex is %s\n", input_dest_vex);

	// convert str to int
	int int_src_vex = atoi(input_src_vex);
	int int_dest_vex = atoi(input_dest_vex);

	recv_prop = strtok(recv_MAPINFO, ";");
	recv_trans = strtok(NULL,";");
	//printf("\nprop is %s, trans is %s\n", recv_prop, recv_trans);

	each_edge = strtok(NULL, ";");
	sscanf(each_edge, "%d%d%f", &src_idx, &dest_idx, &distance_val);
	src_arr[0] = src_idx;
	dest_arr[0] = dest_idx;
	distance_arr[0] = distance_val;
	//printf("\nsrc idx is %d, dest idx is %d, distance idx is %0.2f\n", src_arr[0], dest_arr[0], distance_arr[0]);
	while (i < 40)
	{
		each_edge = strtok(NULL, ";");
		if(each_edge == NULL){
			break;
		}
		sscanf(each_edge, "%d%d%f", &src_idx, &dest_idx, &distance_val);
		src_arr[i] = src_idx;
		dest_arr[i] = dest_idx;
		distance_arr[i] = distance_val;

		//printf("\nsrc idx is %d, dest idx is %d, distance idx is %0.2f\n", src_arr[i], dest_arr[i], distance_arr[i]);
		i++;
	}
	// printf("\nends\n");

	// check ever node in the array if node exists
	for (int j = 0; j < 40; j++)
	{
		if (src_arr[j] == int_src_vex && dest_arr[j] == int_dest_vex)
		{
			return true;
		}
		if (src_arr[j] == int_dest_vex && dest_arr[j] == int_src_vex)
		{
			return true;
		}
	}
	return false;
}

// send back to client
// useless
void sendBackToClientErr(){

}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;
	int num_bytes;
	char buffer[MAXDATASIZE];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, AWS_TCPPORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("aws: socket error");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind error");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("\nThe AWS is up and running.\n");
	printf("\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		//printf("\nserver: got connection from %s\n", s);


		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			/**************************************************************/
			/********		communication with client starts		*******/
			/**************************************************************/

			// received MAP ID from client
			if ((num_bytes = recv(new_fd, buffer, MAXDATASIZE - 1, 0)) == -1)
			{
				perror("aws: recv MAP ID err");
				exit(1);
			}
			buffer[num_bytes] = '\0';
			char MAP_ID[MAXDATASIZE];
			strcpy(MAP_ID, buffer);

			// received Source vertex from client
			if ((num_bytes = recv(new_fd, buffer, MAXDATASIZE - 1, 0)) == -1)
			{
				perror("aws: recv src vertex err");
				exit(1);
			}
			buffer[num_bytes] = '\0';
			char src_vertex[MAXDATASIZE];
			strcpy(src_vertex, buffer);

			// use global pointer
			input_src_vex = src_vertex;

			// received End vertex from client
			if ((num_bytes = recv(new_fd, buffer, MAXDATASIZE - 1, 0)) == -1)
			{
				perror("aws: recv dest vertex err");
				exit(1);
			}
			buffer[num_bytes] = '\0';
			char dest_vertex[MAXDATASIZE];
			strcpy(dest_vertex, buffer);

			// use global pointer
			input_dest_vex = dest_vertex;

			// received file size from client
			if ((num_bytes = recv(new_fd, buffer, MAXDATASIZE - 1, 0)) == -1)
			{
				perror("aws: recv file size err");
				exit(1);
			}
			buffer[num_bytes] = '\0';
			char file_size[MAXDATASIZE];
			stpcpy(file_size, buffer);

			// use global pointer
			strcpy(recv_file_size,file_size);

			//printf("\n");
			printf("\nThe AWS has received map ID <%s>, ", MAP_ID);
			printf("start vertex <%s>, ", src_vertex);
			printf("destination vertex <%s>, ", dest_vertex);
			printf("and file size <%s> from the client using TCP over port <%s>\n", file_size, AWS_TCPPORT);
			printf("\n");

			/**************************************************************/
			/********		communication with client suspend		*******/
			/**************************************************************/



			/**************************************************************/
			/********	communication with server A B C starts		*******/
			/**************************************************************/

			// send map id to serverA check if mapid exists
			if(runUDPtoA(MAP_ID)== true){

				// printf("\nThe map id exists in serverA %s\n", MAP_ID);
				if (checkNodes(recv_MAPINFO) == true){

					//printf("\nindex exist in map %s!\n", MAP_ID);
					// send to serverC for calculation
					printf("\nThe source and destination vertex are in the graph\n");
					runUDPtoC(MAP_ID);
					
				}
				else{
					// need modification
					printf("\n<%s/%s> vertex not found in the graph, sending error to client using TCP over port <%s>\n", input_src_vex, input_dest_vex, AWS_TCPPORT);

					// send error message to client
					// "No map id <mad id> found or No vertex id <vertex index> found"

					// 1. send a flag first
					client_flag[0] = '0';

					if (send(new_fd, client_flag, MAXDATASIZE, 0) == -1)
					{
						perror("send back to client error\n");
					}

					//printf("\n send flag to client done\n");


					// program  ends
					close(new_fd);
					exit(0);
				}
				
			}
			// then send map id to serverB
			else if (runUDPtoB(MAP_ID) == true){

				// printf("\nThe receivd info from serverB is %s\n", MAP_ID);
				if (checkNodes(recv_MAPINFO) == true)
				{
					// send to serverC for calculation
					printf("\nThe source and destination vertex are in the graph\n");
					runUDPtoC(MAP_ID);
					// receive info from serverC
				}
				else{
					printf("\n<%s/%s> vertex not found in the graph, sending error to client using TCP over port <%s>\n", input_src_vex, input_dest_vex, AWS_TCPPORT);

					// send error message to client
					// "No map id <mad id> found or No vertex id <vertex index> found"

					// 1. send a flag first
					client_flag[0] = '0';

					if (send(new_fd, client_flag, MAXDATASIZE, 0) == -1)
					{
						perror("send back to client error\n");
					}

					//printf("\nsend flag to client done\n");

					// program  ends
					close(new_fd);
					exit(0);
				}

			}

			// then map id does not  exist in both server A serverB
			else{
				//printf("\nmapid %s does not exits in both\n", MAP_ID);

				// send error message to client
				// "No map id <mad id> found or No vertex id <vertex index> found"

				// 1. send a flag first
				client_flag[0] = '0';

				if (send(new_fd, client_flag, MAXDATASIZE, 0) == -1)
				{
					perror("send back to client error\n");
				}

				//printf("\nsend flag to client done\n");

				// program  ends
				close(new_fd);
				exit(0);
			}

			/**************************************************************/
			/********	communication with server A B C ends		*******/
			/**************************************************************/



			/**************************************************************/
			/********	communication with client restarts!			*******/
			/**************************************************************/

			// send path back to client

			// 1. send flag first
			char flag_succeed[MAXDATASIZE];
			flag_succeed[0] = '1';

			if (send(new_fd, client_flag, MAXDATASIZE - 1, 0) == -1)
			{
				perror("send path back to client error\n");
			}
			//printf("\nthe server has sent succeed flag back info to client\n");


			// 2. send path
			if (send(new_fd, path, MAXDATASIZE - 1, 0) == -1)
			{
				perror("send path back to client error\n");
			}
			//printf("\nthe server has sent path back info to client\n");


			// 3. send distance
			if (send(new_fd, DISTANCE, MAXDATASIZE - 1, 0) == -1)
			{
				perror("send path back to client error\n");
			}
			//printf("\nthe server has sent distance back info to client\n");

			// 4. send trans delay
			if (send(new_fd, T_Delay, MAXDATASIZE - 1, 0) == -1)
			{
				perror("send path back to client error\n");
			}
			//printf("\nthe server has sent trans delay back info to client\n");

			// 5. send prop delay
			if (send(new_fd, P_Delay, MAXDATASIZE - 1, 0) == -1)
			{
				perror("send path back to client error\n");
			}
			//printf("\nthe server has sent prop delay back info to client\n");

			printf("\nthe server has sent back info to client using TCP over port<%s>\n", AWS_TCPPORT);
			printf("\n");



			/**************************************************************/
			/********	communication with client ends!			*******/
			/**************************************************************/

			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

