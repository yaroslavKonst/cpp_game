#ifndef CONNECTION_H
#define CONNECTION_H

class Listener
{
public:
	Listener(std::string ip, uint16_t port, void (*callback)(Connection));

	void OpenSocket();

	void Listen(bool loop);

	void Stop();

	Connection GetPipe();

private:
	void (*_callback)(Connection);
	int _descriptor;
	std::string _ip;
	uint16_t _port;
	bool _work;
	void AcceptConnection();
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
		_valid = true;
	}

	enum Type { Socket, Pipe };

	void Send(std::vector<char> data);

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
