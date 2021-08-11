#include "client.h"

#include <cstring>

void Client::Start()
{
	_work = true;

	_thread = new std::thread(Thread, this);
}

void Client::Stop()
{
	_work = false;

	_thread->join();
	delete _thread;
}

void Client::Thread(Client* client)
{
	while (client->_work && client->_connection.isValid()) {
		std::vector<bool> msg = client->_connection.Receive();
	}
}

void Client::VideoThread(Client* client)
{
	client->_video->Start();
}

// VideoController
void VideoController::WSKey(int key, int action, void* data)
{
	VideoController* controller =
		reinterpret_cast<VideoController*>(data);

	controller->_connectionMutex.lock();

	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_W) {
			controller->_client->_connection.Send(
				std::vector<char>({'f'}));
		} else if (key == GLFW_KEY_S) {
			controller->_client->_connection.Send(
				std::vector<char>({'b'}));
		}
	} else if (action == GLFW_RELEASE) {
		controller->_client->_connection.Send(
			std::vector<char>({'s'}));
	}

	controller->_connectionMutex.unlock();
}

void VideoController::ADKey(int key, int action, void* data)
{
	VideoController* controller =
		reinterpret_cast<VideoController*>(data);

	controller->_connectionMutex.lock();

	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_A) {
			controller->_client->_connection.Send(
				std::vector<char>({'l'}));
		} else if (key == GLFW_KEY_D) {
			controller->_client->_connection.Send(
				std::vector<char>({'r'}));
		}
	} else if (action == GLFW_RELEASE) {
		controller->_client->_connection.Send(
			std::vector<char>({'n'}));
	}

	controller->_connectionMutex.unlock();
}

void VideoController::QKey(int key, int action, void* data)
{
	VideoController* controller =
		reinterpret_cast<VideoController*>(data);

	controller->_connectionMutex.lock();
	controller->_client->_connection.Close();
	controller->_connectionMutex.unlock();
}

void VideoController::CursorMove(double xpos, double ypos, void* data)
{
	VideoController* controller =
		reinterpret_cast<VideoController*>(data);

	std::vector<char> msg(1 + 2 * sizeof(double));

	double offsetX = xpos - controller->_oldX;
	double offsetY = ypos - controller->_oldY;

	conroller->_oldX = xpos;
	conroller->_oldY = ypos;

	memcpy(msg.data() + 1, &offsetX, sizeof(double));
	memcpy(msg.data() + 1 + sizeof(double), &offsetY, sizeof(double));

	controller->_connectionMutex.lock();
	controller->_client->_connection.Send(msg);
	controller->_connectionMutex.unlock();
}

void VideoController::MouseButton(int button, int action, void* data)
{
	VideoController* controller =
		reinterpret_cast<VideoController*>(data);
}

void VideoController::Scroll(double xpos, double ypos, void* data)
{
	VideoController* controller =
		reinterpret_cast<VideoController*>(data);
}
