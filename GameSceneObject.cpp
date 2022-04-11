#include "GameSceneObject.h"
#include "PlayerObject.h"
#include "CameraObject.h"
#include "Game.h"
#include "Ruka.h"
#include "PlaneObject.h"
#include "TimeObject.h"
#include "PointLightObject.h"
#include "RemainNumObject.h"
#include "ScoreTextObject.h"
#include "TargetFactoryObject.h"
#include "TargetObject.h"
#include "PauseMenuObject.h"
#include "ResultSceneObject.h"
#include "TitleSceneObject.h"
#include "ControlCaptionObject.h"
#include "AudioComponent.h"
#include "AudioSystem.h"
#include "InputSystem.h"
#include "FreeCameraObject.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#include <fstream>
#include <memory>
#include "RadioObject.h"
#include "ModelObject.h"

using namespace DirectX;

GameSceneObject::GameSceneObject(Game* game, const char* stageFile):
SceneObject(game),
editMode_(false)
{
	//基準の座標の設定
	SetWorldPos(0, 0, 0);
	SetWorldRot(0, 0, 0);

	stageFile_ = stageFile;

	//ゲームオブジェクトの配置
	if (!Init(stageFile_.c_str())) {
		assert(0 &&"初期化に失敗しました");
	}
	objId_.resize(256);
	objName_.resize(256);
}

GameSceneObject::~GameSceneObject() {
	game_->SetStopUpdateGameObjects(false);
}

SceneObject*
GameSceneObject::UpdateScene(float deltaTime) {
	//デフォルトはこのシーン
	SceneObject* nextScene = this;
#ifdef _DEBUG
	MakeImguiWindow();
	if (editMode_) {
		player_->SetState(InActive);
		time_->SetState(InActive);
		freeCamera_->SetState(Active);
	}
	else {
		player_->SetState(Active);
		time_->SetState(Active);
		freeCamera_->SetState(InActive);
	}
#endif // _DEBUG
	//もうすぐ死ぬオブジェクトは参照しないようにする
	RemoveDeadObjectFromMap();
	
	//終了条件を確認
	if (time_->GetTime() < 0 
		&& stageMode_ == StageMode::SCOREATTACK) {
		AudioSystem::Instance()->StopAll(true);
		auto result = static_cast<float>(score_->GetScore());
		nextScene = new ResultSceneObject(game_, result, stageMode_,
			rankFile_.c_str());
	}
	else if (targets_.size() == 0 && stageMode_ == StageMode::TIMEATTACK) {
		AudioSystem::Instance()->StopAll(true);
		nextScene = new ResultSceneObject(game_, time_->GetTime(), stageMode_,
			rankFile_.c_str());
	}

	if (game_->GetStopUpdateGameObjects()) {//ポーズ
		pause_->SetState(Active);
		pause_->Update(deltaTime);
	}
	else {
		pause_->SetState(InActive);
	}

	//数字表示テキストの更新
	if (stageMode_ == TIMEATTACK) {//表示する数の種類をステージのモードに合わせる
		rNumText_->SetNum(targets_.size());
		//弾数は0にしない
		player_->SetBulletNum(10);
	}
	else if (stageMode_ == SCOREATTACK) {
		rNumText_->SetNum(player_->GetBulletNum());
	}

	return nextScene;
}

SceneObject*
GameSceneObject::SceneInput(const InputState& inputState) {
	//デフォルトはこのシーン
	SceneObject* nextScene = this;
	
	auto shift = inputState.kb.GetButtonState(VK_SHIFT);
	if (shift == PRESSED) {//操作説明の表示
		if (cco_->GetState() == Active) {
			cco_->SetState(InActive);
		}
		else if (cco_->GetState() == InActive) {
			cco_->SetState(Active);
		}
	}

	//ポーズするかどうか
	auto esc = inputState.kb.GetButtonState(VK_ESCAPE);
	bool doStop = game_->GetStopUpdateGameObjects();
	if (esc == PRESSED) {//ポーズの切り替え
		game_->SetStopUpdateGameObjects(!doStop);
	}

	if (doStop) {//ポーズメニュー
		pause_->ProcessInput(inputState);
		if (inputState.kb.IsOn(VK_RETURN)) {
			auto selected = pause_->GetSelectedScene();
			float result = 0;
			//遷移先を選択
			switch (selected)
			{
			case PauseMenuObject::THISSCENE:
				nextScene = this;
				game_->SetStopUpdateGameObjects(false);
				break;
			case PauseMenuObject::TITLESCENE:
				nextScene = new TitleSceneObject(game_);
				break;
			case PauseMenuObject::RESULTSCENE:
				if (stageMode_ == StageMode::SCOREATTACK) {
					result = static_cast<float>(score_->GetScore());
				}
				else if (stageMode_ == StageMode::TIMEATTACK) {
					result = -1;
				}
				nextScene = new ResultSceneObject(game_, result, stageMode_,
					rankFile_.c_str());
				break;
			case PauseMenuObject::RESTARTSCENE:
				nextScene = new GameSceneObject(game_, stageFile_.c_str());
				break;
			default:
				assert(0);
				break;
			}
		}
	}

	return nextScene;
}

