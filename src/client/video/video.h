#ifndef VIDEO_H
#define VIDEO_H

#include "includes.h"

#include <string>

#include "instance_holder.h"

class Video
{
public:
	Video(const std::string& name, uint32_t width, uint32_t height);
	~Video();

private:
	InstanceHolder _instanceHolder;
}

#endif
