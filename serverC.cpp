/*
** serverC.cpp
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
#include <set>
#include <vector>
#include <string>
#include <algorithm>

#define ServerC_Port "32738" // UDP port backend server C will be connected
#define MAXDATASIZE 1000    // max number of bytes we can get at once
#define MAX_DISTANCE 300000.0

std::string pa = "";
int cc = 0;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// convert real nodes into corresponding nodes
int realToCorresponding(int real_node, std::vector<int> input)
{
    std::vector<int>::iterator it;
    it = std::find(input.begin(), input.end(), real_node);
    if (it != input.end())
    {
        return it - input.begin();
    }
}

// ref:https://www.geeksforgeeks.org/printing-paths-dijkstras-shortest-path-algorithm/
// A utility function to find the vertex with minimum distance value, from
// the set of vertices not yet included in shortest path tree
int minDistance(float shortest_dist[], bool sptSet[], int graph_size)
{
    // Initialize min value
    float min = MAX_DISTANCE;
    int min_index;

    for (int v = 0; v < graph_size; v++)
        if (sptSet[v] == false && shortest_dist[v] <= min)
        {
            min = shortest_dist[v];
            min_index = v;
        }

    return min_index;
}

// ref: https://www.geeksforgeeks.org/printing-paths-dijkstras-shortest-path-algorithm/
// Function to print shortest
// path from source to j
// using parent array
void printPath(int parent[], int j, std::vector<int> input)
{
    // Base Case : If j is source
    if (parent[j] == -1)
        return;

    printPath(parent, parent[j],input);
    pa += " -- ";
    pa += std::to_string(input.at(j));
    printf(" -- %s", std::to_string(input.at(j)).c_str());
    cc ++;
}


// ref:https://www.geeksforgeeks.org/printing-paths-dijkstras-shortest-path-algorithm/
// A utility function to print the constructed distance array
void printSolution(float shortest_dist[], int graph_size, int parent[], int src, int dest, std::vector<int> input)
{
    //printf("Vertex \t\t Distance from Source \t\t Path\n");
    // for (int i = 0; i < graph_size; i++)
    // {
    //printf("\n%d -> %d \t\t  %0.2f \t\t %d ", src, dest, shortest_dist[dest], input.at(src));
    printf("shortest path is %s", std::to_string(input.at(src)).c_str());
    printPath(parent, dest, input);
        
    //}
    
}




using namespace std;

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

    // received map info from AWS
    char received_mapid[MAXDATASIZE];
    char received_mapinfo[MAXDATASIZE];
    char received_mapinfo_COPY[MAXDATASIZE];
    char received_src_idx[MAXDATASIZE];
    char received_dest_idx[MAXDATASIZE];
    char recv_file_size[MAXDATASIZE];
    char* recv_prop_aws;
    char* recv_trans_aws;
    char* each_line;
    int v1,v2;
    float dist;
    int i = 1;
    int cnt = 0;

    int v1_arr[40];
    int v2_arr[40];
    float dist_arr[40];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, ServerC_Port, &hints, &servinfo)) != 0)
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
            perror("serverC: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("serverC: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "serverC: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    // server C boots up
    printf("The Server C is up and running using UDP on port <%s>\n", ServerC_Port);
    printf("\n");

    while (1)
    {

        addr_len = sizeof their_addr;

        /**************************************************************/
        /********		           recv from aws     	       	*******/
        /**************************************************************/


        // server C receives mapid from aws
        if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE - 1, 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom aws mapid error\n");
            exit(1);
        }
        buf[numbytes] = '\0';
        strcpy(received_mapid, buf);

        // server C receives file size from aws
        if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom aws filesize error\n");
            exit(1);
        }
        buf[numbytes] = '\0';
        strcpy(recv_file_size, buf);

        //printf("\nrecv from aws file size is %s\n", recv_file_size);

        // server C receives src idx from aws
        if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom aws src error\n");
            exit(1);
        }
        buf[numbytes] = '\0';
        strcpy(received_src_idx, buf);

        // server C receives dest idx from aws
        if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE - 1, 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom aws dest error\n");
            exit(1);
        }
        buf[numbytes] = '\0';
        strcpy(received_dest_idx, buf);

        // server C receives mapINFO from aws
        if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE - 1, 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom aws mapINFO error\n");
            exit(1);
        }
        buf[numbytes] = '\0';
        strcpy(received_mapinfo, buf);

        // make a copy for received_mapinfo
        strcpy(received_mapinfo_COPY, received_mapinfo);

        //printf("\nreceived map info from aws is <%s> \n", received_mapinfo);

        // process recv mapinfo from aws
        recv_prop_aws = strtok(received_mapinfo, ";"); // char
        recv_trans_aws = strtok(NULL, ";");
        each_line = strtok(NULL, ";");
        sscanf(each_line, "%d%d%f", &v1, &v2, &dist);
        v1_arr[0] = v1;
        v2_arr[0] = v2;
        dist_arr[0] = dist;
        //printf("\nsrc idx is %d, dest idx is %d, distance idx is %0.2f\n", v1_arr[0], v2_arr[0], dist_arr[0]);
        while (i < 40)
        {
            each_line = strtok(NULL, ";");
            if (each_line == NULL)
            {
                break;
            }
            sscanf(each_line, "%d%d%f", &v1, &v2, &dist);
            v1_arr[i] = v1;
            v2_arr[i] = v2;
            dist_arr[i] = dist;
            //printf("\nsrc idx is %d, dest idx is %d, distance idx is %0.2f\n", v1_arr[i], v2_arr[i], dist_arr[i]);
            i++;
        }
        //printf("\nends\n");


        printf("\nThe Server C has received data for calculation: \n");
        printf("* Propagation speed: <%s> km/s;\n", recv_prop_aws);
        printf("* Transmission speed: <%s> kB/s;\n", recv_trans_aws);
        printf("* map ID: <%s>;\n", received_mapid);
        printf("* Source ID: <%s>    Destination ID: <%s>;\n", received_src_idx, received_dest_idx);



        /**************************************************************/
        /********		         Do calculation            		*******/
        /**************************************************************/

        // create a map for calculation
        // 1. determine gragh size 
        set<int>nodes;
        int j;
        for (j = 0; j < i; j++)
        {
            nodes.insert(v1_arr[j]);
        }
        for (j = 0; j < i; j++)
        {
            nodes.insert(v2_arr[j]);
        }

        // print set
        //cout << "\nthe set is " << endl;
        // for (auto it = nodes.begin(); it != nodes.end(); ++it)
        // {
        //     cout << ' ' << *it;
        // }
        //cout << "\nthe set ends " << endl;

        int gragh_size = nodes.size();
        //cout << "\nthe gragh size is  "<< gragh_size << endl;
        
        // make a gragh
        int node_idx[gragh_size];
        vector<int> nodes_idx_vector;

        // // read from set
        for (auto it = nodes.begin(); it != nodes.end(); ++it)
        {
            nodes_idx_vector.push_back(*it);
        }
        //cout << "\nread from set ends \n"<< endl;

        // // assign vector to array
        for (j = 0; j < nodes_idx_vector.size(); j++)
        {
            node_idx[j] = nodes_idx_vector.at(j);
        }
        //std::cout << "\nassign vector to arr done\n"<< std::endl;

        // // print array
        // for (j = 0; j < gragh_size; j++)
        // {
        //     std::cout <<" "<< node_idx[j];
        // }
        // std::cout << "\nnode_idx[" << gragh_size << "] is above\n"
        //           << std::endl;

        // initialize the gragh
        float gragh[gragh_size][gragh_size];
        for (int m = 0; m < gragh_size; m++)
        {
            for (int n = 0; n < gragh_size; n++)
            {
                gragh[m][n] = 0;
            }
        }

        // complete the gragh  need modification
        std::vector<int>::iterator iter1;
        std::vector<int>::iterator iter2;

        int temp1 = 0, temp2 = 0;
        for (j = 0; j < i; j++)
        {
            iter1 = find(nodes_idx_vector.begin(), nodes_idx_vector.end(), v1_arr[j]);
            if (iter1 != nodes_idx_vector.end()){
                temp1 = iter1 - nodes_idx_vector.begin();
            }
            iter2 = find(nodes_idx_vector.begin(), nodes_idx_vector.end(), v2_arr[j]);
            if (iter2 != nodes_idx_vector.end())
            {
                temp2 = iter2 - nodes_idx_vector.begin();
            }
            gragh[temp1][temp2] = dist_arr[j];
            gragh[temp2][temp1] = dist_arr[j];
        }
        
        // // print gragh
        // cout << "assigned gragh is below"<<endl;
        // for (int m = 0; m < gragh_size; m++)
        // {
        //     for (int n = 0; n < gragh_size; n++)
        //     {
        //         cout <<"  "<< gragh[m][n];
        //     }
        //     cout << endl;
        //     cout << endl;
        // }
        // cout << "\nassigned gragh ends" << endl;

        // do dijkstra
        /**
         * the following code for calculation
         * is from https://www.geeksforgeeks.org/printing-paths-dijkstras-shortest-path-algorithm/
         */
        int sourse_node = atoi(received_src_idx);
        int end_node = atoi(received_dest_idx);

        // find corresponding index [0-graph_size]
        int corr_idx_s, corr_idx_d;
        corr_idx_s = realToCorresponding(sourse_node, nodes_idx_vector);
        corr_idx_d = realToCorresponding(end_node, nodes_idx_vector);

        // iter1 = find(nodes_idx_vector.begin(), nodes_idx_vector.end(), sourse_node);
        // if (iter1 != nodes_idx_vector.end()){
        //     corr_idx_s = iter1 - nodes_idx_vector.begin();
        // }

        // iter2 = find(nodes_idx_vector.begin(), nodes_idx_vector.end(), end_node);
        // if (iter2 != nodes_idx_vector.end())
        // {
        //     corr_idx_d = iter2 - nodes_idx_vector.begin();
        // }

        //printf("\nsrc node is %d, corresponding src is %d\n", sourse_node, corr_idx_s);
        //printf("\ndest node is %d, corresponding dest is %d\n", end_node, corr_idx_d);

        float shortest_dist[gragh_size];  // output array
        bool sptSet[gragh_size];        // sptSet[i] will be true if vertex i is included in shortest
                                        // path tree or shortest distance from src to i is finalized
        int parent[gragh_size];         // Parent array to store shortest path tree

        // Initialize all distances as INFINITE and stpSet[] as false
        for (int s = 0; s < gragh_size; s++){
            shortest_dist[s] = MAX_DISTANCE,
            sptSet[s] = false;
            parent[corr_idx_s] = -1;
        }

        // Distance of source vertex from itself is always 0
        shortest_dist[corr_idx_s] = 0.0;

        vector <int> station;

        // Find shortest path for all vertices
        for (int count = 0; count < gragh_size - 1; count++)
        {
            // Pick the minimum distance vertex from the set of vertices not
            // yet processed. u is always equal to src in the first iteration.
            int u = minDistance(shortest_dist, sptSet, gragh_size);

            // Mark the picked vertex as processed
            sptSet[u] = true;

            // Update dist value of the adjacent vertices of the picked vertex.
            for (int v = 0; v < gragh_size; v++){
                // Update dist[v] only if is not in sptSet, there is an edge from
                // u to v, and total weight of path from src to  v through u is
                // smaller than current value of dist[v]
                if (!sptSet[v] && gragh[u][v] 
                    && shortest_dist[u] != MAX_DISTANCE 
                    && shortest_dist[u] + gragh[u][v] < shortest_dist[v])
                {
                    parent[v] = u;
                    shortest_dist[v] = shortest_dist[u] + gragh[u][v];
                }
            }
            station.push_back(u);             
        }
        // printf("\ninter stations are: \n");
        // for ( int a = 0; a < station.size(); a++)
        // {
        //     cout << "  "<<station.at(a);
        // }
        // cout << endl;
        // for (int a = 0; a < station.size(); a++)
        // {
        //     cout << "  " << nodes_idx_vector.at(station.at(a));
        // }

        // printf("\ninter stations done\n");


        //printf("\nsrc to dest is %0.2f\n", shortest_dist[corr_idx_d]);
        // print the constructed distance array
        //printSolution(shortest_dist, gragh_size, parent, corr_idx_s, corr_idx_d, nodes_idx_vector);

        // calculate delay
        float trans_speed = atof(recv_trans_aws);
        float prop_speed = atof(recv_prop_aws);
        float file_size = atof(recv_file_size);
        float trans_delay = 0.0;
        float prop_delay = 0.0;

        for (int a = 0; a < station.size(); a++)
        {
            trans_delay += file_size / trans_speed;
        }

        // prop delay
        prop_delay = shortest_dist[corr_idx_d]/prop_speed;

        pa += to_string(sourse_node);

        //printf("\ncc is %d\n", cc);

        // show results
        printf("\nThe Server C has finished the calculation: \n");
        printSolution(shortest_dist, gragh_size, parent, corr_idx_s, corr_idx_d, nodes_idx_vector);
        printf("\nShortest distance: <%0.2f> km\n", shortest_dist[corr_idx_d]);
        printf("Transmission delay: <%0.2f> s\n", trans_delay);
        printf("Propagation delay: <%0.2f> s\n", prop_delay);
        printf("\n");

        // make path into a string
        //printf("\nShortest path: %s\n", pa.c_str());

        // for (int a = 0; a < station.size() - 1; a++)
        // {
        //     path += "<";
        //     path += to_string(nodes_idx_vector.at(station.at(a)));
        //     path += ">";
        //     path += " -- ";
        // }
        // path += "<";
        // path += to_string(nodes_idx_vector.at(station.at(station.size() - 1)));
        // path += ">";

        //cout << "\npath is "<< path <<endl;

        // make distance into string
        string DISTANCE = to_string(shortest_dist[corr_idx_d]);

        // make trans delay to string
        string T_delay = to_string(trans_delay);

        // make prop delay to string
        string P_delay = to_string(prop_delay);

        /**************************************************************/
        /********		    send calculation to aws     		*******/
        /**************************************************************/

        // 1. send path to aws
        if ((sendto(sockfd, pa.c_str(), MAXDATASIZE, 0, (struct sockaddr *)&their_addr, addr_len)) == -1)
        {
            perror("ServerC: sendto path aws error\n");
            //exit(1);
            
        }
        usleep(1000);
        //printf("\nThe Server C has sent path to AWS\n");

        // 2. send min distance to aws
        if ((sendto(sockfd, DISTANCE.c_str(), MAXDATASIZE, 0, (struct sockaddr *)&their_addr, addr_len)) == -1)
        {
            perror("ServerC: sendto distance aws error\n");
            exit(1);
        }
        //printf("\nThe Server C has sent min distance to AWS\n");

        // 3. send trans delay  to aws
        if ((sendto(sockfd, T_delay.c_str(), MAXDATASIZE, 0, (struct sockaddr *)&their_addr, addr_len)) == -1)
        {
            perror("ServerC: sendto delay aws error\n");
            exit(1);
        }
        //printf("\nThe Server C has sent trans delay to AWS\n");

        // 4. send min distance to aws
        if ((sendto(sockfd, P_delay.c_str(), MAXDATASIZE, 0, (struct sockaddr *)&their_addr, addr_len)) == -1)
        {
            perror("ServerC: sendto delay aws error\n");
            exit(1);
        }
        //printf("\nThe Server C has sent prop delay to AWS\n");

        printf("The Server C has finished sending the output to AWS\n");

        strcpy(received_mapinfo, "");
        //printf("\nafter, recv info is %s\n", received_mapinfo);



        // after calculation, reset every thing for the next round
        int a = 0;
        while (a < 40)
        {
            dist_arr[a] = 0.0;
            v1_arr[a] = 0;
            v2_arr[a] = 0;
            a++;
        }

        i = 1;          // reset i
        gragh_size = 0; // reset gragh size
        nodes.clear();
        nodes_idx_vector.clear();
        strcpy(received_mapinfo, "");
        cnt = 0;
        cc = 0;

        pa = "";

        // printf("\nthe size of nodes set is %d\n", nodes.size());
        // printf("after, i is %d\n", i);
        // printf("after, received_mapinfo is %s\n", received_mapinfo);
    }
    close(sockfd);
    return 0;
}
