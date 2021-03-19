#ifndef ENTITY_H
#define ENTITY_H

#include "common/map.h"

class Entity
{
	virtual void Tick(Map* map) = 0;
};

#endif
