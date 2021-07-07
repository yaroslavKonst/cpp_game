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

	float zCoord;

	bool cModeInvert;
	bool cModeCamera;

	double angleX;
	double angleY;

	double posXOld;
	double posYOld;

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

		zCoord = 2.0;

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

		video->BindKey(GLFW_KEY_C, GLFW_PRESS, this, KeyAction);

		video->SetCursorMoveCallback(this, CursorMove);
		video->SetMouseButtonCallback(this, MouseButton);

		video->SetScrollCallback(this, Scroll);

		cModeInvert = false;
		cModeCamera = false;

		angleX = 222;
		angleY = -33;
	}

	static void KeyAction(int key, int action, void* data)
	{
		CameraController* controller =
			reinterpret_cast<CameraController*>(data);

		float* axis = nullptr;

		bool movingKey = false;

		if (key == GLFW_KEY_W) {
			controller->mult = -1.0;
			axis = &controller->deltaX;
			movingKey = true;
		} else if (key == GLFW_KEY_A) {
			controller->mult = -1.0;
			axis = &controller->deltaY;
			movingKey = true;
		} else if (key == GLFW_KEY_S) {
			controller->mult = 1.0;
			axis = &controller->deltaX;
			movingKey = true;
		} else if (key == GLFW_KEY_D) {
			controller->mult = 1.0;
			axis = &controller->deltaY;
			movingKey = true;
		} else if (key == GLFW_KEY_C) {
			controller->cModeInvert = true;
		}

		if (movingKey && action == GLFW_PRESS) {
			controller->startTime =
				std::chrono::high_resolution_clock::now();
			controller->pos = controller->currPos;
			*axis = 1.0;
		} else if (movingKey && action == GLFW_RELEASE) {
			controller->startTime =
				std::chrono::high_resolution_clock::now();
			controller->pos = controller->currPos;
			*axis = 0.0;
		}
	}

	static void CursorMove(double posX, double posY, void* data)
	{
		CameraController* controller =
			reinterpret_cast<CameraController*>(data);

		if (controller->cModeCamera) {
			float offsetX = posX - controller->posXOld;
			float offsetY = posY - controller->posYOld;

			controller->angleX -= offsetX / 12.0;
			controller->angleY -= offsetY / 12.0;
		}

		controller->posXOld = posX;
		controller->posYOld = posY;
	}

	static void MouseButton(int button, int action, void* data)
	{
		CameraController* controller =
			reinterpret_cast<CameraController*>(data);

		controller->cModeInvert = true;
	}

	static void Scroll(double xoffset, double yoffset, void* data)
	{
		CameraController* controller =
			reinterpret_cast<CameraController*>(data);

		controller->zCoord += yoffset / 50.0f;
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
				controller->zCoord
			};

			glm::vec3 dir;

			float angleX = glm::radians(controller->angleX);
			float angleY = glm::radians(controller->angleY);

			dir.x = cos(angleX) * cos(angleY);
			dir.y = sin(angleX) * cos(angleY);
			dir.z = sin(angleY);

			dir += controller->currPos;

			controller->video->SetCamera(
				&controller->currPos,
				&dir,
				nullptr);

			if (controller->cModeInvert) {
				if (controller->cModeCamera) {
					controller->video->SetNormalMouseMode();
				} else {
					controller->video->SetCameraMouseMode();
				}

				controller->cModeCamera =
					!(controller->cModeCamera);
				controller->cModeInvert = false;
			}

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

	video.CreateSkybox("../src/client/video/textures/skybox.png");

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
