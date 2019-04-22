#include <iostream>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <unistd.h>
#include <vector>

using namespace std;

char socket_path[]="/tmp/centralhub";
volatile int signal_status;
const int BUF_LEN=100;


static void sigHandler(int sig);

int main()
{
    //Create the socket for inter-process communications
    struct sockaddr_un addr;
    char buf[BUF_LEN];
    int len;
    int master_fd,max_fd,rc;
    fd_set activefds;
    fd_set readfds;
    int ret, MAX_CLIENTS;
    vector<string> interface_name;
    string name;
	int signal_status = 1;
	
	sighandler_t err=signal(SIGINT, sigHandler);
	if (err==SIG_ERR) {
		cout << "Cannot create signal handler" << endl;
		cout << strerror(errno) << endl;
		return -1;
	}
	
	
    // Query user for number of interfaces
    cout << "Enter number of interfaces: ";
    cin >> MAX_CLIENTS;
    // Query user for each interface name
    for (int i = 0; i < MAX_CLIENTS; i++) {
	cout << "Enter interface name: ";
	cin >> name;
	interface_name.push_back(name);
    }
    int cl[MAX_CLIENTS];
	
	for(int i = 0; i < MAX_CLIENTS; i++) {
		cout << "starting the monitor for the interface " << interface_name.at(i) << endl;
		string program = "./intfMonitor " + interface_name.at(i) + "&";
		system(program.c_str());
	}
	
	
    //Create the socket
    memset(&addr, 0, sizeof(addr));
    if ( (master_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        cout << "server: " << strerror(errno) << endl;
        exit(-1);
    }

    addr.sun_family = AF_UNIX;
    //Set the socket path to a local socket file
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    unlink(socket_path);

    //Bind the socket to this local socket file
    if (bind(master_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        cout << "server: " << strerror(errno) << endl;
        close(master_fd);
        exit(-1);
    }


    cout<<"Waiting for the client..."<<endl;
    //Listen for a client to connect to this local socket file
    if (listen(master_fd, 5) == -1) {
        cout << "server: " << strerror(errno) << endl;
        unlink(socket_path);
        close(master_fd);
        exit(-1);
    }


	FD_ZERO(&readfds);
    FD_ZERO(&activefds);
    //Add the master_fd to the socket set
    FD_SET(master_fd, &activefds);
    max_fd = master_fd;

	
	// accept and create separate sockets for clients
    for(int i=0; i<MAX_CLIENTS; i++) {
		readfds = activefds;
        ret = select(max_fd+1, &readfds, NULL, NULL, NULL);
        if(FD_ISSET(master_fd, &readfds)){//incoming connection
            cl[i] = accept(master_fd, NULL, NULL);
            cout<<"server: incoming connection "<<cl[i]<<endl;
            FD_SET(cl[i], &activefds);
            if(max_fd<cl[i]) max_fd=cl[i];
		}
	}
	
	pid_t childpid;
	bool is_running = true;
	//fork of a child for each socket
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		childpid = fork();
		
		if (childpid == 0) {
			// write to client
			readfds = activefds;
			
			while (is_running) {
				signal(SIGINT, sigHandler);
				len = sprintf(buf, "isready?")+1;
				ret = write(cl[i], buf, len);
				
				if(ret==-1) {
				cout<<"server: Write Error"<<endl;
				cout<<strerror(errno)<<endl;
				}
				ret = select(max_fd+1, &readfds, NULL, NULL, NULL); 
				
				// read from client, wait til interface sends ready
				ret = read(cl[i], buf, BUF_LEN);
				if(ret==-1) {
					cout<<"server: Read Error"<<endl;
					cout<<strerror(errno)<<endl;
				}
				if(strcmp(buf, "ready")==0) {
					len = sprintf(buf, "monitor")+1;
					ret = write(cl[i], buf, len);
					if(ret==-1) {
						cout<<"server: Write Error"<<endl;
						cout<<strerror(errno)<<endl;
					}
				} else if (strcmp(buf, "done")==0) {
					sleep(1);
				} else if (strcmp(buf, "link down")==0) {
					len = sprintf(buf, "set link up")+1;
					ret = write(cl[i], buf, len);
					if(ret==-1) {
						cout<<"server: Write Error"<<endl;
						cout<<strerror(errno)<<endl;
					}
					sleep(1);
				} else if (strcmp(buf, "link up")==0) {
					sleep(1);
				}
				
				if (signal_status == 0) {
					is_running = false;
				}
			}
		exit(0);
		} 
	}
	if (signal_status == 0) {
		for (int i = 0; i < MAX_CLIENTS; ++i) {
			len = sprintf(buf, "shut down")+1;
			ret = write(cl[i], buf, len);
			if(ret==-1) {
				cout<<"server: Write Error"<<endl;
				cout<<strerror(errno)<<endl;
			}
			FD_CLR(cl[i], &readfds);
			close(cl[i]);
		}
	}
	
 
    close(master_fd);
    unlink(socket_path);
    return 0;
}

static void sigHandler(int sig) {
	switch (sig) {
			case SIGINT:
				signal_status = 0;
				break;
			default:
				cout << "sigHandler: Undefined signal" << endl;	
	}
}





