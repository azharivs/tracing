/*
 * rpc_client.c
 *
 *  Created on: May 20, 2018
 *      Author: azhari
 *
 * When two processes are related through a resource such as a socket, a mutex, or a file descriptor
 * one would be blocked only when reading the resource and the resource is not yet ready due to the other
 * process.
 *
 * For example, in the case of a network socket as in this example code, the recvfrom will block
 * only when the sendto has not yet filled the buffer. However, if the packet is previously sent to
 * the buffer, then recvfrom would never block and there is no way for us to understand that these
 * two processes are dependent.
 *
 * Note that the file descriptors used by each process are numbered locally so we can't use them
 * to discover such dependencies.
 *
 * However, one solution is to record all reads and writes to various file descriptors made
 * locally by each process and only establish their dependency when one "happens" to block on
 * some file descriptor and then is waken up by the other upon write to another file descriptor.
 *
 * Let us assume P1 accesses FD1 and P2 accesses FD2 in a non-blocking way. However, once it
 * happens that P2 blocks upon read on FD2 but which is later waken up from P1 while P1 is
 * attempting a write on FD1. In this case we will relate P1 to P2 through <FD1|FD2> as a potential
 * critical execution path. The significance of this dependency will be the frequency of such waits.
 *
 * TODO: Investigate applicability for cases in Fig. 5(c,d) of Wait Analysis paper.
 * TODO: Try with client and server on different machines (not using loopback)
 * TODO: How generally applicable is this technique? Check for different scenarios IPI and Network and ?
 * TODO: Does the same technique apply to threads waiting on mutexes?
 *
 * TODO: Try to make the blocking happen due to scheduler preemption
 * In this case the sched_waking and sched_wakeup happen within the softirq#1 context (Timer interrupt)
 * So this is an indication that the scheduler is issuing the wakeup of the second process. In this case
 * softirq#1 is active while scheduler IRQ (softirq#7) is also raised so this means that the timer is
 * due to scheduler and hence we have "preemption blocking".
 * Sometime preemption happens as a result of some IRQ line being raised such as for SCSI devices
 * In this case there is no sched_waking/wakeup involved and we only have sortirqs/tasklets running. This
 * will be "preemption due to interrupts".
 */
#include <sys/types.h>   // Types used in sys/socket.h and netinet/in.h
#include <netinet/in.h>  // Internet domain address structures and functions
#include <sys/socket.h>  // Structures and functions used for socket API
#include <netdb.h>       // Used for domain/DNS hostname lookup
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
//#include <math.h>

#define MAX_MSG_LEN 100 //maximum length of messages
#define MAX_ITER 20000

main(){
	int sockfd;
	struct sockaddr_in sock_remote;
	struct hostent *hptr;
	int port_number = 6000;
	char *remote_host = "localhost"; //loopback interface
	char msg[MAX_MSG_LEN] = "Hello server!\n";

	//load remote host info
	//	sleep(0.000001); //1us used to mark the trace
	if ((hptr=gethostbyname(remote_host)) == NULL){
		printf("Remote host %s not resolved!!\n",remote_host);
		exit(EXIT_FAILURE);
	}
	//create udp socket
	sleep(0.000001); //1us used to mark the trace
	if ((sockfd= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))<0){
		printf("Error creating socket!\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	//initialize socket info to zero
	memset((char *)&sock_remote, 0, sizeof(sock_remote));
	sock_remote.sin_family = AF_INET;
	memcpy((char *)&sock_remote.sin_addr, hptr->h_addr, hptr->h_length);
	/*if (inet_aton("127.0.0.1" , &sock_remote.sin_addr) == 0){
    	printf("Error\n");
    	exit(EXIT_FAILURE);
    }*/
	sock_remote.sin_port = htons(port_number);

	//send message
	//	sleep(0.000001); //1us used to mark the trace
	if (sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&sock_remote, sizeof(sock_remote) ) <0){
		printf("Error sending message!\n");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	/*
	 * softirq #3 (Network RX) is raised at the of the sendto system call on the client process.
	 * This happens when the destination is the loopback so the execution flow immediately continues
	 * to the receive section through running the softirq. It is not supposed to happen in non-loopback case
	 * This is the flow of events as a result of sendto in the loopback case:
	 * net_dev_queue: Queue a buffer for transmission to a network device
	 * net_if_rx:
	 * irq_softirq_raise (#3):
	 * net_dev_xmit: Transmit buffer
	 * irq_softirq_entry
	 * net_if_receive_skb:
	 * sched_waking: scheduler tracepoint called when the server process is to be woken up by the client thread
	 * sched_wakeup: actual wakeup
	 * napi_poll: poll any remaining packets while interrupts are disabled (performance improvement)
	 * irq_softirq_exit
	 */

	int sock_len = sizeof(sock_remote);
	memset((char *)msg, 0, MAX_MSG_LEN);
	/* insert delay so that server has already sent back something by the time recvfrom is called
	 * It is expected that we do not block in this state so I am going to study the trace in this case
	 */
	//sleep(1);
	/* Now instead of inserting sleep, do some intensive computation so that preemptions happens
	 *
	 */
	printf("start busy loop ...\n");
	long i,j;
	double res = 0;
	for (i=MAX_ITER;i>0;i--){
		for (j=MAX_ITER;j>0;j--)
			res = res + i*j;
	}
	printf("end busy loop\n");

	int rx_bytes = recvfrom(sockfd, msg, MAX_MSG_LEN, 0, (struct sockaddr *)&sock_remote, &sock_len);
	//	sleep(0.000001); //1us used to mark the trace
	printf("Client Received %d bytes from %s:%d\n%s\n",
			rx_bytes, inet_ntoa(sock_remote.sin_addr), ntohs(sock_remote.sin_port), msg);

	/*
	 * after calling recvfrom the process is blocked.
	 * This is indicated by the sched_switch scheduler tracepoint which
	 * implies that the flow of execution is blocked. However we know it is
	 * not preemption because there is no scheduler interrupt involved. (need to confirm this)
	 */
	//	sleep(0.000001); //1us used to mark the trace
	close(sockfd);
}



