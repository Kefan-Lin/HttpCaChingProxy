#include"cache.hpp"
#include"response_parser.hpp"
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
#include<mutex>
std::mutex my_mutex;

bool Cache::put(string request_line, ResponseParser resp_parser){
	std::lock_guard<std::mutex> lock(my_mutex);
	// if full, kick out begin(), decrement size
	if(size==capacity){
		mymap.erase(mymap.begin());
		size--;
	}
	// insert
	pair<map<string,ResponseParser>::iterator,bool> insert_pair;
	insert_pair = mymap.insert(pair<string,ResponseParser>(request_line,resp_parser));
	
	//insert success, increment size
	if(insert_pair.second == true){
		size++;
	}
	//cout<<"Add "<<resp_parser.getFirstline()<<" to cache!!!"<<endl;
	return insert_pair.second;
}

ResponseParser * Cache::get(string request_line){
	std::lock_guard<std::mutex> lock(my_mutex);
	map<string,ResponseParser>::iterator it;
	it = mymap.find(request_line);
	if(it == mymap.end()){
		return NULL;
	}
	//cout<<"Get "<<mymap[request_line].getFirstline()<<" from cache!!!"<<endl;
	return &mymap[request_line];
}
