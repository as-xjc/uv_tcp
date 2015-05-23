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

#include <cassert>
#include <uv.h>

#include "buffer/block_buffer.hpp"

namespace net
{

enum class tcp_status {init, connected, destroy};

template <typename T, size_t input_block_size = 1024, size_t output_block_size = 1024>
class tcp {

static_assert(std::is_pointer<T>::value, "template T must be pointer");

public:
	tcp(uv_loop_t* loop, T manager):
		_input(input_block_size),
		_output(output_block_size),
		_manager(manager)
	{
		assert(loop != nullptr);
		assert(manager != nullptr);

		_socket.data = reinterpret_cast<void*>(this);
		uv_tcp_init(loop, &_socket);
	}

	~tcp() {}

	inline size_t id() const {return _id;}
	inline void id(size_t id) {_id = id;}

	inline buffer::block_buffer* input() {return &_input;}
	inline buffer::block_buffer* output() {return &_output;}

	inline tcp_status status() {return _status;}
	inline void status(tcp_status status) {_status = status;}

	inline uv_tcp_t* socket() {return &_socket;}

	inline T manager() {return _manager;}
	inline void clean_manager() {_manager = nullptr;}

private:
	size_t _id = 0;

	T _manager;
	uv_tcp_t _socket;
	tcp_status _status = tcp_status::init;

	buffer::block_buffer _input;
	buffer::block_buffer _output;
};

}
