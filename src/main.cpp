#include "server/server.h"
#include "client/client.h"

int main(int argc, char** argv)
{
	std::pair<Connection, Connection> connection = Listener::GetPipe();
	return 0;
}
