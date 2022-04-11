#include "PlayerObject.h"
#include "CameraObject.h"
#include "Game.h"
#include "MeshComponent.h"
#include "PlayerMoveComponent.h"
#include "CapsuleColliderComponent.h"
#include "AABBColliderComponent.h"
#include "OBBColliderComponent.h"
#include "Collider.h"
#include "InputSystem.h"
#include "ReticleObject.h"
#include "BulletObject.h"
#include "Dx12Wrapper.h"
#include "AudioSystem.h"
PlayerObject::PlayerObject(Object* parent):
Object(parent),
forOnGround_(false),
onGround_(false),
doRun_(false),
bNum_(15)
{
	//モデルの読み込み
	auto model = new MeshComponent(this, "Model/miku_gun.pmd");
	model->SetIsLeftHanded(false);
	//モーションの読み込み
	breath_ = model->AddAnimationNode("Motion/1.呼吸_(90f_移動なし).vmd");
	run_ = model->AddAnimationNode("Motion/2.走り40L_ジョギング_(20f_前移動40).vmd");
	jump_ = model->AddAnimationNode("Motion/1.歩きから片足ジャンプ(22f_前移動25).vmd", false, 0);
	fall1_ = model->AddAnimationNode("Motion/2.落下_(10f_前移動10~20・下移動20~30).vmd");
	fall2_ = model->AddAnimationNode("Motion/3.┗落下L_足伸ばし姿勢_(25f_前・下移動任意).vmd");
	gun_ = model->AddAnimationNode("Motion/ハンドガン構え.vmd", false, 0);
	gunkeep_ = model->AddAnimationNode("Motion/ハンドガン構え維持.vmd", true, 0);
	gunshoot_ = model->AddAnimationNode("Motion/ハンドガン射撃.vmd", true, 5);

	model->SetEntryAnimationNode(breath_);
	//breath~
	model->MakeAnimationTransit(breath_, run_, &doRun_);
	model->MakeAnimationTransit(breath_, jump_, &doJump_);
	model->MakeAnimationTransit(breath_, fall1_, &doFall_);
	model->MakeAnimationTransit(breath_, gun_, &doGunReady_);

	//run~
	model->MakeAnimationTransit(run_, breath_, &doNeutral_);
	model->MakeAnimationTransit(run_, jump_, &doJump_);
	model->MakeAnimationTransit(run_, fall1_, &doFall_);
	model->MakeAnimationTransit(run_, gun_, &doGunReady_);
	
	//jump~
	model->MakeAnimationTransit(jump_, fall1_, nullptr, true, false);
	model->MakeAnimationTransit(jump_, breath_, &doNeutral_);
	model->MakeAnimationTransit(jump_, gun_, &doGunReady_);

	//fall1~
	model->MakeAnimationTransit(fall1_, fall2_, nullptr, true, false);
	model->MakeAnimationTransit(fall1_, breath_, &doNeutral_);
	model->MakeAnimationTransit(fall1_, gun_, &doGunReady_);
	model->MakeAnimationTransit(fall1_, run_, &doRun_);

	//fall2~
	model->MakeAnimationTransit(fall2_, breath_, &doFall_, false);
	model->MakeAnimationTransit(fall2_, gun_, &doGunReady_);
	model->MakeAnimationTransit(fall2_, run_, &doRun_);

	//gun~
	model->MakeAnimationTransit(gun_, breath_, &doGunReady_, false);
	model->MakeAnimationTransit(gun_, gunkeep_, nullptr, true, false);

	//gunkeep_
	model->MakeAnimationTransit(gunkeep_, gunshoot_, &doGunShoot_);
	model->MakeAnimationTransit(gunkeep_, breath_, &doGunReady_, false);

	//gunshoot~
	model->MakeAnimationTransit(gunshoot_, gunkeep_, &doGunShoot_, false, false);

	//リスナーに登録
	AudioSystem::Instance()->SetListenerObject(this,
		Vector3(0, 17, 0));

	//プレイヤー用の動きをさせる
	playComp_ = new PlayerMoveComponent(this);
	
		Capsule cap;
	cap.radius_ = 3;
	cap.line_.start_ = Vector3(0, 3, 0);
	cap.line_.end_ = Vector3(0, 17, 0);
	//auto ccol = new CapsuleColliderComponent(this, cap);

	OBB obb;
	obb.center_ = Vector3(0, 10, 0);
	obb.sizedAxis_[0] = Vector3(4, 0, 0);
	obb.sizedAxis_[1] = Vector3(0, 20, 0);
	obb.sizedAxis_[2] = Vector3(0, 0, 4);
	auto ccol = new OBBColliderComponent(this, obb);
	
	ccol->SetType(
		TYPE_PLAYER | TYPE_TARGET);
	//モデルをもらってくる
	std::vector<VertexData> vd;
	std::vector<Index> id;
	std::vector<Material> mats;
	ccol->GetLocalAABB().GetModel(vd, id);
	mats.emplace_back(
		Material(id.size() * 3, Vector3(0.5, 1.0, 1.0), 1.0)
	);
	
	// auto aabbmodel = new MeshComponent(this, vd, id, mats);
	//着地判定用
	AABB ground;
	ground.minP_ = Vector3(-2, -0.5f, -2);
	ground.maxP_ = Vector3(2, 0.5f, 2);
	auto grcol = new AABBColliderComponent(this, ground, true);
	grcol->SetType(TYPE_GROUNDTRRIGER);
	grcol->GetLocalAABB().GetModel(vd, id);
	//auto grcolmodel = new MeshComponent(this, vd, id);


	//子オブジェクトを作る
	camera_ = new CameraObject(this);
	camera_->SetLocalPos(5, 25, -30);
	camera_->SetLocalRot(XM_PI / 6, 0, 0);

	//レティクルの表示
	reticle_ = new ReticleObject(this);

	//オーディオの作成
	unsigned int flags = 0;
	flags |= AUDIO_FLAG_SE | AUDIO_FLAG_USEREVERB;
	stepSound_ = new AudioComponent(this, "sound/砂利の上を走る.wav", flags);
	stepSound_->SetLoop(true);
	walkSound_ = new AudioComponent(this, "sound/砂利の上を歩く.wav", flags);
	walkSound_->SetLoop(true);
	landSound_ = new AudioComponent(this, "sound/ジャンプの着地.wav", flags);
	readyShootSound_ = new AudioComponent(this, "sound/拳銃をチャッと構える.wav", flags);
	for (auto& ss : shootSound_) {
		ss = new AudioComponent(this, "sound/拳銃1.wav", flags);
	}
	
}

