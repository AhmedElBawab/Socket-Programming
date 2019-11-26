Programming Assignment 1

Introduction to Socket Programming in C/C++

Part 1: Multi-threaded Web Server
- To run the server you need to open the terminal the write:
./main “port number”
- The server starts by checking if the user has entered a port number or not. Then it starts to create its local address structure. After that it loops in the linked list tell it finds an available address info. Once the server finds it the creation of the socket starts then force the system to free the port and then bind itself for that port. The next step is to free the linked list as no need for it then the the server listens on this port and waits for any new connection.
- The server enters a while(1) loop to still alive and in it the server can accept new connections. Once a new connection is in the server parses its ip and print it then it creates a child process for it and pass it to a method to deal with it.
- handelClientConnectio takes 2 parameters. The first parameter is the socket and the second one is the dynamic timeout which is calculated on the form:
(100 - numberOfConnections) / 100) * DEFAULTTIMEOUT
where numberOfConnections is the number of child process connecting to the server.
DEFAULTTIMEOUT is the timeout for the system when no process connecting to it.
- The method starts with creating a request buffer to receive the requests in it and then it receives the requests and do the following:
Fetches the path of the file from the requests.
Check the type of the request and passed it to an appropriate method to handle it.
In case of post requests it fetches the size of the data from the request and pass it to the method which will handle the request
- The handleGetRequests starts by creating a sendBuffer to send the data in it then it uses the path of the file to check if it is found and in case of it is found it reads the file and sends it to the client then returns to the handelClient method
- The handelPostRequests start by creating a recvBuffer to receive the data in it then it receives the data from the client and writes it and after that it sends ACK to the client to tell it that it is done.
- The ACK  was added to stop the client from sending another request while the server is still working in the last one.
- The time of the timeOut of each connection is calculated due to the number of the connections in the server so it is a linear relationship as the more connections it is the less faster the connection will be closed.

- The timeout of each process is done by a signal and alarm which calls a method to kill the process if no requests have been submitted.
- The server can handle any number of headers.
- Another way to handle the time out is to build a hash table for each process and with each action the process takes the table is updated and with thread that loops on the table to kill the process that isn’t moving.
- The server can handle html,txt files.
- Images handling code is done but not inserted in the server code yet.
- When using from a browser I have got the following :

Part 2: HTTP Web Client
- To run the client you need to run the terminal and write:
./main hostname portnumber
- The client starts with checking the number of the parameters if it is less than 2 or more than 3 and then it uses each parameter and in case of no port number the client works on 8080 port number as a default port number.
- The client then starts to construct the address data structure and check the status then it loops on the linked list to find the available one and once it is found the client creates the socket then it connects to the server on the given host name and port number.
- The client after connecting calls a methods with reads a file of requests like the following :
- The method works on extracting each request with each headers by finding 2 new lines.
- Once the request is fetched from the file the client works on parsing it to detect its type and pass it to the appropriate method.
- The get method receives the status from the server then it receives the data in case of the status in OK and it writes the data in a file.
- The post method works on getting the path file and checks if it is found the send it to the server and waits for the ACK  from the server.
- When the requests file is over the client closes the connection.

