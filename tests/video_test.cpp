#include <unistd.h>
#include "../src/client/video/video.h"

int main()
{
	Video video(800, 600, true);
	video.Start();
	return 0;
}