bool
GameSceneObject::Init(const char* fileName) {

#ifdef _DEBUG
	auto ruka = new Ruka(this);
	ruka->SetWorldPos(50, 0, 50);
	ruka->SetWorldRot(0, 0, 0);
#endif // _DEBUG

	//プレイヤー
	player_ = new PlayerObject(this);
	player_->SetWorldPos(0, 0, 0);
	player_->SetWorldRot(0, 0, 0);
	//デバッグ用カメラ
	freeCamera_ = new FreeCameraObject(this);
	freeCamera_->SetState(InActive);
	freeCamera_->SetLocalPos(0, 30, 0);

	//UI
	score_ = new ScoreTextObject(this);
	cco_ = new ControlCaptionObject(this);
	pause_ = new PauseMenuObject(this);

	//Stage
		//ファイルから読み込んでステージを構築する
	//jsonデータの読み込み
	FILE* jsonF = nullptr;
	fopen_s(&jsonF, fileName, "rb");
	if (jsonF == nullptr) {
		fclose(jsonF);
		assert(0);
		return false;
	}
	size_t size;
	if (fseek(jsonF, 0, SEEK_END) == 0) {
		size = ftell(jsonF);
		fseek(jsonF, 0, SEEK_SET);
	}
	std::string content;
	content.resize(size);
	fread(content.data(), size, 1, jsonF);
	fclose(jsonF);

	auto jsonInfo =json::parse(content);

	//ステージデータの設定
	auto& sinfo = jsonInfo["Info"];
	stageMode_ = sinfo["StageMode"].get<int>();
	bulletNum_ = sinfo["BulletNum"].get<int>();
	player_->SetBulletNum(bulletNum_);
	rankFile_ = sinfo["RankData"].get<std::string>();
	//時間表示の設定
	time_ = new TimeObject(this, stageMode_);
	timeLimit_ = sinfo["Time"].get<float>();
	time_->SetTime(timeLimit_);
	int nummode = 0;
	if (stageMode_ == TIMEATTACK) {//表示する数の種類をステージのモードに合わせる
		nummode = RemainNumObject::MODE_TARGETNUM;
	}
	else if (stageMode_ == SCOREATTACK) {
		nummode = RemainNumObject::MODE_BULLETNUM;
	}
	rNumText_ = new RemainNumObject(this, nummode);
	
	for (auto& item : jsonInfo["Objects"]) {//情報をもとに作製
		Object* obj = AddObject(item["ObjID"].get<std::string>(),
			item["Name"].get<std::string>());
		if (obj!=nullptr) {//情報を反映
			obj->SetObjDataFromJson(item);
		}
	}
	

	return true;
}

void//オブジェクトに共通の設定をimguiに表示する
MakeObjectDefaultSettingsWindow(Object* obj) {
	auto pos = obj->GetLocalPos();
	auto rot = obj->GetLocalRot() * (180.0f / XM_PI);
	auto sca = obj->GetLocalScale();
	bool isDraw = obj->IsDraw();
	bool isK = obj->IsKinematic();
	ImGui::InputFloat3("Position", (float*)&pos);
	ImGui::InputFloat3("Rotation", (float*)&rot);
	ImGui::InputFloat3("Scale", (float*)&sca);
	ImGui::Checkbox("IsDraw", &isDraw);
	ImGui::Checkbox("IsKinematic", &isK);
	rot *= (XM_PI / 180.0f);
	obj->SetLocalPos(pos.x, pos.y, pos.z);
	obj->SetLocalRot(rot.x, rot.y, rot.z);
	obj->SetLocalScale(sca);
	obj->SetDraw(isDraw);
	obj->SetIsKinematic(isK);
}


