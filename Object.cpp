#include "Object.h"
#include "Game.h"
#include "Component.h"
#include "ColliderComponent.h"
#include <algorithm>
#include <assert.h>

Object::Object(Object* parent, bool iskinematic):
	state_(Object::Active),
	updatingObject_(false),
	position_(0, 0, 0),
	rotation_(0, 0, 0),
	scale_(1,1,1),
	isKinematic(iskinematic),
	isDraw(true)
{
	parent_ = parent;
	if (parent_ != nullptr) {
		parent->AddChild(this);
	}
}

Object::~Object() {
	while (!components_.empty())
	{
		//�R���|�[�l���g�̍폜
		delete components_.back();
	}
	while (!children_.empty())
	{
		//�q�I�u�W�F�N�g�̍폜
		delete children_.back();
	}

	if (parent_ != nullptr) {
		parent_->RemoveChild(this);
	}
}

void 
Object::Update(float deltaTime) {
	updatingObject_ = true;
	if (state_ == Active) {
		UpdateChildren(deltaTime);
		UpdateComponents(deltaTime);
		UpdateObject(deltaTime);
	}
	updatingObject_ = false;

	//���ǉ��I�u�W�F�N�g��ǉ�����
	for (auto& pending_obj : pendingObjects_) {
		children_.emplace_back(pending_obj);
	}
	pendingObjects_.clear();


}

void
Object::DeleteDeadObject() {
	//State:DEAD�̃I�u�W�F�N�g���폜
	std::vector<Object*> deadObj;

	for (auto& obj : children_) {
		if (obj->GetState() == Object::Dead) {
			deadObj.emplace_back(obj);
		}
	}

	for (auto& obj : deadObj) {
		delete obj;//�f�X�g���N�^��removeChildren���Ă�ł���
		obj = nullptr;
	}
}

void 
Object::UpdateOnCollision(const CollisionInfo& ci) {
	Vector3 outV;
	if (isKinematic) {
		if (ci.collider[1]->GetOwner().IsKinematic()) {
			//�����������kinematic�Ȃ甼�����ǂ�
			outV = ci.outVec_ / 2.0f;
		}
		else {
			//��������kinematic�Ȃ玩�����ǂ�
			outV = ci.outVec_;
		}
	}
	else {
		//������kinematic����Ȃ��Ȃ�ǂ��Ȃ�
		outV = Vector3::Zero();
	}
	if (!ci.collider[0]->IsTrigger() &&
		!ci.collider[1]->IsTrigger()) {
		//�ǂ�������trigger�Ȃ炻�������ʒu�𓮂����Ȃ�
		AddWorldPos(outV.x, outV.y, outV.z);
	}

	OnCollision(ci);
}

void 
Object::OnCollision(const CollisionInfo& ci) {
	//�Փˎ��̏���
}

void
Object::ProcessInput(const InputState& inputState) {
	if (state_ != Active) {
		return;
	}
	updatingObject_ = true;
		for (auto& child : children_) {
			child->ProcessInput(inputState);
		}
		for (auto& comp : components_) {
			if (comp->GetState() == Component::CActive) {
				comp->ProcessInput(inputState);
			}
		}
		ActorInput(inputState);
	updatingObject_ = false;
}

void
Object::ActorInput(const InputState& inputState) {

}

void
Object::SetObjDataFromJson(const nlohmann::json& objjs) {
	//���ʂ̃p�����[�^���Z�b�g
	float pos[3];
	float rot[3];
	float sca[3];
	for (int i = 0; i < 3; i++) {
		pos[i] = objjs["Pos"][i].get<int>();
		rot[i] = objjs["Rot"][i].get<int>() * (XM_PI / 180.0f);
		sca[i] = objjs["Scale"][i].get<int>() * 0.01f;
	}
	SetLocalPos(pos[0], pos[1], pos[2]);
	SetLocalRot(rot[0], rot[1], rot[2]);
	SetLocalScale(Vector3(
		sca[0], sca[1], sca[2]));
	//�`�悷�邩���Ȃ���
	SetDraw(
		objjs["IsDraw"].get<bool>()
	);

	SetIsKinematic(
		objjs["IsKinematic"].get<bool>()
	);
	
	//�A�f�B�V���i��������΂�����ݒ肷��
	if (objjs.count("Additional") != 0) {
		SetAdditionalDataFromJson(objjs["Additional"]);
	}
}

