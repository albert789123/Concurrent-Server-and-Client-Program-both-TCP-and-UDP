#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _MSC_VER
	#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef WIN32
	#include <winsock2.h>
	#include <windows.h>
#else
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1
	#include <signal.h>
	#include <cstring>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <iostream>
#include <math.h>
using namespace std;

void print_manual();
int host_mode(char *argv[]);
int send_mode(int argc, char *argv[]);
int recv_mode(int argc, char *argv[]);
int response_mode(int argc, char *argv[]);
void InitializeSockets();
string greeting_to_string(struct greeting_message greeting);
string add_zero(string old_string, int n_zero);
char* string_to_char(string old_string);

struct greeting_message {
	unsigned short mode; // 0 -> send client, 1 -> recv client, 2 -> response client
	unsigned short proto; // 0 -> udp, 1 -> tcp
	int pktsize;
	int pktnum;
	int pktrate;
};

int main(int argc, char *argv[]) {
	if (argc == 1) {
		print_manual();
	}
	else if (argc > 1) {

		//for (int i = 0; i < argc; i++)
			//printf("argv [%d] is %s\n", i, argv[i]);

		if (strcmp(argv[1], "-send") == 0) {
			send_mode(argc, argv);
		}
		else if (strcmp(argv[1], "-recv") == 0) {
			recv_mode(argc, argv);
		}
		else if (strcmp(argv[1], "-response") == 0) {
			response_mode(argc, argv);
		}

	}

	return 0;

}


