#include "PhysicsSystem.h"
#include "Object.h"

PhysicsSystem* PhysicsSystem::instance_ = nullptr;

void 
PhysicsSystem::Create() {
	assert(!instance_ && "��x����");
	instance_ = new PhysicsSystem();
}

void 
PhysicsSystem::Destroy() {
	delete instance_;
}

PhysicsSystem*
PhysicsSystem::Instance() {
	return instance_;
}

void
PhysicsSystem::Update(float deltaTime) {
	//�R���C�_�[�̌`���X�V
	for (auto& co : colliders_) {
		co->ClearCollisionInfo();
		co->UpdateCollider();
	}
	auto it = colliders_.begin();
	//����������s��
	for (; it < colliders_.end(); it++) {
		for (auto iit = it + 1; iit < colliders_.end(); iit++) {
			auto& self = *it;
			auto& other = *iit;
			//�ǂ����Active��
			if (self->GetOwner().GetState() == Object::Active &&
				self->GetState() == Component::CActive &&
				other->GetOwner().GetState() == Object::Active &&
				other->GetState() == Component::CActive) {
				if (self->BroadIntersect(other->GetWorldAABB())) {
					self->Intersect(other);
				}
			}
		}
	}
	ResolveConflict();

	//�ǉ����s��
	for (auto& p_obj : pendColliders_) {
		colliders_.emplace_back(p_obj);
	}
	pendColliders_.clear();
}

void
PhysicsSystem::ResolveConflict() {
	for (auto& ccomp : colliders_) {
		auto& cis = ccomp->GetCollisionInfos();
		for (auto ci : cis) {
			auto a = ci.collider[0];
			auto b = ci.collider[1];
			a->GetOwner().UpdateOnCollision(ci);
			//�����ς���
			std::swap(ci.collider[0], ci.collider[1]);
			ci.outVec_ *= -1;
			b->GetOwner().UpdateOnCollision(ci);
		}
	}
}

void
PhysicsSystem::AddCollider(ColliderComponent* co) {
	pendColliders_.emplace_back(co);
}

void
PhysicsSystem::RemoveCollider(ColliderComponent* co) {
	auto idx = std::find(colliders_.begin(), colliders_.end(), co);
	if (idx != colliders_.end()) {
		colliders_.erase(idx);
		return;
	}

	auto pidx = std::find(pendColliders_.begin(), pendColliders_.end(), co);
	if (pidx == pendColliders_.end()) {
		assert(NULL && "�R���C�_�[��������܂���");
	}
	else {
		pendColliders_.erase(pidx);
	}
}