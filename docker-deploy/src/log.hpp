#ifndef __LOG__
#define __LOG__

//#include"proxy.hpp"
//#include"handler.hpp"
//#include"request_parser.hpp"
//#include"response_parser.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <ostream>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "request_parser.hpp"
#include "response_parser.hpp"
using namespace std;
class log{
public:
	// from client to proxy
	void writeNewRequest(size_t thread_id, string request, char* ip){
		time_t requesttime;
		struct tm * realtime;
		time(&requesttime);
		realtime = localtime(&requesttime);
		ofstream file;
		file.open("/var/log/erss/proxy.log", ios::app|ios::out);
		file << thread_id << " : " << '"'<< request << '"' << " from " << ip << " @ " << asctime(realtime) << endl;
		file.close();
	}
	// from proxy to web server
	void writeRequestWebserver(size_t thread_id, RequestParser req_parser){
		string web_hostname = req_parser.getWebHostname();
		string request_content = req_parser.getFirstline();
		ofstream file;
		file.open("/var/log/erss/proxy.log", ios::app|ios::out);
		file << thread_id << " : " << '"'<< " Requesting " << request_content << " from " << web_hostname<< endl;
		file.close();
	}
	// from webserver to proxy
	void writeResponseWebserver(size_t thread_id, RequestParser req_parser,ResponseParser resp_parser){
		string web_hostname = req_parser.getWebHostname();
		string response_content = resp_parser.getFirstline();
		ofstream file;
		file.open("/var/log/erss/proxy.log", ios::app|ios::out);
		file << thread_id << " : " << '"'<< " Received " << response_content << " from " << web_hostname<< endl;
		file.close();	
	}
	// from proxy to client
	void writeResponseClient(size_t thread_id, ResponseParser resp_parser){
		//string web_hostname = client_response.getWebHostname();
		string response_content = resp_parser.getFirstline();
		ofstream file;
		file.open("/var/log/erss/proxy.log", ios::app|ios::out);
		file << thread_id << " : " << '"'<< " Responding " << response_content << endl;
		file.close();	
	}


};

#endif
