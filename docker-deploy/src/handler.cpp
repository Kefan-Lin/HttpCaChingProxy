#include "handler.hpp"
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
#include<fstream>
#include<exception>
#include<assert.h>
#include"log.hpp"
using namespace std;

void Handler::connectWebServer(const char *hostname, const char * port){
  int status;
  struct addrinfo remote_info;
  struct addrinfo *remote_info_list = NULL;
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
  freeaddrinfo(remote_info_list);
}//connect to webserver

void Handler::sendToFd(int fd,string to_send){
  int total_size = to_send.size();
  if(total_size==0){
    return;
  }
  int nbytes = 0;
  while(true){
    nbytes += send(fd,to_send.substr(nbytes).c_str(),total_size-nbytes,0);
    //cout<<"Send "<<nbytes<<" Bytes"<<endl;
    if(nbytes==total_size){
      break;
    }
  }
  assert(nbytes==total_size);
}

string Handler::receiveHeader(int fd){
  vector<char> header(1, 0);
  int index = 0;
  int nbytes;
  while ((nbytes = recv(fd, &header.data()[index], 1 ,MSG_WAITALL)) > 0) {
    if (header.size() > 4) {
      if (header.back() == '\n' && header[header.size() - 2] == '\r' &&
          header[header.size() - 3] == '\n' &&
          header[header.size() - 4] == '\r') {
          break;
      }
    }
    header.resize(header.size() + 1);
    index += nbytes;
  }
  string ans(header.begin(),header.end());
  return ans;
}

string Handler::receiveContent(int fd,int content_length){
  //cout<<"Initial Content-length = "<<content_length<<endl;
  int original_length = content_length;
  vector<char> content(1,0);
  int index = 0;
  int nbytes = 0;
  //while ((nbytes = recv(fd, &content.data()[index], 1 ,MSG_WAITALL)) > 0){
  while (true){
    nbytes = recv(fd, &content.data()[index], 1 ,MSG_WAITALL);
    if(nbytes==0){
      break;
    }
    if(nbytes==-1){
      continue;
    }
    index += nbytes;
    content_length -= nbytes;
    if(content_length==0){
      break;
    }
    content.resize(content.size()+1);
  }
  //cout<<"After recv, content_length = "<<content_length<<", nbytes = "<<nbytes<<endl;
  //assert(content_length==0);// content_length !=0
  string ans(content.begin(),content.end());
  if((int)ans.size()!=original_length){
    logic_error e("Dirty content!");
    throw exception(e);
  }
  return ans;
}

void Handler::recvEntireResponse(ResponseParser &resp_parser,int client_fd){
  if(resp_parser.getContentLength()!="0"){ //content length
    stringstream ss;
    ss<<resp_parser.getContentLength();
    int response_content_len;
    ss>>response_content_len;
    string content = receiveContent(getWebServerFd(),response_content_len);
    resp_parser.addContent(content);
    //sendToFd(client_fd,resp_parser.getResponse());
  }
  else if(resp_parser.getIsChunked()){  //chunked
    string content = recvChunkedContent(webserver_fd);
    //cout<<"****nmsl****"<<endl;
    //cout<<content<<endl;
    resp_parser.addContent(content);
    //sendToFd(client_fd,resp_parser.getResponse());
  }
  else{
    //sendToFd(client_fd,resp_parser.getResponse());
    vector<char> recv_buf(1024,0);
    int nbytes = loopRecv(recv_buf,getWebServerFd());
    if(nbytes > 0){
      string content(recv_buf.begin(),recv_buf.end());
      resp_parser.addContent(content);
    }
  }
  //sendToFd(client_fd,resp_parser.getResponse());
}

