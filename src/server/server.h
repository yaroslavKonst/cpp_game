#ifndef SERVER_H
#define SERVER_H

#include <queue>
#include <list>
#include <thread>

#include "common/map.h"
#include "common/generator.h"
#include "common/entity.h"
#include "common/connection.h"

class IOModule
{
private:
	std::list<Connection> _connections;
};

class Server
{
public:
	class Event
	{
	};

	Server()
	{
		_tickTime = 1000;
		_work = false;
	}

	void LoadMap();
	void GenerateMap();

	void Start();
	void Stop();

	static void UniversalWorker(Server* server);

private:
	Map* _map;
	std::queue<Event> _eventQueue;
	std::list<Entity*> _entities;
	std::list<Connection> _connections;
	bool _work;
	uint64_t _tickTime;
	std::thread* _workerThread;
};

#endif