void 
GameSceneObject::MakeImguiWindow() {
	//ウィンドウ定義
	if (!ImGui::Begin("GameScene", nullptr,
		ImGuiWindowFlags_MenuBar)) {
		ImGui::End();
		return;
	}
	ImGui::SetWindowSize(
		ImVec2(400, 500),//ウィンドウサイズ
		ImGuiCond_::ImGuiCond_FirstUseEver
	);
	//ステージデータセーブ
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Save")) {
				SaveStageData(stageFile_.c_str());
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	//エディットモード
	ImGui::Checkbox("EditMode", &editMode_);
	 
	//オブジェクト追加
	ImGui::InputText("ObjID", objId_.data(), 256);
	ImGui::InputText("ObjName", objName_.data(), 256);
	if (ImGui::Button("AddObject")) {//オブジェクトの追加

		AddObject(objId_.c_str(), objName_.c_str());
	}

	//オブジェクト一覧
	//ラジオ
	ImGui::BeginChild("Objects", ImVec2(250, 100), true,
		ImGuiWindowFlags_NoTitleBar);
	for (auto& obj : radio_) {
		if (ImGui::TreeNode(obj.first.c_str())) {
			MakeObjectDefaultSettingsWindow(obj.second);
			if (ImGui::Button("DeleteObject")) {//オブジェクトの追加
				DeleteObject("Radio", obj.first);
				ImGui::TreePop();
				break;
			}
			ImGui::TreePop();
		}
	}
	//モデル表示するだけのモノ
	for (auto& obj : models_) {
		if (ImGui::TreeNode(obj.first.c_str())) {
			MakeObjectDefaultSettingsWindow(obj.second);
			ImGui::InputText("ModelFileName", modelName_.data(), 256);
			if (ImGui::Button("CreateModel")) {//モデルの作製
				obj.second->CreateModel(modelName_.c_str());
			}
			if (ImGui::Button("DeleteObject")) {//オブジェクトの追加
				DeleteObject("Model", obj.first);
				ImGui::TreePop();
				break;
			}
			ImGui::TreePop();
		}
	}
	//地面
	for (auto& obj : ground_) {
		if (ImGui::TreeNode(obj.first.c_str())) {
			MakeObjectDefaultSettingsWindow(obj.second);
			if (ImGui::Button("DeleteObject")) {//オブジェクトの追加
				DeleteObject("Ground", obj.first);
				ImGui::TreePop();
				break;
			}
			ImGui::TreePop();
		}
	}
	//壁
	for (auto& obj : wall_) {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode(obj.first.c_str())) {
			MakeObjectDefaultSettingsWindow(obj.second);
			if (ImGui::Button("DeleteObject")) {//オブジェクトの追加
				DeleteObject("Wall", obj.first);
				ImGui::TreePop();
				break;
			}
			ImGui::TreePop();
		}
	}
	//ターゲットfac
	for (auto& obj : targetFac_) {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode(obj.first.c_str())) {
			MakeObjectDefaultSettingsWindow(obj.second);
			if (ImGui::Button("DeleteObject")) {//オブジェクトの追加
				DeleteObject("TargetFac", obj.first);
				ImGui::TreePop();
				break;
			}
			ImGui::TreePop();
		}
	}
	//ターゲット
	for (auto& obj : targets_) {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode(obj.first.c_str())) {
			MakeObjectDefaultSettingsWindow(obj.second);
			//ターゲットの動きの設定
			int moveMode = obj.second->GetMoveType();
			float span = obj.second->GetSpan();
			ImGui::InputInt("MoveMode", &moveMode);
			ImGui::InputFloat("Span", &span);
			if (moveMode != obj.second->GetMoveType()) {
				moveMode = (moveMode > 3) ? 3 : moveMode;
				moveMode = (moveMode < 0) ? 3 : moveMode;
				obj.second->SetMoveType(moveMode);
			}
			obj.second->SetSpan(span);
			auto bi = obj.second->GetBezierInfo();
			auto ci = obj.second->GetCircleInfo();
			float p = ci.pitch * 180.0f / XM_PI;
			switch (moveMode)
			{
			case TargetObject::MOVE_NONE:
			case TargetObject::MOVE_RANDOM:
				break;
			case TargetObject::MOVE_BEZIERLINE:
				ImGui::InputFloat3("Start", (float*)&bi.start);
				ImGui::InputFloat3("Control", (float*)&bi.control);
				ImGui::InputFloat3("End", (float*)&bi.end);
				obj.second->SetBezierInfo(bi);
				break;
			case TargetObject::MOVE_CIRCLE:
				ImGui::InputFloat3("Center", (float*)&ci.center);
				ImGui::InputFloat2("Rad", (float*)&ci.Rad());
				ImGui::InputFloat("Pitch", &p);
				ci.pitch = p * XM_PI / 180.0f;
				obj.second->SetCircleInfo(ci);
				break;
			default:
				break;
			}
			if (ImGui::Button("DeleteObject")) {//オブジェクトの追加
				DeleteObject("Target", obj.first);
				ImGui::TreePop();
				break;
			}
			ImGui::TreePop();
		}
	}
	//光
	for (auto& obj : lights_) {
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNode(obj.first.c_str())) {
			MakeObjectDefaultSettingsWindow(obj.second);
			//光の設定
			auto col = obj.second->GetColor();
			float range = obj.second->GetRange();
			float rad = obj.second->GetRadius();
			ImGui::ColorPicker3("Color", (float*)&col);
			ImGui::InputFloat("Range", &range);
			ImGui::InputFloat("Radius", &rad);

			obj.second->SetLightParam(col, range, rad);
			if (ImGui::Button("DeleteObject")) {//オブジェクトの追加
				DeleteObject("Light", obj.first);
				ImGui::TreePop();
				break;
			}
			ImGui::TreePop();
		}
	}
	ImGui::EndChild();

	//ステージの変数設定
	ImGui::InputInt("BulletNum", &bulletNum_);
	ImGui::InputFloat("TimeLimit", &timeLimit_);
	ImGui::InputText("RankData", rankFile_.data(), 256);
	ImGui::InputInt("StageMode", &stageMode_);

	ImGui::End();
}