void Handler::handleGET(int client_fd, RequestParser req_parser, size_t id,Cache & mycache){//should this be refrence?
  ofstream file;
  file.open("/var/log/erss/proxy.log", ios::app|ios::out);
  if(req_parser.getContentLength()!="0"){
    //cout<<"*****Enter recv normal content*****"<<endl;
    stringstream ss;
    ss<<req_parser.getContentLength();
    int content_length;
    ss>>content_length;
    string content = receiveContent(client_fd,content_length);
    req_parser.addContent(content);
  }
  string request_line = req_parser.getFirstline();
  ResponseParser * resp_ptr = mycache.get(request_line);
  // not in cache
  if(resp_ptr == NULL){
    file<<id<<": not in cache"<<endl;
    //cout<<"****Into not in cache****"<<id<<endl;
    //connect server
    connectWebServer(req_parser.getWebHostname().c_str(),req_parser.getWebPort().c_str());
    //send entire request
    sendToFd(getWebServerFd(),req_parser.getRequest());
    file<<id<<": Requesting \""<<req_parser.getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
    //recv server response's header
    string response_header = receiveHeader(getWebServerFd());
    
    ResponseParser resp_parser(response_header);

    resp_parser.parseHeader();
    file<<id<<": Received \""<<resp_parser.getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
      //if cacheable, put into cache
    recvEntireResponse(resp_parser,client_fd);
    if(resp_parser.getCacheable()==true){
      mycache.put(request_line,resp_parser);
    }
    //send back to client
    file<<id<<": Responding \""<<resp_parser.getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
    sendToFd(client_fd,resp_parser.getResponse());
    file.close();
    return;
  }
  else{
    assert(resp_ptr!=NULL);
    //cout<<"$$$$$$$$$$$$$response in cache: \n"<<resp_ptr->getResponse()<<endl;
    time_t cur_time = time(NULL);
    time_t exp_time = 0;
    if(resp_ptr->getExpire().size()!=0 && stoi(resp_ptr->getExpire())>0 ){ // has expire info
      //cout<<"++++++++++++++++"<<resp_ptr->getExpire()<<"++++++++++++"<<endl;
      struct tm expire_time;
      memset(&expire_time,0,sizeof(struct tm));
      strptime(resp_ptr->getExpire().c_str(),"%a, %d %b %Y %H:%M:%S GMT",&expire_time);
      exp_time = mktime(&expire_time);
      //cout<<"exp_time = "<<exp_time<<endl;
    }
    if(resp_ptr->getAge()!="" && resp_ptr->getDate()!=""){
      struct tm create_time;
      memset(&create_time,0,sizeof(struct tm));
      strptime(resp_ptr->getDate().c_str(),"%a, %d %b %Y %H:%M:%S GMT",&create_time);
      time_t cre_time = mktime(&create_time);
      //cout<<"@@@@@@@getAge():\n"<<resp_ptr->getAge()<<endl;
      //cout<<"@@@@@@@getDate():\n"<<resp_ptr->getDate()<<endl;
      exp_time = cre_time + stoi(resp_ptr->getAge());
    }
    if(exp_time!=0&&exp_time<cur_time){
      //cout<<"****Into expire****"<<id<<endl;
      //1. expire, 
      //connect server
      file<<id<<": in cache, but expired at "<<stoi(resp_ptr->getExpire())<<endl;
      string new_request = revalidate(req_parser,*resp_ptr);
      connectWebServer(req_parser.getWebHostname().c_str(),req_parser.getWebPort().c_str());
      //recv server new response
      sendToFd(getWebServerFd(),new_request);
      size_t pos = new_request.find("\r\n");
      string new_firstline = new_request.substr(0,pos);
      file<<id<<": Requesting \""<<new_firstline<<"\" from "<<req_parser.getWebHostname()<<endl;
      string response_header = receiveHeader(getWebServerFd());
      ResponseParser new_resp_parser(response_header);
      
      new_resp_parser.parseHeader();
      file<<id<<": Received \""<<new_resp_parser.getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
      string resp_firstline = new_resp_parser.getFirstline();
        //if 304, get from cache and send
      if(resp_firstline.find("304")!=string::npos){ //if 304
        sendToFd(client_fd,resp_ptr->getResponse());
        file<<id<<": Responding \""<<resp_ptr->getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
        file.close();
        return;
      }
        //else, send new response
      recvEntireResponse(new_resp_parser,client_fd);
      sendToFd(client_fd,new_resp_parser.getResponse());
      file<<id<<": Responding \""<<new_resp_parser.getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
      file.close();
      return;
    }
    //2. revalidate
    if(resp_ptr->getMustRevalidate()==true){
      //cout<<"****Into revalidate****"<<id<<endl;
      //connect server
      file<<id<<": in cache, requires validation"<<endl;
      string new_request = revalidate(req_parser,*resp_ptr);
      connectWebServer(req_parser.getWebHostname().c_str(),req_parser.getWebPort().c_str());
      sendToFd(getWebServerFd(),new_request);
      size_t pos = new_request.find("\r\n");
      string new_firstline = new_request.substr(0,pos);
      file<<id<<": Requesting \""<<new_firstline<<"\" from "<<req_parser.getWebHostname()<<endl;
      //recv server new response
      string response_header = receiveHeader(getWebServerFd());
      ResponseParser new_resp_parser(response_header);
      new_resp_parser.parseHeader();
      string resp_firstline = new_resp_parser.getFirstline();
        //if 304, get from cache and send
      if(resp_firstline.find("304")!=string::npos){ //if 304
        sendToFd(client_fd,resp_ptr->getResponse());
        file<<id<<": Responding \""<<resp_ptr->getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
        file.close();
        return;
      }
        //else, send new response
      recvEntireResponse(new_resp_parser,client_fd);
      sendToFd(client_fd,new_resp_parser.getResponse());
      file<<id<<": Responding \""<<new_resp_parser.getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
      file.close();
      return;
    }
    //3. normal
    file<<id<<": in cache, valid"<<endl;
    //cout<<"****Into normal****"<<id<<endl;
      //get from cache and send back
    sendToFd(client_fd,resp_ptr->getResponse());
    file<<id<<": Responding \""<<resp_ptr->getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
    file.close();
    return;
  }



  // connectWebServer(req_parser.getWebHostname().c_str(),req_parser.getWebPort().c_str());
  // sendToFd(getWebServerFd(),req_parser.getRequest());
  // // sendToFd(getWebServerFd(),req_parser.getHeader());
  // // sendToFd(getWebServerFd(),req_parser.getContent());
  
  // string response_header = receiveHeader(getWebServerFd());
  // cout<<"This is response_header\n"<<response_header;
  // ResponseParser resp_parser(response_header);
  // resp_parser.parseHeader();

  // // 

  // sendEntireResponse(resp_parser,client_fd);

  // if(resp_parser.getContentLength()!="0"){ //content length
  //   stringstream ss;
  //   ss<<resp_parser.getContentLength();
  //   int response_content_len;
  //   ss>>response_content_len;
  //   string content = receiveContent(getWebServerFd(),response_content_len);
  //   resp_parser.addContent(content);
  //   //sendToFd(client_fd,resp_parser.getResponse());
  // }
  // else if(resp_parser.getIsChunked()){  //chunked
  //   string content = recvChunkedContent(webserver_fd);
  //   //cout<<"****nmsl****"<<endl;
  //   //cout<<content<<endl;
  //   resp_parser.addContent(content);
  //   //sendToFd(client_fd,resp_parser.getResponse());
  // }
  // else{
  //   //sendToFd(client_fd,resp_parser.getResponse());
  //   vector<char> recv_buf(1024,0);
  //   int nbytes = loopRecv(recv_buf,getWebServerFd());
  //   if(nbytes > 0){
  //     string content(recv_buf.begin(),recv_buf.end());
  //     resp_parser.addContent(content);
  //   }
  // }
  // sendToFd(client_fd,resp_parser.getResponse());
}

