#ifndef SERVER_H
#define SERVER_H

#include <queue>
#include <list>
#include <thread>
#include <atomic>
#include <mutex>

#include "common/map.h"
#include "common/generator.h"
#include "common/entity.h"
#include "common/connection.h"

class Server;

class Event
{
public:
	enum Type
	{
		MESSAGE
	};

	Type type;

	std::vector<char> message;
};

class Player
{
public:
	Connection connection;
	int ID;

	std::list<Event> outQueue;
	std::mutex outMutex;

	std::list<Event> inQueue;
	std::mutex InMmutex;
};

class IOModule
{
public:
	struct StartInfo
	{
		std::string ip;
		uint16_t port;
	};

	IOModule(Server* server, bool listen, StartInfo* startInfo);
	~IOModule();

	void Start();
	void Stop();

	std::list<Player> players;
	std::mutex playerListMutex;

private:
	Server* _server;
	Listener* _listener;

	std::atomic<bool> _work;
	std::thread* _thread;

	bool _listen;

	static void Thread(IOModule* ioModule);
};

class Server
{
public:
	Server(bool listen, IOModule::StartInfo* startInfo)
	{
		_tickTime = 10000;
		_work = false;
		_ioModule = new IOModule(this, listen, startInfo);
		_ioModule->Start();

	}

	~Server()
	{
		_ioModule->Stop();
		delete _ioModule;
	}

	void LoadMap();
	void GenerateMap();

	void Start();
	void Stop();

	static void UniversalWorker(Server* server);

private:
	IOModule* _ioModule;

	Map* _map;
	std::queue<Event> _eventQueue;
	std::list<Entity*> _entities;
	std::atomic<bool> _work;
	uint64_t _tickTime;
	std::thread* _workerThread;
};

#endif
