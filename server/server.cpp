#include "server/server.h"

#include <chrono>
#include <unistd.h>

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
	_workerThread = new std::thread(Server::UniversalWorker(this));
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
