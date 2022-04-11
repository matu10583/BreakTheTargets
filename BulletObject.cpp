#include "BulletObject.h"
#include "Collider.h"
#include "LineColliderComponent.h"
#include "MeshComponent.h"
#include "MoveComponent.h"

BulletObject::BulletObject(Object* parent, Object* shooter):
Object(parent),
age_(0),
lifespan_(5),
vel_(1000),
shooter_(shooter)
{
	//判定は線.点をスイープさせて判定するのでとりあえず長さは零
	Line bul;
	bul.start_ = Vector3::Zero();
	bul.end_ = Vector3::Zero();
	col_ = new LineColliderComponent(this, bul, true);
	col_->SetType(TYPE_BULLET);

	move_ = new MoveComponent(this);
	move_->SetVelocity(0, 0, vel_);
	move_->SetMaxVel(vel_);


	model_ = new MeshComponent(this, "Model/bullet.pmd");
}

void
BulletObject::UpdateObject(float deltaTime) {
	age_ += deltaTime;
	if (age_ > lifespan_) {
		//寿命
		SetState(Dead);
	}
	//点をスイープさせる
	auto moveVec = Vector3(0, 0, 1) * vel_ * deltaTime;
	Line line;
	line.start_ = Vector3::Zero();
	line.end_ = moveVec;
	col_->SetLine(line);
}

void
BulletObject::OnCollision(const CollisionInfo& ci) {
	const auto self = ci.collider[0];
	const auto other = ci.collider[1];
	auto otype = other->GetType();

	if (otype & TYPE_TARGET
		&&
		&other->GetOwner() != shooter_) {
		//当たったのがターゲットで打ったやつでなければInactive
		//Factoryに作られてるやつは殺すと再生成が必要になるので再利用する
		if (other->GetOwner().GetParent()->GetParent() == nullptr) {
			other->GetOwner().SetState(Dead);
		}
		else {
			other->GetOwner().SetState(InActive);
		}

	}
}

