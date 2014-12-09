#include <iostream>
#include <ctime>
#include <string>
#include "client.hpp"

uv_work_t req;
uv_loop_t* loop;

net::tcp_client* c;

void input_socket(uv_async_t *handle)
{
	std::string* msg = (std::string*)handle->data;
	c->send((char*)msg->c_str(), msg->length());
	delete msg;
}

void input_check(uv_work_t *req)
{
	for (;;) {
		std::string* input = new std::string; 
		std::printf("input:");
		std::cin >> *input;
		if (*input == "exit") break;

		uv_async_t* async = new uv_async_t;
		async->data = (void*)input;
		uv_async_init(loop, async, input_socket);
		uv_async_send(async);
	}
}

void after(uv_work_t *req, int status)
{
	uv_stop(loop);
}	 

int main(int argc, char* argv[])
{

	loop = uv_default_loop();
	net::tcp_client a(loop);
	c = &a;

	a.set_read_cb([](net::client_socket* tcp) {
			auto buffer = tcp->input();
			buffer->debug(buffer::debug_type::chars);
			buffer->clear();
	});

	char ip[] = "127.0.0.1";
	bool result = a.connect(ip, 5000);
	std::cout << "listen result " << result << std::endl;

	uv_queue_work(loop, &req, input_check, after);

	uv_run(loop, UV_RUN_DEFAULT);
	return 0;
}
