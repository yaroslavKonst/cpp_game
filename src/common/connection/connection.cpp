#include "connection.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdexcept>

Listener::Listener(std::string ip, uint16_t port, void (*callback)(Connection))
{
	_callback = callback;
	_ip = ip;
	_port = port;
}

void Listener::OpenSocket()
{
	_descriptor = socket(AF_INET, SOCK_STREAM, 0);

	if (_descriptor < 0) {
		// TODO
		// error handling
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = _port;
	address.sin_addr = inet_addr(_ip.c_str());

	int err = bind(_descriptor, &address, sizeof(struct sockaddr_in));

	if (err < 0) {
		// TODO
		// error handling
	}

	err = listen(_descriptor, 8);

	if (err < 0) {
		// TODO
		// error handling
	}
}

void Listener::Listen(bool loop)
{
	if (!loop) {
		AcceptConnection();
	} else {
		_work = true;

		while (_work) {
			AcceptConnection();
		}
	}
}

void Listener::Stop()
{
	_work = false;
}

Connection Listener::GetPipe()
{
	int pipe1[2];
	int pipe2[2];

	int err = pipe(pipe1);

	if (err < 0) {
		// TODO
		// error handling
	}

	err = pipe(pipe2);

	if (err < 0) {
		close(pipe1[0]);
		close(pipe1[1]);
		// TODO
		// error handling
	}

	Connection conn1;
	Connection conn2;

	conn1._type = Connection::Pipe;
	conn2._type = Connection::Pipe;

	conn1._descriptor[0] = pipe1[0];
	conn2._descriptor[0] = pipe2[0];

	conn1._descriptor[1] = pipe2[1];
	conn2._descriptor[2] = pipe1[1];

	_callback(conn1);

	return conn2;
}

void Listener::AcceptConnection()
{
	int sock = accept(_descriptor, NULL, NULL);

	if (sock < 0) {
		// TODO
		// error handling
	}

	Connection conn;
	conn._type = Connection::Socket;
	conn._descriptor[0] = sock;

	_callback(conn);
}

void Connector::Connect(std::string ip, uint16_t port)
{
	_descriptor = socket(AF_INET, SOCK_STREAM, 0);

	if (_descriptor < 0) {
		// TODO
		// error handling
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = port;
	address.sin_addr = inet_addr(ip.c_str());

	int err = connect(_descriptor, &address, sizeof(struct sockaddr_in));

	if (err < 0) {
		// TODO
		// error handling
	}

	Connection conn;
	conn._type = Connection::Socket;
	conn._descriptor[0] = sock;
	return conn;
}

void Connection::Send(std::vector<char> data)
{
	int fd;

	if (_type == Pipe) {
		fd = _descriptor[1];
	} else {
		fd = _descriptor[0];
	}

	int sz = data.size();

	int err = write(fd, &sz, sizeof(int));

	if (err != sizeof(int)) {
		CloseStreams();
		_valid = false;
		return;
	}

	int sent = 0;

	while (sent < data.size()) {
		int ret = write(fd, data.data() + sent, data.size() - sent);
		if (ret < 0) {
			CloseStreams();
			_valid = false;
			break;
		}

		sent += ret;
	}
}

std::vector<char> Receive()
{
	int fd = _descriptor[0];

	int sz;

	int err = read(fd, &sz, sizeof(int));

	if (err != sizeof(int)) {
		CloseStreams();
		_valid = false;
		return std::vector<char>();
	}

	if (sz == 0) {
		CloseStreams();
		_valid = false;
	}

	std::vector<char> data(0);
	data.reserve(sz);

	int got = 0;

	while (got < sz) {
		int ret = read(fd, data.data() + got, sz - got);
		if (ret < 0) {
			_valid = false;
			break;
		}

		got += ret;
	}

	return data;
}

void Connection::CloseStreams()
{
	if (_type == Socket) {
		shutdown(_descriptor[0], SHUT_RDWR);
		close(_descriptor[0]);
	} else {
		close(_descriptor[0]);
		close(_descriptor[1]);
	}
}

void Connection::Close()
{
	int sz = 0;

	int err = write(fd, &sz, sizeof(int));

	if (err != sizeof(int)) {
		_valid = false;
		CloseStreams();
		return;
	}
}
