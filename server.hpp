// The MIT License (MIT)
// 
// Copyright (c) 2014 xiao jian cheng
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// 

#pragma once

#include <map>
#include <uv.h>
#include <functional>

#include "tcp.hpp"

namespace net
{

typedef tcp<1024, 1024> server_socket;

class tcp_server {

public:
	tcp_server(uv_loop_t* loop);
	~tcp_server();

	bool listen(char* ip, int port, int backlog = 128);
	bool disconnect(size_t id);
	bool is_connect(size_t id);
	bool send(size_t id, char* src, size_t len);

	void set_tcp_connection_cb(std::function<void(server_socket*)> cb);
	void set_tcp_close_cb(std::function<void(server_socket*)> cb);
	void set_tcp_read_cb(std::function<void(server_socket*)> cb);

private:
	size_t gen_id();

	uv_tcp_t _server;
	uv_prepare_t _send_loop;
	uv_loop_t* _loop;

	size_t _id = 0;

	size_t add(server_socket* tcp);
	bool disconnect(server_socket* tcp);

	std::map<int, server_socket*> _tcps;

	std::function<void(server_socket*)> _tcp_close;
	std::function<void(server_socket*)> _tcp_read;
	std::function<void(server_socket*)> _tcp_connect;

	static void on_tcp_close(uv_handle_t* handle);
	static void on_new_connection(uv_stream_t *socket, int status);
	static void on_tcp_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
	static void on_tcp_alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void send_loop(uv_prepare_t* handle);
	static void on_data_write(uv_write_t* req, int status);
};

}
