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

#include <uv.h>
#include <functional>

#include "tcp.hpp"

namespace net
{

class tcp_client;
typedef tcp<tcp_client*, 1024, 1024> client_socket;

class tcp_client {

public:
	tcp_client(uv_loop_t* loop);
	~tcp_client();

	bool disconnect();
	bool is_connect();
	bool connect(char* ip, int port);
	bool send(char* src, size_t len);

	void set_tcp_connection_cb(std::function<void(client_socket*)> cb);
	void set_tcp_close_cb(std::function<void(client_socket*)> cb);
	void set_read_cb(std::function<void(client_socket*)> cb);

private:
	uv_loop_t* _loop = nullptr;
	client_socket* _socket = nullptr;
	uv_check_t _send_loop;

	std::function<void(client_socket*)> _tcp_read;
	std::function<void(client_socket*)> _tcp_connect;
	std::function<void(client_socket*)> _tcp_close;

	static void on_connect(uv_connect_t* req, int status);
	static void on_tcp_alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
	static void on_tcp_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
	static void on_tcp_close(uv_handle_t* handle);
	static void send_loop(uv_check_t* handle);
	static void on_data_write(uv_write_t* req, int status);
};

}
