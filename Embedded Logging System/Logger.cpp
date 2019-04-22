// File: Logger.cpp
//
// April 11, 2019 - Created
// 
// Sends logs to the LogServer from the process

#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "Logger.h"

using namespace std;

const int BUF_LEN = 4096;
char buf[BUF_LEN];
const char IP[] = "127.0.0.1";
const int PORT = 1111;
bool is_running;
struct sockaddr_in servaddr;
socklen_t addrlen;
pthread_mutex_t mutex;
pthread_t tid;
int fd;
LOG_LEVEL logLevel;


void *recv_func(void *arg);

int InitializeLog() {
	int ret;
	addrlen = sizeof(servaddr);

	fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	if (fd < 0) {
		cout << strerror(errno) << endl;
		return -1;
	}

	memset((char *)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;

	ret = inet_pton(AF_INET, IP, &servaddr.sin_addr);
	if (ret == 0) {
		cout << strerror(errno) << endl;
		return -1;
	}
	servaddr.sin_port = htons(PORT);
	
	pthread_mutex_init(&mutex, NULL);
	
	ret = pthread_create(&tid, NULL, recv_func, &fd);
	if (ret != 0) {
		cout << strerror(errno) << endl;
		return -1;
	}
}

void SetLogLevel(LOG_LEVEL level) {
	logLevel = level;
}

void Log(LOG_LEVEL level, const char *prog, const char *func, int line, const char *message) {
	if (level >= logLevel) {
		int len, ret;
		time_t t;
		time(&t);
		char *timestamp = ctime(&t);
		memset(buf, 0, BUF_LEN);
		char levelStr[][16] = { "DEBUG", "WARNING", "ERROR", "CRITICAL" };
		len = sprintf(buf, "%s %s %s:%s:%d %s\n", timestamp, levelStr[level], prog, func, line, message)+1;
		ret = sendto(fd, buf, len, 0, (struct sockaddr *)&servaddr, addrlen);
	}
}

void ExitLog() {
	is_running = false; 
	close(fd);
}

void *recv_func(void *arg) {
	int fd = *(int*) arg;
	// remote address
	struct sockaddr_in remaddr;
	addrlen = sizeof(remaddr);
	int len;
	is_running = true;
	while (is_running) {
		// locks recvfrom function so only 1 thread can access at a time.
		pthread_mutex_lock(&mutex);
		// receives command from the LogServer to override level severity
		len = recvfrom(fd, buf, BUF_LEN, 0, (struct sockaddr * ) &remaddr, & addrlen);
		pthread_mutex_unlock(&mutex); // unlocks recvfrom function
		if (len < 0) {
			sleep(1);
		} else {
			// Sets the log level depending on the message received from the LogServer
			if (strncmp("Set Log Level=0", buf, 15)==0) {
				SetLogLevel(DEBUG);
			} else if (strncmp("Set Log Level=1", buf, 15)==0) {
				SetLogLevel(WARNING);
			} else if (strncmp("Set Log Level=2", buf, 15)==0) {
				SetLogLevel(ERROR);
			} else if (strncmp("Set Log Level=3", buf, 15)==0) {
				SetLogLevel(CRITICAL);
			}
		}
	}
	cout << "Exiting Logger" << endl;
	pthread_exit(NULL);
}
