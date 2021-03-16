#ifndef SERVER_H
#define SERVER_H

#include <queue>
#include <list>

#include "common/map.h"
#include "common/generator.h"
#include "common/entity.h"

class Server
{
public:
	class Event
	{
	};

	void LoadMap();
	void GenerateMap();

	void Start();
	void Stop();

	void UniversalWorker();

private:
	Map* _map;
	std::queue<Event> _eventQueue;
	std::list<Entity*> _entities;
};

#endif
