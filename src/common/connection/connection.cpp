#include "connection.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

Listener::Listener(std::string ip, uint16_t port)
{
	_ip = ip;
	_port = port;
	_work = false;
}

Listener::~Listener()
{
	CloseSocket();
}

bool Listener::OpenSocket()
{
	if (_work) {
		CloseSocket();
	}
	
	_descriptor = socket(AF_INET, SOCK_STREAM, 0);

	if (_descriptor < 0) {
		return false;
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = _port;
	address.sin_addr.s_addr = inet_addr(_ip.c_str());

	int err = bind(_descriptor, (sockaddr*)&address,
			sizeof(struct sockaddr_in));

	if (err < 0) {
		close(_descriptor);
		return false;
	}

	err = listen(_descriptor, 5);

	if (err < 0) {
		close(_descriptor);
		return false;
	}

	_work = true;
	return true;
}

void Listener::CloseSocket()
{
	if (!_work) {
		return;
	}

	close(_descriptor);
	_work = false;
}

std::pair<Connection, Connection> Listener::GetPipe()
{
	int pipe1[2];
	int pipe2[2];

	int err = pipe(pipe1);

	if (err < 0) {
		Connection conn;
		return std::pair<Connection, Connection>(conn, conn);
	}

	err = pipe(pipe2);

	if (err < 0) {
		close(pipe1[0]);
		close(pipe1[1]);
		Connection conn;
		return std::pair<Connection, Connection>(conn, conn);
	}

	Connection conn1;
	Connection conn2;

	conn1._valid = true;
	conn2._valid = true;

	conn1._type = Connection::Pipe;
	conn2._type = Connection::Pipe;

	conn1._descriptor[0] = pipe1[0];
	conn2._descriptor[0] = pipe2[0];

	conn1._descriptor[1] = pipe2[1];
	conn2._descriptor[1] = pipe1[1];

	return std::pair<Connection, Connection>(conn1, conn2);
}

Connection Listener::Accept()
{
	int sock = accept(_descriptor, NULL, NULL);

	if (sock < 0) {
		Connection conn;
		conn._valid = false;
		return conn;
	}

	Connection conn;
	conn._type = Connection::Socket;
	conn._descriptor[0] = sock;
	conn._valid = true;

	return conn;
}

Connection Connector::Connect(std::string ip, uint16_t port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock < 0) {
		Connection conn;
		conn._valid = false;
		return conn;
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = port;
	address.sin_addr.s_addr = inet_addr(ip.c_str());

	int err = connect(sock, (struct sockaddr*)&address,
			sizeof(struct sockaddr_in));

	if (err < 0) {
		close(sock);
		Connection conn;
		conn._valid = false;
		return conn;
	}

	Connection conn;
	conn._type = Connection::Socket;
	conn._descriptor[0] = sock;
	conn._valid = true;
	return conn;
}

void Connection::Send(const std::vector<char>& data)
{
	if (!_valid) {
		return;
	}

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

	size_t sent = 0;

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

std::vector<char> Connection::Receive()
{
	if (!_valid) {
		return std::vector<char>();
	}

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
		return std::vector<char>();
	}

	std::vector<char> data(sz);

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
	if (!_valid) {
		return;
	}

	int fd;

	if (_type == Pipe) {
		fd = _descriptor[1];
	} else {
		fd = _descriptor[0];
	}

	int sz = 0;

	int err = write(fd, &sz, sizeof(int));

	if (err != sizeof(int)) {
		_valid = false;
		CloseStreams();
		return;
	}

	CloseStreams();
	_valid = false;
}
