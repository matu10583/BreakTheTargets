#pragma once
#include "Object.h"
class FreeCameraObject :
    public Object
{
public:

    FreeCameraObject(class Object* parent);
    ~FreeCameraObject();

private:
    void UpdateObject(float deltaTime) override;

    class PlayerMoveComponent* playerMoveComp_;
    class CameraComponent* cameraComp_;
    class CameraMoveComponent* moveComp_;

};

