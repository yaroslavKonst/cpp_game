#include <unistd.h>
#include "../src/client/video/video.h"

int main()
{
	Video video(800, 600, true);
	sleep(10);
	return 0;
}
