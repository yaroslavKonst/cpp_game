#include <unistd.h>
#include <chrono>
#include <cmath>
#include <thread>
#include "../src/client/video/video.h"

bool op = true;

void thr(Video* video)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	while (op) {
		auto currentTime = std::chrono::high_resolution_clock::now();

		float time = std::chrono::duration<float,
			std::chrono::seconds::period>(
			currentTime - startTime).count();

		time /= 10.0f;

		glm::vec3 pos(
			sin(time) + 2.0f,
			cos(time) + 2.0f,
			2.0f);

		video->SetCamera(&pos, nullptr, nullptr);

		usleep(1000);
	}
}

int main()
{
	std::vector<Model::Vertex> vertices = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {2.0f, 0.0f}},
		{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 2.0f}},
		{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {2.0f, 2.0f}}
	};

	std::vector<Model::VertexIndexType> indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};

	Video video("Test App", 800, 600);

	glm::vec3 pos(2.0f, 2.0f, 2.0f);
	glm::vec3 targ(0.0f, 0.0f, 0.0f);
	glm::vec3 up(0.0f, 0.0f, 1.0f);

	video.SetCamera(&pos, &targ, &up);

	std::thread camThr(thr, &video);


	std::string modelName("../src/client/video/models/viking_room.obj");
	Model* md = video.CreateModel(&modelName);

	md->SetTextureName("../src/client/video/textures/viking_room.png");

	md->modelPosition = glm::rotate(
		glm::mat4(1.0f),
		glm::radians(30.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	md->active = true;

	video.LoadModel(md);


	vertices = {
		{{-0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

		{{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {2.0f, 0.0f}},
		{{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 2.0f}},
		{{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {2.0f, 2.0f}}
	};

	Model* md1 = video.CreateModel(nullptr);

	md1->UpdateBuffers(vertices, indices);

	md1->SetTextureName("../src/client/video/textures/test_texture.png");

	md1->modelPosition = glm::rotate(
		glm::mat4(1.0f),
		glm::radians(60.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));

	md1->active = false;

	video.LoadModel(md1);


	video.Start();

	op = false;

	camThr.join();

	video.UnloadModel(md);
	video.DestroyModel(md);

	video.UnloadModel(md1);
	video.DestroyModel(md1);

	return 0;
}
