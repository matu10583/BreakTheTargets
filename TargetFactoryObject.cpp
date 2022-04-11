#include "TargetFactoryObject.h"
#include "TargetObject.h"
#include <random>

TargetFactoryObject::TargetFactoryObject(
	Object* parent, ScoreTextObject* score
) :
	Object(parent),
	score_(score){
	targets_.resize(10);
	//new は遅いので最初に作って再利用する
	for (auto& t : targets_) {
		t= new TargetObject(this, score_);
		t->SetMoveType(TargetObject::MOVE_RANDOM);

		//場所の決定
		std::random_device rnd;
		int r = static_cast<int>(rnd());
		r = std::abs(r);
		float x = (r % 100) - 50;
		r = static_cast<int>(rnd());
		r = std::abs(r);
		float y = (r % 30) + 50;
		r = static_cast<int>(rnd());
		r = std::abs(r);
		float z = (r % 100) - 50;
		r = static_cast<int>(rnd());
		r = std::abs(r);
		float yrot = XM_PI / ((r % 6) + 1);
		t->SetWorldPos(x, y, z);
		t->SetWorldRot(
			XM_PIDIV2,
			yrot,
			0
		);
	}

}

void
TargetFactoryObject::UpdateObject(float deltaTime) {
	//InActiveになったターゲットの再利用を行う
	for (auto t : targets_) {
		if (t->GetState() != InActive) {
			continue;
		}
		t->Reset();
		std::random_device rnd;
		int r = static_cast<int>(rnd());
		r = std::abs(r);
		float x = (r % 100) - 50;
		r = static_cast<int>(rnd());
		r = std::abs(r);
		float y = (r % 30) + 50;
		r = static_cast<int>(rnd());
		r = std::abs(r);
		float z = (r % 100) - 50;
		r = static_cast<int>(rnd());
		r = std::abs(r);
		float yrot = XM_PI / ((r % 6) + 1);	
			
		t->SetWorldRot(
			XM_PIDIV2,
			yrot,
			0
		);
		t->SetWorldPos(x, y, z);

		t->SetState(Active);
		t->Update(deltaTime);
	}
}