#include "SceneObject.h"
#include "Game.h"



SceneObject::SceneObject(Game* game) :
	Object(nullptr),
	game_(game)
{
}


SceneObject* 
SceneObject::Update(float deltaTime) {
	//シーンにある物体の更新
	if (!game_->GetStopUpdateGameObjects()) {
		Object::Update(deltaTime);
	}
	//次のシーンを返す
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