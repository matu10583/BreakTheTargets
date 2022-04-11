#include "MeshComponent.h"
#include "Renderer.h"
#include "Object.h"
#include "MathFunc.h"
#include <d3dx12.h>
#include <d3d12.h>
#include <math.h>

MeshComponent::MeshComponent(Object* owner, const char* fileName, int updateOrder)
	:Component(owner, updateOrder),
	isLeftHanded(true),
	elapsedTime_(0),
	nowAnimeNodeID_(-1),
	isDraw(true)
{
	
	Renderer* renderer = Renderer::Instance();
	renderer->AddPMDMeshComponent(this);

	//modelの情報を得る
	dx12_ = Dx12Wrapper::Instance();
	model_ = dx12_->GetModelByPath(fileName);
	//ボーン用行列ベクタのサイズを決める
	boneMatrices_.resize(model_->boneNameArray_.size());
	std::fill(
		boneMatrices_.begin(), boneMatrices_.end(),
		XMMatrixIdentity()
	);

	for (auto& v : model_->vertices_) {
		localAABB_.UpdateBox(v);
	}

	//位置情報用のリソースを作る
	auto result = CreateTransformResourceAndView();
	if (FAILED(result)) {
		assert(0);
	}

}

//頂点配列からモデルを生成
MeshComponent::MeshComponent(Object* owner, const std::vector<VertexData>& vertices,
	const std::vector<Index>& indecies, const std::vector<Material>& materials,
	const char* name,
	int updateOrder)
	:Component(owner, updateOrder),
	isLeftHanded(true),
	elapsedTime_(0),
	nowAnimeNodeID_(-1)
{
	Renderer* renderer = Renderer::Instance();
	renderer->AddVertMeshComponent(this);
	//modelの情報を得る
	dx12_ = Dx12Wrapper::Instance();
	model_ = dx12_->GetModelByVertices(vertices, indecies, materials, name);

	for (auto& v : model_->vertices_) {
		localAABB_.UpdateBox(v);
	}

	//位置情報用のリソースを作る
	auto result = CreateTransformResourceAndView();
	if (FAILED(result)) {
		assert(0);
	}
}

MeshComponent::~MeshComponent() {
	Renderer* renderer = Renderer::Instance();
	renderer->RemoveMeshComponent(this);
}

unsigned int
MeshComponent::AddAnimationNode(const char* fileName, 
	bool isLoop, unsigned int transitFrame) {
	//アニメーションのノードを追加してそのノードのIDを返す
	const unsigned int id = aniID_;
	aniID_++;//進める
	AnimationNode animNode;
	animNode.anime = dx12_->GetAnimeByPath(fileName);
	animNode.isLoop = isLoop;
	animNode.transitFrameNum = (int)transitFrame;
	animeNodes_[id] = animNode;
	return id;
}

void
MeshComponent::DeleteAnimationNode(const unsigned int id) {

	//ノードから生えてる遷移線の削除
	transitMap_.erase(id);
	//ノードを差す遷移線を削除
	std::vector<unsigned int> eraseList;
	for (auto& tlvec : transitMap_) {
		auto vec = tlvec.second;
		for (auto tl : vec) {
			auto p = tl.end;
			if (p == id) {
				eraseList.emplace_back(tlvec.first);
			}
		}
	}
	for (auto e : eraseList) {
		transitMap_.erase(e);
	}
	//ノードの削除
	animeNodes_.erase(id);
}

void
MeshComponent::MakeAnimationTransit(
	const unsigned int start, const unsigned int end, 
	const bool* condition, bool value, bool isinter) {
	//遷移線を作製
	transitMap_[start].emplace_back(end, condition, value, isinter);
}

void
MeshComponent::SetEntryAnimationNode(const unsigned int entryID) {
	nowAnimeNodeID_ = entryID;
}


void
MeshComponent::SetIsLeftHanded(bool isLH) {
	isLeftHanded = isLH;
}

