#ifndef CONNECTION_H
#define CONNECTION_H

#include <unistd.h>
#include <socket.h>
#include <fcntl.h>

class Connection
{
public:
	enum Type { Socket, Pipe };

	void Send(std::vector<char> data);

	std::vector<char> Receive(size_t* size);



private:
	Type _type;
	int _descriptor[2];
};

class Listener
{
public:
	Listener(void (*callback)(Connection))
	{
		_callback = callback;
	}

	void Listen(bool loop);

	void Stop();

	Connection GetPipe();

private:
	void (*_callback)(Connection);
	int _descriptor;
};

class Connector
{
public:
	Connection Connect();
};

#endif
