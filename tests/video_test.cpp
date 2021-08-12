#include <unistd.h>
#include <chrono>
#include <cmath>
#include <thread>
#include <mutex>
#include "../src/client/video/video.h"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

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

class VideoController
{
	Video* video;

public:
	VideoController()
	{
		video = new Video("Test App", 1700, 900);
	}

	~VideoController()
	{
		delete video;
	}

	Video* GetVideo()
	{
		return video;
	}

	static void thr(VideoController* controller)
	{
		controller->video->Start();
	}
};

class InterfaceTestObject: public InterfaceObject
{
public:
	InterfaceTestObject(bool v) : InterfaceObject(v)
	{ }

private:
	virtual bool MouseButton(int button, int action)
	{
		std::cout << "Button action\n";
		return false;
	}

	virtual bool CursorMove(double posX, double posY, bool inArea)
	{
		if (inArea) {
			std::cout << "Cursor " << posX << " " << posY << "\n";
		}

		return false;
	}

	virtual bool Scroll(double posX, double posY)
	{
		std::cout << "Scroll " << posX << " " << posY << "\n";

		return false;
	}
};

int main()
{
	VideoController videoController;

	Video* video = videoController.GetVideo();

	glm::vec3 pos(2.0f, 2.0f, 2.0f);
	glm::vec3 targ(0.0f, 0.0f, 0.0f);
	glm::vec3 up(0.0f, 0.0f, 1.0f);

	video->SetCamera(&pos, &targ, &up);

	CameraController controller(video);

	std::thread camThr(CameraController::thr, &controller);

	video->CreateSkybox("../src/client/video/textures/skybox.png");

	std::string modelName("../src/client/video/models/viking_room.obj");
	Model* md = video->CreateModel(&modelName);

	md->SetTextureName("../src/client/video/textures/viking_room.png");

	auto& inst1 = video->AddInstance(md, 1);
	auto& inst2 = video->AddInstance(md, 3);

	inst1.modelPosition = glm::mat4(1.0f);
	inst1.partPosition = glm::mat4(1.0f);
	inst1.instancePositions[0] = glm::mat4(1.0f);

	inst1.active = true;

	inst2.modelPosition = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(0.0f, 2.0f, 0.0f));

	inst2.modelPosition = glm::rotate(
		inst2.modelPosition,
		glm::radians(-90.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	inst2.partPosition = glm::mat4(1.0f);

	inst2.instancePositions[0] = glm::scale(glm::vec3(0.5f, 0.5f, 0.5f));

	inst2.instancePositions[1] = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(0.0f, -2.0f, 0.0f));

	inst2.instancePositions[2] = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(0.0f, -4.0f, 0.0f));

	inst2.active = true;

	video->LoadModel(md);

	InterfaceObject interf1(true);
	interf1.textureName = "../src/client/video/textures/skybox.png";
	interf1.area.x0 = -0.9;
	interf1.area.y0 = -0.9;
	interf1.area.x1 = -0.4;
	interf1.area.y1 = -0.4;
	interf1.active = true;
	interf1.depth = 0;

	InterfaceObject interf3(true);
	interf3.textureName = "../src/client/video/textures/skybox.png";
	interf3.area.x0 = -0.8;
	interf3.area.y0 = -0.8;
	interf3.area.x1 = -0.5;
	interf3.area.y1 = -0.5;
	interf3.active = true;
	interf3.depth = 4;

	InterfaceTestObject interf2(true);
	interf2.textureName = "../src/client/video/textures/viking_room.png";
	interf2.area.x0 = -0.9;
	interf2.area.y0 = 0.7;
	interf2.area.x1 = -0.4;
	interf2.area.y1 = 0.9;
	interf2.active = true;
	interf2.depth = 1;

	InterfaceTestObject interf4(false);
	interf4.area.x0 = 0.6;
	interf4.area.y0 = 0.7;
	interf4.area.x1 = 0.9;
	interf4.area.y1 = 0.9;
	interf4.active = true;
	interf4.depth = 1;

	video->LoadInterface(&interf1);
	video->LoadInterface(&interf2);
	video->LoadInterface(&interf3);
	video->LoadInterface(&interf4);


	Model* leaves = video->CreateModel(nullptr);
	leaves->SetTextureName("../src/client/video/textures/leaf.png");

	std::vector<Model::Vertex> leafVertices = {
		{{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
		{{0.3f, 0.0f, 0.0f}, {1.0f, 1.0f}},
		{{0.2f, 0.0f, 1.0f}, {0.0f, 0.0f}}
	};

	std::vector<Model::VertexIndexType> leafIndices = {
		0, 1, 2, 0, 2, 1
	};

	leaves->UpdateBuffers(leafVertices, leafIndices);

	int leafWidth = 10;
	int leafHeight = 10;

	auto& leafInst = video->AddInstance(leaves, leafWidth * leafHeight);
	leafInst.active = true;

	leafInst.modelPosition = glm::mat4(1.0f);
	leafInst.partPosition = glm::mat4(1.0f);

	for (int x = 0; x < leafWidth; ++x) {
		for (int y = 0; y < leafHeight; ++y) {
			int idx = y * leafWidth + x;

			leafInst.instancePositions[idx] =
				glm::translate(glm::vec3(
					float(x) * 0.1,
					float(y) * 0.1,
					-1.0f));
		}
	}

	video->LoadModel(leaves);

	std::thread videoThr(VideoController::thr, &videoController);

	sleep(5);

	while (video->Work()) {
		inst1.modelPosition = glm::translate(
			inst1.modelPosition,
			glm::vec3(0.0f, 0.0001f, 0.0f));

		usleep(1000);
	}

	videoThr.join();

	controller.op = false;

	camThr.join();

	video->UnloadModel(md);
	video->DestroyModel(md);

	video->UnloadInterface(&interf1);
	video->UnloadInterface(&interf2);
	video->UnloadInterface(&interf3);
	video->UnloadInterface(&interf4);

	return 0;
}
