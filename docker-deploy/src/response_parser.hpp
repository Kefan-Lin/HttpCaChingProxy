#ifndef __RESPONSE_PARSER__
#define __RESPONSE_PARSER__
//#include "cache.hpp"
//#include "proxy.hpp"
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
 
using namespace std;
class ResponseParser
{
public:
	string header;
	string content_length;
	string content;
	string response;//the whole response
	string status; //200 OK
	string status_num; //200
	string cache_control; //if cacheable and the existing time
	string expire; //when it will expire
	string date; //when the response is sent
	string last_modified; //last modified time
	string E_tag;
	string age;
	string first_line; 

	bool is_chunked;//the chunk for get
	int status_valid; //use to determine if failed
	bool must_revalidate;
	bool cacheable;
	bool privated;
	bool no_store;
	//int age; //time existed in cache
	//int content_length; //length of content
	//constructor;

  ResponseParser():	header(""),
										content_length("0"),
										content(""),
										response(""),
										status(""),
										status_num(""),
										cache_control(""),
										expire(""),
										date(""),
										last_modified(""),
										E_tag(""),
										age(""),
										first_line(""),
										is_chunked(false),
										status_valid(0),
										must_revalidate(false),
										cacheable(false),
										privated(false),
										no_store(false)
										//content_length(0),
										{}
	ResponseParser(const ResponseParser & rhs):header(rhs.header),
										content_length(rhs.content_length),
										content(rhs.content),
										response(rhs.response),
										status(rhs.status),
										status_num(rhs.status_num),
										cache_control(rhs.cache_control),
										expire(rhs.expire),
										date(rhs.date),
										last_modified(rhs.last_modified),
										E_tag(rhs.E_tag),
										age(rhs.age),
										first_line(rhs.first_line),
										is_chunked(rhs.is_chunked),
										status_valid(rhs.status_valid),
										must_revalidate(rhs.must_revalidate),
										cacheable(rhs.cacheable),
										privated(rhs.privated),
										no_store(rhs.no_store){}

  ResponseParser(string recv_header):	header(recv_header),
										content_length("0"),
										content(""),
										response(recv_header),
										status(""),
										status_num(""),
										cache_control(""),
										expire(""),
										date(""),
										last_modified(""),
										E_tag(""),
										age(""),
										first_line(""),
										is_chunked(false),
										status_valid(0),
										must_revalidate(false),
										cacheable(false),
										privated(false),
										no_store(false)
										//content_length(0),
										{}
	string getHeader(){return header;}
	string getContentLength(){return content_length;}
	string getContent(){return content;}
	string getStatus(){return status;}
	string getResponse(){return response;}
	string getFirstline(){return first_line;}
	string getCacheControl(){return cache_control;}
	string getEtag(){return E_tag;}
	string getExpire(){return expire;}
	string getDate(){return date;}
	string getLastModified(){return last_modified;}
	string getAge(){return age;}
	bool getIsChunked(){return is_chunked;}
	bool getMustRevalidate(){return must_revalidate;}
	bool getCacheable(){return cacheable;}
	bool getPrivated(){return privated;}
	bool getNoStore(){return no_store;}
	void parseHeader();
	void addContent(string content);

	~ResponseParser(){};
	friend class Proxy;
};

#endif
