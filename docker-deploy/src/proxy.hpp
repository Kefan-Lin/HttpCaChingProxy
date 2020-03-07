#ifndef __PROXY__
#define __PROXY__
#include <cstdlib>
#include <iostream>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include "request_parser.hpp"
#include "response_parser.hpp"
using namespace std;
class Proxy{
private:
	//int webserver_fd;//fd for website
	int client_fd;// fd for user
	int listen_fd;//dummy fd

	struct addrinfo host_info;
	struct addrinfo *host_info_list = NULL;
	// struct addrinfo remote_info;
	// struct addrinfo *remote_info_list = NULL;

	const char * hostname;
	const char * proxy_port;//for user to send request
	//const char * webserver_port;//for proxy to send request to web
public:
	void getAddressInfo();
	void createSocketFd();
	void startListening();//set, bind, listen
	int acceptConnection();//get the socket fd of accept

	// void connectWebServer(const char *hostname, const char * port);//connect to webserver
	// void sendToFd(int fd,string to_send);
	int getClientFd(){return client_fd;}
	// int getListenFd(){return listen_fd;}
	// int getWebServerFd(){return webserver_fd;}
	// string receiveHeader(int fd); //recv the header from certain fd
	// string receiveContent(int fd,int content_length); //recv the content
	// int loopRecv(vector<char> & recv_buf,int fd);
	// void loopSend(vector<char> & recv_buf, int fd, int byte_size);

	// void handleGET(RequestParser &req_parser, size_t id);
	// void handlePOST(RequestParser &req_parser, size_t id);
	// void handleCONNECT(RequestParser &req_parser, size_t id);

	//Proxy():webserver_fd(-1),client_fd(-1),listen_fd(-1),hostname(NULL),proxy_port("4567"),webserver_port("80"){};
	Proxy():client_fd(-1),listen_fd(-1),hostname(NULL),proxy_port("12345"){};
	~Proxy(){
		//close(webserver_fd);
		close(client_fd);
		close(listen_fd);
		if(host_info_list){
			freeaddrinfo(host_info_list);
		}
		// if(remote_info_list){
		// 	freeaddrinfo(remote_info_list);
		// }
	}

};

#endif

