#include <tuple>
#include <cstdio>
#include "server.hpp"

namespace net
{

void tcp_server::on_new_connection(uv_stream_t *socket, int status)
{
	if (status != 0) return;

	auto server = reinterpret_cast<tcp_server*>(socket->data);

	auto _connect = new server_socket(server->_loop, server);
	auto con_socket = _connect->socket();

	if (uv_accept(socket, reinterpret_cast<uv_stream_t*>(con_socket)) == 0) {
		uv_read_start(reinterpret_cast<uv_stream_t*>(con_socket), tcp_server::on_tcp_alloc_buffer, tcp_server::on_tcp_read);
		server->add(_connect);

		if (server->_tcp_connect) server->_tcp_connect(_connect);

	} else {
		server->disconnect(_connect);
	}
}

void tcp_server::on_tcp_close(uv_handle_t* handle)
{
	auto _tcp = reinterpret_cast<server_socket*>(handle->data);
	auto server = _tcp->manager();

	if (server->_tcp_close && _tcp->id() > 0) server->_tcp_close(_tcp);

	delete _tcp;
}

void tcp_server::on_tcp_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
	auto _tcp = reinterpret_cast<server_socket*>(stream->data);
	auto server = _tcp->manager();

	if (nread > 0) {
		auto buffer = _tcp->input();
		buffer.skip(buffer::skip_type::write, nread);
		if (server->_tcp_read) server->_tcp_read(_tcp);
	} else {
		server->disconnect(_tcp);
	}
}

void tcp_server::on_tcp_alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
	auto _tcp = reinterpret_cast<server_socket*>(handle->data);
	auto buffer = _tcp->input();

	void* buff = nullptr;
	size_t len = 0;
	std::tie(buff, len) = buffer.malloc();
	buf->base = reinterpret_cast<char*>(buff);
	buf->len = len;
}

void tcp_server::send_loop(uv_prepare_t* handle)
{
	tcp_server* server = reinterpret_cast<tcp_server*>(handle->data);

	for (auto& it : server->_tcps) {
		auto tcp = it.second;
		auto socket = tcp->socket();

		for (;;) {
			auto block = tcp->output().pop();
			if (block == nullptr) break;

			if (block->size() < 1) {
				tcp->output().free(block);
				continue;
			}

			uv_write_t* req = new uv_write_t;
			req->data = reinterpret_cast<void*>(block);

			uv_buf_t buf = {
				.base = reinterpret_cast<char*>(block->data()), 
				.len = block->size()
			};

			if (uv_write(req, reinterpret_cast<uv_stream_t*>(socket), &buf, 1, net::tcp_server::on_data_write) == 0) {
				server->_sending_count[tcp->id()] += 1;
			}
		}
	}
}

void tcp_server::on_data_write(uv_write_t* req, int status)
{
	auto block = reinterpret_cast<buffer::block*>(req->data);
	auto _tcp = reinterpret_cast<server_socket*>(req->handle->data);
	_tcp->output().free(block);

	auto server = _tcp->manager();
	auto it = server->_sending_count.find(_tcp->id());
	if (it != server->_sending_count.end()) {
		it->second -= 1;
		if (it->second < 1) server->_sending_count.erase(_tcp->id());
	}

	if (status < 0) {
		server->disconnect(_tcp);
	}

	delete req;
}

tcp_server::tcp_server(uv_loop_t* loop)
{
	_loop = loop;
	_loop->data = reinterpret_cast<void*>(this);

	uv_tcp_init(loop, &_server);
	_server.data = reinterpret_cast<void*>(this);

	uv_prepare_init(loop, &_send_loop);
	_send_loop.data = reinterpret_cast<void*>(this);
}

tcp_server::~tcp_server()
{
}

bool tcp_server::listen(char* ip, int port, int backlog)
{
	struct sockaddr_in bind_addr;
	uv_ip4_addr(ip, port, &bind_addr);
	uv_tcp_bind(&_server, reinterpret_cast<struct sockaddr*>(&bind_addr), 0);
	if (uv_listen(reinterpret_cast<uv_stream_t*>(&_server), backlog, tcp_server::on_new_connection) != 0) {
		return false;
	} else {
		uv_prepare_start(&_send_loop, tcp_server::send_loop);
		return true;
	}
}

bool tcp_server::disconnect(size_t id)
{
	auto result = _tcps.find(id);
	if (result == _tcps.end()) return false;

	return this->disconnect(result->second);
}

bool tcp_server::disconnect(server_socket* tcp)
{
	tcp->set_status(tcp_status::destroy);

	size_t id = tcp->id();
	if (_sending_count.find(id) != _sending_count.end()) {
		uv_read_stop(reinterpret_cast<uv_stream_t*>(tcp->socket()));

	} else {
		if (id > 0) _tcps.erase(id);

		uv_close(reinterpret_cast<uv_handle_t*>(tcp->socket()), tcp_server::on_tcp_close);
	}

	return true;
}

bool tcp_server::is_connect(size_t id)
{
	auto result = _tcps.find(id);
	if (result == _tcps.end()) return false;

	auto tcp = result->second;

	return tcp->status() == tcp_status::connected;
}
	
size_t tcp_server::add(server_socket* tcp)
{
	size_t new_id =  this->gen_id();
	tcp->set_id(new_id);
	tcp->set_status(tcp_status::connected);
	this->_tcps[new_id] = tcp;
	return new_id;
}

bool tcp_server::send(size_t id, char* src, size_t len)
{
	auto result = _tcps.find(id);
	if (result == _tcps.end()) return false;

	auto _tcp = result->second;
	_tcp->output().write(src, len);

	return true;
}

size_t tcp_server::gen_id()
{
	size_t new_id = ++_id;
	if (new_id == 0) new_id = ++_id;
	return new_id;
}

inline void tcp_server::set_tcp_connection_cb(std::function<void(server_socket*)> cb)
{
	_tcp_connect = cb;
}

inline void tcp_server::set_tcp_close_cb(std::function<void(server_socket*)> cb)
{
	_tcp_close = cb;
}

inline void tcp_server::set_tcp_read_cb(std::function<void(server_socket*)> cb)
{
	_tcp_read = cb;
}

}