void
MeshComponent::Update(float deltaTime) {
	//描画するかしないかをownerから取得
	isDraw = owner_->IsDraw();

	//AABBの更新
	UpdateAABB();

	//ワールド行列の更新
	auto rot = owner_->GetWorldRot();
	auto pos = owner_->GetWorldPos();
	auto sca = owner_->GetWorldScale();

	XMMATRIX k = XMMatrixIdentity();
	if (!isLeftHanded) {//右手系のモデルならz方向を反転させる
		k = XMMatrixRotationRollPitchYaw(0, XM_PI, 0);
	}
	mappedPosMatrix_[0] = XMMatrixTranslation(pos.x, pos.y, pos.z);

	mappedPosMatrix_[1] = k *
		XMMatrixScaling(sca.x, sca.y, sca.z) *
		XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);


	std::fill(boneMatrices_.begin(), boneMatrices_.end(),
		XMMatrixIdentity());
	//モーションの更新
	if (nowAnimeNodeID_ != -1) {
		MotionUpdate(deltaTime);
	}
	//行列のコピー
	std::copy(boneMatrices_.begin(), boneMatrices_.end(), mappedPosMatrix_ + WORLD_MATRIX_COUNT);
}

void
MeshComponent::DrawPMD(bool isShadow) {
	if (owner_->GetState() != Object::Active ||
		GetState() != Component::CActive ||
		!isDraw) {
		return;
	}
	auto cmdList = dx12_->CommandList();
	//vb,ibの設定
	cmdList->IASetVertexBuffers(0, 1, &(model_->vbView_));
	cmdList->IASetIndexBuffer(&(model_->ibView_));

	//位置情報transbufferのdescHeapの設定
	ID3D12DescriptorHeap* transHeaps[] = {
		transformHeap_.Get()
	};
	cmdList->SetDescriptorHeaps(1, transHeaps);
	//1番rootparameterと紐づけ
	cmdList->SetGraphicsRootDescriptorTable(
		1, transformHeap_->GetGPUDescriptorHandleForHeapStart());

	//materialのdescheapの設定
	ID3D12DescriptorHeap* matheap = model_->materialHeap_.Get();
	ID3D12DescriptorHeap* materialheaps[] = {
		matheap
	};
	cmdList->SetDescriptorHeaps(1, materialheaps);

	//マテリアルを適用してそれぞれの部位ごとに描画する
	auto& matInfos = model_->materialInfos_;
	unsigned int idxOffset = 0;//どのIndexまで描画したか
	auto matH = matheap->GetGPUDescriptorHandleForHeapStart();
	auto incSize = 7 * dx12_->Dev()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);//マテリアルヒープは7こ飛ばしにあるので

	if (isShadow) {
		cmdList->DrawIndexedInstanced(
			model_->indicesNum_, 1,
			idxOffset, 0, 0
		);
	}
	else {
		//車道マップ用じゃなきゃ部位ごとに描画する
		for (auto& mInfo : matInfos) {
			cmdList->SetGraphicsRootDescriptorTable(2, matH);
				cmdList->DrawIndexedInstanced(
					mInfo.indicesNum, 1,
					idxOffset, 0, 0
				);

			//次のヒープに進める
			matH.ptr += incSize;
			idxOffset += mInfo.indicesNum;
		}
	}
	
}