nlohmann::json
Object::GetJsonFromObj(std::string objID, std::string objName) {
	//���ʂ̃p�����[�^���Z�b�g
	auto pos = GetLocalPos();
	auto rot = GetLocalRot() * (180.0f / XM_PI);//�x���ɂ���
	auto sca = GetLocalScale() * 100;//�X�P�[���͕S����
	bool isDraw = IsDraw();
	bool isK = IsKinematic();
	nlohmann::json dat =
	{
		{"ObjID", objID},
		{"Name", objName},
		{"Pos", {(int)pos.x, (int)pos.y, (int)pos.z}},
		{"Rot", {(int)rot.x, (int)rot.y, (int)rot.z}},
		{"Scale", {(int)sca.x, (int)sca.y, (int)sca.z}},
		{"IsDraw", isDraw},
		{"IsKinematic", isK}
	};
	GetAdditionalJsonFromObj(dat);

	return dat;
}

void 
Object::AddComponent(Component* comp) {
	components_.emplace_back(comp);
}

void 
Object::RemoveComponent(Component* comp) {
	auto idx = std::find(components_.begin(), components_.end(), comp);
	if (idx == components_.end()) {
		assert(NULL && "�R���|�[�l���g��������܂���");
	}
	else {
		components_.erase(idx);
	}
}

void
Object::RemoveChild(Object* child) {
	//��̃I�u�W�F�N�g���X�g�̒���T��
	auto idx = std::find(pendingObjects_.begin(), pendingObjects_.end(), child);
	if (idx == pendingObjects_.end()) {
		idx = std::find(children_.begin(), children_.end(), child);
		if (idx == children_.end()) {
			assert(NULL && "�I�u�W�F�N�g��������܂���");
		}
		else {
			children_.erase(idx);
		}
	}
	else {
		pendingObjects_.erase(idx);
	}
}

void
Object::AddChild(Object* child) {
	if (updatingObject_) {
		pendingObjects_.emplace_back(child);
	}
	else {
		children_.emplace_back(child);
	}
}

const Object::State 
Object::GetState()const {
	return state_;
}

void 
Object::SetState(State st) {
	state_ = st;
}

void 
Object::SetLocalPos(float x, float y, float z) {
	position_ = Vector3(x, y, z);
}

void 
Object::SetLocalRot(float x, float y, float z) {
	rotation_ = Vector3(x, y, z);
}

const Vector3&
Object::GetLocalPos()const {
	return position_;
}

const Vector3&
Object::GetLocalRot()const {
	return rotation_;
}

void
Object::SetWorldPos(float x, float y, float z) {
	if (parent_ == nullptr) {
		SetLocalPos(x, y, z);
		return;
	}
	auto worldPos = Vector3(x, y, z);
	//���[�J����
	auto punitX = parent_->GetRight();
	auto punitY = parent_->GetUp();
	auto punitZ = parent_->GetForward();
	auto pWorldPos = parent_->GetWorldPos();
	//���[�J���x�N�g��
	auto localVec = worldPos - pWorldPos;
	//���ꂼ�ꎲ�ɓ��e
	position_.x = Dot(punitX, localVec);
	position_.y = Dot(punitY, localVec);
	position_.z = Dot(punitZ, localVec);

}

void
Object::SetWorldRot(float x, float y, float z) {
	if (parent_ == nullptr) {
		SetLocalPos(x, y, z);
		return;
	}
	auto worldRot = Vector3(x, y, z);
	rotation_ = worldRot - (parent_->GetWorldRot());
}

void
Object::SetWorldScale(const Vector3& sca) {
	if (parent_ == nullptr) {
		SetLocalScale(sca);
		return;
	}
	auto worldSca = sca;
	auto pScale = (parent_->GetWorldScale());
	scale_ = Vector3(
		worldSca.x / pScale.x,
		worldSca.y / pScale.y,
		worldSca.z / pScale.z
	);
}

