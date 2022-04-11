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

	int k = 0;//移動方向の係数
	if (cps_ & CAMERA_POS_TRANSIT_TO_NODEFAULT) {
		k = 1;
	}
	else if (cps_ & CAMERA_POS_TRANSIT_TO_DEFAULT) {
		k = -1;
	}
	auto moveVec = Vector3::Zero();//移動量
	if (cps_ & CAMERA_POS_FPS) {
		moveVec = POS_FPS - POS_DEFAULT;
	}
	else if (cps_ & CAMERA_POS_SHOOT) {
		moveVec = POS_SHOOT - POS_DEFAULT;
	}

	//移動処理
		//カメラの移動速度
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
			//長押ししてるなら
			if (cps_ & 
				(CAMERA_POS_DEFAULT |
					CAMERA_POS_TRANSIT_TO_DEFAULT)) {//defからshootへ
				cps_ = (CAMERA_POS_SHOOT |
					CAMERA_POS_TRANSIT_TO_NODEFAULT);
			}
		}
	}
	else if(readyButton == ButtonState::RELEASED){//前まで押してた
		if (count_ < 0.2f) {
			//短く押してる
			if (cps_ &
				(CAMERA_POS_DEFAULT |
					CAMERA_POS_TRANSIT_TO_DEFAULT)) {
				//FPS状態に移行
				cps_ = (CAMERA_POS_FPS |
					CAMERA_POS_TRANSIT_TO_NODEFAULT);
			}
			else if (cps_ & 
				(CAMERA_POS_FPS |
					CAMERA_POS_TRANSIT_TO_NODEFAULT)) {
				//デフォルトに戻る
				cps_ = (CAMERA_POS_FPS |
					CAMERA_POS_TRANSIT_TO_DEFAULT);
			}
		}
		else {
			//長押しから離してる
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
		//離してる
		if (cps_ &
			CAMERA_POS_SHOOT) {
			cps_ = (CAMERA_POS_SHOOT |
				CAMERA_POS_TRANSIT_TO_DEFAULT);
		}
		count_ = 0;
		//なんもしてない
	}

	auto moveVec = Vector3::Zero();
	if (cps_ & CAMERA_POS_FPS) {
		moveVec = POS_FPS - POS_DEFAULT;
	}
	else if (cps_ & CAMERA_POS_SHOOT) {
		moveVec = POS_SHOOT - POS_DEFAULT;
	}
	else {
		//なにもしてない状態なのでここで終わり
		cps_ = CAMERA_POS_DEFAULT;
		return;
	}

	auto posVec = GetLocalPos() - POS_DEFAULT;
	float t = posVec.y / moveVec.y;

	//それぞれ所定の位置を通り過ぎてたら戻す
	if (t >= 1) {
		if (cps_ == (CAMERA_POS_SHOOT |
			CAMERA_POS_TRANSIT_TO_NODEFAULT)) {//shootを通りすぎた
			SetLocalPos(
				POS_SHOOT.x, POS_SHOOT.y, POS_SHOOT.z
			);
			cps_ = CAMERA_POS_SHOOT;
		}
		else if (cps_ == (CAMERA_POS_FPS |
			CAMERA_POS_TRANSIT_TO_NODEFAULT)) {//FPSを通り過ぎた
			SetLocalPos(
				POS_FPS.x, POS_FPS.y, POS_FPS.z
			);
			cps_ = CAMERA_POS_FPS;
		}
	}
	else if (t <= 0 &&
		(cps_ & CAMERA_POS_TRANSIT_TO_DEFAULT)) {//デフォルトを通り過ぎた
		SetLocalPos(
			POS_DEFAULT.x, POS_DEFAULT.y, POS_DEFAULT.z
		);
		cps_ = CAMERA_POS_DEFAULT;
	}

}