void
MeshComponent::DrawVert(bool isShadow) {
	if (owner_->GetState() != Object::Active ||
		GetState() != Component::CActive ||
		!isDraw) {
		return;
	}
	auto cmdList = dx12_->CommandList();
	//vb,ibの設定
	cmdList->IASetVertexBuffers(0, 1, &(model_->vbView_));
	cmdList->IASetIndexBuffer(&(model_->ibView_));

	//位置情報transbufferのdescHeapの設定
	ID3D12DescriptorHeap* transHeaps[] = {
		transformHeap_.Get()
	};
	cmdList->SetDescriptorHeaps(1, transHeaps);
	//1番rootparameterと紐づけ
	cmdList->SetGraphicsRootDescriptorTable(
		1, transformHeap_->GetGPUDescriptorHandleForHeapStart());


	//materialのdescheapの設定
	ID3D12DescriptorHeap* matheap = model_->materialHeap_.Get();
	ID3D12DescriptorHeap* materialheaps[] = {
		matheap
	};
	cmdList->SetDescriptorHeaps(1, materialheaps);

	//マテリアルを適用してそれぞれの部位ごとに描画する
	auto& matInfos = model_->materialInfos_;
	unsigned int idxOffset = 0;//どのIndexまで描画したか
	auto matH = matheap->GetGPUDescriptorHandleForHeapStart();
	auto incSize = 7 * dx12_->Dev()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);//マテリアルヒープは7こ飛ばしにあるので

	if (isShadow) {
		cmdList->DrawIndexedInstanced(
			model_->indicesNum_, 1,
			idxOffset, 0, 0
		);
	}
	else {
		//車道マップ用じゃなきゃ部位ごとに描画する
		for (auto& mInfo : matInfos) {
			cmdList->SetGraphicsRootDescriptorTable(2, matH);
			cmdList->DrawIndexedInstanced(
				mInfo.indicesNum, 1,
				idxOffset, 0, 0
			);

			//次のヒープに進める
			matH.ptr += incSize;
			idxOffset += mInfo.indicesNum;
		}
	}


}

