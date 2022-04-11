#include "CameraObject.h"
#include "Object.h"
#include "CameraComponent.h"
#include "CameraMoveComponent.h"
#include "InputSystem.h"
#include "Collider.h"

CameraObject::CameraObject(Object* parent):
Object(parent),
cps_(CAMERA_POS_DEFAULT),
invMoveTime_(1.0f/0.05)
{
	 cameraComp_ = new CameraComponent(this);
	 moveComp_ = new CameraMoveComponent(this);
}

CameraObject::~CameraObject() {
}

void
CameraObject::UpdateObject(float deltaTime){
	count_ += deltaTime;

	int k = 0;//�ړ������̌W��
	if (cps_ & CAMERA_POS_TRANSIT_TO_NODEFAULT) {
		k = 1;
	}
	else if (cps_ & CAMERA_POS_TRANSIT_TO_DEFAULT) {
		k = -1;
	}
	auto moveVec = Vector3::Zero();//�ړ���
	if (cps_ & CAMERA_POS_FPS) {
		moveVec = POS_FPS - POS_DEFAULT;
	}
	else if (cps_ & CAMERA_POS_SHOOT) {
		moveVec = POS_SHOOT - POS_DEFAULT;
	}

	//�ړ�����
		//�J�����̈ړ����x
	auto v = moveVec * invMoveTime_;
	v *= k * deltaTime;
	AddWorldPos(
		v.x, v.y, v.z
	);
}

void
CameraObject::ActorInput(const InputState& inputState) {
	auto readyButton = inputState.kb.GetMappedButtonState(READY_SHOOT);

	if (inputState.kb.IsMappedButtonOn(READY_SHOOT)) {
		if (count_ > 0.2f) {
			//���������Ă�Ȃ�
			if (cps_ & 
				(CAMERA_POS_DEFAULT |
					CAMERA_POS_TRANSIT_TO_DEFAULT)) {//def����shoot��
				cps_ = (CAMERA_POS_SHOOT |
					CAMERA_POS_TRANSIT_TO_NODEFAULT);
			}
		}
	}
	else if(readyButton == ButtonState::RELEASED){//�O�܂ŉ����Ă�
		if (count_ < 0.2f) {
			//�Z�������Ă�
			if (cps_ &
				(CAMERA_POS_DEFAULT |
					CAMERA_POS_TRANSIT_TO_DEFAULT)) {
				//FPS��ԂɈڍs
				cps_ = (CAMERA_POS_FPS |
					CAMERA_POS_TRANSIT_TO_NODEFAULT);
			}
			else if (cps_ & 
				(CAMERA_POS_FPS |
					CAMERA_POS_TRANSIT_TO_NODEFAULT)) {
				//�f�t�H���g�ɖ߂�
				cps_ = (CAMERA_POS_FPS |
					CAMERA_POS_TRANSIT_TO_DEFAULT);
			}
		}
		else {
			//���������痣���Ă�
			if (cps_ & 
				(CAMERA_POS_SHOOT |
					CAMERA_POS_TRANSIT_TO_NODEFAULT)) {
				cps_ = (CAMERA_POS_SHOOT |
					CAMERA_POS_TRANSIT_TO_DEFAULT);
			}
		}
		count_ = 0;
	}
	else {
		//�����Ă�
		if (cps_ &
			CAMERA_POS_SHOOT) {
			cps_ = (CAMERA_POS_SHOOT |
				CAMERA_POS_TRANSIT_TO_DEFAULT);
		}
		count_ = 0;
		//�Ȃ�����ĂȂ�
	}

	auto moveVec = Vector3::Zero();
	if (cps_ & CAMERA_POS_FPS) {
		moveVec = POS_FPS - POS_DEFAULT;
	}
	else if (cps_ & CAMERA_POS_SHOOT) {
		moveVec = POS_SHOOT - POS_DEFAULT;
	}
	else {
		//�Ȃɂ����ĂȂ���ԂȂ̂ł����ŏI���
		cps_ = CAMERA_POS_DEFAULT;
		return;
	}

	auto posVec = GetLocalPos() - POS_DEFAULT;
	float t = posVec.y / moveVec.y;

	//���ꂼ�ꏊ��̈ʒu��ʂ�߂��Ă���߂�
	if (t >= 1) {
		if (cps_ == (CAMERA_POS_SHOOT |
			CAMERA_POS_TRANSIT_TO_NODEFAULT)) {//shoot��ʂ肷����
			SetLocalPos(
				POS_SHOOT.x, POS_SHOOT.y, POS_SHOOT.z
			);
			cps_ = CAMERA_POS_SHOOT;
		}
		else if (cps_ == (CAMERA_POS_FPS |
			CAMERA_POS_TRANSIT_TO_NODEFAULT)) {//FPS��ʂ�߂���
			SetLocalPos(
				POS_FPS.x, POS_FPS.y, POS_FPS.z
			);
			cps_ = CAMERA_POS_FPS;
		}
	}
	else if (t <= 0 &&
		(cps_ & CAMERA_POS_TRANSIT_TO_DEFAULT)) {//�f�t�H���g��ʂ�߂���
		SetLocalPos(
			POS_DEFAULT.x, POS_DEFAULT.y, POS_DEFAULT.z
		);
		cps_ = CAMERA_POS_DEFAULT;
	}

}