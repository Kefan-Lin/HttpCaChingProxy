SOURCES=main.cpp  proxy.cpp request_parser.cpp response_parser.cpp handler.cpp cache.cpp
OBJS=$(patsubst %.cpp, %.o, $(SOURCES))
CPPFLAGS=-ggdb3 -Wall -Werror -pedantic -std=gnu++11

proxy: $(OBJS)
	g++ $(CPPFLAGS) -o proxy $(OBJS) -lpthread
%.o: %.cpp proxy.hpp request_parser.hpp response_parser.hpp cache.hpp handler.hpp
	g++ $(CPPFLAGS) -c $< -lpthread

clean:
	rm -f proxy *~ *.o