const Vector3
Object::GetWorldPos()const {
	Vector3 worldPos = Vector3(0, 0, 0);
	if (parent_ != nullptr) {//�e�̃X�P�[���̉e����������
		auto pWorldScale = parent_->GetWorldScale();
		auto pworldPos = parent_->GetWorldPos();
		auto unitX = parent_->GetRight() * pWorldScale.x;
		auto unitY = parent_->GetUp() * pWorldScale.y;
		auto unitZ = parent_->GetForward() * pWorldScale.z;
		worldPos =pworldPos +
			(
				unitX * position_.x +
				unitY * position_.y +
				unitZ * position_.z
				);
	}
	return worldPos;
}

const Vector3
Object::GetWorldRot()const {
	auto worldRot = GetLocalRot();
	if (parent_ != nullptr) {
		auto pWorldRot = parent_->GetWorldRot();
		worldRot += pWorldRot;
	}
	return worldRot;
}

const Vector3
Object::GetWorldScale()const {
	auto worldSca = scale_;
	if (parent_ != nullptr) {
		auto pWorldSca = parent_->GetWorldScale();
		worldSca = Vector3(
			scale_.x * pWorldSca.x,
			scale_.y * pWorldSca.y,
			scale_.z * pWorldSca.z
		);
	}
	return worldSca;
}

const Vector3
Object::GetForward()const {
	//��̃x�N�g��
	Vector3 unitZ = Vector3(0, 0, 1);
	auto unitZvec = XMLoadFloat3(&unitZ);
	auto wRot = GetWorldRot();
	//��]�s��
	XMMATRIX rotmat = XMMatrixRotationRollPitchYaw(
		wRot.x, wRot.y, wRot.z);
	auto forVec = XMVector3Transform(unitZvec, rotmat);
	auto ret = Vector3(
		forVec.m128_f32[0],
		forVec.m128_f32[1],
		forVec.m128_f32[2]
	);
	return ret;

}

const Vector3
Object::GetRight()const {
	//��̃x�N�g��
	Vector3 unitX = Vector3(1, 0, 0);
	auto unitXvec = XMLoadFloat3(&unitX);
	auto wRot = GetWorldRot();
	//��]�s��
	XMMATRIX rotmat = XMMatrixRotationRollPitchYaw(
		wRot.x, wRot.y, wRot.z);
	auto forVec = XMVector3Transform(unitXvec, rotmat);
	auto ret = Vector3(
		forVec.m128_f32[0],
		forVec.m128_f32[1],
		forVec.m128_f32[2]
	);
	return ret;

}

const Vector3
Object::GetUp()const {
	//��̃x�N�g��
	Vector3 unitY = Vector3(0, 1, 0);
	auto unitYvec = XMLoadFloat3(&unitY);
	auto wRot = GetWorldRot();
	//��]�s��
	XMMATRIX rotmat = XMMatrixRotationRollPitchYaw(
		wRot.x, wRot.y, wRot.z);
	auto forVec = XMVector3Transform(unitYvec, rotmat);
	auto ret= Vector3(
		forVec.m128_f32[0],
		forVec.m128_f32[1],
		forVec.m128_f32[2]
	);
	return ret;

}

bool
Object::IsKinematic()const {
	return isKinematic;
}

void
Object::AddLocalRot(float roll, float pitch, float yaw) {
	rotation_ += Vector3(roll, pitch, yaw);
}

void
Object::AddLocalPos(float x, float y, float z) {
	position_ += GetRight() * x +
		GetUp() * y +
		GetForward() * z;
}

void
Object::AddWorldRot(float roll, float pitch, float yaw) {
	rotation_ += Vector3(roll, pitch, yaw);
}

void
Object::AddWorldPos(float x, float y, float z) {
	position_ += Vector3(x, y, z);
}

void
Object::UpdateChildren(float deltaTime) {
	//�Q�[������
	for (auto& obj : children_) {
		obj->Update(deltaTime);
		obj->DeleteDeadObject();
	}
}


void Object::UpdateComponents(float deltaTime) {
	for (auto& comp : components_) {
		if (comp->GetState() == Component::CActive) {
			comp->Update(deltaTime);
		}
	}
}