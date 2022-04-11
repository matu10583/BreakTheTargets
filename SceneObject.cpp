#include "SceneObject.h"
#include "Game.h"



SceneObject::SceneObject(Game* game) :
	Object(nullptr),
	game_(game)
{
}


SceneObject* 
SceneObject::Update(float deltaTime) {
	//�V�[���ɂ��镨�̂̍X�V
	if (!game_->GetStopUpdateGameObjects()) {
		Object::Update(deltaTime);
	}
	//���̃V�[����Ԃ�
	auto next = UpdateScene(deltaTime);
	Object::DeleteDeadObject();
	return next;
}

SceneObject*
SceneObject::ProcessInput(const InputState& inputState) {
	if (!game_->GetStopUpdateGameObjects()) {
		Object::ProcessInput(inputState);
	}
	return SceneInput(inputState);
}