void 
GameSceneObject::SaveStageData(const char* fileName) {
	//ステージ周りの変数の取得
	json sData;
	sData["Info"] = json({
		{"BulletNum", bulletNum_},
		{"Time", timeLimit_},
		{"RankData", rankFile_},
		{"StageMode", stageMode_}
		});

	//オブジェクトのデータの保存
	std::vector<json> objDatas;
	//radio
	for (auto& obj : radio_) {
		objDatas.emplace_back(
			obj.second->GetJsonFromObj("Radio", obj.first)
		);
	}
	//ground
	for (auto& obj : ground_) {
		objDatas.emplace_back(
			obj.second->GetJsonFromObj("Ground", obj.first)
		);
	}
	//wall
	for (auto& obj : wall_) {
		objDatas.emplace_back(
			obj.second->GetJsonFromObj("Wall", obj.first)
		);
	}
	//targetFac
	for (auto& obj : targetFac_) {
		objDatas.emplace_back(
			obj.second->GetJsonFromObj("TargetFac", obj.first)
		);
	}
	//target
	for (auto& obj : targets_) {
		objDatas.emplace_back(
			obj.second->GetJsonFromObj("Target", obj.first)
		);
	}
	//light
	for (auto& obj : lights_) {
		objDatas.emplace_back(
			obj.second->GetJsonFromObj("Light", obj.first)
		);
	}
	//models
	for (auto& obj : models_) {
		objDatas.emplace_back(
			obj.second->GetJsonFromObj("Model", obj.first)
		);
	}
	sData["Objects"] = objDatas;

	FILE* stFile;
	fopen_s(&stFile, fileName, "w");
	if (stFile == nullptr) {
		assert(0);
		return;
	}
	std::string json;
	json = sData.dump();
	//データを書き込み
	fputs(json.c_str(), stFile);

	fclose(stFile);
}


