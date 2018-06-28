/*
 * rpc_server.c
 *
 *  Created on: May 20, 2018
 *      Author: azhari
 */
#include <sys/types.h>   // Types used in sys/socket.h and netinet/in.h
#include <netinet/in.h>  // Internet domain address structures and functions
#include <sys/socket.h>  // Structures and functions used for socket API
#include <netdb.h>       // Used for domain/DNS hostname lookup
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAX_MSG_LEN 100 //maximum length of messages

main(){
	int sockfd;
	struct sockaddr_in sock_local, sock_remote;
	int port_number = 6000;
	struct hostent *hptr;
	char msg[MAX_MSG_LEN];

	//create udp socket
//	sleep(0.000001); //1us used to mark the trace
	if ((sockfd= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))<0){
		printf("Error creating socket!\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	//initialize local socket info to zero
	memset(&sock_local, 0, sizeof(struct sockaddr_in));
	sock_local.sin_family = AF_INET;
	//get some interface IP address on local host and convert to network order
	sock_local.sin_addr.s_addr = htonl(INADDR_ANY);
	sock_local.sin_port = htons(port_number);

	//now bind socket to address
//	sleep(0.000001); //10us used to mark the trace
	if (bind(sockfd, (struct sockaddr *) &sock_local, sizeof(struct sockaddr_in)) < 0){
		printf("Error binding socket!\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	int sock_len = sizeof(sock_remote);
//	sleep(0.000001); //10us used to mark the trace
	int rx_bytes = recvfrom(sockfd, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&sock_remote, &sock_len);

	if (rx_bytes == 0){
		printf("Error socket closed!\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	else if (rx_bytes == -1){
		printf("Error socket error !\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
//	sleep(0);
	printf("Server Received %d bytes from %s:%d\n%s\n",
			rx_bytes, inet_ntoa(sock_remote.sin_addr), ntohs(sock_remote.sin_port), msg);

	//sleep(0.0001); //wait for some time

	memset((char *)msg,0,MAX_MSG_LEN);
	sprintf(msg,"Got your message\n");

//	sleep(0.000001); //1us used to mark the trace
	if (sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&sock_remote, sizeof(sock_remote) ) <0){
		printf("Error sending message!\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

//	sleep(0.000001); //1us used to mark the trace
	close(sockfd);
}