PlayerObject::~PlayerObject() {
	//リスナーをリセット
	AudioSystem::Instance()->ResetListenerObject(this);

	//変えた設定は戻しておく
	auto dx12 = Dx12Wrapper::Instance();
	auto audio = AudioSystem::Instance();
	dx12->SetVignette(false);
	audio->ResetFilterParametersToFinalRender();
}

void
PlayerObject::UpdateObject(float deltaTime) {
	//     フラグ管理
	if (!forOnGround_ && onGround_) {
		//地面についた一回目だけonGroundを変える
		playComp_->SetIsOnGround(true);
		landSound_->Play();
	}
	if (forOnGround_ && !onGround_) {
		//地面から離れた
		playComp_->SetIsOnGround(false);
	}
	//前の状態を記録
	forOnGround_ = onGround_;
	onGround_ = false;
	doRun_ = playComp_->DoRun();
	doJump_ = (playComp_->GetVelocity().y > 0);
	doFall_ = !playComp_->IsOnGround();
	doGunReady_ = camera_->CanShoot();
	doNeutral_ = !(doRun_ || doJump_ || doFall_ || 
		doGunReady_ || doGunShoot_);

	//　　　　オーディオ管理
	auto dx12 = Dx12Wrapper::Instance();
	auto audio = AudioSystem::Instance();
	AudioSystem::Filter_Parameters fp;//fps視点の時はろーパスをかける
	fp.type = AudioSystem::AS_LowPassFilter;
	if (doGunReady_) {
		readyShootSound_->Play();
		readyShootSound_->SetCanPlay(false);
		reticle_->SetState(Active);
		if (camera_->CanShoot() == 2) {
			dx12->SetVignette(true);
			fp.frequency = audio->CalcFrequency(400);
			fp.oneOverQ = 1.0f;
			audio->SetFilterParametersToFinalRender(&fp, deltaTime);
		}
	}
	else {
		readyShootSound_->Stop();
		readyShootSound_->SetCanPlay(true);
		reticle_->SetState(InActive);
		dx12->SetVignette(false);
		fp.frequency = 1.0f;
		fp.oneOverQ = 1.0f;
		audio->SetFilterParametersToFinalRender(&fp, deltaTime);
	}
	if (doRun_) {
		if (doGunReady_) {
			stepSound_->Pause();
			walkSound_->Play();
		}
		else {
			walkSound_->Pause();
			stepSound_->Play();
		}
	}
	else {
		stepSound_->Pause();
		walkSound_->Pause();
	}

}

void 
PlayerObject::OnCollision(const struct CollisionInfo& ci) {
	const auto self = ci.collider[0];
	const auto other = ci.collider[1];
	auto stype = self->GetType();
	auto otype = other->GetType();

	if (stype & TYPE_GROUNDTRRIGER &&
		otype & TYPE_GROUND) {
		onGround_ = true;
	}
	if (stype & TYPE_PLAYER &&
		otype & TYPE_GROUND) {
		auto vel = playComp_->GetVelocity();
		auto acc = playComp_->GetAccel();
		playComp_->SetVelocity(vel.x, 0, vel.z);
		playComp_->SetAccel(acc.x, 0, acc.z);
	}
}

void 
PlayerObject::ActorInput(const InputState& inputState) {
	doGunShoot_ = false;
	ButtonState shootButton =
		inputState.kb.GetMappedButtonState(SHOOT_BULLET);
	if (doGunReady_ &&
		shootButton == ButtonState::PRESSED) {
		if (bNum_ > 0) {
			//弾を前にとばす
			auto pos = camera_->GetWorldPos();
			auto dir = camera_->GetForward();
			pos += dir * 10;
			auto rot = camera_->GetWorldRot();
			auto b = new BulletObject(GetParent(), this);
			b->SetWorldPos(pos.x, pos.y, pos.z);
			b->SetWorldRot(rot.x, rot.y, rot.z);

			bNum_--;
			doGunShoot_ = true;
			for (auto ss : shootSound_) {//再生してないコンポーネントがあればそれを使う
				if (!ss->IsPlaying()) {
					ss->Play();
					break;
				}
			}
		}
	}
}

float
AudioSystem::CalcFrequency(float hz) {
	XAUDIO2_VOICE_DETAILS details;
	masterVoice_->GetVoiceDetails(&details);
	return hz / details.InputSampleRate * 6.0f;
}