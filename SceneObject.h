#pragma once
#include "Object.h"
class SceneObject :
    public Object
{
public:
    SceneObject(class Game* game);
    SceneObject* Update(float deltaTime);
    SceneObject* ProcessInput(const struct InputState&);

private:
    virtual void UpdateObject(float deltaTime)override final{}
    virtual SceneObject* UpdateScene(float deltaTime) = 0;
    virtual SceneObject* SceneInput(const InputState& inputState) = 0;
    SceneObject(class Object* parent);

protected:
    class Game* game_;
};

