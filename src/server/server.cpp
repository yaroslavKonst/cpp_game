#include "server/server.h"

#include <chrono>
#include <unistd.h>
#include <select.h>

// Server
void Server::UniversalWorker(Server* server)
{
	std::chrono::high_resolution_clock::time_point currentTime =
		std::chrono::high_resolution_clock.now();

	std::chrono::high_resolution_clock::time_point processingTime;

	while (server->_work) {
		for (Entity* entity : server->_entities) {
			entity->tick(server->_map);
		}

		processingTime = std::chrono::high_resolution_clock.now();

		uint64_t dur = std::chrono::duration_cast<
			std::chrono::microseconds>(
					processingTime - currentTime).count();

		if (dur < _tickTime) {
			uint64_t remaining = _ticktime - dur;
			usleep(remaining);
		}

		currentTime = std::chrono::high_resolution_clock.now();
	}
}

void Server::Start()
{
	if (_work) {
		return;
	}

	_work = true;
	_workerThread = new std::thread(Server::UniversalWorker, this);
}

void Server::Stop()
{
	if (!_work) {
		return;
	}

	_work = false;
	_workerThread->join();
	delete _workerThread;
}

// IOModule
IOModule::IOModule(Server* server, bool listen, StartInfo* startInfo)
{
	_server = server;
	_work = false;
	_listen = listen;

	if (_listen) {
		_listener = new Listener(startInfo.ip, startInfo.port);
		_listener->OpenSocket();
	}
}

IOModule::~IOModule()
{
	if (_work) {
		Stop();
	}

	if (_listen) {
		_listener->CloseSocket();
		delete _listener;
	}
}

void IOModule::Start()
{
	if (_work) {
		return;
	}

	_work = true;
	_thread = new std::thread(IOModule::Thread, this);
}

void IOModule::Stop()
{
	if (!_work) {
		return;
	}

	_work = false;
	_thread->join();
	delete _thread;
}

void IOModule::Thread(IOModule* ioModule)
{
	while(ioModule->_work) {
		fd_set readSet;
		fd_set writeSet;

		int maxFD = 0;

		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);

		if (ioModule->_listen) {
			int fd = ioModule->_listener->GetReadHandle();
			FD_SET(fd, &readSet);

			if (fd > maxFD) {
				maxFD = fd;
			}
		}

		ioModule->playerListMutex.lock();

		for (Player& player : ioModule->_players) {
			int fd = player.connection.GetReadHandle();
			FD_SET(fd, &readSet);

			if (fd > maxFD) {
				maxFD = fd;
			}

			player.outMutex.lock();

			if (player.outQueue.size() > 0) {
				int fd = player.connection.GetWriteHandle();
				FD_SET(fd, &writeSet);

				if (fd > maxFD) {
					maxFD = fd;
				}
			}

			player.outMutex.unlock();
		}

		ioModule->playerListMutex.unlock();

		struct timeval waitingTime;
		waitingTime.tv_sec = 0;
		waitingTime.tv_usec = 1000;

		int err = select(
			maxFD + 1,
			&readSet,
			&writeSet,
			NULL,
			&waitingTime);

		if (err < 0) {
			throw std::runtime_error("failure in select call");
		}

		if (ioModule->_listen) {
			int fd = ioModule->_listener->GetReadHandle();

			if (FD_ISSET(fd, &readSet)) {
				Player player;
				player.connection =
					ioModule->_listener->Accept();

				ioModule->playerListMutex.lock();
				ioModule->_players.push_back(player);
				ioModule->playerListMutex.unlock();
			}
		}

		ioModule->playerListMutex.lock();

		for (Player& player : ioModule->players) {
			int fd = player.connection.GetReadHandle();

			if (FD_ISSET(fd, &readSet)) {
				player.inMutex.lock();

				Event event;
				event.type = Event::MESSAGE;
				event.message = player.connection.Receive();

				player.inQueue.push_back(event);

				player.inMutex.unlock();
			}

			player.outMutex.lock();

			if (player.outQueue.size() > 0) {
				int fd = player.connection.GetWriteHandle();

				if (FD_ISSET(fd, &writeSet)) {
					Event& event = player.outQueue.front();

					player.connection.Send(event.message);

					player.outQueue.pop_front();
				}
			}

			player.outMutex.unlock();
		}

		ioModule->playerListMutex.unlock();
	}
}