Object*
GameSceneObject::AddObject(const std::string id, const std::string name) {

	if (id == "Radio") {
		if (radio_.count(name) != 0) {//既に同じ名前で存在している
			return nullptr;
		}
		radio_[name] = new RadioObject(this);
		return radio_[name];
	}
	if (id == "Ground") {
		if (ground_.count(name) != 0) {//既に同じ名前で存在している
			return nullptr;
		}
		auto minp = Vector3(-100, -10, -100);
		auto maxp = Vector3(100, 0, 100);
		ground_[name] = new PlaneObject(this,
			minp, maxp);
		ground_[name]->SetRepeatNum(10);
		ground_[name]->AddMaterialInfo(
			"Ground022_256_Color.png",
			"Ground022_256_NormalDX.png",
			"Ground022_256_AmbientOcclusion.png"
		);
		ground_[name]->SetType(TYPE_GROUND);
		ground_[name]->CreateModel();
		return ground_[name];
	}
	else if (id == "Wall") {
		if (wall_.count(name) != 0) {//既に同じ名前で存在している
			return nullptr;
		}
		auto minp = Vector3(-100, -10, -100);
		auto maxp = Vector3(100, 0, 100);
		wall_[name] = new PlaneObject(this, minp, maxp);
		wall_[name]->SetRepeatNum(10);
		wall_[name]->AddMaterialInfo(
			"concrete_floor_worn_001_diff_256.png",
			"concrete_floor_worn_001_nor_dx_256.png",
			"concrete_floor_worn_001_ao_256.png"
		);
		wall_[name]->SetType(TYPE_WALL);
		wall_[name]->CreateModel();
		return wall_[name];
	}
	else if (id == "TargetFac") {
		if (targetFac_.count(name) != 0) {//既に同じ名前で存在している
			return nullptr;
		}
		targetFac_[name] = new TargetFactoryObject(this, score_);
		return targetFac_[name];
	}
	else if (id == "Target") {
		if (targets_.count(name) != 0) {//既に同じ名前で存在している
			return nullptr;
		}
		targets_[name] = new TargetObject(this, score_);
		return targets_[name];
	}
	else if (id == "Light") {
		if (lights_.count(name) != 0) {//既に同じ名前で存在している
			return nullptr;
		}
		lights_[name] = new PointLightObject(this, Vector3(1, 1, 1),
			50, 20);
		return lights_[name];
	}
	else if (id == "Model") {
		if (models_.count(name) != 0) {//既に同じ名前で存在している
			return nullptr;
		}
		models_[name] = new ModelObject(this);
		return models_[name];
	}
	else {
		return nullptr;
	}
}


void
GameSceneObject::DeleteObject(const std::string id, const std::string name) {
	if (id == "Radio") {
		if (radio_.count(name) != 0) {//既に同じ名前で存在している
			delete radio_[name];
			radio_.erase(name);
		}
	}
	else if (id == "Ground") {
		if (ground_.count(name) != 0) {//既に同じ名前で存在している
			delete ground_[name];
			ground_.erase(name);
		}
	}
	else if (id == "Wall") {
		if (wall_.count(name) != 0) {//既に同じ名前で存在している
			delete wall_[name];
			wall_.erase(name);
		}
	}
	else if (id == "TargetFac") {
		if (targetFac_.count(name) != 0) {//既に同じ名前で存在している
			delete targetFac_[name];
			targetFac_.erase(name);
		}
	}
	else if (id == "Target") {
		if (targets_.count(name) != 0) {//既に同じ名前で存在している
			delete targets_[name];
			targets_.erase(name);
		}
	}
	else if (id == "Light") {
		if (lights_.count(name) != 0) {//既に同じ名前で存在している
			delete lights_[name];
			lights_.erase(name);
		}
	}
	else if (id == "Model") {
		if (models_.count(name) != 0) {//既に同じ名前で存在している
			delete models_[name];
			models_.erase(name);
		}
	}
}

void
GameSceneObject::RemoveDeadObjectFromMap() {
	//ラジオ
	for (auto it = radio_.begin(); it != radio_.end();) {
		if (it->second->GetState() == Dead) {
			it = radio_.erase(it);
		}
		else {
			it++;
		}
	}
	//ライト
	for (auto it = lights_.begin(); it != lights_.end();) {
		if (it->second->GetState() == Dead) {
			it = lights_.erase(it);
		}
		else {
			it++;
		}
	}
	//地面
	for (auto it = ground_.begin(); it != ground_.end();) {
		if (it->second->GetState() == Dead) {
			it = ground_.erase(it);
		}
		else {
			it++;
		}
	}
	//wall
	for (auto it = wall_.begin(); it != wall_.end();) {
		if (it->second->GetState() == Dead) {
			it = wall_.erase(it);
		}
		else {
			it++;
		}
	}
	//targetFactory
	for (auto it = targetFac_.begin(); it != targetFac_.end();) {
		if (it->second->GetState() == Dead) {
			it = targetFac_.erase(it);
		}
		else {
			it++;
		}
	}
	//targets
	for (auto it = targets_.begin(); it != targets_.end();) {
		if (it->second->GetState() == Dead) {
			it = targets_.erase(it);
		}
		else {
			it++;
		}
	}
}