HRESULT
MeshComponent::CreateTransformResourceAndView() {
	//Create buffer
	//256の倍数に合わせる
	//サイズはボーンの行列と位置と回転の行列分
	auto buffSize = (
		(boneMatrices_.size() +WORLD_MATRIX_COUNT )* sizeof(XMMATRIX) + 0xff) & ~0xff;
	auto heapprop =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resdesc = CD3DX12_RESOURCE_DESC::Buffer(buffSize);

	auto result = dx12_->Dev()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(transformBuff_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	//マップ
	result = transformBuff_->Map(
		0, nullptr,
		(void**)&mappedPosMatrix_
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	//初期値
	for (int i = 0; i < WORLD_MATRIX_COUNT; i++) {
		mappedPosMatrix_[i] = XMMatrixIdentity();
	}



	//create heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags =
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NodeMask = 0;
	descHeapDesc.NumDescriptors = 1;
	descHeapDesc.Type =
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//cbv

	result = dx12_->Dev()->CreateDescriptorHeap(
		&descHeapDesc,
		IID_PPV_ARGS(transformHeap_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = transformBuff_->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = buffSize;
	dx12_->Dev()->CreateConstantBufferView(
		&cbvDesc,
		transformHeap_->GetCPUDescriptorHandleForHeapStart()
	);
	return S_OK;
}

void
MeshComponent::MotionUpdate(float deltaTime) {
	elapsedTime_ += deltaTime;
	//今のフレーム数
	int frameNo = 30 * elapsedTime_;
	auto& nowNode = animeNodes_[nowAnimeNodeID_];

	//遷移先を探す(まだ遷移しない)
	int nextAnimeNodeID = -1;

	if (frameNo >= (int)nowNode.anime->duration &&
		nowNode.isLoop) {
		//遷移先がみつからなくてdurationがつきたら次も同じアニメ
		nextAnimeNodeID = nowAnimeNodeID_;
	}

	for (auto tl : transitMap_[nowAnimeNodeID_]) {
		TransitLine* nextTl = nullptr;
		if (tl.condition == nullptr) {
				//遷移条件がないのでそちらにいく
			nextTl = &tl;
		}
		else if (*tl.condition == tl.value) {
			//遷移可能なのでそちらに行く
			nextTl = &tl;
		}
		if (nextTl == nullptr) {
			continue;
		}
		//遷移先にいけるかチェック
		if (nextTl->isInterrupted ||
			frameNo >= (int)nowNode.anime->duration) {
			//遷移先が見つかったのでそっちに行く
			nextAnimeNodeID = nextTl->end;
			break;
		}
	}

	//遷移しないならnextAnimeNodeID=-1のまま
	auto animData = nowNode.anime;
	//前のアニメーションと何フレームかけてほかんするか
	const int transitFrameNum = nowNode.transitFrameNum;
	if (transitFrameNum == 0) {
		isTransiting = false;
		isFirstFrame = false;
	}

	//前のモーションを覚える用
	std::unordered_map<std::string, KeyFrame> forMot;

	for (auto& bonemot : animData->motiondata) {
		//アニメーションに指定されているボーンがあるか
		auto it = model_->boneNodeTable_.find(bonemot.first);
		if (it == model_->boneNodeTable_.end()) {
			continue;
			//ないなら無視する
		}
		auto& node = model_->boneNodeTable_[bonemot.first];
		auto& mots = bonemot.second;
		//最後に指定されたモーションデータ
		auto rit = std::find_if(
			mots.rbegin(), mots.rend(),
			[=](const KeyFrame& mot) {
				return (int)mot.frameNo <= frameNo;
			}
		);

		//どのキーフレーム間の補間をするか決める
		int rFrameNo;
		XMVECTOR rQuaternion;
		Vector3 rOffset;
		auto motit = rit.base();//デフォルトはritの次のフレーム

		if (isTransiting) {
			//遷移補間中
			auto it = forMotionData_.find(bonemot.first);
			if (it == forMotionData_.end()) {
				continue;
				//ないなら無視する
			}
			if (isFirstFrame) {
				frameNo = -transitFrameNum;
				elapsedTime_ = frameNo / 30.0f;
				isFirstFrame = false;
			}
			rFrameNo = -transitFrameNum;//前のモーション
			rQuaternion = forMotionData_[bonemot.first].quaternion;
			rOffset = forMotionData_[bonemot.first].offset;
			motit = mots.begin();//今のモーションの最初のキーフレーム
		}
		else if (rit == mots.rend()) {
			//最初のキーフレームが設定されていないので前のポーズを想定する
					//アニメーションに指定されているボーンがあるか
			auto it = forMotionData_.find(bonemot.first);
			if (it == forMotionData_.end()) {
				continue;
				//ないなら無視する
			}
			rFrameNo = 0;
			rQuaternion = forMotionData_[bonemot.first].quaternion;
			rOffset = forMotionData_[bonemot.first].offset;
		}
		else {
			//普通に前と、後のキーフレームの補間
			rFrameNo = rit->frameNo;
			rQuaternion = rit->quaternion;
			rOffset = rit->offset;
		}

		XMVECTOR quatern;
		XMMATRIX rotmat;
		XMVECTOR offset;
		if (motit != mots.end()) {
			//ritがrbeginだったらmotitがendになってしまう
			//線形補間の係数を取得
			auto t = static_cast<float>(frameNo - rFrameNo)
				/ static_cast<float>(motit->frameNo - rFrameNo);
			//ベジェ曲線状の係数を求める
			t = GetYFromXOnBezier(t, motit->p1, motit->p2, 15);
			//線形補間を行う
			quatern = XMQuaternionSlerp(rQuaternion, motit->quaternion, t);
			rotmat = XMMatrixRotationQuaternion(quatern);
			offset = XMVectorLerp(
				XMLoadFloat3(&rOffset), XMLoadFloat3(&motit->offset), t
			);
		}
		else {
			//補間の必要なし
			quatern = rQuaternion;
			rotmat =
				XMMatrixRotationQuaternion(rQuaternion);
			offset = XMLoadFloat3(&rOffset);
		}
		auto& pos = node.startPos;
		//回転
		auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* rotmat
			* XMMatrixTranslation(pos.x, pos.y, pos.z);
		//移動
		boneMatrices_[node.boneIdx] = mat *
			XMMatrixTranslationFromVector(offset);

		//遷移するなら前のモーションを覚えておく
		if (nextAnimeNodeID != -1 ||
			(frameNo >= 0 && isTransiting)) {
			auto ofst = Vector3(offset);
			unsigned int fn = static_cast<unsigned int>(frameNo);
			forMot[bonemot.first] = KeyFrame(
				fn,
				quatern,
				ofst,
				Vector2::Zero(), Vector2::Zero()
			);
		}
	}
	//センターボーンから全身のボーンにかけていく
	RecursiveMatrixMultiply(&(model_->boneNodeTable_["センター"]), XMMatrixIdentity());
	IKSolve(frameNo);

	if (frameNo >= 0 && isTransiting) {
		isTransiting = false;//フレーム番号が正になったら遷移終了
		forMotionData_.clear();
		forMotionData_ = forMot;//遷移終了時のモーションに更新

	}
	//遷移の実行
	if (nextAnimeNodeID != -1) {
		elapsedTime_ = 0;
		forMotionData_.clear();
		forMotionData_ = forMot;
		if (nowAnimeNodeID_ != nextAnimeNodeID) {
			//ループじゃなくて遷移なら
			isTransiting = true;
			isFirstFrame = true;
		}
		nowAnimeNodeID_ = nextAnimeNodeID;
	}

}

void
MeshComponent::RecursiveMatrixMultiply(
	BoneNode* node, const DirectX::XMMATRIX& mat
) {
	boneMatrices_[node->boneIdx] *= mat;
	for (auto& cnode : node->children) {
		RecursiveMatrixMultiply(cnode, boneMatrices_[node->boneIdx]);
	}
}



void
MeshComponent::IKSolve(unsigned int frameNo) {
	//IK対称ボーンをさがす
	auto anime = animeNodes_[nowAnimeNodeID_].anime;
	auto rit = std::find_if(anime->ikEnableData.rbegin(),
		anime->ikEnableData.rend(),
		[frameNo](const VMDIKEnable& val) {
			return val.frameNo <= frameNo;
		});
	for (auto& ik : model_->ikData_) {
		if (rit != anime->ikEnableData.rend()) {
			//IKボーンのIKEnableデータを探す
			auto ikEnableIt =
				rit->ikEnableTable.find(model_->boneNameArray_[ik.boneIdx]);
			if (ikEnableIt != rit->ikEnableTable.end()) {
				if (!ikEnableIt->second) {
					//IKは無効
					continue;
				}
			}
		}
		//IKが有効なのでIK処理をする
		auto childNodeNum = ik.nodeIdxes.size();

		switch (childNodeNum)
		{
		case 0:
			//IKがなりたたない
			assert(0);
			continue;
		case 1://二点のIK
			SolveLookAt(ik);
			break;
		case 2://3点のik
			SolveCosine(ik);
			break;
		default://それ以上
			SolveCCDIK(ik);
			break;
		}
	}
}

//LookAtによるIKの解決
void
MeshComponent::SolveLookAt(const PMDIK& ik) {
	//ノードが二つの場合
	auto root = model_->boneNodeAddressArray_[ik.nodeIdxes[0]];
	auto target = model_->boneNodeAddressArray_[ik.targetIdx];
	auto rpos1 = root->startPos;
	auto tpos1 = target->startPos;
	//動かした後の位置
	auto rpos2 = Vector3(
		XMVector3Transform(
			XMLoadFloat3(&rpos1),
			boneMatrices_[ik.nodeIdxes[0]]
		)
	);
	auto tpos2 = Vector3(
		XMVector3Transform(
			XMLoadFloat3(&tpos1),
			boneMatrices_[ik.boneIdx]
		)
	);
	//それぞれのボーンベクトル
	auto before = tpos1 - rpos1;
	auto after = tpos2 - rpos2;
	boneMatrices_[ik.nodeIdxes[0]] *=
		XMMatrixTranslation(-rpos2.x, -rpos2.y, -rpos2.z)
		* LookAtMatrix(before, after,
			Vector3(0, 1, 0),
			Vector3(1, 0, 0)
		)
		* XMMatrixTranslation(rpos2.x, rpos2.y, rpos2.z);
}

//余弦定理によるIKの解決
void
MeshComponent::SolveCosine(const PMDIK& ik) {
	//IKの構成ノード
	std::vector<Vector3> pos;
	//ボーンの長さ
	std::array<float, 2> edgeLens;

	//目標地点のボーン
	auto& target = model_->boneNodeAddressArray_[ik.boneIdx];
	auto tpos1 = target->startPos;
	auto tpos2 = Vector3(
		XMVector3Transform(
			XMLoadFloat3(&tpos1),
			boneMatrices_[ik.boneIdx]
		)
	);

	//構成ノードの場所を覚える
		//末端のノード
	auto endNode =
		model_->boneNodeAddressArray_[ik.targetIdx];
	pos.push_back(endNode->startPos);
	for (auto& idx : ik.nodeIdxes) {
		auto& boneNode =
			model_->boneNodeAddressArray_[idx];
		pos.emplace_back(boneNode->startPos);
	}
	//ルートからの並び順にする
	std::reverse(pos.begin(), pos.end());

	//それぞれのボーンの長さ
	edgeLens[0] = (pos[1] - pos[0]).Magnitude();
	edgeLens[1] = (pos[2] - pos[1]).Magnitude();

	//ボーンの位置に行列を適用する
	pos[0] = Vector3(XMVector3Transform(
		XMLoadFloat3(&pos[0]),
		boneMatrices_[ik.nodeIdxes[1]]
	));
	pos[2] = Vector3(XMVector3Transform(
		XMLoadFloat3(&pos[2]),
		boneMatrices_[ik.boneIdx]
	));

	//余弦定理により中間ノードの位置を計算する
	// 辺ベクトル
	auto Avec = pos[2] - pos[0];
	//3辺の長さ
	float A = Avec.Magnitude();
	float B = edgeLens[0];
	float C = edgeLens[1];

	float ab = (A * A + B * B - C * C) / (2.0f * A * B);
	float bc = (B * B + C * C - A * A) / (2.0f * B * C);
	//誤差のため、clampする
	if (ab > 1.0) {
		ab = 1.0f;
	}
	else if (ab < -1.0) {
		ab = -1.0f;
	}
	if (bc > 1.0) {
		bc = 1.0f;
	}
	else if (bc < -1.0) {
		bc = -1.0f;
	}

	float thetaAB = std::acosf(ab);
	float thetaBC = std::acosf(bc);


	Vector3 axis;//回転の軸を決める
	//膝の回転なら絶対にx軸
	if (std::find(model_->kneeIdxes_.begin(), model_->kneeIdxes_.end(),
		ik.nodeIdxes[0]) == model_->kneeIdxes_.end()) {
		//回転の平面に垂直なじくを決める
		auto StoT = tpos1 - pos[0];
		auto StoE = pos[2] - pos[0];
		axis = Cross(StoT, StoE);
		axis.Normalize();
	}
	else {
		//膝なので回転はx軸
		axis = Vector3(1, 0, 0);
	}

	//行列を作る
	auto mat1 = XMMatrixTranslation(-pos[0].x, -pos[0].y, -pos[0].z);
	mat1 *= XMMatrixRotationAxis(XMLoadFloat3(&axis), thetaAB);
	mat1 *= XMMatrixTranslation(pos[0].x, pos[0].y, pos[0].z);

	auto mat2 = XMMatrixTranslation(-pos[1].x, -pos[1].y, -pos[1].z);
	mat2 *= XMMatrixRotationAxis(XMLoadFloat3(&axis), thetaBC-XM_PI);
	mat2 *= XMMatrixTranslation(pos[1].x, pos[1].y, pos[1].z);

	boneMatrices_[ik.nodeIdxes[1]] *= mat1;
	boneMatrices_[ik.nodeIdxes[0]] = mat2 * boneMatrices_[ik.nodeIdxes[1]];
	boneMatrices_[ik.targetIdx] = boneMatrices_[ik.nodeIdxes[0]];
}


//CCDIKによるIKの解決
void
MeshComponent::SolveCCDIK(const PMDIK& ik) {

	//IKの構成点
	std::vector<Vector3> pos;
	//ターゲットを決める
	auto targetNode = model_->boneNodeAddressArray_[ik.boneIdx];
	auto tpos1 = targetNode->startPos;

	//ここではIKボーンのみを考えて相対的な角度を出すので親の座標変換を無効にしておく
	auto parentmat = boneMatrices_[targetNode->ikParentBone];
	XMVECTOR det;
	auto invPmat = XMMatrixInverse(&det, parentmat);
	auto tpos2 = Vector3(XMVector3Transform(
		XMLoadFloat3(&tpos1),
		boneMatrices_[ik.boneIdx] * invPmat
	));

	//末端の場所を求める
	auto endPos = model_->boneNodeAddressArray_[ik.targetIdx]->startPos;
	//それぞれのノードの場所
	for (auto idx : ik.nodeIdxes) {
		pos.push_back(model_->boneNodeAddressArray_[idx]->startPos);
	}

	//それぞれのノードにかける行列べくた
	std::vector<XMMATRIX> mats(pos.size());
	std::fill(mats.begin(), mats.end(), XMMatrixIdentity());

	//一度に曲げる角度の制限
	auto lim = ik.limit * XM_PI;
	//誤差
	constexpr float eps = 0.0005f;
	for (int i = 0; i < ik.iterations; i++) {
		if ((tpos2 - endPos).Magnitude() <= eps) {
			//誤差の範囲内
			break;
		}
		//targetに近づける
		for (int bidx = 0; bidx < pos.size(); bidx++) {
			//StartPosToTarget
			auto StoT = tpos2 - pos[bidx];
			//StartPosToEndPos
			auto StoE = endPos - pos[bidx];
			StoT.Normalize();
			StoE.Normalize();
			if ((StoT - StoE).Magnitude() <= eps) {
				//誤差の範囲内
				continue;
			}
			//回転させてTとEを近づける
			auto axis = Cross(StoE, StoT);
			axis.Normalize();
			float angle = XMVector3AngleBetweenVectors(
				XMLoadFloat3(&StoE),
				XMLoadFloat3(&StoT)
			).m128_f32[0];
			//角度制限にひっかからないか
			angle = min(angle, lim);

			//行列の作成
			auto rot = XMMatrixRotationAxis(
				XMLoadFloat3(&axis), angle);
			auto invpos = -pos[bidx];
			auto mat =
				XMMatrixTranslationFromVector(XMLoadFloat3(&invpos))
				* rot
				* XMMatrixTranslationFromVector(XMLoadFloat3(&pos[bidx]));
			mats[bidx] *= mat;

			//末端側にもこの行列を適用する
			for (int idx = bidx - 1; idx >= 0; --idx) {
				pos[idx] = Vector3(XMVector3Transform(
					XMLoadFloat3(&pos[idx]), mat
				));
			}
			endPos = Vector3(XMVector3Transform(
				XMLoadFloat3(&endPos), mat
			));

			if ((endPos - tpos2).Magnitude() <= eps) {
				//誤差の範囲内
				break;
			}
		}
	}
	//行列を適用
	int idx = 0;
	for (auto& cidx : ik.nodeIdxes) {
		boneMatrices_[cidx] = mats[idx];
		idx++;
	}
	//ikのルートノードから末端まで一旦とりのぞいた親行列をかけていく
	auto rootNode =
		model_->boneNodeAddressArray_[ik.nodeIdxes.back()];
	auto rootMat = boneMatrices_[rootNode->ikParentBone];
	RecursiveMatrixMultiply(rootNode, rootMat);
}

void
MeshComponent::UpdateAABB() {
	auto rot = owner_->GetWorldRot();
	auto rotMat = XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
	auto sca = owner_->GetWorldScale();
	auto scaMat = XMMatrixScaling(sca.x, sca.y, sca.z);
	auto pos = owner_->GetWorldPos();
	//localをコピーして回転,移動,スケールさせる
	worldAABB_ = localAABB_;
	worldAABB_.Scale(scaMat);
	worldAABB_.Rotate(rotMat);
	worldAABB_.Move(pos);
}