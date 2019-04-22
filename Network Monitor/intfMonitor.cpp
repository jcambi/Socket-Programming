#include <iostream>
#include <fstream>
#include <signal.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

using namespace std;

char socket_path[]="/tmp/centralhub";
bool is_running;
const int BUF_LEN=100;

int main(int argc, char* argv[])
{
    //Set up socket communications
    struct sockaddr_un addr;
    char buf[BUF_LEN];
    int len, ret;
    int fd,rc;

    int up_count = 0, down_count = 0;
    string interface_name = argv[1], operstate;
    string rx_bytes, rx_dropped, rx_errors, rx_packets;
    string tx_bytes, tx_dropped, tx_errors, tx_packets;
    ifstream infile;
    bool isRunning = true;
    string path = "/sys/class/net/" + interface_name;
	string same;
    cout<<"client("<<getpid()<<"): running..."<<endl;
    memset(&addr, 0, sizeof(addr));
    //Create the socket
    if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        cout << "client("<<getpid()<<"): "<<strerror(errno)<<endl;
        exit(-1);
    }

    addr.sun_family = AF_UNIX;
    //Set the socket path to a local socket file
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);

    //Connect to the local socket
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        cout << "client("<<getpid()<<"): " << strerror(errno) << endl;
        close(fd);
        exit(-1);
    }

    is_running=true;
    while (is_running) {
        ret = read(fd,buf,BUF_LEN);
        if(ret<0) {
            cout<<"client("<<getpid()<<"): error reading the socket"<<endl;
            cout<<strerror(errno)<<endl;
        }
		if(strcmp(buf, "isready?")==0) {
        	len = sprintf(buf, "ready")+1;
        	ret = write(fd, buf, len);
        	if(ret==-1) {
           		cout<<"client("<<getpid()<<"): Write Error"<<endl;
           		cout<<strerror(errno)<<endl;
        	}
        } else if(strcmp(buf, "monitor")==0) {
				// operstate
				infile.open(path + "/operstate");
				if (infile.is_open()) {
					infile >> operstate;
				} else {
					throw "File cannot be opened";
				}
				infile.close();
				if (operstate == "up")
					up_count = 1;
				else if (operstate == "down")
					down_count = 1;
				// rx_bytes
				infile.open(path + "/statistics/rx_bytes");
				if (infile.is_open()) {
				  infile >> rx_bytes;
				} else {
						throw "File cannot be opened";
				}
				infile.close();
				// rx_dropped
				infile.open(path + "/statistics/rx_dropped");
				if (infile.is_open()) {
					infile >> rx_dropped;
				} else {
					throw "File cannot be opened";
				}
				infile.close();
				// rx_errors
				infile.open(path + "/statistics/rx_errors");
				if (infile.is_open()) {
					infile >> rx_errors;
				} else {
					throw "File cannot be opened";
				}
				infile.close();
				// rx_packets
				infile.open(path + "/statistics/rx_packets");
				if (infile.is_open()) {
					infile >> rx_packets;
				} else {
					throw "File cannot be opened";
				}
				infile.close();
				// tx_bytes
				infile.open(path + "/statistics/tx_bytes");
				if (infile.is_open()) {
					infile >> tx_bytes;
				} else {
					throw "File cannot be opened";
				}
				infile.close();
				// tx_dropped
				infile.open(path + "/statistics/tx_dropped");
				if (infile.is_open()) {
					infile >> tx_dropped;
				} else {
					throw "File cannot be opened";
				}
				infile.close();
				// tx_errors
				infile.open(path + "/statistics/tx_errors");
				if (infile.is_open()) {
					infile >> tx_errors;
				} else {
					throw "File cannot be opened";
				}
				infile.close();
				// tx_packets
				infile.open(path + "/statistics/tx_packets");
				if (infile.is_open()) {
					infile >> tx_packets;
				} else {
					throw "File cannot be opened";
				}
				infile.close();

				cout << "Interface name:" << interface_name << " state:" << operstate << " up-count:" << up_count << " down-count:" << down_count << endl;
				cout <<	"rx_bytes:" << rx_bytes << " rx_dropped:" << rx_dropped << " rx_errors:" << rx_errors << " rx_packets:" << rx_packets << endl;
				cout << "tx_bytes:" << tx_bytes << " tx_dropped:" << tx_dropped << " tx_errors:" << tx_errors << " tx_packets:" << tx_packets << endl << endl;
				
				if (operstate == "down") {
					down_count++;
					len = sprintf(buf, "link down")+1;
					ret = write(fd, buf, len);
				} else {
					len = sprintf(buf, "done")+1;
					ret = write(fd, buf, len);
				}
		} else if (strcmp(buf, "set link up")==0) {
			up_count++;
			struct ifreq ifr;
			memset(&ifr, 0, sizeof(ifr));

			strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ);

			ifr.ifr_flags |= IFF_UP;
			ioctl(fd, SIOCSIFFLAGS, &ifr);
		} else if (strcmp(buf, "shut down")==0) {
			is_running = false;
		}
	}
	
    cout << "Terminating Interface Monitor..." << endl;
    unlink(socket_path);
    close(fd);
    return 0;
}
