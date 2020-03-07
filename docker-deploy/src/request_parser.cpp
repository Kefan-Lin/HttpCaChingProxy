#include "request_parser.hpp"

void RequestParser::parseHeader(){
	//find the first line
	size_t end_of_lineone = header.find_first_of("\r\n");
	first_line = header.substr(0, end_of_lineone);
	//find the end of first line,find the first space on first line, get the method
	size_t line_one_space = header.find_first_of(" ");
	method = header.substr(0,line_one_space);

	if(method=="CONNECT"){
		web_port = "443";
	}

	//get the url following after method
	size_t url_pos = line_one_space + 1;
	size_t url_end = header.find_first_of(" ", url_pos);
	size_t url_len = url_end - url_pos;
	url = header.substr(url_pos, url_len);

	//get hostname(and port number if it has)
	if(header.find("Host:")!=string::npos){
		size_t host_pos = header.find("Host:") + 6;
		size_t host_end = header.find_first_of("\r\n",host_pos);
		size_t host_len = host_end - host_pos;
		web_hostname = header.substr(host_pos, host_len);
	}
	// size_t host_pos = header.find("Host:") + 6;
	// size_t host_end = header.find_first_of("\r\n",host_pos);
	// size_t host_len = host_end - host_pos;
	// web_hostname = header.substr(host_pos, host_len);

	//see if hostname contains ':port_num'
	if(web_hostname.find(":")!=string::npos){
		size_t port_pos = web_hostname.find(":") + 1;
		//size_t port_end = host_end;
		//size_t port_len = port_end - port_pos;
		//web_port = web_hostname.substr(port_pos, port_len);
		web_port = web_hostname.substr(port_pos);
		//host_len = host_len - port_len;
		web_hostname = web_hostname.substr(0, web_hostname.find(":"));
	}
	if(header.find("Content-Length: ")!=string::npos){
		size_t length_pos = header.find("Content-Length: ") + 16;
		size_t length_end = header.find_first_of("\r\n",length_pos);
		size_t length_len = length_end - length_pos;
		content_length = header.substr(length_pos,length_len);
	}

	if((header.find("chunked") != string::npos)){
		is_chunked = true;
	}

}

void RequestParser::addContent(string content){
	this->content = content;
	this->request += content;
}
