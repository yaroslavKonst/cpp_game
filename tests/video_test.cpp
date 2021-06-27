#include <unistd.h>
#include "../src/client/video/video.h"

int main()
{
	Video video("Test App", 800, 600);
	Model* md = video.LoadModel("");
	md->ubo.model = glm::rotate(
		glm::mat4(1.0f),
		glm::radians(30.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));
	md->ubo.view = glm::lookAt(
		glm::vec3(2.0f, 2.0f, 2.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));
	video.Start();
	video.DestroyModel(md);
	return 0;
}
