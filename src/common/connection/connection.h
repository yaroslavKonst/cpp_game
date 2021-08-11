#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <vector>
#include <utility>
#include <sys/types.h>

class Listener;
class Connector;
class Connection;

class Listener
{
public:
	Listener(std::string ip, uint16_t port);
	~Listener();

	bool OpenSocket();

	void CloseSocket();

	Connection Accept();

	static std::pair<Connection, Connection> GetPipe();

private:
	int _descriptor;
	std::string _ip;
	uint16_t _port;
	bool _work;
};

class Connector
{
public:
	Connection Connect(std::string ip, uint16_t port);
};

class Connection
{
public:
	Connection()
	{
		_valid = false;
	}

	enum Type { Socket, Pipe };

	void Send(const std::vector<char>& data);

	std::vector<char> Receive();

	void Close();

	bool isValid()
	{
		return _valid;
	}

	friend class Listener;
	friend class Connector;

private:
	Type _type;
	int _descriptor[2];
	bool _valid;

	void CloseStreams();
};

#endif
