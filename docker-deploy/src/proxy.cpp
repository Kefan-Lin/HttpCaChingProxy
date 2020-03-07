#include "proxy.hpp"
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include<sys/time.h>
#include<time.h>
#include<sstream>
#include<exception>
#include<assert.h>
using namespace std;
void Proxy::getAddressInfo(){
	int status;
 	//const char *hostname = NULL;
  	//const char *port     = "4444";
 	memset(&host_info, 0, sizeof(host_info));
	host_info.ai_family   = AF_UNSPEC;
  	host_info.ai_socktype = SOCK_STREAM;
  	host_info.ai_flags    = AI_PASSIVE;
	status = getaddrinfo(hostname, proxy_port, &host_info, &host_info_list);
	if (status != 0) {
    	cerr << "Error: cannot get address info for host" << endl;
    //	cerr << "  (" << hostname << "," << port << ")" << endl;
    	exit(EXIT_FAILURE);
  	}
}

void Proxy::createSocketFd(){
	listen_fd = socket(host_info_list->ai_family, 
					host_info_list->ai_socktype, 
					host_info_list->ai_protocol);
	if (listen_fd == -1) {
    	cerr << "Error: cannot create socket" << endl;
    //	cerr << "  (" << hostname << "," << port << ")" << endl;
    	exit(EXIT_FAILURE);
  }
}

