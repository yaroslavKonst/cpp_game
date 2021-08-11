#ifndef CLIENT_H
#define CLIENT_H

#include <atomic>
#include <mutex>
#include <thread>
#include "common/connection.h"
#include "video/video.h"
#include "common/entity.h"

class Client;

class VideoController
{
	Video* _video;
	Client* _client;

	double _oldX;
	double _oldY;

public:
	VideoController(Video* video, Client* client)
	{
		_video = video;
		_client = client;

		_video->BindKey(GLFW_KEY_W, GLFW_PRESS, this, WSKey);
		_video->BindKey(GLFW_KEY_W, GLFW_RELEASE, this, WSKey);
		_video->BindKey(GLFW_KEY_S, GLFW_PRESS, this, WSKey);
		_video->BindKey(GLFW_KEY_S, GLFW_RELEASE, this, WSKey);

		_video->BindKey(GLFW_KEY_A, GLFW_PRESS, this, ADKey);
		_video->BindKey(GLFW_KEY_A, GLFW_RELEASE, this, ADKey);
		_video->BindKey(GLFW_KEY_D, GLFW_PRESS, this, ADKey);
		_video->BindKey(GLFW_KEY_D, GLFW_RELEASE, this, ADKey);

		_video->BindKey(GLFW_KEY_Q, GLFW_PRESS, this, QKey);

		_video->SetCursorMoveCallback(this, CursorMove);
		_video->SetMouseButtonCallback(this, MouseButton);
		_video->SetScrollCallback(this, Scroll);
	}

	static void WSKey(int key, int action, void* data);
	static void ADKey(int key, int action, void* data);
	static void QKey(int key, int action, void* data);
	static void CursorMove(double xpos, double ypos, void* data);
	static void MouseButton(int button, int action, void* data);
	static void Scroll(double xpos, double ypos, void* data);
};

class Client
{
public:
	Client()
	{
		_work = false;
		_video = new Video("Moving Castles", 1000, 700);
		_videoController = new VideoController(_video, this);
		_videoThread = new std::thread(VideoThread, this);
	}

	~Client()
	{
		_video->Stop();
		_videoThread->join();
		delete _videoThread;
		delete _videoController;
		delete _video;
	}

	void SetConnection(Connection connection)
	{
		_connection = connection;
	}

	static void Thread(Client* client);
	static void VideoThread(Client* client);

	void Start();
	void Stop();

private:
	Connection _connection;
	std::mutex _connectionMutex;

	Video* _video;
	VideoController* _videoController;

	std::atomic<bool> _work;

	std::thread* _thread;
	std::thread* _videoThread;
};

#endif