void Handler::handlePOST(int client_fd,RequestParser req_parser, size_t id){ //should this be refrence?
  ofstream file;
  file.open("/var/log/erss/proxy.log", ios::app|ios::out);
  if(req_parser.getContentLength()!="0"){ //normal content
    stringstream ss;
    ss<<req_parser.getContentLength();
    int content_length;
    ss>>content_length;
    string content = receiveContent(client_fd,content_length);
    req_parser.addContent(content);
    //cout<<"After addContent, content is:\n"<<req_parser.getContent()<<endl;
  }
  else if(req_parser.getIsChunked()){ //chunk content
    string content = recvChunkedContent(client_fd);
    req_parser.addContent(content);
  }
  else{
    vector<char> recv_buf(1024,0);
    int nbytes = loopRecv(recv_buf,client_fd);
    if(nbytes > 0){
      string content(recv_buf.begin(),recv_buf.end());
      req_parser.addContent(content);
    }
  }

  //cout<<"*******the post request is "<<req_parser.getRequest()<<"***********"<<endl;
  connectWebServer(req_parser.getWebHostname().c_str(),req_parser.getWebPort().c_str());
  
  //sendToFd(getWebServerFd(),req_parser.getHeader());
  //sendToFd(getWebServerFd(),req_parser.getContent());
  sendToFd(getWebServerFd(),req_parser.getRequest());
  file<<id<<": Requesting \""<<req_parser.getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
  //below is response 
  string response_header = receiveHeader(getWebServerFd());
  ResponseParser resp_parser(response_header);
  resp_parser.parseHeader();
  file<<id<<": Received \""<<resp_parser.getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
  
  if(resp_parser.getContentLength()!="0"){
    stringstream ss;
    ss<<resp_parser.getContentLength();
    int response_content_len;
    ss>>response_content_len;
    string content = receiveContent(getWebServerFd(),response_content_len);
    //cout<<"*******3"<<endl;
    resp_parser.addContent(content);
    // sendToFd(client_fd,resp_parser.getHeader());
    // sendToFd(client_fd,resp_parser.getContent());
    //cout<<"*******4"<<endl;
  }
  else if(resp_parser.getIsChunked()){
    string content = recvChunkedContent(getWebServerFd());
    resp_parser.addContent(content);
    // sendToFd(client_fd,resp_parser.getHeader());
    // sendToFd(client_fd,resp_parser.getContent());
  }
  else{
    vector<char> recv_buf(1024,0);
    int nbytes = loopRecv(recv_buf,getWebServerFd());
    if(nbytes > 0){
      string content(recv_buf.begin(),recv_buf.end());
      resp_parser.addContent(content);
    }
  }

  sendToFd(client_fd,resp_parser.getResponse());
  file<<id<<": Responding \""<<resp_parser.getFirstline()<<"\" from "<<req_parser.getWebHostname()<<endl;
  file.close();
  // sendToFd(client_fd,resp_parser.getHeader());
  // sendToFd(client_fd,resp_parser.getContent());
  
}

