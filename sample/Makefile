all:
	g++ --std=c++11 -o echo-server echo-server.cpp ../buffer/block_buffer.cpp ../server.cpp -I../ -luv
	g++ --std=c++11 -o echo-client echo-client.cpp ../buffer/block_buffer.cpp ../client.cpp -I../ -luv
