#ifndef ENTITY_H
#define ENTITY_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

class Entity
{
public:
	uint64_t id;

	virtual ~Entity()
	{ }

	virtual glm::mat4 GetPosition() = 0;

	virtual void Tick() = 0;
};

class EntityFactory
{
public:
	virtual ~EntityFactory();
	{ }

	virtual Entity* GetEntity(int id) = 0;
};

#endif
