// LogServer.cpp
// 
// April 15, 2019
//
// This is the central hub for all the logs sent by the logger of the process -- TravelSimulator


#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <net/if.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>

using namespace std;

const char IP[] = "127.0.0.1";
const int PORT = 1111;
const char logFile[] = "logFile.txt";
const int BUF_LEN = 4096;
char buf[BUF_LEN];
bool is_running;
struct sockaddr_in remaddr;
socklen_t addrlen;

pthread_mutex_t mutex;


// receive thread
void *recvThread(void *arg);


// handles ctrl+c interrupt
static void shutdownHandler(int sig) {
  if(sig == SIGINT) 
    is_running = false;
}

int main() {
  int fd, ret, len;
  struct sockaddr_in myaddr;
  addrlen = sizeof(remaddr);

  signal(SIGINT, shutdownHandler);

  fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
  if (fd < 0) {
    cout << "Error" << endl;
    return -1;
  }

  memset((char * ) & myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  ret = inet_pton(AF_INET, IP, & myaddr.sin_addr);

  if (ret == 0) {
    cout << "Error" << endl;
    close(fd);
    return -1;
  }
  myaddr.sin_port = htons(PORT);

  ret = bind(fd, (struct sockaddr * ) & myaddr, sizeof(myaddr));
  if (ret < 0) {
    cout << "Error" << endl;
    return -1;
  }

  // initialize mutex
  pthread_mutex_init(&mutex, NULL);
  pthread_t tid;

  // spawn a thread
  ret = pthread_create(&tid, NULL, recvThread, &fd);

  int selection = -1;
  while (selection != 0) {
    system("clear");
    cout << " 1. Set the log level" << endl;
    cout << " 2. Dump the log file here" << endl;
    cout << " 0. Shut down" << endl;
    cin >> selection;

    if (selection >= 0 && selection <= 2) {
      cout << endl;
      int level, fdIn, numRead;
      char key;

      if (selection == 0) {
        is_running = false;
      } else if (selection == 1) {
        cout << "Set Log level. 0-Debug, 1-Warning, 2-Error, 3-Critical: ";
        cin >> level;
        if (level < 0 || level > 3)
          cout << "Incorrect level" << endl;
        else {
	  // clear the buffer and send the command to override log level
          memset(buf, 0, BUF_LEN);
          len = sprintf(buf, "Set Log Level=%d", level) + 1;
          sendto(fd, buf, len, 0, (struct sockaddr * )&remaddr, addrlen);
        }
      } else if (selection == 2) {
        // open logFile as read only
        fdIn = open(logFile, O_RDONLY);
        numRead = 0;
        do {
          numRead = read(fdIn, buf, BUF_LEN);
          cout << buf;
        } while (numRead > 0);
        close(fdIn);
        cout << endl << "Press any key to continue: ";
        cin >> key;
      }
    }
  }

  pthread_join(tid, NULL);
  close(fd);
  return 0;
}

void *recvThread(void *arg) {
  int fd = *(int*) arg;
  // set file flags
  int openFlags = O_CREAT | O_WRONLY | O_TRUNC;
  // set file permissions
  mode_t filePerms = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  // open file as read and write
  int fdOut = open(logFile, openFlags, filePerms);
  int len;
  is_running = true;
  
  while (is_running) {
    // lock recvfrom function so only 1 thread can access at a time.
    pthread_mutex_lock( & mutex);
    // receive from data from logger
    len = recvfrom(fd, buf, BUF_LEN, 0, (struct sockaddr * ) &remaddr, &addrlen) - 1;
    pthread_mutex_unlock( & mutex); // unlock recvfrom function for other threads to access
    if (len < 0)
      sleep(1);
    else {
      // write data to logFile.txt
      write(fdOut, buf, len);
    }
  }
  cout << "Exiting" << endl;
  pthread_exit(NULL);
}
