#pragma once
#include "Object.h"

class ReticleObject:
	public Object
{
public:
	ReticleObject(class Object* parent);

private:
	void UpdateObject(float deltaTime)override;
	class SpriteComponent* reticleSprite_;
};

