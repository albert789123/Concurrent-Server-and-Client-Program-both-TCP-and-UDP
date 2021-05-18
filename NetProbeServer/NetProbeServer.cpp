#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <cstdint>
#include <inttypes.h>
#include "threadpool.h"

using namespace std;

#define TRUE   1
#define FALSE  0

void print_manual();
int select_mode(int argc, char *argv[]);
void TCP_SEND_thread(void* arg);
void TCP_RECV_thread(void* arg);
void TCP_RESPONSE_thread(void* arg);
void UDP_SEND_thread(void* arg);
void UDP_RECV_thread(void* arg);
void UDP_RESPONSE_thread(void* arg);


struct Socket_Config { //stuct for list of socket in select_mode
	int pktsize;
	int pktnum;
	int pktrate;
};

struct TCP_Thread_Argument { //stuct for passing more than one argument to the thread
	int socket;
	char* control;
};

struct UDP_Thread_Argument { //stuct for passing more than one argument to the thread
	int socket;
	struct sockaddr_in cliaddr;
	char* control;
};

int main(int argc, char *argv[]) {
		print_manual();

		//for (int i = 0; i < argc; i++)
			//printf("argv [%d] is %s\n", i, argv[i]);

		for (int i = 1; i < argc; i++) { //checking whether it is select_mode or threadpool_mode
			if ((strcmp(argv[i], "-servermodel") == 0)) {
				if ((strcmp(argv[i + 1], "select") == 0))
						select_mode(argc, argv);
			}
			if (argv[i] == NULL)
				break;
		}

		//the start of threadpool_mode
		//read the configurations
		int stat = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-stat") == 0)) {
				stat = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int sbufsize = 65536;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-sbufsize") == 0)) {
				sbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int rbufsize = 65536;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rbufsize") == 0)) {
				rbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		char* lhost = (char*)"IN_ADDR_ANY";
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-lhost") == 0)) {
				lhost = argv[i + 1];
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int lport = 4180;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-lport") == 0)) {
				lport = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int psize = 8;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-poolsize") == 0)) {
				lport = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}

		//print the configurations
		printf("NetProbeServer Configurations: \n");
		printf("-sbufsize = %d\n", sbufsize);
		printf("-rbufsize = %d\n", rbufsize);
		printf("-lhost = %s\n", lhost);
		printf("-lport = %d\n", lport);
		printf("-servermodel = threadpool\n");
		printf("-psize = %d\n\n", psize);

		//create sockets and fill the information
		int listenfd, connfd, udpfd, udpconfd;
		struct sockaddr_in servaddr, cliaddr, udpconaddr;
		socklen_t len;

    servaddr.sin_family = AF_INET;
		if (strcmp(lhost, "IN_ADDR_ANY") == 0)
			servaddr.sin_addr.s_addr = INADDR_ANY;
		else
			servaddr.sin_addr.s_addr = inet_addr(lhost);
    servaddr.sin_port = htons(lport);

		udpconaddr.sin_family = AF_INET;
		udpconaddr.sin_addr.s_addr = INADDR_ANY;
		udpconaddr.sin_port = htons(41800);

		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		udpfd = socket(AF_INET, SOCK_DGRAM, 0);
		udpconfd = socket(AF_INET, SOCK_DGRAM, 0);

		//bind the sockets
		bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
		bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
		bind(udpconfd, (struct sockaddr*)&udpconaddr, sizeof(udpconaddr));
		printf("Binding local socket to port number %d with late binding ... successful.\n", lport);

		listen(listenfd, 10);
		printf("Listening to incoming connection request ...\n");

		//set the socket buffers
		if (setsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, (char*)(&sbufsize), sizeof(sbufsize)) < 0) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}
		if (setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (char*)(&rbufsize), sizeof(rbufsize)) < 0) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}

		if (setsockopt(udpfd, SOL_SOCKET, SO_SNDBUF, (char*)(&sbufsize), sizeof(sbufsize)) < 0) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}
		if (setsockopt(udpfd, SOL_SOCKET, SO_RCVBUF, (char*)(&rbufsize), sizeof(rbufsize)) < 0) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}

		//create thread pool using threadpool.h
		threadpool tp;
		tp = create_threadpool(psize);

		char TCP_greeting[1000];
		char UDP_greeting[1000];
		int used_thread = 0;
		int TCP_client_num = 0;
		int UDP_client_num = 0;

		struct TCP_Thread_Argument TCP_Thread_arg;
		struct UDP_Thread_Argument UDP_Thread_arg;
		fd_set fdReadSet;
		int max_fd = 0;
		int mode = -1;
		int proto = -1;
		FD_ZERO(&fdReadSet);
		struct timeval timeout = {0, 500};
		if (stat == 0)
			timeout = {0, 500};
		else
			timeout = {0, stat};
		long double elapsed = 0;
		clock_t start = clock();
		clock_t now = clock();
		clock_t round_start = clock();

		//loop of the main function
		while (1) {
			FD_SET(listenfd, &fdReadSet);
			FD_SET(udpconfd, &fdReadSet);
			max_fd = max(listenfd, udpconfd);
			int ret;
			if ((ret = select(max_fd + 1, &fdReadSet, NULL, NULL, &timeout)) < 0){ //used non blocking select() to read incoming connection
				printf("select() faild with error\n");
			}

			if (FD_ISSET(listenfd, &fdReadSet)){ //accept TCP connection by accept() and recv() the control message
				len = sizeof(cliaddr);
				connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
				int recv_len = sizeof(TCP_greeting);
				int numread = recv(connfd, TCP_greeting, recv_len, 0);
				mode = TCP_greeting[0] - '0'; //decode the control message to get the mode and protocol of client
				proto = TCP_greeting[1] - '0';
			}
			if ((mode == 0) && (proto == 1)) {
				printf("Connected to %s port %d SEND, TCP\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
				TCP_Thread_arg.socket = connfd;
				TCP_Thread_arg.control = TCP_greeting;
				dispatch(tp, TCP_RECV_thread, &TCP_Thread_arg); //add task to the thread pool using threadpool.h
				used_thread++;
				TCP_client_num++;
				mode = -1;
				proto = -1;
			}
			else if ((mode == 1) && (proto == 1)) {
				printf("Connected to %s port %d RECV, TCP\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
				TCP_Thread_arg.socket = connfd;
				TCP_Thread_arg.control = TCP_greeting;
				dispatch(tp, TCP_SEND_thread, &TCP_Thread_arg); //add task to the thread pool using threadpool.h
				used_thread++;
				TCP_client_num++;
				mode = -1;
				proto = -1;
			}
			else if ((mode == 2) && (proto == 1)) {
				printf("Connected to %s port %d RESPONSE, TCP\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
				TCP_Thread_arg.socket = connfd;
				TCP_Thread_arg.control = TCP_greeting;
				dispatch(tp, TCP_RESPONSE_thread, &TCP_Thread_arg); //add task to the thread pool using threadpool.h
				used_thread++;
				TCP_client_num++;
				mode = -1;
				proto = -1;
			}

			if (FD_ISSET(udpconfd, &fdReadSet)){ //accept UDP connection by recvfrom() the control message
				len = sizeof(udpconaddr);
				int numread = recvfrom(udpconfd, UDP_greeting, 999, 0, (struct sockaddr*)&udpconaddr, &len);
				mode = UDP_greeting[0] - '0'; //decode the control message to get the mode and protocol of client
				proto = UDP_greeting[1] - '0';
			}
			if ((mode == 0) && (proto == 0)) {
				printf("Connected to %s port %d SEND, UDP\n", inet_ntoa(udpconaddr.sin_addr), udpconaddr.sin_port);
				UDP_Thread_arg.socket = udpfd;
				UDP_Thread_arg.control = UDP_greeting;
				UDP_Thread_arg.cliaddr = udpconaddr;
				dispatch(tp, UDP_RECV_thread, &UDP_Thread_arg); //add task to the thread pool using threadpool.h
				used_thread++;
				UDP_client_num++;
				mode = -1;
				proto = -1;
			}
			else if ((mode == 1) && (proto == 0)) {
				printf("Connected to %s port %d RECV, UDP\n", inet_ntoa(udpconaddr.sin_addr), udpconaddr.sin_port);
				UDP_Thread_arg.socket = udpfd;
				UDP_Thread_arg.control = UDP_greeting;
				UDP_Thread_arg.cliaddr = udpconaddr;
				dispatch(tp, UDP_SEND_thread, &UDP_Thread_arg); //add task to the thread pool using threadpool.h
				used_thread++;
				UDP_client_num++;
				mode = -1;
				proto = -1;
			}
			else if ((mode == 2) && (proto == 0)) {
				printf("Connected to %s port %d RESPONSE, UDP\n", inet_ntoa(udpconaddr.sin_addr), udpconaddr.sin_port);
				UDP_Thread_arg.socket = udpfd;
				UDP_Thread_arg.control = UDP_greeting;
				UDP_Thread_arg.cliaddr = udpconaddr;
				dispatch(tp, UDP_RESPONSE_thread, &UDP_Thread_arg);
				used_thread++;
				UDP_client_num++;
				mode = -1;
				proto = -1;
			}

			now = clock();
			elapsed = ((long double)now - start) / (double)CLOCKS_PER_SEC;
			if ((stat != 0) && ((((long double)now - round_start) / (double)CLOCKS_PER_SEC * 1000) >= stat)){ //print the statistics
				printf("Elapsed [%.1Lfs] ThreadPool [%d|%d] TCP Clients [%d] UDP Clients [%d]\n", elapsed, psize, used_thread, TCP_client_num, UDP_client_num);
				round_start = clock();
			}

		}

		return 0;

}

//implentation of thread handler for TCP
void TCP_SEND_thread(void* arg) {
	struct TCP_Thread_Argument *Thread_arg = (struct TCP_Thread_Argument *)arg;
	int sock = Thread_arg->socket;
	int pktnum = 0;
	int pktsize = 0;
	int pktrate = 0;

	char temp[11]; //decoding the control message
	memcpy(temp, &Thread_arg->control[2], 10);
	temp[10] = '\0';
	pktnum = atoi(temp);
	memcpy(temp, &Thread_arg->control[12], 10);
	temp[10] = '\0';
	pktsize = atoi(temp);
	memcpy(temp, &Thread_arg->control[22], 10);
	temp[10] = '\0';
	pktrate = atoi(temp);

	int pkts = 0;
	int rate_counter = 0;
	clock_t rate_start = clock();
	while (1) {
		char buf[pktsize];
		memset(buf, 0, pktsize);
		if ((rate_counter < pktrate)||(pktrate == 0)) {
			int numwrite = send(sock, buf, sizeof(buf), 0);
			pkts++;
			rate_counter+= pktsize;
			//printf("sent\n");
		}
		else {
			if ((clock() - rate_start) / CLOCKS_PER_SEC >= 1) {
				rate_start = clock();
				rate_counter = 0;
			}
		}
		if ((pktnum > 0) && (pkts >= pktnum))
			break;
	}
	close(sock);
}
void TCP_RECV_thread(void* arg) {
	struct TCP_Thread_Argument *Thread_arg = (struct TCP_Thread_Argument *)arg;
	int sock = Thread_arg->socket;

	int pktnum = 0;
	int pktsize = 0;
	int pktrate = 0;

	char temp[11]; //decoding the control message
	memcpy(temp, &Thread_arg->control[2], 10);
	temp[10] = '\0';
	pktnum = atoi(temp);
	memcpy(temp, &Thread_arg->control[12], 10);
	temp[10] = '\0';
	pktsize = atoi(temp);
	memcpy(temp, &Thread_arg->control[22], 10);
	temp[10] = '\0';
	pktrate = atoi(temp);
	//printf("%d %d %d\n", pktnum, pktsize, pktrate);

	int pkts = 0;
	while (1) {
		char buf[pktsize];
		int len = sizeof(buf);
		int numread = recv(sock, buf, len, 0);
		pkts++;
		//printf("received\n");
		if ((pktnum > 0) && (pkts >= pktnum))
			break;
	}
	close(sock);
}
void TCP_RESPONSE_thread(void* arg) {
	struct TCP_Thread_Argument *Thread_arg = (struct TCP_Thread_Argument *)arg;
	int sock = Thread_arg->socket;

	int pktnum = 0;
	int pktsize = 0;
	int pktrate = 0;

	char temp[11]; //decoding the control message
	memcpy(temp, &Thread_arg->control[2], 10);
	temp[10] = '\0';
	pktnum = atoi(temp);
	memcpy(temp, &Thread_arg->control[12], 10);
	temp[10] = '\0';
	pktsize = atoi(temp);
	memcpy(temp, &Thread_arg->control[22], 10);
	temp[10] = '\0';
	pktrate = atoi(temp);

	int pkts = 0;
	int rate_counter = 0;
	clock_t rate_start = clock();
	while (1) {
		char buf[pktsize];
		int len = sizeof(buf);
		int numread = recv(sock, buf, len, 0);
		pkts++;
		char reply[pktsize];
		memset(reply, 0, pktsize);
		if ((rate_counter < pktrate)||(pktrate == 0)) {
			int numwrite = send(sock, reply, sizeof(buf), 0);
			rate_counter+= pktsize;
			//printf("sent\n");
		}
		else {
			if ((clock() - rate_start) / CLOCKS_PER_SEC >= 1) {
				rate_start = clock();
				rate_counter = 0;
			}
		}
		if ((pktnum > 0) && (pkts >= pktnum))
			break;
	}
	close(sock);
}

//implentation of thread handler for UDP
void UDP_SEND_thread(void* arg) {
	struct UDP_Thread_Argument *Thread_arg = (struct UDP_Thread_Argument *)arg;
	int sock = Thread_arg->socket;
	struct sockaddr_in cliaddr = Thread_arg->cliaddr;

	int pktnum = 0;
	int pktsize = 0;
	int pktrate = 0;

	char temp[11]; //decoding the control message
	memcpy(temp, &Thread_arg->control[2], 10);
	temp[10] = '\0';
	pktnum = atoi(temp);
	memcpy(temp, &Thread_arg->control[12], 10);
	temp[10] = '\0';
	pktsize = atoi(temp);
	memcpy(temp, &Thread_arg->control[22], 10);
	temp[10] = '\0';
	pktrate = atoi(temp);

	int pkts = 0;
	int rate_counter = 0;
	clock_t rate_start = clock();
	while (1) {
		char buf[pktsize];
		memset(buf, 0, pktsize);
		if ((rate_counter < pktrate)||(pktrate == 0)) {
			pkts++;
			*((unsigned long*)buf) = htonl(pkts);
			int slen = sizeof(cliaddr);
			int numwrite = sendto(sock, buf, sizeof(buf), 0, (struct sockaddr*)&cliaddr, slen);
			rate_counter+= pktsize;
			//printf("sent %s %d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
		}
		else {
			if ((clock() - rate_start) / CLOCKS_PER_SEC >= 1) {
				rate_start = clock();
				rate_counter = 0;
			}
		}
		if ((pktnum > 0) && (pkts >= pktnum))
			break;
	}
	close(sock);
}
void UDP_RECV_thread(void* arg) {
	struct UDP_Thread_Argument *Thread_arg = (struct UDP_Thread_Argument *)arg;
	int sock = Thread_arg->socket;
	struct sockaddr_in cliaddr = Thread_arg->cliaddr;

	int pktnum = 0;
	int pktsize = 0;
	int pktrate = 0;

	char temp[11]; //decoding the control message
	memcpy(temp, &Thread_arg->control[2], 10);
	temp[10] = '\0';
	pktnum = atoi(temp);
	memcpy(temp, &Thread_arg->control[12], 10);
	temp[10] = '\0';
	pktsize = atoi(temp);
	memcpy(temp, &Thread_arg->control[22], 10);
	temp[10] = '\0';
	pktrate = atoi(temp);

	int pkts = 0;
	while (1) {
		char buf[pktsize];
		socklen_t len = sizeof(cliaddr);
		int numread = recvfrom(sock, buf, pktsize, 0, (struct sockaddr*)&cliaddr, &len);
		pkts++;
		//printf("received\n");
		if ((pktnum > 0) && (pkts >= pktnum))
			break;
	}
	close(sock);
}
void UDP_RESPONSE_thread(void* arg) {
	struct UDP_Thread_Argument *Thread_arg = (struct UDP_Thread_Argument *)arg;
	int sock = Thread_arg->socket;
	struct sockaddr_in cliaddr = Thread_arg->cliaddr;

	int pktnum = 0;
	int pktsize = 0;
	int pktrate = 0;

	char temp[11]; //decoding the control message
	memcpy(temp, &Thread_arg->control[2], 10);
	temp[10] = '\0';
	pktnum = atoi(temp);
	memcpy(temp, &Thread_arg->control[12], 10);
	temp[10] = '\0';
	pktsize = atoi(temp);
	memcpy(temp, &Thread_arg->control[22], 10);
	temp[10] = '\0';
	pktrate = atoi(temp);

	int pkts = 0;
	int rate_counter = 0;
	clock_t rate_start = clock();
	while (1) {
		char buf[pktsize];
		socklen_t len = sizeof(cliaddr);
		int numread = recvfrom(sock, buf, pktsize, 0, (struct sockaddr*)&cliaddr, &len);
		pkts++;
		char reply[pktsize];
		memset(reply, 0, pktsize);
		if ((rate_counter < pktrate)||(pktrate == 0)) {
			int slen = sizeof(cliaddr);
			int numwrite = sendto(sock, reply, sizeof(buf), 0, (struct sockaddr*)&cliaddr, slen);
			rate_counter+= pktsize;
			//printf("sent %s %d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
		}
		else {
			if ((clock() - rate_start) / CLOCKS_PER_SEC >= 1) {
				rate_start = clock();
				rate_counter = 0;
			}
		}
		if ((pktnum > 0) && (pkts >= pktnum))
			break;
	}
	close(sock);
}

int select_mode(int argc, char *argv[]) { //the select mode implemented at project 2
		int sbufsize = 65536;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-sbufsize") == 0)) {
				sbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int rbufsize = 65536;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rbufsize") == 0)) {
				rbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		char* lhost = (char*)"IN_ADDR_ANY";
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-lhost") == 0)) {
				lhost = argv[i + 1];
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int lport = 4180;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-lport") == 0)) {
				lport = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}

		printf("NetProbeServer Configurations: \n");
		printf("-sbufsize = %d\n", sbufsize);
		printf("-rbufsize = %d\n", rbufsize);
		printf("-lhost = %s\n", lhost);
		printf("-lport = %d\n", lport);
		printf("-servermodel = select\n\n");

		int listenfd, connfd, udpfd, udpconfd;
    struct sockaddr_in servaddr, cliaddr, udpconaddr;
		socklen_t len;

    servaddr.sin_family = AF_INET;
		if (strcmp(lhost, "IN_ADDR_ANY") == 0)
			servaddr.sin_addr.s_addr = INADDR_ANY;
		else
			servaddr.sin_addr.s_addr = inet_addr(lhost);
    servaddr.sin_port = htons(lport);

		udpconaddr.sin_family = AF_INET;
		udpconaddr.sin_addr.s_addr = INADDR_ANY;
		udpconaddr.sin_port = htons(41800);

		listenfd = socket(AF_INET, SOCK_STREAM, 0);
		udpfd = socket(AF_INET, SOCK_DGRAM, 0);
		udpconfd = socket(AF_INET, SOCK_DGRAM, 0);

		bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
		bind(udpfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
		bind(udpconfd, (struct sockaddr*)&udpconaddr, sizeof(udpconaddr));
		printf("Binding local socket to port number %d with late binding ... successful.\n", lport);

		listen(listenfd, 5);
		printf("Listening to incoming connection request ...\n");

		const int maxSockets = 10;

		int TCP_RECV_Socket[maxSockets];
		struct Socket_Config TCP_RECV_Socket_Config[maxSockets];
		int TCP_SEND_Socket[maxSockets];
		struct Socket_Config TCP_SEND_Socket_Config[maxSockets];

		int UDP_RECV_Socket[maxSockets];
		struct sockaddr_in UDP_RECV_ADDR[maxSockets];
		int UDP_RECV_PORT[maxSockets];
		struct Socket_Config UDP_RECV_Socket_Config[maxSockets];
		int UDP_SEND_Socket[maxSockets];
		struct sockaddr_in UDP_SEND_ADDR[maxSockets];
		int UDP_SEND_PORT[maxSockets];
		int UDP_SEND_Packet_Num[maxSockets];
		struct Socket_Config UDP_SEND_Socket_Config[maxSockets];

		bool TCP_RECV_SocketValid[maxSockets];
		bool UDP_RECV_SocketValid[maxSockets];
		bool TCP_SEND_SocketValid[maxSockets];
		bool UDP_SEND_SocketValid[maxSockets];

		int TCP_Num_Active_RECV_Socket = 1;
		int UDP_Num_Active_RECV_Socket = 1;
		int TCP_Num_Active_SEND_Socket = 1;
		int UDP_Num_Active_SEND_Socket = 1;

		for (int i = 0; i < maxSockets; i++){
			TCP_RECV_SocketValid[i] = false;
			UDP_RECV_SocketValid[i] = false;
			UDP_RECV_PORT[i] = 0;

			TCP_SEND_SocketValid[i] = false;
			UDP_SEND_SocketValid[i] = false;
			UDP_SEND_PORT[i] = 0;
			UDP_SEND_Packet_Num[i] = 0;
		}

		TCP_RECV_Socket[0] = listenfd;
		TCP_RECV_SocketValid[0] = true;
		UDP_RECV_Socket[0] = udpfd;
		UDP_RECV_SocketValid[0] = true;

		TCP_SEND_Socket[0] = listenfd;
		TCP_SEND_SocketValid[0] = true;
		UDP_SEND_Socket[0] = udpfd;
		UDP_SEND_SocketValid[0] = true;

		fd_set fdReadSet;
		fd_set fdWriteSet;

		char greeting[1000];
		int mode = 0;
		int proto = 0;
		int max_fd = 0;

		if (setsockopt(listenfd, SOL_SOCKET, SO_SNDBUF, (char*)(&sbufsize), sizeof(sbufsize)) < 0) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}
		if (setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, (char*)(&rbufsize), sizeof(rbufsize)) < 0) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}

		if (setsockopt(udpfd, SOL_SOCKET, SO_SNDBUF, (char*)(&sbufsize), sizeof(sbufsize)) < 0) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}
		if (setsockopt(udpfd, SOL_SOCKET, SO_RCVBUF, (char*)(&rbufsize), sizeof(rbufsize)) < 0) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}


		while (1){

			struct sockaddr_in current_UDP_address;

			int TCP_Top_RECV_Socket = 0;
			int UDP_Top_RECV_Socket = 0;
			int TCP_Top_SEND_Socket = 0;
			int UDP_Top_SEND_Socket = 0;
			FD_ZERO(&fdReadSet);
			FD_ZERO(&fdWriteSet);
			for (int i = 0; i < maxSockets; i++){

				if(TCP_RECV_SocketValid[i] == true){
					FD_SET(TCP_RECV_Socket[i], &fdReadSet);
					if (TCP_RECV_Socket[i] > TCP_Top_RECV_Socket)
						TCP_Top_RECV_Socket = TCP_RECV_Socket[i];
				}

				if(TCP_SEND_SocketValid[i] == true){
					FD_SET(TCP_SEND_Socket[i], &fdWriteSet);
					if (TCP_SEND_Socket[i] > TCP_Top_SEND_Socket)
						TCP_Top_SEND_Socket = TCP_SEND_Socket[i];
				}


			}
			FD_SET(udpconfd, &fdReadSet);
			max_fd = max(TCP_Top_RECV_Socket, TCP_Top_SEND_Socket);
			max_fd = max(max_fd, udpconfd);
			//printf("XD %d %d %d\n", listenfd, TCP_RECV_Socket[0], TCP_Top_RECV_Socket);
			//printf("XD %d %d %d\n", udpfd, UDP_RECV_Socket[0], UDP_Top_RECV_Socket);
			int ret;
			//printf("%d\n", 0);
			if ((ret = select(max_fd + 1, &fdReadSet, &fdWriteSet, NULL, NULL)) < 0){
				printf("select() faild with error\n");
				//perror("test");
				//exit(EXIT_FAILURE);
			}

			for (int i = 0; i < maxSockets; i++){
				if (!TCP_RECV_SocketValid[i])
					continue;
				if (FD_ISSET(TCP_RECV_Socket[i], &fdReadSet)){
					if (i == 0){
						len = sizeof(cliaddr);
						connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
						int recv_len = sizeof(greeting);
						int numread = recv(connfd, greeting, recv_len, 0);
						mode = greeting[0] - '0';
						proto = greeting[1] - '0';
						//printf("addr %s port %d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
					}
					bool RECV_exist = false;
					for (int j = 1; j < TCP_Num_Active_RECV_Socket; j++){
						if (TCP_RECV_Socket[j] == connfd){
							RECV_exist = true;
							break;
						}
					}
					bool SEND_exist = false;
					for (int j = 1; j < TCP_Num_Active_SEND_Socket; j++){
						if (TCP_SEND_Socket[j] == connfd){
							SEND_exist = true;
							break;
						}
					}
					for (int j = 1; j < maxSockets; j++){
						if((TCP_RECV_SocketValid[j] == false)&&(RECV_exist == false)&&(mode == 0)&&(proto == 1)){
							TCP_RECV_SocketValid[j] = true;
							TCP_RECV_Socket[j] = connfd;
							TCP_Num_Active_RECV_Socket++;

							char temp[11];
							memcpy(temp, &greeting[2], 10);
							temp[10] = '\0';
							TCP_RECV_Socket_Config[i].pktnum = atoi(temp);
							memcpy(temp, &greeting[12], 10);
							temp[10] = '\0';
							TCP_RECV_Socket_Config[i].pktsize = atoi(temp);
							memcpy(temp, &greeting[22], 10);
							temp[10] = '\0';
							TCP_RECV_Socket_Config[i].pktrate = atoi(temp) * 8;
							printf("Connected to %s port %d SEND, TCP, %d bps\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port, TCP_RECV_Socket_Config[i].pktrate);

							if (TCP_Num_Active_RECV_Socket == maxSockets)
								TCP_RECV_SocketValid[0] = false;
							break;
						}

						else if((TCP_RECV_SocketValid[j] == false)&&(SEND_exist == false)&&(mode == 1)&&(proto == 1)){
							TCP_SEND_SocketValid[j] = true;
							TCP_SEND_Socket[j] = connfd;
							TCP_Num_Active_SEND_Socket++;

							char temp[11];
							memcpy(temp, &greeting[2], 10);
							temp[10] = '\0';
							TCP_SEND_Socket_Config[i].pktnum = atoi(temp);
							memcpy(temp, &greeting[12], 10);
							temp[10] = '\0';
							TCP_SEND_Socket_Config[i].pktsize = atoi(temp);
							memcpy(temp, &greeting[22], 10);
							temp[10] = '\0';
							TCP_SEND_Socket_Config[i].pktrate = atoi(temp) * 8;
							printf("Connected to %s port %d RECV, TCP, %d bps\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port, TCP_SEND_Socket_Config[i].pktrate);

							if (TCP_Num_Active_SEND_Socket == maxSockets)
								TCP_SEND_SocketValid[0] = false;
							break;
						}

						else{
							if (i != 0){
								char buf[1000];
								int len = sizeof(buf);
								//printf("receving from i %d, TCP_RECV_Socket[%d] %d\n", i, i, TCP_RECV_Socket[i]);
								int numread = recv(TCP_RECV_Socket[i], buf, len, 0);
								if (numread < 0){
									printf("recv() failed\n");
									exit(EXIT_FAILURE);
								}
								else if (numread == 0){
									TCP_RECV_SocketValid[i] = false;
									TCP_Num_Active_RECV_Socket--;
									if (TCP_Num_Active_RECV_Socket == (maxSockets - 1))
										TCP_RECV_SocketValid[0] = true;
									close(TCP_RECV_Socket[i]);
								}

							}

						}
						if (ret-- == 0)
							break;

					}
				}

			}

			if (FD_ISSET(udpconfd, &fdReadSet)){
				len = sizeof(udpconaddr);
				recvfrom(udpconfd, greeting, 999, 0, (struct sockaddr*)&udpconaddr, &len);
				mode = greeting[0] - '0';
				proto = greeting[1] - '0';
				current_UDP_address = udpconaddr;
				//printf("addr %s port %d SEND, UDP\n", inet_ntoa(udpconaddr.sin_addr), udpconaddr.sin_port);
				bool RECV_exist = false;
				for (int j = 1; j < UDP_Num_Active_RECV_Socket; j++){
					if (UDP_RECV_PORT[j] == udpconaddr.sin_port){
						RECV_exist = true;
						break;
					}
				}
				bool SEND_exist = false;
				for (int j = 1; j < UDP_Num_Active_SEND_Socket; j++){
					if (UDP_SEND_PORT[j] == udpconaddr.sin_port){
						SEND_exist = true;
						break;
					}
				}

				for (int j = 1; j < maxSockets; j++){
					if((UDP_RECV_SocketValid[j] == false)&&(RECV_exist == false)&&(mode == 0)&&(proto == 0)){
						UDP_RECV_SocketValid[j] = true;
						UDP_RECV_Socket[j] = udpfd;
						UDP_RECV_ADDR[j] = current_UDP_address;
						UDP_RECV_PORT[j] = current_UDP_address.sin_port;
						UDP_Num_Active_RECV_Socket++;

						char temp[11];
						memcpy(temp, &greeting[2], 10);
						temp[10] = '\0';
						UDP_RECV_Socket_Config[j].pktnum = atoi(temp);
						memcpy(temp, &greeting[12], 10);
						temp[10] = '\0';
						UDP_RECV_Socket_Config[j].pktsize = atoi(temp);
						memcpy(temp, &greeting[22], 10);
						temp[10] = '\0';
						UDP_RECV_Socket_Config[j].pktrate = atoi(temp) * 8;
						printf("Connected to %s port %d SEND, UDP, %d bps\n", inet_ntoa(udpconaddr.sin_addr), udpconaddr.sin_port, UDP_RECV_Socket_Config[j].pktrate);

						if (UDP_Num_Active_RECV_Socket == maxSockets)
							UDP_RECV_SocketValid[0] = false;
						break;
					}
				}

				for (int j = 1; j < maxSockets; j++){
					if((UDP_SEND_SocketValid[j] == false)&&(SEND_exist == false)&&(mode == 1)&&(proto == 0)){
						UDP_SEND_SocketValid[j] = true;
						UDP_SEND_Socket[j] = udpfd;
						UDP_SEND_ADDR[j] = current_UDP_address;
						UDP_SEND_PORT[j] = current_UDP_address.sin_port;
						UDP_Num_Active_SEND_Socket++;

						char temp[11];
						memcpy(temp, &greeting[2], 10);
						temp[10] = '\0';
						UDP_SEND_Socket_Config[j].pktnum = atoi(temp);
						memcpy(temp, &greeting[12], 10);
						temp[10] = '\0';
						UDP_SEND_Socket_Config[j].pktsize = atoi(temp);
						memcpy(temp, &greeting[22], 10);
						temp[10] = '\0';
						UDP_SEND_Socket_Config[j].pktrate = atoi(temp) * 8;
						printf("Connected to %s port %d RECV, UDP, %d bps\n", inet_ntoa(udpconaddr.sin_addr), udpconaddr.sin_port, UDP_SEND_Socket_Config[j].pktrate);

						if (UDP_Num_Active_SEND_Socket == maxSockets)
							UDP_SEND_SocketValid[0] = false;
						break;
					}
				}

			}

			for (int i = 1; i < maxSockets; i++){
				if (!UDP_RECV_SocketValid[i])
					continue;
				if (FD_ISSET(udpfd, &fdReadSet)){
					for (int j = 1; j < maxSockets; j++){
							if (i != 0){
								char buf[1000];
								len = sizeof(UDP_RECV_ADDR[i]);
								//printf("receving from i %d, UDP_RECV_Socket[%d] %d\n", i, i, UDP_RECV_Socket[i]);
								int numread = recvfrom(udpfd, buf, 999, 0, (struct sockaddr*)&UDP_RECV_ADDR[i], &len);
								if (numread < 0){
									printf("recvfrom() failed\n");
									exit(EXIT_FAILURE);
								}
								else if (numread == 0){
									UDP_RECV_SocketValid[i] = false;
									UDP_Num_Active_RECV_Socket--;
									if (UDP_Num_Active_RECV_Socket == (maxSockets - 1))
										UDP_RECV_SocketValid[0] = true;
									close(UDP_RECV_Socket[i]);
								}

							}

						}
						if (ret-- == 0)
							break;

					}
				}


				for (int i = 0; i < maxSockets; i++){
					if (!TCP_SEND_SocketValid[i])
						continue;
					if (i != 0){
						char buf[TCP_SEND_Socket_Config[i].pktsize];
						memset(buf, 0, TCP_SEND_Socket_Config[i].pktsize);
						//printf("sending to i %d, TCP_SEND_Socket[%d] %d\n", i, i, TCP_SEND_Socket[i]);
						int numwrite = send(TCP_SEND_Socket[i], buf, sizeof(buf), 0);
						if (numwrite < 0){
							printf("send() failed\n");
							exit(EXIT_FAILURE);
						}
						else if (numwrite == 0){
							TCP_SEND_SocketValid[i] = false;
							TCP_Num_Active_SEND_Socket--;
							if (TCP_Num_Active_SEND_Socket == (maxSockets - 1))
								TCP_SEND_SocketValid[0] = true;
							close(TCP_SEND_Socket[i]);
						}

					}

				}

				for (int i = 0; i < maxSockets; i++){
					if (!UDP_SEND_SocketValid[i])
						continue;
					if (i != 0){
						char buf[UDP_SEND_Socket_Config[i].pktsize];
						memset(buf, 0, UDP_SEND_Socket_Config[i].pktsize);
						int slen = sizeof(UDP_SEND_ADDR[i]);
						//printf("sending to i %d, TCP_SEND_Socket[%d] %d\n", i, i, TCP_SEND_Socket[i]);
						UDP_SEND_Packet_Num[i]++;
						*((unsigned long*)buf) = htonl(UDP_SEND_Packet_Num[i]);
						int numwrite = sendto(udpfd, buf, sizeof(buf), 0, (struct sockaddr*)&UDP_SEND_ADDR[i], slen);
						if (numwrite < 0){
							printf("sendto() failed\n");
							exit(EXIT_FAILURE);
						}
						else if (numwrite == 0){
							UDP_SEND_SocketValid[i] = false;
							UDP_Num_Active_SEND_Socket--;
							if (UDP_Num_Active_SEND_Socket == (maxSockets - 1))
								UDP_SEND_SocketValid[0] = true;
							close(UDP_SEND_Socket[i]);
						}

					}

				}

		}

	return 0;
}

void print_manual() { //print the user manual
	printf("NetProbeSrv <parameters>, see below:\n");
	printf("    <-stat yyy>         set update statistics display to be once yyy ms. (Default = 0 = no stat display)\n");
	printf("    <-lhost hostname>   hostname to bind to. (Default late binding)\n");
	printf("    <-lport portnum>    port number to bind to. (Default '4180')\n");
	printf("    <-sbufsize bsize>   set the outgoing socket buffer size to bsize bytes.\n");
	printf("    <-rbufsize bsize>   set the incoming socket buffer size to bsize bytes.\n");
	printf("    <-servermodel [select|threadpool]>    set the concurrent servermodel to either select()-based or thread pool\n");
	printf("    <-poolsize psize>   set the initial thread pool size (default 8 threads), valid for thread-pool server model only\n\n");
}
