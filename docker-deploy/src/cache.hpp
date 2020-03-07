#ifndef __CACHE__
#define __CACHE__
#include <cstdlib>
#include <iostream>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <vector>
#include"response_parser.hpp"
using namespace std;
class Cache{
	map<string, ResponseParser> mymap; //<request_line,resp_parser>
	int capacity;	// max size
	int size;		// current size
public:
	Cache():capacity(32),size(0){}
	Cache(int cap):capacity(cap),size(0){}
	~Cache(){}
	int getCapacity(){return capacity;}
	int getSize(){return size;}

	bool put(string request_line, ResponseParser resp_parser);
	ResponseParser * get(string request_line);
};

#endif
