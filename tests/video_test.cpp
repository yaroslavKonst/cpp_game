#include <unistd.h>
#include <chrono>
#include <cmath>
#include <thread>
#include "../src/client/video/video.h"

class CameraController
{
public:
	Video* video;
	bool op;

	float deltaX;
	float deltaY;
	float mult;

	std::chrono::high_resolution_clock::time_point startTime;

	glm::vec3 pos;
	glm::vec3 currPos;

	CameraController(Video* video)
	{
		this->video = video;
		op = true;

		deltaX = 0.0;
		deltaY = 0.0;
		mult = 0.0;

		pos = {2.0, 2.0, 2.0};
		currPos = pos;

		startTime = std::chrono::high_resolution_clock::now();

		video->BindKey(GLFW_KEY_W, GLFW_PRESS, this, KeyAction);
		video->BindKey(GLFW_KEY_W, GLFW_RELEASE, this, KeyAction);

		video->BindKey(GLFW_KEY_A, GLFW_PRESS, this, KeyAction);
		video->BindKey(GLFW_KEY_A, GLFW_RELEASE, this, KeyAction);

		video->BindKey(GLFW_KEY_S, GLFW_PRESS, this, KeyAction);
		video->BindKey(GLFW_KEY_S, GLFW_RELEASE, this, KeyAction);

		video->BindKey(GLFW_KEY_D, GLFW_PRESS, this, KeyAction);
		video->BindKey(GLFW_KEY_D, GLFW_RELEASE, this, KeyAction);
	}

	static void KeyAction(int key, int action, void* data)
	{
		CameraController* controller =
			reinterpret_cast<CameraController*>(data);

		float* axis = nullptr;

		if (key == GLFW_KEY_W) {
			controller->mult = -1.0;
			axis = &controller->deltaX;
		} else if (key == GLFW_KEY_A) {
			controller->mult = -1.0;
			axis = &controller->deltaY;
		} else if (key == GLFW_KEY_S) {
			controller->mult = 1.0;
			axis = &controller->deltaX;
		} else if (key == GLFW_KEY_D) {
			controller->mult = 1.0;
			axis = &controller->deltaY;
		}

		if (action == GLFW_PRESS) {
			controller->startTime =
				std::chrono::high_resolution_clock::now();
			controller->pos = controller->currPos;
			*axis = 1.0;
		} else if (action == GLFW_RELEASE) {
			controller->startTime =
				std::chrono::high_resolution_clock::now();
			controller->pos = controller->currPos;
			*axis = 0.0;
		}
	}

	static void thr(CameraController* controller)
	{
		while (controller->op) {
			auto currentTime =
				std::chrono::high_resolution_clock::now();

			float time = std::chrono::duration<float,
				std::chrono::seconds::period>(
				currentTime - controller->startTime).count();

			time /= 2.0f;

			controller->currPos = {
				controller->pos.x +
					controller->deltaX * controller->mult *
					time,
				controller->pos.y +
					controller->deltaY * controller->mult *
					time,
				2.0
			};

			controller->video->SetCamera(
				&controller->currPos,
				nullptr,
				nullptr);

			usleep(1000);
		}
	}
};

int main()
{
	Video video("Test App", 800, 600);

	glm::vec3 pos(2.0f, 2.0f, 2.0f);
	glm::vec3 targ(0.0f, 0.0f, 0.0f);
	glm::vec3 up(0.0f, 0.0f, 1.0f);

	video.SetCamera(&pos, &targ, &up);

	CameraController controller(&video);

	std::thread camThr(CameraController::thr, &controller);

	std::string modelName("../src/client/video/models/viking_room.obj");
	Model* md = video.CreateModel(&modelName);

	md->SetTextureName("../src/client/video/textures/viking_room.png");

	md->modelPosition = glm::rotate(
		glm::mat4(1.0f),
		glm::radians(0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	md->active = true;

	video.LoadModel(md);

	video.Start();

	controller.op = false;

	camThr.join();

	video.UnloadModel(md);
	video.DestroyModel(md);

	return 0;
}
