
class Parser{
	virtual void parse() = 0;
};

class RequestParser:public Parser{
	
};

class ResponseParser:public Parser{

};