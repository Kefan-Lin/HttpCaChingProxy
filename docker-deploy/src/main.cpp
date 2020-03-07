#include"proxy.hpp"
#include"handler.hpp"
#include"request_parser.hpp"
#include"response_parser.hpp"
#include<stdlib.h>
#include<cstdlib>
#include<sstream>
#include<exception>
#include<thread>
#include<csignal>
#include "log.hpp"
using namespace std;

log mylog;
Cache mycache;

void workHorse(int client_fd, size_t id){
	cout<<"accept connection success! Thread id = "<<id<<endl;
	Handler handler;
	string header = handler.receiveHeader(client_fd);
	RequestParser req_parser(header);
	req_parser.parseHeader();
	cout<<"++++Thread id = "<<id<<"++++"<<endl;
	cout<<header<<endl;
	cout<<"++++Thread id = "<<id<<" Header ends++++"<<endl;

	//log mylog = new log();//create new log
	//log mylog;

	//get my ip
	struct hostent *host_entry;
	char myhostname[512];
	memset(myhostname,0,512);
  	gethostname(myhostname, sizeof(myhostname));
  	host_entry = gethostbyname(myhostname);
  	char *ip = inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));


	// determine the value of req_parser.getMethod();
	//GET
	if(req_parser.getMethod()=="GET"){
		mylog.writeNewRequest(id, req_parser.getFirstline(), ip);
		cout<<"***enter GET*****"<<endl;
		handler.handleGET(client_fd,req_parser,id,mycache);
	}
	//POST
	else if(req_parser.getMethod()=="POST"){
		mylog.writeNewRequest(id, req_parser.getFirstline(), ip);
		cout<<"***enter POST*****"<<endl;
		handler.handlePOST(client_fd,req_parser,id);
	}
	//CONNECT
	else if(req_parser.getMethod()=="CONNECT"){
	  	mylog.writeNewRequest(id, req_parser.getFirstline(), ip);
		cout<<"***enter CONNECT*****"<<endl;
		handler.handleCONNECT(client_fd,req_parser,id);
	}
	close(client_fd);
	// error
	//exit(EXIT_SUCCESS);
}
// void test(Proxy &proxy){
// 	proxy.acceptConnection();
// 	string header = proxy.receiveHeader(proxy.getClientFd());
// 	//cout<<"This is header:\n"<<header<<endl;
// 	RequestParser req_parser(header);
// 	req_parser.parseHeader();
// 	if(req_parser.getContentLength().size()!=0){
// 		stringstream ss;
// 		ss<<req_parser.getContentLength();
// 		int content_length;
// 		ss>>content_length;
// 		string content = proxy.receiveContent(proxy.getClientFd(),content_length);
// 		req_parser.addContent(content);
// 		cout<<"After addContent, content is:\n"<<req_parser.getContent()<<endl;
// 	}
// 	//cout<<"@@@@@@@@"<<req_parser.getWebHostname()<<"@@@@@@@@"<<req_parser.getWebPort()<<"@@@@@@@@"<<endl;
// 	proxy.connectWebServer(req_parser.getWebHostname().c_str(),req_parser.getWebPort().c_str());
// 	cout<<"Connect web server success!"<<endl;
// 	proxy.sendToFd(proxy.getWebServerFd(),req_parser.getHeader());
// 	proxy.sendToFd(proxy.getWebServerFd(),req_parser.getContent());
// 	cout<<"Send to web server success!"<<endl;
// 	// 2.24 lkf test 
// 	string response_header = proxy.receiveHeader(proxy.getWebServerFd());
// 	cout<<"This is response_header\n"<<response_header;
// 	ResponseParser resp_parser(response_header);
// 	cout<<"*******1"<<endl;
// 	resp_parser.parseHeader();
// 	cout<<"*******2"<<endl;
// 	if(resp_parser.getContentLength().size()!=0){
// 		stringstream ss;
// 		ss<<resp_parser.getContentLength();
// 		int response_content_len;
// 		ss>>response_content_len;
// 		string content = proxy.receiveContent(proxy.getWebServerFd(),response_content_len);
// 		cout<<"*******3"<<endl;
// 		resp_parser.addContent(content);
// 		cout<<"*******4"<<endl;
// 	}
// 	proxy.sendToFd(proxy.getClientFd(),resp_parser.getHeader());
// 	cout<<"*******5"<<endl;
// 	proxy.sendToFd(proxy.getClientFd(),resp_parser.getContent());
// 	cout<<"*******6"<<endl;
// }
int main(){
	Proxy proxy;
	// create a cache
	proxy.getAddressInfo();
	proxy.createSocketFd();
	proxy.startListening();//set, bind, listen
	//****Test start
	//test(proxy);
	//****Test end
	size_t id = 0;
	signal(SIGPIPE,SIG_IGN);
	while(true){

		//cout<<"****Before acceptConnection****"<<"Thread id = "<<id<<endl;
		
		int client_fd = proxy.acceptConnection(); 

		//cout<<"****After acceptConnection****"<<"Thread id = "<<id<<endl;
		if(client_fd==-1){
			cout<<"Thread id = "<<id<<", acceptConnection failed!"<<endl;
			continue;
		}
		try{
			//thread new_thread(workHorse,ref(proxy),id); // lack of cache
			thread new_thread(workHorse,client_fd,id); // lack of cache
			id++;
			new_thread.detach();
			//sleep(1);
			//cout<<"Thread id = "<<id<<" detached!"<<endl;
		}
		catch(exception & e){
			cout<<e.what()<<endl;
		}
	}

	return 0;
}
