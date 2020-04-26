EE450
session 1
Haoran Zhang

------------------------------------------------------------------
2088-8017-38

------------------------------------------------------------------
### What I have done
In this assignment, I used c++ to do socket communication between client and four servers. 
Client send queries to aws(using TCP) about calcultating the shortest distance between source and destination in a map.
aws received queries from client and send mapid to serverA and B and C (using UDP)to do the checking and calcultation.

Work flow is below:

    Client sends queries to aws

    aws receives from client, starts communication with A B C respectively:

    if mapid exists in A, then A sends the map infomation back to aws, aws starts checking src and dest nodes in the map;
        if src and dest nodes exist in map, then aws sends these to C directly to calcultate shortest distance and delay.
        else aws sends error to client

    else if mapid does not exist in A, aws send mapid to B.

        if mapid exists in B, then B sends map infomation to aws, aws starts checking src and dest nodes in the map;
            if src and dest nodes exist in map, then aws send these to C directly to calcultate shortest distance and delay.
            else aws sends error to client

    else, mapid does not exist in both A and B, send error message to client

--------------------------------------------------------------------------
### Program structure
1. client.cpp: send query message to aws
    message format is <mapid> <src> <dest> <filesize>
    for example: ./client a 1 2 3
    
    Compile:
    just type 'make'

    when booting up servers and clients, we should use:
    ./serverA
    ./serverB
    ./serverC
    ./aws
    ./client a 1 2 3

2. aws.cpp: receive queries from client and make communication with server A B and C. Workflow is above aws also checks if src and dest nodes exist in map infomation (receive from A or B)


3. serverA.cpp: check if mapid from aws exists, 
                send mapinfo to aws if mapid exists, 
                else, send a flag to inform aws mapid does not exist.

4. serverB.cpp: Same function with serverA, but this program only works when serverA cannot find mapid.
                check if mapid from aws exists, 
                send mapinfo to aws if mapid exists, 
                else, send a flag to inform aws mapid does not exist.

5. serverC.cpp: receive mapinfo from aws and calcultate shortest distance and delay.

-------------------------------------------------------------------------
### Format of all messages

    Compile:
    just type 'make'

    Run:
    ./serverA
    ./serverB
    ./serverC
    ./aws
    ./client a 1 2 3



---------------------------------------------------------------------------
### Known limitations so far
    The program runs well on MacOS and ubuntu. But it does have some limitations.
    
    1.one of the limits is that in serverC, the transmission delay result is not always true.
    
    2.As display in PDF page 17,table 7, last row. I thought it was a whole message, so I did not spilit it into two conditions, but it also shows all infomation.I do not think it is a limitation, but a different way to display.
    
    3.The display result in client.cpp is not perfectly neat. I tried different ways to adjust, but there is still some exceptions while printing. But it does show all required information.
    
    4.This condition is more complicated. 'aws.cpp' sometimes has weird performance runing in ubuntu VM when the first time using 'make' to compile. For example, when it boots up first time, it may show 'invalid argument' when reciveing calculation from serverC. But this DOES NOT mean it cannot work. It will work when you turn it off and RE-BOOT it second or third time.(DO NOT need to re-compile).
    
    troubleshoot: when you see 'invalid arguments':
      1. use 'ctrl c'turn off aws, do not need to interrupt others. 
      2. Then turn ./aws on.
      3. use maybe './client a 1 2 3' as an example to re-transmit.
      4. if still does not work, try two or three times maybe or use different client input such as '.client b 1 2 3', './client Q 0 65 100', it will work for sure. 


---------------------------------------------------------------
### Reused Code

serverC.cpp:

    https://www.geeksforgeeks.org/dijkstras-shortest-path-algorithm-greedy-algo-7/
    https://blog.csdn.net/HEYIAMCOMING/article/details/76535677
    https://www.geeksforgeeks.org/printing-paths-dijkstras-shortest-path-algorithm/
    Beej’s Guide to Network Programming.pdf

serverA.cpp and serverB.cpp:

    Beej’s Guide to Network Programming.pdf
    http://www.cplusplus.com/reference/fstream/ifstream/ifstream/
    http://www.cplusplus.com/reference/algorithm/find/?kw=find
    https://blog.csdn.net/u010429424/article/details/75332700

aws.cpp:

    http://www.cplusplus.com/reference/cstring/strtok/?kw=strtok
    Beej’s Guide to Network Programming.pdf


client.cpp:

    Beej’s Guide to Network Programming.pdf
    
makefile:

    Beej's Guide
    https://www.cs.swarthmore.edu/%7Enewhall/unixhelp howto_makefiles.html
