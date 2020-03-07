#include "response_parser.hpp"

void ResponseParser::parseHeader(){
	
	size_t end_of_lineone = header.find_first_of("\r\n");
	first_line = header.substr(0, end_of_lineone);
	//parse status code
	size_t status_end = header.find_first_of("\r\n");
	size_t status_begin = header.find_first_of(" ") + 1;
	size_t status_len = status_end - status_begin;
	status = header.substr(status_begin, status_len);
	//cout<<"Status = "<<status<<endl;
	//parse status number
	status_num = header.substr(status_begin, 3);

	if(first_line.find("200")!=string::npos){
		cacheable = true;
	}
	//parse cache_control
	size_t cache_control_begin;
	if((cache_control_begin = header.find("Cache-Control:")) != string::npos){
		cache_control_begin ++;
		size_t cache_control_end = header.find("\r\n", cache_control_begin);
		size_t cache_control_len = cache_control_end - cache_control_begin;
		cache_control = header.substr(cache_control_begin, cache_control_len);

		if(cache_control.find("private")!=string::npos || cache_control.find("no-store")!=string::npos){
			if(cache_control.find("private")!=string::npos){
				privated = true;
				no_store = false;
			}
			if(cache_control.find("no-store")!=string::npos){
				privated = false;
				no_store = true;
			}
			cacheable = false;
		}
		else{
			cacheable = true;
		}

		if(cache_control.find("must-revalidate")!=string::npos||cache_control.find("no-cache")!=string::npos){
			must_revalidate = true;
		}
		if(cache_control.find("max-age")!=string::npos){
			string s_age = cache_control.substr(cache_control.find("max-age=")+8);
			if(s_age.find(",")!=string::npos){
				s_age = s_age.substr(0,s_age.find_first_of(","));
			}
			age = s_age;
		}
	}


	
	//parse expire
	size_t expire_begin;
	if((expire_begin = header.find("Expires:")) != string::npos){
		expire_begin = expire_begin + 8 + 1; //skip Expires:
		size_t expire_end = header.find("\r\n", expire_begin);
		size_t expire_len = expire_end - expire_begin;
		expire = header.substr(expire_begin, expire_len);
	}
	
	//parse date
	size_t date_begin;
	if((date_begin = header.find("Date:")) != string::npos){
		date_begin = date_begin + 5 + 1;
		size_t date_end = header.find("\r\n", date_begin);
		size_t date_len = date_end - date_begin;
		date = header.substr(date_begin, date_len);
	}
	
	//parse last_modifed
	size_t last_modified_begin;
	if((last_modified_begin = header.find("Last-Modified:")) != string::npos){
		last_modified_begin = last_modified_begin + 14 + 1;
		size_t last_modified_end = header.find("\r\n", last_modified_begin);
		size_t last_modified_len = last_modified_end - last_modified_begin;
		last_modified = header.substr(last_modified_begin, last_modified_len);
	}
	
	//parse e-tag 
	size_t E_tag_begin;
	if((E_tag_begin = header.find("ETag:")) != string::npos){
		E_tag_begin = E_tag_begin + 5 + 1;
		size_t E_tag_end = header.find("\r\n", E_tag_begin);
		size_t E_tag_len = E_tag_begin - E_tag_end;
		E_tag = header.substr(E_tag_begin, E_tag_len);
	}
	
	//parse content-length
	size_t content_length_begin,content_length_end;
	if((content_length_begin = header.find("Content-Length:")) != string::npos){
		content_length_begin = content_length_begin + 15 + 1;
		content_length_end = header.find("\r\n", content_length_begin);
		size_t content_length_len = content_length_end - content_length_begin;
		content_length = header.substr(content_length_begin, content_length_len);
	}

	
	//see if use chunk to transmit
	if((response.find("chunked") != string::npos)){
		is_chunked = true;
	}
	
	//status valid or invalid
	if(status == "200 OK"){
		status_valid = 1;
	}
	//find age in cache-control

	//size_t age_begin = cache_control.find("max-age=");
	//size_t age_begin;
	
	
	//cout<<"age_begin = "<<age_begin<<endl;
	//age = cache_control.substr(age_begin);
}	

void ResponseParser::addContent(string content){
	this->content = content;
	this->response += content;

}

