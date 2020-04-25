/*
** serverB.cpp
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
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <map>
#include <set>

using namespace std;
#define ServerB_Port "31738" // UDP port backend server A will be connected
#define MAXDATASIZE 1000     // max number of bytes we can get at once


// global vars
// struct Gragh
// {
//     char MapID;
//     float propagation_speed;
//     int transmission_speed;
//     int numEdges;
// };
// map<char, Gragh> map2;
map<char, string> edgedata;
char checked_map_id[MAXDATASIZE];
char checked_map_info[MAXDATASIZE];

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// check if mapid exists
bool checkMapID(char received_mapid)
{

    if (edgedata.count(received_mapid) == 1)
    {
        // if map id exists, return true
        return true;
    }
    else
    {
        return false; // map id does not exist in map1.txt
    }
}

// read map
bool readMap(char received_mapid)
{
    string line;
    string tp;
    char read_map_id;
    float read_prop;
    int read_trans;
    int read_src, read_dest;
    float read_distance;

    //printf("start reading from map1.txt");
    //Gragh city;
    // declare an input file stream variable
    ifstream inFile;
    inFile.open("./map2.txt"); // open the file stream

    while (getline(inFile, line))
    {
        // get the end of map document(no new line)
        if (line.empty() == true)
        {
            break;
        }

        // make a temp pointer to read info from current line
        const char *curr = line.c_str();
        char first_char = curr[0];

        // if first char is true, it means new mapid
        if (isalpha(first_char))
        {
            read_map_id = first_char;

            // initialize the map
            // city.propagation_speed = 0;
            // city.transmission_speed = 0;
            // city.numEdges = 0;

            // store mapid, trans, prop speed into the map object "map1"
            //map2.insert(pair<char, Gragh>(read_map_id, city));
            edgedata.insert(pair<char, string>(read_map_id, tp));

            getline(inFile, line); // read nextline
            edgedata[read_map_id].append(line);
            edgedata[read_map_id].append(";");

            getline(inFile, line); // read nextline
            edgedata[read_map_id].append(line);
            edgedata[read_map_id].append(";");
        }
        // if the first char in a new line is not alphabet
        if (!isalpha(first_char))
        {
            edgedata[read_map_id].append(line);
            edgedata[read_map_id].append(";");
        }
    }
    inFile.close();

    // check mapid
    return checkMapID(received_mapid);
}

int main(void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXDATASIZE];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    // received info from AWS
    char received_mapid[MAXDATASIZE];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, ServerB_Port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1)
        {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }
    freeaddrinfo(servinfo);

    // ServerB boots up
    printf("The Server B is up and running using UDP on port <%s>\n", ServerB_Port);
    printf("\n");

    while (1){
        addr_len = sizeof their_addr;

        // server A receives mapid from aws
        if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom aws mapid error");
            exit(1);
        }
        buf[numbytes] = '\0';
        strcpy(received_mapid, buf);

        printf("\nThe Server B has received input for finding graph of map <%s>\n", received_mapid);
        printf("\n");

        // read map2.txt and check if mapid exists
        char received_id_from_aws = received_mapid[0];
        //printf("befor get into readmap function\n");

        if (readMap(received_id_from_aws) == true)
        {
            // send map_id checking result to aws to notify if map_id exists
            checked_map_id[0] = received_id_from_aws;
            if ((sendto(sockfd, checked_map_id, strlen(checked_map_id), 0, (struct sockaddr *)&their_addr, addr_len)) == -1)
            {
                perror("ServerB: sendto aws error\n");
                exit(1);
            }

            // printf("\nThe Server B has sent checked mapid <%c> to AWS\n", checked_map_id[0]);
            // printf("\n");

            // send gragh info to aws
            if ((sendto(sockfd, edgedata[received_id_from_aws].c_str(), strlen(edgedata[received_id_from_aws].c_str()), 0, (struct sockaddr *)&their_addr, addr_len)) == -1)
            {
                perror("ServerB: sendto aws error\n");
                exit(1);
            }

            // signal to end
            printf("\nThe Server B has sent Graph to AWS\n");
            printf("\n");

            edgedata.clear();   // reset map
        }
        else{
            // checked mapid, it does not exist in map1.txt
            // send '0' as a signal to notify aws
            printf("\nThe Server B does not have the required graph id <%c>\n", received_id_from_aws);
            printf("\n");

            checked_map_id[0] = '0';
            if ((sendto(sockfd, checked_map_id, strlen(checked_map_id), 0, (struct sockaddr *)&their_addr, addr_len)) == -1)
            {
                perror("ServerB: sendto aws error\n");
                exit(1);
            }

            // send an empty info
            checked_map_info[0] = '1';
            if ((sendto(sockfd, checked_map_info, strlen(checked_map_info), 0, (struct sockaddr *)&their_addr, addr_len)) == -1)
            {
                perror("ServerB: sendto aws error\n");
                exit(1);
            }
            printf("\nThe Server B has sent 'Graph not Found' to AWS \n");
            printf("\n");

            edgedata.clear();
        }
    }
    close(sockfd);
    return 0;
}

