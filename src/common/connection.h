#ifndef CONNECTION_H
#define CONNECTION_H

#include <unistd.h>
#include <socket.h>
#include <fcntl.h>

class Connection
{
public:
	enum Type { Socket, Pipe };

private:
	Type _type;
	int _descriptor;
};

#endif
