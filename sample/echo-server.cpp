#include <iostream>
#include <ctime>
#include "server.hpp"

int main(int argc, char* argv[])
{
	uv_loop_t* loop = uv_default_loop();
	net::tcp_server a(loop);

	a.set_read_cb([](net::server_socket* tcp){
			size_t len = tcp->input()->size();
			std::cout << "server revc " << len << std::endl;
			//tcp->input()->debug(buffer::debug_type::chars);
			tcp->output()->append(*(tcp->input()));
			tcp->input()->clear();
	});

	a.set_connection_cb([](net::server_socket* tcp){
			std::cout << "server accept connection :" << tcp->id() << std::endl;
	});

	a.set_close_cb([](net::server_socket* tcp){
			std::cout << "server disconnect connection :" << tcp->id() << std::endl;
	});

	char ip[] = "127.0.0.1";
	bool result = a.listen(ip, 5000);
	std::cout << "listen result " << result << std::endl;

	uv_run(loop, UV_RUN_DEFAULT);
	return 0;
}
