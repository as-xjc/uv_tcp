#include <tuple>
#include <cassert>
#include "client.hpp"

namespace net
{

void tcp_client::on_connect(uv_connect_t* req, int status)
{
	if (status == 0) {
		uv_read_start(req->handle, 
				net::tcp_client::on_tcp_alloc_buffer, 
				net::tcp_client::on_tcp_read);
		
		auto tcp = reinterpret_cast<client_socket*>(req->handle->data);
		auto client = tcp->manager();

		tcp->set_status(tcp_status::connected);

		if (client->_tcp_connect) client->_tcp_connect(tcp);

		uv_check_start(&(client->_send_loop), net::tcp_client::send_loop);
	} else {

	}

	delete req;
}

void tcp_client::on_tcp_alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	auto tcp = reinterpret_cast<client_socket*>(handle->data);
	auto buffer = tcp->input();

	void* buff = nullptr;
	size_t len = 0;
	std::tie(buff, len) = buffer->malloc();
	buf->base = reinterpret_cast<char*>(buff);
	buf->len = len;
}

void tcp_client::on_tcp_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
	auto tcp = reinterpret_cast<client_socket*>(stream->data);
	auto client = tcp->manager();

	if (nread > 0) {
		auto buffer = tcp->input();
		buffer->skip(buffer::skip_type::write, nread);
		if (client->_tcp_read) client->_tcp_read(tcp);
	} else {
		client->disconnect();
	}
}

void tcp_client::on_tcp_close(uv_handle_t* handle)
{
	auto tcp = reinterpret_cast<client_socket*>(handle->data);
	auto client = tcp->manager();

	client->_socket = nullptr;
	delete tcp;
}

void tcp_client::send_loop(uv_check_t* handle)
{
	tcp_client* client = reinterpret_cast<tcp_client*>(handle->data);

	if (client->_socket == nullptr) return;

	auto tcp = client->_socket;
	auto buffer = tcp->output();
	for (;;) {
		auto block = buffer->pop();
		if (block == nullptr) break;

		if (block->size() < 1) {
			buffer->free(block);
			continue;
		}

		uv_write_t* req = new uv_write_t;
		req->data = reinterpret_cast<void*>(block);

		uv_buf_t buf = {
			.base = reinterpret_cast<char*>(block->data()), 
			.len = block->size()
		};

		uv_write(req, reinterpret_cast<uv_stream_t*>(tcp->socket()), 
				&buf, 1, net::tcp_client::on_data_write);
	}
}

void tcp_client::on_data_write(uv_write_t* req, int status)
{
	auto block = reinterpret_cast<buffer::block*>(req->data);
	auto tcp = reinterpret_cast<client_socket*>(req->handle->data);
	tcp->output()->free(block);

	if (status < 0) {
		auto client = tcp->manager();
		client->disconnect();
	}

	delete req;
}

tcp_client::tcp_client(uv_loop_t* loop)
{
	_loop = loop;

	uv_check_init(loop, &_send_loop);
	_send_loop.data = reinterpret_cast<void*>(this);
}

tcp_client::~tcp_client()
{
	disconnect();
}

bool tcp_client::disconnect()
{
	if (_socket == nullptr) return true;
	if (_socket->status() == tcp_status::destroy) return true;

	_socket->set_status(tcp_status::destroy);

	if (_tcp_close) _tcp_close(_socket);

	uv_check_stop(&_send_loop);
	uv_close(reinterpret_cast<uv_handle_t*>(_socket->socket()), tcp_client::on_tcp_close);

	return true;
}

bool tcp_client::is_connect()
{
	if (_socket == nullptr) return false;
	return _socket->status() == tcp_status::connected;
}

bool tcp_client::connect(char* ip, int port)
{
	assert(_socket == nullptr);

	struct sockaddr_in dest;
	if (uv_ip4_addr(ip, port, &dest) != 0) return false;

	_socket = new client_socket(_loop, this);
	uv_connect_t* connect = new uv_connect_t;
	int r = uv_tcp_connect(connect, _socket->socket(), 
			reinterpret_cast<struct sockaddr*>(&dest), 
			net::tcp_client::on_connect);

	if (r != 0) {
		delete _socket;
		_socket = nullptr;
	}

	return r == 0;
}
	
bool tcp_client::send(char* src, size_t len)
{
	if (_socket == nullptr) return false;
	if (_socket->status() != tcp_status::connected) return false;

	_socket->output()->write(src, len);
	return true;
}

void tcp_client::set_tcp_connection_cb(std::function<void(client_socket*)> cb)
{
	_tcp_connect = cb;
}

void tcp_client::set_tcp_close_cb(std::function<void(client_socket*)> cb)
{
	_tcp_close = cb;
}

void tcp_client::set_read_cb(std::function<void(client_socket*)> cb)
{
	_tcp_read = cb;
}

}