int send_mode(int argc, char *argv[]) {
	if (argc <= 2)
		argv[2] = (char *)"";
	if ((strcmp(argv[2], "-tcp") != 0) || (strcmp(argv[2], "-udp") == 0)) {
		printf("pConfig->eMode = 0\n");
		printf("pConfig->eProtocol = 0\n");

		int stat = 500;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-stat") == 0)) {
				stat = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktsize = 1000;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktsize") == 0)) {
				pktsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktrate = 1000;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktrate") == 0)) {
				pktrate = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktnum = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktnum") == 0)) {
				pktnum = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		long sbufsize = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-sbufsize") == 0)) {
				sbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		long rbufsize = 0;
		char *rhost = (char*)"127.0.0.1";
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rhost") == 0)) {
				rhost = argv[i + 1];
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int rport = 4180;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rport") == 0)) {
				rport = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}

		printf("NetProbe Configurations:\n");
		printf("  Mode: SEND    Protocol: UDP\n");
		printf("  -stat = %d\n", stat);
		printf("  -pktsize = %d\n", pktsize);
		printf("  -pktrate = %d\n", pktrate);
		printf("  -pktnum = %d\n", pktnum);
		printf("  -sbufsize = %ld\n", sbufsize);
		printf("  -rbufsize = %ld\n", rbufsize);
		printf("  -rhost = %s\n", rhost);
		printf("  -rport = %d\n\n", rport);

		long send_buffer_size = 65536;
		long recv_buffer_size = 65536;
		InitializeSockets();
		struct sockaddr_in si_other;
		#ifdef WIN32
			SOCKET ConnectSocket;
		#else
			int ConnectSocket;
		#endif
		int slen = sizeof(si_other);
		char* message = new char[pktsize];
		memset(message, 0, pktsize);

		if ((ConnectSocket = socket(AF_INET, SOCK_DGRAM, 0)) == SOCKET_ERROR) {
			printf("socket() failed with error");
			exit(EXIT_FAILURE);
		}

		int pkts = 0;
		long double rate = 0;
		printf("Default send buffer size = %ld bytes\n", send_buffer_size);
		printf("Default recv buffer size = %ld bytes\n\n", recv_buffer_size);
		if (sbufsize != 0)
			send_buffer_size = sbufsize;

		#ifdef WIN32
			Sleep(1000);
		#else
			usleep(1000);
		#endif
		if (setsockopt(ConnectSocket, SOL_SOCKET, SO_SNDBUF, (char*)(&send_buffer_size), sizeof(send_buffer_size)) == SOCKET_ERROR) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}

		memset((char*)&si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET;
		si_other.sin_port = htons(rport);
		#ifdef WIN32
			si_other.sin_addr.S_un.S_addr = inet_addr(rhost);
		#else
			si_other.sin_addr.s_addr = inet_addr(rhost);
		#endif

		#ifdef WIN32
			Sleep(1000);
		#else
			usleep(1000);
		#endif

		struct sockaddr_in controladdr;
		controladdr.sin_family = AF_INET;
		#ifdef WIN32
			controladdr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		#else
			controladdr.sin_addr.s_addr = inet_addr("127.0.0.1");
		#endif
		controladdr.sin_port = htons(41800);

		greeting_message greeting;
		greeting.mode = 0;
		greeting.proto = 0;
		greeting.pktnum = pktnum;
		greeting.pktsize = pktsize;
		greeting.pktrate = pktrate;

		string temp_string;
		temp_string = greeting_to_string(greeting);
		char* greeting_packet;
		greeting_packet = string_to_char(temp_string);
		if (sendto(ConnectSocket, greeting_packet, strlen(greeting_packet), 0, (struct sockaddr*)&controladdr, slen) == SOCKET_ERROR) {
			printf("sendto() failed with error");
			exit(EXIT_FAILURE);
		}

		clock_t start = clock();
		int rate_counter = 0;
		clock_t rate_start = clock();
		int first_time = 1;
		clock_t round_start, now;
		long double elapsed = 0;
		round_start = clock();
		while (1){
			if ((first_time) || pktrate == 0) {
				pkts++;
				*((unsigned long*)message) = htonl(pkts);
				if (sendto(ConnectSocket, message, pktsize, 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR) {
					printf("sendto() failed with error");
					exit(EXIT_FAILURE);
				}
				if (first_time)
					printf("Sending to [%s:%d]:\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
				first_time = 0;
				rate_counter += pktsize;
			}
			else if (rate_counter < pktrate) {
				pkts++;
				*((unsigned long*)message) = htonl(pkts);
				if (sendto(ConnectSocket, message, pktsize, 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR) {
					printf("sendto() failed with error");
					exit(EXIT_FAILURE);
				}
				rate_counter += pktsize;
			}
			else if (rate_counter >= pktrate) {
				if (clock() - rate_start >= 1000) {
					rate_start = clock();
					rate_counter = 0;
				}
			}
			now = clock();
			elapsed = ((long double)now - start) / (double)CLOCKS_PER_SEC;
			rate = (((long double)pkts * (long double)pktsize) / elapsed) / 1000 * 8;

			if ((pkts >= pktnum) && (pktnum != 0)) {
				printf("Elapsed [%.1Lfs] Rate [%.1Lfbps]\n", elapsed, rate);
				printf("All packets are sent.\n");
				break;
			}
			if ((((long double)now - round_start) / (double)CLOCKS_PER_SEC * 1000) >= stat) {
				printf("Elapsed [%.1Lfs] Rate [%.1Lfbps]\n", elapsed, rate);
				round_start = clock();
			}

		}

	}
	else if (strcmp(argv[2], "-tcp") == 0) {
		printf("pConfig->eMode = 0\n");
		printf("pConfig->eProtocol = 1\n");
		int stat = 500;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-stat") == 0)) {
				stat = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktsize = 1000;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktsize") == 0)) {
				pktsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktrate = 1000;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktrate") == 0)) {
				pktrate = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktnum = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktnum") == 0)) {
				pktnum = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		long sbufsize = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-sbufsize") == 0)) {
				sbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		long rbufsize = 0;
		char* rhost = (char*)"127.0.0.1";
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rhost") == 0)) {
				rhost = argv[i + 1];
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int rport = 4180;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rport") == 0)) {
				rport = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}

		printf("NetProbe Configurations:\n");
		printf("  Mode: SEND    Protocol: TCP\n");
		printf("  -stat = %d\n", stat);
		printf("  -pktsize = %d\n", pktsize);
		printf("  -pktrate = %d\n", pktrate);
		printf("  -pktnum = %d\n", pktnum);
		printf("  -sbufsize = %ld\n", sbufsize);
		printf("  -rbufsize = %ld\n", rbufsize);
		printf("  -rhost = %s\n", rhost);
		printf("  -rport = %d\n\n", rport);

		long send_buffer_size = 65536;
		long recv_buffer_size = 65536;
		InitializeSockets();
		struct sockaddr_in si_other;
		#ifdef WIN32
			SOCKET ConnectSocket;
		#else
			int ConnectSocket;
		#endif
		int slen = sizeof(si_other);
		char* message = new char[pktsize];
		memset(message, 0, pktsize);

		memset((char*)&si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET;
		si_other.sin_port = htons(rport);
		#ifdef WIN32
			si_other.sin_addr.S_un.S_addr = inet_addr(rhost);
		#else
			si_other.sin_addr.s_addr = inet_addr(rhost);
		#endif

		if ((ConnectSocket = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR) {
			printf("socket() failed with error");
			exit(EXIT_FAILURE);
		}

		if (connect(ConnectSocket, (struct sockaddr*)&si_other, sizeof(si_other)) == SOCKET_ERROR) {
			printf("connect() failed with error");
			exit(EXIT_FAILURE);
		}
		printf("Remote host localhost lookup successful, IP address is %s\n", rhost);

		int pkts = 0;
		long double rate = 0;
		printf("Default send buffer size = %ld bytes\n", send_buffer_size);
		printf("Default recv buffer size = %ld bytes\n\n", recv_buffer_size);
		if (sbufsize != 0)
			send_buffer_size = sbufsize;

		if (setsockopt(ConnectSocket, SOL_SOCKET, SO_SNDBUF, (char*)(&send_buffer_size), sizeof(send_buffer_size)) == SOCKET_ERROR) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}

		greeting_message greeting;
		greeting.mode = 0;
		greeting.proto = 1;
		greeting.pktnum = pktnum;
		greeting.pktsize = pktsize;
		greeting.pktrate = pktrate;

		string temp_string;
		temp_string = greeting_to_string(greeting);
		char* greeting_packet;
		greeting_packet = string_to_char(temp_string);
		if (send(ConnectSocket, greeting_packet, strlen(greeting_packet), 0) == SOCKET_ERROR) {
			printf("sendto() failed with error");
			exit(EXIT_FAILURE);
		}

		printf("Sending to [%s:%d]:\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

		#ifdef WIN32
			Sleep(1000);
		#else
			usleep(1000);
		#endif

		clock_t start = clock();
		int rate_counter = 0;
		clock_t rate_start = clock();
		int first_time = 1;
		clock_t round_start, now;
		long double elapsed = 0;
		round_start = clock();
		while (1) {
			if ((first_time) || pktrate == 0) {
				first_time = 0;
				pkts++;
				*((unsigned long*)message) = htonl(pkts);
				if (send(ConnectSocket, message, pktsize, 0) == SOCKET_ERROR) {
					printf("send() failed with error");
					exit(EXIT_FAILURE);
				}
				rate_counter += pktsize;
			}
			else if (rate_counter < pktrate) {
				pkts++;
				*((unsigned long*)message) = htonl(pkts);
				if (send(ConnectSocket, message, pktsize, 0) == SOCKET_ERROR) {
					printf("send() failed with error");
					exit(EXIT_FAILURE);
				}
				rate_counter += pktsize;
			}
			else if (rate_counter >= pktrate) {
				if (clock() - rate_start >= 1000) {
					rate_start = clock();
					rate_counter = 0;
				}
			}
			now = clock();
			elapsed = ((long double)now - start) / (double)CLOCKS_PER_SEC;
			rate = (((long double)pkts * (long double)pktsize) / elapsed) / 1000 * 8;

			if ((pkts >= pktnum) && (pktnum != 0)) {
				printf("Elapsed [%.1Lfs] Rate [%.1Lfbps]\n", elapsed, rate);
				printf("All packets are sent.\n");
				break;
			}
			if ((((long double)now - round_start) / (double)CLOCKS_PER_SEC * 1000) >= stat) {
				printf("Elapsed [%.1Lfs] Rate [%.1Lfbps]\n", elapsed, rate);
				round_start = clock();
			}

		}
	}
	return 0;
}

int recv_mode(int argc, char *argv[]) {
	if (argc <= 2)
		argv[2] = (char*)"";
	if ((strcmp(argv[2], "-tcp") != 0) || (strcmp(argv[2], "-udp") == 0)) {
		printf("pConfig->eMode = 1\n");
		printf("pConfig->eProtocol = 0\n");

		int stat = 500;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-stat") == 0)) {
				stat = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktsize = 1000;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktsize") == 0)) {
				pktsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktrate = 1000;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktrate") == 0)) {
				pktrate = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktnum = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktnum") == 0)) {
				pktnum = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		long sbufsize = 0;
		long rbufsize = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rbufsize") == 0)) {
				rbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		char *rhost = (char*)"127.0.0.1";
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rhost") == 0)) {
				rhost = argv[i + 1];
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int rport = 4180;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rport") == 0)) {
				rport = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}

		printf("NetProbe Configurations:\n");
		printf("  Mode: RECV    Protocol: UDP\n");
		printf("  -stat = %d\n", stat);
		printf("  -pktsize = %d\n", pktsize);
		printf("  -pktrate = %d\n", pktrate);
		printf("  -pktnum = %d\n", pktnum);
		printf("  -sbufsize = %ld\n", sbufsize);
		printf("  -rbufsize = %ld\n", rbufsize);
		printf("  -rhost = %s\n", rhost);
		printf("  -rport = %d\n\n", rport);

		long send_buffer_size = 65536;
		long recv_buffer_size = 65536;
		InitializeSockets();

		#ifdef WIN32
			SOCKET TargetSocket;
		#else
			int TargetSocket;
		#endif
		struct sockaddr_in si_other;
		int recv_len;
		#ifdef WIN32
			int slen;
		#else
			socklen_t slen;
		#endif
		char *buf = new char[pktsize];

		slen = sizeof(si_other);

		if ((TargetSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)	{
			printf("Could not create socket");
		}

		int pkts = 0;
		int lost = 0;
		long double rate = 0;
		long double jitter = 0;
		printf("Default send buffer size = %ld bytes\n", send_buffer_size);
		printf("Default recv buffer size = %ld bytes\n", recv_buffer_size);
		if (rbufsize != 0)
			recv_buffer_size = rbufsize;

		if (setsockopt(TargetSocket, SOL_SOCKET, SO_RCVBUF, (char*)(&recv_buffer_size), sizeof(recv_buffer_size)) == SOCKET_ERROR) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}

		si_other.sin_family = AF_INET;
		si_other.sin_port = htons(rport);
		#ifdef WIN32
			si_other.sin_addr.S_un.S_addr = inet_addr(rhost);
		#else
			si_other.sin_addr.s_addr = inet_addr(rhost);
		#endif

		struct sockaddr_in controladdr;
		controladdr.sin_family = AF_INET;
		#ifdef WIN32
			controladdr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		#else
			controladdr.sin_addr.s_addr = inet_addr("127.0.0.1");
		#endif
		controladdr.sin_port = htons(41800);

		greeting_message greeting;
		greeting.mode = 1;
		greeting.proto = 0;
		greeting.pktnum = pktnum;
		greeting.pktsize = pktsize;
		greeting.pktrate = pktrate;

		string temp_string;
		temp_string = greeting_to_string(greeting);
		char* greeting_packet;
		greeting_packet = string_to_char(temp_string);
		if (sendto(TargetSocket, greeting_packet, strlen(greeting_packet), 0, (struct sockaddr*)&controladdr, slen) == SOCKET_ERROR) {
			printf("sendto() failed with error");
			exit(EXIT_FAILURE);
		}

		printf("Waiting for incoming datagrames ...\n\n");

		clock_t start = clock();
		clock_t round_start, now;
		clock_t jitter_start;
		long double jitter_sum = 0;
		long double current_jitter = 0;
		long double last_jitter = 0;
		long double jitter_difference = 0;
		int sequence_num = 0;
		long double elapsed = 0;
		int first_time = 1;
		round_start = clock();
		while (1) {
			jitter_start = clock();
			if ((recv_len = recvfrom(TargetSocket, buf, pktsize, 0, (struct sockaddr*)&si_other, &slen)) == SOCKET_ERROR) {
				printf("recvfrom() failed with error");
				exit(EXIT_FAILURE);
			}
			now = clock();
			current_jitter = (long double)now - jitter_start;
			if (last_jitter != 0) {
				jitter_difference = fabs(last_jitter - current_jitter);
			}
			last_jitter = current_jitter;

			pkts++;
			sequence_num++;
			if (first_time) {
				first_time = 0;
				printf("Receiving from [%s:%d]:\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
			}

			if (ntohl(*((unsigned long*)buf)) != sequence_num) {
				lost += ntohl(*((unsigned long*)buf)) - sequence_num - 1;
				sequence_num = ntohl(*((unsigned long*)buf));
			}

			elapsed = ((long double)now - start) / (double)CLOCKS_PER_SEC;
			if ((((long double)now - round_start) / (double)CLOCKS_PER_SEC * 1000) >= stat) {
				rate = (((long double)pkts * (long double)pktsize) / elapsed) / 1000 * 8;
				jitter_sum += jitter_difference;
				jitter = jitter_sum / pkts;
				printf("Elapsed [%.1Lfs] Pkts [%d] Lost [%d, %.1Lf%%] Rate [%.1Lfbps] Jitter [%.8Lfms]\n", elapsed, pkts, lost, (long double)lost / (long double)pkts * 100, rate, jitter);
				round_start = clock();
			}

		}

	}
	else if (strcmp(argv[2], "-tcp") == 0) {
		printf("pConfig->eMode = 1\n");
		printf("pConfig->eProtocol = 1\n");

		int stat = 500;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-stat") == 0)) {
				stat = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktsize = 1000;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktsize") == 0)) {
				pktsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktrate = 1000;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktrate") == 0)) {
				pktrate = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktnum = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktnum") == 0)) {
				pktnum = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		long sbufsize = 0;
		long rbufsize = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rbufsize") == 0)) {
				rbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		char *rhost = (char*)"127.0.0.1";
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rhost") == 0)) {
				rhost = argv[i + 1];
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int rport = 4180;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rport") == 0)) {
				rport = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}

		printf("NetProbe Configurations:\n");
		printf("  Mode: RECV    Protocol: TCP\n");
		printf("  -stat = %d\n", stat);
		printf("  -pktsize = %d\n", pktsize);
		printf("  -pktrate = %d\n", pktrate);
		printf("  -pktnum = %d\n", pktnum);
		printf("  -sbufsize = %ld\n", sbufsize);
		printf("  -rbufsize = %ld\n", rbufsize);
		printf("  -rhost = %s\n", rhost);
		printf("  -rport = %d\n\n", rport);

		long send_buffer_size = 65536;
		long recv_buffer_size = 65536;
		InitializeSockets();

		#ifdef WIN32
			SOCKET TargetSocket;
		#else
			int TargetSocket;
		#endif
		struct sockaddr_in si_other;
		int recv_len;
		#ifdef WIN32
			int slen;
		#else
			socklen_t slen;
		#endif
		char* buf = new char[pktsize];
		si_other.sin_family = AF_INET;
		si_other.sin_port = htons(rport);
		#ifdef WIN32
			si_other.sin_addr.S_un.S_addr = inet_addr(rhost);
		#else
			si_other.sin_addr.s_addr = inet_addr(rhost);
		#endif
		slen = sizeof(si_other);

		if ((TargetSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
			printf("Could not create socket");
		}

		if (connect(TargetSocket, (struct sockaddr*)&si_other, sizeof(si_other)) == SOCKET_ERROR) {
			printf("connect() failed with error");
			exit(EXIT_FAILURE);
		}

		printf("Default send buffer size = %ld bytes\n", send_buffer_size);
		printf("Default recv buffer size = %ld bytes\n", recv_buffer_size);
		printf("Binding local socket to port %d using late binding ... successful.\n", rport);
		printf("Listening to incoming connection request ... connected to %s port %d\n\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

		int pkts = 0;
		int lost = 0;
		long double rate = 0;
		long double jitter = 0;
		if (rbufsize != 0)
			recv_buffer_size = rbufsize;

		if (setsockopt(TargetSocket, SOL_SOCKET, SO_RCVBUF, (char*)(&recv_buffer_size), sizeof(recv_buffer_size)) == SOCKET_ERROR) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}

		greeting_message greeting;
		greeting.mode = 1;
		greeting.proto = 1;
		greeting.pktnum = pktnum;
		greeting.pktsize = pktsize;
		greeting.pktrate = pktrate;

		string temp_string;
		temp_string = greeting_to_string(greeting);
		char* greeting_packet;
		greeting_packet = string_to_char(temp_string);
		if (send(TargetSocket, greeting_packet, strlen(greeting_packet), 0) == SOCKET_ERROR) {
			printf("send() failed with error");
			exit(EXIT_FAILURE);
		}

		clock_t start = clock();
		clock_t round_start, now;
		clock_t jitter_start;
		long double jitter_sum = 0;
		long double current_jitter = 0;
		long double last_jitter = 0;
		long double jitter_difference = 0;
		int sequence_num = 0;
		long double elapsed = 0;
		int first_time = 1;
		round_start = clock();
		while (1) {
			jitter_start = clock();
			if ((recv_len = recv(TargetSocket, buf, pktsize, 0)) == SOCKET_ERROR) {
				printf("recv() failed with error");
				exit(EXIT_FAILURE);
			}
			now = clock();
			current_jitter = (long double)now - jitter_start;
			if (last_jitter != 0) {
				jitter_difference = fabs(last_jitter - current_jitter);
			}
			last_jitter = current_jitter;

			pkts++;
			sequence_num++;
			if (first_time) {
				first_time = 0;
				printf("Receiving from [%s:%d]:\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
			}

			elapsed = ((long double)now - start) / (double)CLOCKS_PER_SEC;
			if ((((long double)now - round_start) / (double)CLOCKS_PER_SEC * 1000) >= stat) {
				rate = (((long double)pkts * (long double)pktsize) / elapsed) / 1000 * 8;
				jitter_sum += jitter_difference;
				jitter = jitter_sum / pkts;
				printf("Elapsed [%.1Lfs] Pkts [%d] Lost [%d, %.1Lf%%] Rate [%.1Lfbps] Jitter [%.8Lfms]\n", elapsed, pkts, lost, (long double)lost / (long double)pkts * 100, rate, jitter);
				round_start = clock();
			}

		}
	}
	return 0;
}

int response_mode(int argc, char *argv[]) {
	if (argc <= 2)
		argv[2] = (char *)"";
	if ((strcmp(argv[2], "-tcp") != 0) || (strcmp(argv[2], "-udp") == 0)) {
		printf("pConfig->eMode = 2\n");
		printf("pConfig->eProtocol = 0\n");

		int stat = 500;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-stat") == 0)) {
				stat = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktsize = 1000;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktsize") == 0)) {
				pktsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktrate = 10;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktrate") == 0)) {
				pktrate = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktnum = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktnum") == 0)) {
				pktnum = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		long sbufsize = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-sbufsize") == 0)) {
				sbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		long rbufsize = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rbufsize") == 0)) {
				rbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		char *rhost = (char*)"127.0.0.1";
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rhost") == 0)) {
				rhost = argv[i + 1];
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int rport = 4180;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rport") == 0)) {
				rport = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}

		printf("NetProbe Configurations:\n");
		printf("  Mode: RESPONSE    Protocol: UDP\n");
		printf("  -stat = %d\n", stat);
		printf("  -pktsize = %d\n", pktsize);
		printf("  -pktrate = %d\n", pktrate);
		printf("  -pktnum = %d\n", pktnum);
		printf("  -sbufsize = %ld\n", sbufsize);
		printf("  -rbufsize = %ld\n", rbufsize);
		printf("  -rhost = %s\n", rhost);
		printf("  -rport = %d\n\n", rport);

		long send_buffer_size = 65536;
		long recv_buffer_size = 65536;
		InitializeSockets();
		struct sockaddr_in si_other;
		#ifdef WIN32
			SOCKET ConnectSocket;
		#else
			int ConnectSocket;
		#endif
		int recv_len;
		#ifdef WIN32
			int slen;
		#else
			socklen_t slen;
		#endif
		slen = sizeof(si_other);
		char* message = new char[pktsize];
		memset(message, 0, pktsize);
		char* buf = new char[pktsize];

		if ((ConnectSocket = socket(AF_INET, SOCK_DGRAM, 0)) == SOCKET_ERROR) {
			printf("socket() failed with error");
			exit(EXIT_FAILURE);
		}

		int pkts = 0;
		long double rate = 0;
		printf("Default send buffer size = %ld bytes\n", send_buffer_size);
		printf("Default recv buffer size = %ld bytes\n\n", recv_buffer_size);
		if (sbufsize != 0)
			send_buffer_size = sbufsize;
		if (rbufsize != 0)
				recv_buffer_size = rbufsize;

		#ifdef WIN32
			Sleep(1000);
		#else
			usleep(1000);
		#endif
		if (setsockopt(ConnectSocket, SOL_SOCKET, SO_SNDBUF, (char*)(&send_buffer_size), sizeof(send_buffer_size)) == SOCKET_ERROR) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}
		if (setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVBUF, (char*)(&recv_buffer_size), sizeof(recv_buffer_size)) == SOCKET_ERROR) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}

		memset((char*)&si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET;
		si_other.sin_port = htons(rport);
		#ifdef WIN32
			si_other.sin_addr.S_un.S_addr = inet_addr(rhost);
		#else
			si_other.sin_addr.s_addr = inet_addr(rhost);
		#endif

		#ifdef WIN32
			Sleep(1000);
		#else
			usleep(1000);
		#endif

		struct sockaddr_in controladdr;
		controladdr.sin_family = AF_INET;
		#ifdef WIN32
			controladdr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		#else
			controladdr.sin_addr.s_addr = inet_addr("127.0.0.1");
		#endif
		controladdr.sin_port = htons(41800);

		greeting_message greeting;
		greeting.mode = 2;
		greeting.proto = 0;
		greeting.pktnum = pktnum;
		greeting.pktsize = pktsize;
		greeting.pktrate = pktrate;

		string temp_string;
		temp_string = greeting_to_string(greeting);
		char* greeting_packet;
		greeting_packet = string_to_char(temp_string);
		if (sendto(ConnectSocket, greeting_packet, strlen(greeting_packet), 0, (struct sockaddr*)&controladdr, slen) == SOCKET_ERROR) {
			printf("sendto() failed with error");
			exit(EXIT_FAILURE);
		}

		clock_t start = clock();
		int rate_counter = 0;
		clock_t rate_start = clock();
		int first_time = 1;
		clock_t round_start, now, reponse_counter, jitter_start;
		long double elapsed = 0;
		round_start = clock();
		int reply = 0;
		long double min_time, max_time, avg_time, avg_counter, jitter;
		avg_counter = 0;
		long double jitter_sum = 0;
		long double current_jitter = 0;
		long double last_jitter = 0;
		long double jitter_difference = 0;

		while (1){
			if ((first_time) || pktrate == 0) {
				pkts++;
				reponse_counter = clock();
				if (sendto(ConnectSocket, message, pktsize, 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR) {
					printf("sendto() failed with error");
					exit(EXIT_FAILURE);
				}
				jitter_start = clock();
				if ((recv_len = recvfrom(ConnectSocket, buf, pktsize, 0, (struct sockaddr*)&si_other, &slen)) == SOCKET_ERROR) {
					printf("recvfrom() failed with error");
					exit(EXIT_FAILURE);
				}
				now = clock();
				reply++;
				if (first_time) {
					printf("Communicating with [%s:%d]:\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
					min_time = (long double)now - reponse_counter;
					max_time = (long double)now - reponse_counter;
				}
				first_time = 0;
				rate_counter += pktsize;
				if (min_time > (long double)now - reponse_counter)
					min_time = (long double)now - reponse_counter;
				else if (max_time < (long double)now - reponse_counter)
					max_time = (long double)now - reponse_counter;
				avg_counter += (long double)now - reponse_counter;

				current_jitter = (long double)now - jitter_start;
				if (last_jitter != 0) {
					jitter_difference = fabs(last_jitter - current_jitter);
				}
				last_jitter = current_jitter;
			}
			else if (rate_counter < pktrate) {
				pkts++;
				reponse_counter = clock();
				if (sendto(ConnectSocket, message, pktsize, 0, (struct sockaddr*)&si_other, slen) == SOCKET_ERROR) {
					printf("sendto() failed with error");
					exit(EXIT_FAILURE);
				}
				jitter_start = clock();
				if ((recv_len = recvfrom(ConnectSocket, buf, pktsize, 0, (struct sockaddr*)&si_other, &slen)) == SOCKET_ERROR) {
					printf("recvfrom() failed with error");
					exit(EXIT_FAILURE);
				}
				now = clock();
				reply++;
				rate_counter += 1;
				if (min_time > (long double)now - reponse_counter)
					min_time = (long double)now - reponse_counter;
				else if (max_time < (long double)now - reponse_counter)
					max_time = (long double)now - reponse_counter;
				avg_counter += (long double)now - reponse_counter;

				current_jitter = (long double)now - jitter_start;
				if (last_jitter != 0) {
					jitter_difference = fabs(last_jitter - current_jitter);
				}
				last_jitter = current_jitter;
			}
			else if (rate_counter >= pktrate) {
				if (clock() - rate_start >= 1000) {
					rate_start = clock();
					rate_counter = 0;
				}
			}
			now = clock();
			elapsed = ((long double)now - start) / (double)CLOCKS_PER_SEC;
			avg_time = avg_counter / reply;
			jitter_sum += jitter_difference;
			jitter = jitter_sum / reply / (double)CLOCKS_PER_SEC * 1000;

			if ((pkts >= pktnum) && (pktnum != 0)) {
				printf("Elapsed [%.1Lfs] Replies [%d] Min [%.3Lfms] Max [%.3Lfms] Avg [%.3Lfms] Jitter [%.8Lfms]\n", elapsed, reply, min_time / (double)CLOCKS_PER_SEC * 1000, max_time / (double)CLOCKS_PER_SEC * 1000, avg_time / (double)CLOCKS_PER_SEC * 1000, jitter);
				printf("All packets are sent.\n");
				break;
			}
			if ((((long double)now - round_start) / (double)CLOCKS_PER_SEC * 1000) >= stat) {
				printf("Elapsed [%.1Lfs] Replies [%d] Min [%.3Lfms] Max [%.3Lfms] Avg [%.3Lfms] Jitter [%.8Lfms]\n", elapsed, reply, min_time / (double)CLOCKS_PER_SEC * 1000, max_time / (double)CLOCKS_PER_SEC * 1000, avg_time / (double)CLOCKS_PER_SEC * 1000, jitter);
				round_start = clock();
			}

		}

	}
	else if (strcmp(argv[2], "-tcp") == 0) {
		printf("pConfig->eMode = 2\n");
		printf("pConfig->eProtocol = 1\n");
		int stat = 500;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-stat") == 0)) {
				stat = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktsize = 1000;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktsize") == 0)) {
				pktsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktrate = 10;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktrate") == 0)) {
				pktrate = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int pktnum = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-pktnum") == 0)) {
				pktnum = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		long sbufsize = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-sbufsize") == 0)) {
				sbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		long rbufsize = 0;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rbufsize") == 0)) {
				rbufsize = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		char* rhost = (char*)"127.0.0.1";
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rhost") == 0)) {
				rhost = argv[i + 1];
				break;
			}
			if (argv[i] == NULL)
				break;
		}
		int rport = 4180;
		for (int i = 1; i < argc; i++) {
			if ((strcmp(argv[i], "-rport") == 0)) {
				rport = atoi(argv[i + 1]);
				break;
			}
			if (argv[i] == NULL)
				break;
		}

		printf("NetProbe Configurations:\n");
		printf("  Mode: SEND    Protocol: TCP\n");
		printf("  -stat = %d\n", stat);
		printf("  -pktsize = %d\n", pktsize);
		printf("  -pktrate = %d\n", pktrate);
		printf("  -pktnum = %d\n", pktnum);
		printf("  -sbufsize = %ld\n", sbufsize);
		printf("  -rbufsize = %ld\n", rbufsize);
		printf("  -rhost = %s\n", rhost);
		printf("  -rport = %d\n\n", rport);

		long send_buffer_size = 65536;
		long recv_buffer_size = 65536;
		InitializeSockets();
		struct sockaddr_in si_other;
		#ifdef WIN32
			SOCKET ConnectSocket;
		#else
			int ConnectSocket;
		#endif
		int recv_len;
		char* message = new char[pktsize];
		memset(message, 0, pktsize);
		char* buf = new char[pktsize];

		memset((char*)&si_other, 0, sizeof(si_other));
		si_other.sin_family = AF_INET;
		si_other.sin_port = htons(rport);
		#ifdef WIN32
			si_other.sin_addr.S_un.S_addr = inet_addr(rhost);
		#else
			si_other.sin_addr.s_addr = inet_addr(rhost);
		#endif

		if ((ConnectSocket = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR) {
			printf("socket() failed with error");
			exit(EXIT_FAILURE);
		}

		if (connect(ConnectSocket, (struct sockaddr*)&si_other, sizeof(si_other)) == SOCKET_ERROR) {
			printf("connect() failed with error");
			exit(EXIT_FAILURE);
		}
		printf("Remote host localhost lookup successful, IP address is %s\n", rhost);

		int pkts = 0;
		long double rate = 0;
		printf("Default send buffer size = %ld bytes\n", send_buffer_size);
		printf("Default recv buffer size = %ld bytes\n\n", recv_buffer_size);
		if (sbufsize != 0)
			send_buffer_size = sbufsize;
		if (rbufsize != 0)
			recv_buffer_size = rbufsize;

		if (setsockopt(ConnectSocket, SOL_SOCKET, SO_SNDBUF, (char*)(&send_buffer_size), sizeof(send_buffer_size)) == SOCKET_ERROR) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}
		if (setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVBUF, (char*)(&recv_buffer_size), sizeof(recv_buffer_size)) == SOCKET_ERROR) {
			printf("Setsockopt failed with error");
			exit(EXIT_FAILURE);
		}

		greeting_message greeting;
		greeting.mode = 2;
		greeting.proto = 1;
		greeting.pktnum = pktnum;
		greeting.pktsize = pktsize;
		greeting.pktrate = pktrate;

		string temp_string;
		temp_string = greeting_to_string(greeting);
		char* greeting_packet;
		greeting_packet = string_to_char(temp_string);
		if (send(ConnectSocket, greeting_packet, strlen(greeting_packet), 0) == SOCKET_ERROR) {
			printf("sendto() failed with error");
			exit(EXIT_FAILURE);
		}

		#ifdef WIN32
			Sleep(1000);
		#else
			usleep(1000);
		#endif

		printf("Communicating with [%s:%d]:\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

		clock_t start = clock();
		int rate_counter = 0;
		clock_t rate_start = clock();
		int first_time = 1;
		clock_t round_start, now, reponse_counter, jitter_start;
		long double elapsed = 0;
		round_start = clock();
		int reply = 0;
		long double min_time, max_time, avg_time, avg_counter, jitter;
		avg_counter = 0;
		long double jitter_sum = 0;
		long double current_jitter = 0;
		long double last_jitter = 0;
		long double jitter_difference = 0;

		while (1){
			if ((first_time) || pktrate == 0) {
				pkts++;
				reponse_counter = clock();
				if (send(ConnectSocket, message, pktsize, 0) == SOCKET_ERROR) {
					printf("send() failed with error");
					exit(EXIT_FAILURE);
				}
				jitter_start = clock();
				if ((recv_len = recv(ConnectSocket, buf, pktsize, 0)) == SOCKET_ERROR) {
					printf("recv() failed with error");
					exit(EXIT_FAILURE);
				}
				now = clock();
				reply++;
				if (first_time) {
					min_time = (long double)now - reponse_counter;
					max_time = (long double)now - reponse_counter;
				}
				first_time = 0;
				rate_counter += pktsize;
				if (min_time > (long double)now - reponse_counter)
					min_time = (long double)now - reponse_counter;
				else if (max_time < (long double)now - reponse_counter)
					max_time = (long double)now - reponse_counter;
				avg_counter += (long double)now - reponse_counter;

				current_jitter = (long double)now - jitter_start;
				if (last_jitter != 0) {
					jitter_difference = fabs(last_jitter - current_jitter);
				}
				last_jitter = current_jitter;
			}
			else if (rate_counter < pktrate) {
				pkts++;
				reponse_counter = clock();
				if (send(ConnectSocket, message, pktsize, 0) == SOCKET_ERROR) {
					printf("send() failed with error");
					exit(EXIT_FAILURE);
				}
				jitter_start = clock();
				if ((recv_len = recv(ConnectSocket, buf, pktsize, 0)) == SOCKET_ERROR) {
					printf("recv() failed with error");
					exit(EXIT_FAILURE);
				}
				now = clock();
				reply++;
				rate_counter += 1;
				if (min_time > (long double)now - reponse_counter)
					min_time = (long double)now - reponse_counter;
				else if (max_time < (long double)now - reponse_counter)
					max_time = (long double)now - reponse_counter;
				avg_counter += (long double)now - reponse_counter;

				current_jitter = (long double)now - jitter_start;
				if (last_jitter != 0) {
					jitter_difference = fabs(last_jitter - current_jitter);
				}
				last_jitter = current_jitter;
			}
			else if (rate_counter >= pktrate) {
				if (clock() - rate_start >= 1000) {
					rate_start = clock();
					rate_counter = 0;
				}
			}
			now = clock();
			elapsed = ((long double)now - start) / (double)CLOCKS_PER_SEC;
			avg_time = avg_counter / reply;
			jitter_sum += jitter_difference;
			jitter = jitter_sum / reply / (double)CLOCKS_PER_SEC * 1000;

			if ((pkts >= pktnum) && (pktnum != 0)) {
				printf("Elapsed [%.1Lfs] Replies [%d] Min [%.3Lfms] Max [%.3Lfms] Avg [%.3Lfms] Jitter [%.8Lfms]\n", elapsed, reply, min_time / (double)CLOCKS_PER_SEC * 1000, max_time / (double)CLOCKS_PER_SEC * 1000, avg_time / (double)CLOCKS_PER_SEC * 1000, jitter);
				printf("All packets are sent.\n");
				break;
			}
			if ((((long double)now - round_start) / (double)CLOCKS_PER_SEC * 1000) >= stat) {
				printf("Elapsed [%.1Lfs] Replies [%d] Min [%.3Lfms] Max [%.3Lfms] Avg [%.3Lfms] Jitter [%.8Lfms]\n", elapsed, reply, min_time / (double)CLOCKS_PER_SEC * 1000, max_time / (double)CLOCKS_PER_SEC * 1000, avg_time / (double)CLOCKS_PER_SEC * 1000, jitter);
				round_start = clock();
			}

		}

	}
	return 0;
}

void print_manual() {
	printf("NetProbeClient [mode] <more parameters, see below>\n");
	printf("[mode] :  -send means sending mode (the client sends packets to the server);\n");
	printf("          -recv means receiving mode;\n");
	printf("          -response means response time mode(the client sends requests to the server and the server sends reply messages to the clients);\n");
	printf("For all modes the followings are the supported parameters: \n");
	printf("   <-stat yyy>         update statistics once every yyy ms. (Default = 500 ms)\n");
	printf("   <-rhost hostname>   send data to host specified by hostname. (Default 'localhost')\n");
	printf("   <-rport portnum>	   server port number. (Default '4180')\n");
	printf("   <-tcp|udp>          send data using TCP or UDP. (Default UDP)\n");
	printf("   <-pktsize bsize>    send message of bsize bytes. (Default 1000 bytes)\n");
	printf("   <-pktrate txrate>   send data at a data rate of txrate bytes per second,\n");
	printf("                       response mode: reauest rate(per second, default 10/s)\n");
	printf("                       0 means as fast as possible. (Default 1000 Bps)\n");
	printf("   <-pktnum num>       send or receive a total of num messages. (Default = 0 = infinite)\n");
	printf("   <-sbufsize bsize>   set the outgoing socket buffer size to bsize bytes.\n");
	printf("   <-rbufsize bsize>   set the incoming socket buffer size to bsize bytes.\n\n");
}

void InitializeSockets() {

#ifdef WIN32

	WSADATA wsaData; // For external access.
	WORD wVersionRequested;
	wVersionRequested = MAKEWORD(2, 0);
	int err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we couldn't find a useable */
		/* winsock.dll. */
		printf("\nUnable to initialize winsock.dll!");
		exit(-1);
	}
	if (LOBYTE(wsaData.wVersion) != 2 ||
		HIBYTE(wsaData.wVersion) != 0) {
		/* Tell the user that we couldn't find a useable */
		/* winsock.dll. */
		WSACleanup();
		printf("\nIncorrect winsock version, expecting 2.0 !");
		exit(-1);
	}

#else

	setbuf(stdout, NULL);
	signal(SIGPIPE, SIG_IGN);

#endif // WIN32

}

string greeting_to_string(struct greeting_message greeting) {
	string temp;

	string mode = to_string(greeting.mode);
	string proto = to_string(greeting.proto);

	temp = to_string(greeting.pktnum);
	string pktnum = add_zero(temp, 10);
	temp = to_string(greeting.pktsize);
	string pktsize = add_zero(temp, 10);
	temp = to_string(greeting.pktrate);
	string pktrate = add_zero(temp, 10);

	temp = mode + proto + pktnum + pktsize + pktrate;
	return temp;
}

string add_zero(string old_string, int n_zero) {
	char ch = '0';
	string new_string;
	new_string = string(n_zero - old_string.length(), ch) + old_string;
	return new_string;
}

char* string_to_char(string old_string) {
	char* temp = new char[old_string.length()];
	strcpy(temp, old_string.c_str());
	return temp;
}