void Handler::handleCONNECT(int client_fd,RequestParser req_parser, size_t id){//should this be refrence?
  //cout<<"In CONNECT, web hostname = "<<req_parser.getWebHostname()<<" ,web port = "<<req_parser.getWebPort();
  if(req_parser.getWebHostname()==""){
    return;
  }
  connectWebServer(req_parser.getWebHostname().c_str(),req_parser.getWebPort().c_str());
  string success_msg = "HTTP/1.1 200 OK\r\n\r\n";
  sendToFd(client_fd,success_msg);
  
  int status = 0;
  fd_set readfds;
  int fdmax = max(client_fd,getWebServerFd());
  int fds[2] = {client_fd,getWebServerFd()};
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
    // if(status == 0){ //2.27 4:40 pm
    //   return;
    //   //continue;
    // }

    //bool destroy_tunnel = false // if true, destroy the CONNECT tunnel
    for(size_t i = 0; i < 2; i++){
      if(FD_ISSET(fds[i],&readfds)){
        vector<char> recv_buf(1024,0);
        int nbytes = loopRecv(recv_buf, fds[i]);//get the data recieved
        //cout<<"receive success!"<<endl;
        if(nbytes == 0){ // destroy tunnel
          ofstream file;
          file.open("/var/log/erss/proxy.log", ios::app|ios::out);
          file<<id<<"Tunnel closed"<<endl;
          return;
        }
        loopSend(recv_buf, fds[1-i], nbytes);//send all data out
        //cout<<"send success!"<<endl;
        break;
      }
    }
  }
}

string Handler::recvChunkedContent(int fd){
  vector<char> buf(1, 0);
  int index = 0;
  int nbytes;
  //while ((nbytes = recv(fd, &buf.data()[index], 1 ,MSG_WAITALL)) >= 0) {
  while (true) {
    nbytes = recv(fd, &buf.data()[index], 1 ,MSG_WAITALL);
    if(nbytes==0){
      break;
    }
    if(nbytes==-1){
      continue;
    }
    if (buf.size() > 5) {
      if (buf.back() == '\n' && buf[buf.size() - 2] == '\r' &&
          buf[buf.size() - 3] == '\n' &&
          buf[buf.size() - 4] == '\r' && buf[buf.size() - 5] == '0') {
          break;
      }
    }
    buf.resize(buf.size() + 1);
    index += nbytes;
  }
  string ans(buf.begin(),buf.end());
  return ans;
}
string Handler::revalidate(RequestParser req_parser, ResponseParser saved_response){
  string request = req_parser.getRequest();
  string etag = saved_response.getEtag();
  if(etag.size()>0){
    request = request.substr(0,request.find("\r\n\r\n")+2);
    request += "If-None-Match: " + etag + "\r\n\r\n";
  }
  string last_modified = saved_response.getLastModified();
  if(last_modified.size()>0){
    request = request.substr(0,request.find("\r\n\r\n")+2);
    request += "If-Modified-Since" + last_modified + "\r\n\r\n";
  }
  return request;
}

int Handler::loopRecv(vector<char> & recv_buf,int fd){
  //cout<<"+++++Enter loopRecv"<<endl;
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
  recv_buf.resize(nbytes);
  return nbytes;
}

//continue send date until all is sent
void Handler::loopSend(vector<char> & recv_buf, int fd, int byte_size){
  //cout<<"+++++Enter loopSend"<<endl;
  int byte_sent = 0;
  while(byte_sent < byte_size){
    byte_sent += send(fd, &recv_buf.data()[byte_sent], byte_size - byte_sent, 0);
  }
  //cout<<"In loop send, byte_sent = "<<byte_sent<<", total_byte = "<<byte_size<<endl;
  //assert(byte_sent == byte_size);
} 