void Proxy::startListening(){
	int status;
	int yes = 1;
	status = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	status = bind(listen_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
	if (status == -1) {
    	cerr << "Error: cannot bind socket" << endl;
    //	cerr << "  (" << hostname << "," << port << ")" << endl;
    	exit(EXIT_FAILURE);
	} //if
	status = listen(listen_fd, 100);
	if (status == -1) {
    	cerr << "Error: cannot listen on socket" << endl; 
    //	cerr << "  (" << hostname << "," << port << ")" << endl;
    	exit(EXIT_FAILURE);
	} //if
}//set, bind, listen

int Proxy::acceptConnection(){

	struct sockaddr_storage socket_addr;
	socklen_t socket_addr_len = sizeof(socket_addr);
  cout<<"***Before acceptConnection***"<<endl;
	int client_fd = accept(listen_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  //cout<<"***After acceptConnection***"<<endl;
	if (client_fd == -1) {
    	cerr << "Error: cannot accept connection on socket" << endl;
    	exit(EXIT_FAILURE);
	} //if
  return client_fd;
}//get the socket fd of accept

/*  2.26 move to Handler

void Proxy::connectWebServer(const char *hostname, const char * port){
  int status;
  memset(&remote_info, 0, sizeof(remote_info));
  remote_info.ai_family   = AF_UNSPEC;
  remote_info.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(hostname, port, &remote_info, &remote_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for webserver" << endl;
    //cerr << "  (" << hostname << "," << port << ")" << endl;
    exit(EXIT_FAILURE);
  } //if

  webserver_fd = socket(remote_info_list->ai_family, 
		     remote_info_list->ai_socktype, 
		     remote_info_list->ai_protocol);
  if (webserver_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    //cerr << "  (" << hostname << "," << port << ")" << endl;
    exit(EXIT_FAILURE);
  } //if
  cout << "Connecting to " << hostname << " on port " << port << "..." << endl;
  
  status = connect(webserver_fd, remote_info_list->ai_addr, remote_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    //cerr << "  (" << hostname << "," << port << ")" << endl;
    exit(EXIT_FAILURE);
  } //if
}//connect to webserver

void Proxy::sendToFd(int fd,string to_send){
  int total_size = to_send.size();
  if(total_size==0){
    return;
  }
  int nbytes = 0;
  while(true){
    nbytes += send(fd,to_send.substr(nbytes).c_str(),total_size-nbytes,0);
    cout<<"Send "<<nbytes<<" Bytes"<<endl;
    if(nbytes==total_size){
      break;
    }
  }
  //assert(nbytes==total_size);
}

string Proxy::receiveHeader(int fd){
  vector<char> header(1, 0);
  int index = 0;
  int nbytes;
  while ((nbytes = recv(fd, &header.data()[index], 1 ,MSG_WAITALL)) > 0) {
    if (header.size() > 4) {
      if (header.back() == '\n' && header[header.size() - 2] == '\r' &&
          header[header.size() - 3] == '\n' &&
          header[header.size() - 4] == '\r') {
          // std::cout << "GOT HEADER!" << std::endl;
          //find = 1;
          break;
      }
    }
    header.resize(header.size() + 1);
    index += nbytes;
  }
  string ans(header.begin(),header.end());
  return ans;
}

string Proxy::receiveContent(int fd,int content_length){
  cout<<"Initial Content-length = "<<content_length<<endl;
  vector<char> content(1,0);
  int index = 0;
  int nbytes = 0;
  while ((nbytes = recv(fd, &content.data()[index], 1 ,MSG_WAITALL)) > 0){
    content.resize(content.size()+1);
    index += nbytes;
    content_length -= nbytes;
    if(content_length==0){
      break;
    }
  }
  cout<<"After recv, content_length = "<<content_length<<", nbytes = "<<nbytes<<endl;
  //assert(content_length==0);// content_length !=0
  string ans(content.begin(),content.end());
  return ans;
}

void Proxy::handleGET(RequestParser &req_parser, size_t id){
  if(req_parser.getContentLength().size()!=0){
    stringstream ss;
    ss<<req_parser.getContentLength();
    int content_length;
    ss>>content_length;
    string content = receiveContent(getClientFd(),content_length);
    req_parser.addContent(content);
    cout<<"After addContent, content is:\n"<<req_parser.getContent()<<endl;
  }
  connectWebServer(req_parser.getWebHostname().c_str(),req_parser.getWebPort().c_str());
  cout<<"Connect web server success!"<<endl;
  sendToFd(getWebServerFd(),req_parser.getHeader());
  sendToFd(getWebServerFd(),req_parser.getContent());
  cout<<"Send to web server success!"<<endl;
  string response_header = receiveHeader(getWebServerFd());
  cout<<"This is response_header\n"<<response_header;
  ResponseParser resp_parser(response_header);
  resp_parser.parseHeader();
  if(resp_parser.getContentLength().size()!=0){
    stringstream ss;
    ss<<resp_parser.getContentLength();
    int response_content_len;
    ss>>response_content_len;
    string content = receiveContent(getWebServerFd(),response_content_len);
    resp_parser.addContent(content);
  }
  sendToFd(getClientFd(),resp_parser.getHeader());
  sendToFd(getClientFd(),resp_parser.getContent());
}

void Proxy::handlePOST(RequestParser &req_parser, size_t id){
  if(req_parser.getContentLength().size()!=0){
    stringstream ss;
    ss<<req_parser.getContentLength();
    int content_length;
    ss>>content_length;
    string content = receiveContent(getClientFd(),content_length);
    req_parser.addContent(content);
    cout<<"After addContent, content is:\n"<<req_parser.getContent()<<endl;
  }
  //cout<<"@@@@@@@@"<<req_parser.getWebHostname()<<"@@@@@@@@"<<req_parser.getWebPort()<<"@@@@@@@@"<<endl;
  connectWebServer(req_parser.getWebHostname().c_str(),req_parser.getWebPort().c_str());
  cout<<"Connect web server success!"<<endl;
  sendToFd(getWebServerFd(),req_parser.getHeader());
  sendToFd(getWebServerFd(),req_parser.getContent());
  cout<<"Send to web server success!"<<endl;
  string response_header = receiveHeader(getWebServerFd());
  cout<<"This is response_header\n"<<response_header;
  ResponseParser resp_parser(response_header);
  //cout<<"*******1"<<endl;
  resp_parser.parseHeader();
  //cout<<"*******2"<<endl;
  if(resp_parser.getContentLength().size()!=0){
    stringstream ss;
    ss<<resp_parser.getContentLength();
    int response_content_len;
    ss>>response_content_len;
    string content = receiveContent(getWebServerFd(),response_content_len);
    //cout<<"*******3"<<endl;
    resp_parser.addContent(content);
    //cout<<"*******4"<<endl;
  }
  sendToFd(getClientFd(),resp_parser.getHeader());
  //cout<<"*******5"<<endl;
  sendToFd(getClientFd(),resp_parser.getContent());
  //cout<<"*******6"<<endl;
}
void Proxy::handleCONNECT(RequestParser &req_parser, size_t id){
  //cout<<"In CONNECT, web hostname = "<<req_parser.getWebHostname()<<" ,web port = "<<req_parser.getWebPort();
  if(req_parser.getWebHostname()==""){
    return;
  }
  connectWebServer(req_parser.getWebHostname().c_str(),req_parser.getWebPort().c_str());
  //cout<<"Connect web server success! id = "<<id<<endl;
  string success_msg = "HTTP/1.1 200 OK\r\n\r\n";
  sendToFd(getClientFd(),success_msg);
  int status = 0;
  fd_set readfds;
  int fdmax = max(getClientFd(),getWebServerFd());
  int fds[2] = {getClientFd(),getWebServerFd()};
  while(true){
    FD_ZERO(&readfds);
    FD_SET(fds[0],&readfds);
    FD_SET(fds[1],&readfds);
    
    status = select(fdmax+1,&readfds,NULL,NULL,NULL);
    if(status == -1){
      cerr<<"Select failed!"<<endl;
      logic_error e("select failed!");
      throw exception(e);
    }
    if(status == 0){
      return;
    }

    //bool destroy_tunnel = false // if true, destroy the CONNECT tunnel
    for(size_t i = 0; i < 2; i++){
      if(FD_ISSET(fds[i],&readfds)){
        vector<char> recv_buf(1024,0);
        int nbytes = loopRecv(recv_buf, fds[i]);//get the data recieved
        //cout<<"receive success!"<<endl;
        if(nbytes == 0){ // destroy tunnel
          return;
        }
        loopSend(recv_buf, fds[1-i], nbytes);//send all data out
        //cout<<"send success!"<<endl;
        break;
      }
    }
  }
}

int Proxy::loopRecv(vector<char> & recv_buf,int fd){
  int nbytes = 0; //total bytes received
  int byte_recv = 0; // bytes received in each iteration
  int prev_recv_size = 0;
  while((byte_recv = recv(fd, &recv_buf.data()[nbytes],(int)recv_buf.size()-prev_recv_size,0))==(int)recv_buf.size()){
    prev_recv_size = (int)recv_buf.size();
    recv_buf.resize(2*recv_buf.size());
    nbytes += byte_recv;
    continue;
  }
  nbytes+=byte_recv;
  return nbytes;
}

//continue send date until all is sent
void Proxy::loopSend(vector<char> & recv_buf, int fd, int byte_size){
  int byte_sent = 0;
  while(byte_sent < byte_size){
    byte_sent += send(fd, &recv_buf.data()[byte_sent], byte_size - byte_sent, 0);
  }
  //cout<<"In loop send, byte_sent = "<<byte_sent<<", total_byte = "<<byte_size<<endl;
  //assert(byte_sent == byte_size);
} 

   2.26 end      */























