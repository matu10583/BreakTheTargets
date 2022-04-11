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

	//model�̏��𓾂�
	dx12_ = Dx12Wrapper::Instance();
	model_ = dx12_->GetModelByPath(fileName);
	//�{�[���p�s��x�N�^�̃T�C�Y�����߂�
	boneMatrices_.resize(model_->boneNameArray_.size());
	std::fill(
		boneMatrices_.begin(), boneMatrices_.end(),
		XMMatrixIdentity()
	);

	for (auto& v : model_->vertices_) {
		localAABB_.UpdateBox(v);
	}

	//�ʒu���p�̃��\�[�X�����
	auto result = CreateTransformResourceAndView();
	if (FAILED(result)) {
		assert(0);
	}

}

//���_�z�񂩂烂�f���𐶐�
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
	//model�̏��𓾂�
	dx12_ = Dx12Wrapper::Instance();
	model_ = dx12_->GetModelByVertices(vertices, indecies, materials, name);

	for (auto& v : model_->vertices_) {
		localAABB_.UpdateBox(v);
	}

	//�ʒu���p�̃��\�[�X�����
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
	//�A�j���[�V�����̃m�[�h��ǉ����Ă��̃m�[�h��ID��Ԃ�
	const unsigned int id = aniID_;
	aniID_++;//�i�߂�
	AnimationNode animNode;
	animNode.anime = dx12_->GetAnimeByPath(fileName);
	animNode.isLoop = isLoop;
	animNode.transitFrameNum = (int)transitFrame;
	animeNodes_[id] = animNode;
	return id;
}

void
MeshComponent::DeleteAnimationNode(const unsigned int id) {

	//�m�[�h���琶���Ă�J�ڐ��̍폜
	transitMap_.erase(id);
	//�m�[�h�������J�ڐ����폜
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
	//�m�[�h�̍폜
	animeNodes_.erase(id);
}

void
MeshComponent::MakeAnimationTransit(
	const unsigned int start, const unsigned int end, 
	const bool* condition, bool value, bool isinter) {
	//�J�ڐ����쐻
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
	//�`�悷�邩���Ȃ�����owner����擾
	isDraw = owner_->IsDraw();

	//AABB�̍X�V
	UpdateAABB();

	//���[���h�s��̍X�V
	auto rot = owner_->GetWorldRot();
	auto pos = owner_->GetWorldPos();
	auto sca = owner_->GetWorldScale();

	XMMATRIX k = XMMatrixIdentity();
	if (!isLeftHanded) {//�E��n�̃��f���Ȃ�z�����𔽓]������
		k = XMMatrixRotationRollPitchYaw(0, XM_PI, 0);
	}
	mappedPosMatrix_[0] = XMMatrixTranslation(pos.x, pos.y, pos.z);

	mappedPosMatrix_[1] = k *
		XMMatrixScaling(sca.x, sca.y, sca.z) *
		XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);


	std::fill(boneMatrices_.begin(), boneMatrices_.end(),
		XMMatrixIdentity());
	//���[�V�����̍X�V
	if (nowAnimeNodeID_ != -1) {
		MotionUpdate(deltaTime);
	}
	//�s��̃R�s�[
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
	//vb,ib�̐ݒ�
	cmdList->IASetVertexBuffers(0, 1, &(model_->vbView_));
	cmdList->IASetIndexBuffer(&(model_->ibView_));

	//�ʒu���transbuffer��descHeap�̐ݒ�
	ID3D12DescriptorHeap* transHeaps[] = {
		transformHeap_.Get()
	};
	cmdList->SetDescriptorHeaps(1, transHeaps);
	//1��rootparameter�ƕR�Â�
	cmdList->SetGraphicsRootDescriptorTable(
		1, transformHeap_->GetGPUDescriptorHandleForHeapStart());

	//material��descheap�̐ݒ�
	ID3D12DescriptorHeap* matheap = model_->materialHeap_.Get();
	ID3D12DescriptorHeap* materialheaps[] = {
		matheap
	};
	cmdList->SetDescriptorHeaps(1, materialheaps);

	//�}�e���A����K�p���Ă��ꂼ��̕��ʂ��Ƃɕ`�悷��
	auto& matInfos = model_->materialInfos_;
	unsigned int idxOffset = 0;//�ǂ�Index�܂ŕ`�悵����
	auto matH = matheap->GetGPUDescriptorHandleForHeapStart();
	auto incSize = 7 * dx12_->Dev()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);//�}�e���A���q�[�v��7����΂��ɂ���̂�

	if (isShadow) {
		cmdList->DrawIndexedInstanced(
			model_->indicesNum_, 1,
			idxOffset, 0, 0
		);
	}
	else {
		//�ԓ��}�b�v�p����Ȃ��ᕔ�ʂ��Ƃɕ`�悷��
		for (auto& mInfo : matInfos) {
			cmdList->SetGraphicsRootDescriptorTable(2, matH);
				cmdList->DrawIndexedInstanced(
					mInfo.indicesNum, 1,
					idxOffset, 0, 0
				);

			//���̃q�[�v�ɐi�߂�
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
	//vb,ib�̐ݒ�
	cmdList->IASetVertexBuffers(0, 1, &(model_->vbView_));
	cmdList->IASetIndexBuffer(&(model_->ibView_));

	//�ʒu���transbuffer��descHeap�̐ݒ�
	ID3D12DescriptorHeap* transHeaps[] = {
		transformHeap_.Get()
	};
	cmdList->SetDescriptorHeaps(1, transHeaps);
	//1��rootparameter�ƕR�Â�
	cmdList->SetGraphicsRootDescriptorTable(
		1, transformHeap_->GetGPUDescriptorHandleForHeapStart());


	//material��descheap�̐ݒ�
	ID3D12DescriptorHeap* matheap = model_->materialHeap_.Get();
	ID3D12DescriptorHeap* materialheaps[] = {
		matheap
	};
	cmdList->SetDescriptorHeaps(1, materialheaps);

	//�}�e���A����K�p���Ă��ꂼ��̕��ʂ��Ƃɕ`�悷��
	auto& matInfos = model_->materialInfos_;
	unsigned int idxOffset = 0;//�ǂ�Index�܂ŕ`�悵����
	auto matH = matheap->GetGPUDescriptorHandleForHeapStart();
	auto incSize = 7 * dx12_->Dev()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);//�}�e���A���q�[�v��7����΂��ɂ���̂�

	if (isShadow) {
		cmdList->DrawIndexedInstanced(
			model_->indicesNum_, 1,
			idxOffset, 0, 0
		);
	}
	else {
		//�ԓ��}�b�v�p����Ȃ��ᕔ�ʂ��Ƃɕ`�悷��
		for (auto& mInfo : matInfos) {
			cmdList->SetGraphicsRootDescriptorTable(2, matH);
			cmdList->DrawIndexedInstanced(
				mInfo.indicesNum, 1,
				idxOffset, 0, 0
			);

			//���̃q�[�v�ɐi�߂�
			matH.ptr += incSize;
			idxOffset += mInfo.indicesNum;
		}
	}


}

HRESULT
MeshComponent::CreateTransformResourceAndView() {
	//Create buffer
	//256�̔{���ɍ��킹��
	//�T�C�Y�̓{�[���̍s��ƈʒu�Ɖ�]�̍s��
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

	//�}�b�v
	result = transformBuff_->Map(
		0, nullptr,
		(void**)&mappedPosMatrix_
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	//�����l
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
	//���̃t���[����
	int frameNo = 30 * elapsedTime_;
	auto& nowNode = animeNodes_[nowAnimeNodeID_];

	//�J�ڐ��T��(�܂��J�ڂ��Ȃ�)
	int nextAnimeNodeID = -1;

	if (frameNo >= (int)nowNode.anime->duration &&
		nowNode.isLoop) {
		//�J�ڐ悪�݂���Ȃ���duration�������玟�������A�j��
		nextAnimeNodeID = nowAnimeNodeID_;
	}

	for (auto tl : transitMap_[nowAnimeNodeID_]) {
		TransitLine* nextTl = nullptr;
		if (tl.condition == nullptr) {
				//�J�ڏ������Ȃ��̂ł�����ɂ���
			nextTl = &tl;
		}
		else if (*tl.condition == tl.value) {
			//�J�ډ\�Ȃ̂ł�����ɍs��
			nextTl = &tl;
		}
		if (nextTl == nullptr) {
			continue;
		}
		//�J�ڐ�ɂ����邩�`�F�b�N
		if (nextTl->isInterrupted ||
			frameNo >= (int)nowNode.anime->duration) {
			//�J�ڐ悪���������̂ł������ɍs��
			nextAnimeNodeID = nextTl->end;
			break;
		}
	}

	//�J�ڂ��Ȃ��Ȃ�nextAnimeNodeID=-1�̂܂�
	auto animData = nowNode.anime;
	//�O�̃A�j���[�V�����Ɖ��t���[�������Ăق��񂷂邩
	const int transitFrameNum = nowNode.transitFrameNum;
	if (transitFrameNum == 0) {
		isTransiting = false;
		isFirstFrame = false;
	}

	//�O�̃��[�V�������o����p
	std::unordered_map<std::string, KeyFrame> forMot;

	for (auto& bonemot : animData->motiondata) {
		//�A�j���[�V�����Ɏw�肳��Ă���{�[�������邩
		auto it = model_->boneNodeTable_.find(bonemot.first);
		if (it == model_->boneNodeTable_.end()) {
			continue;
			//�Ȃ��Ȃ疳������
		}
		auto& node = model_->boneNodeTable_[bonemot.first];
		auto& mots = bonemot.second;
		//�Ō�Ɏw�肳�ꂽ���[�V�����f�[�^
		auto rit = std::find_if(
			mots.rbegin(), mots.rend(),
			[=](const KeyFrame& mot) {
				return (int)mot.frameNo <= frameNo;
			}
		);

		//�ǂ̃L�[�t���[���Ԃ̕�Ԃ����邩���߂�
		int rFrameNo;
		XMVECTOR rQuaternion;
		Vector3 rOffset;
		auto motit = rit.base();//�f�t�H���g��rit�̎��̃t���[��

		if (isTransiting) {
			//�J�ڕ�Ԓ�
			auto it = forMotionData_.find(bonemot.first);
			if (it == forMotionData_.end()) {
				continue;
				//�Ȃ��Ȃ疳������
			}
			if (isFirstFrame) {
				frameNo = -transitFrameNum;
				elapsedTime_ = frameNo / 30.0f;
				isFirstFrame = false;
			}
			rFrameNo = -transitFrameNum;//�O�̃��[�V����
			rQuaternion = forMotionData_[bonemot.first].quaternion;
			rOffset = forMotionData_[bonemot.first].offset;
			motit = mots.begin();//���̃��[�V�����̍ŏ��̃L�[�t���[��
		}
		else if (rit == mots.rend()) {
			//�ŏ��̃L�[�t���[�����ݒ肳��Ă��Ȃ��̂őO�̃|�[�Y��z�肷��
					//�A�j���[�V�����Ɏw�肳��Ă���{�[�������邩
			auto it = forMotionData_.find(bonemot.first);
			if (it == forMotionData_.end()) {
				continue;
				//�Ȃ��Ȃ疳������
			}
			rFrameNo = 0;
			rQuaternion = forMotionData_[bonemot.first].quaternion;
			rOffset = forMotionData_[bonemot.first].offset;
		}
		else {
			//���ʂɑO�ƁA��̃L�[�t���[���̕��
			rFrameNo = rit->frameNo;
			rQuaternion = rit->quaternion;
			rOffset = rit->offset;
		}

		XMVECTOR quatern;
		XMMATRIX rotmat;
		XMVECTOR offset;
		if (motit != mots.end()) {
			//rit��rbegin��������motit��end�ɂȂ��Ă��܂�
			//���`��Ԃ̌W�����擾
			auto t = static_cast<float>(frameNo - rFrameNo)
				/ static_cast<float>(motit->frameNo - rFrameNo);
			//�x�W�F�Ȑ���̌W�������߂�
			t = GetYFromXOnBezier(t, motit->p1, motit->p2, 15);
			//���`��Ԃ��s��
			quatern = XMQuaternionSlerp(rQuaternion, motit->quaternion, t);
			rotmat = XMMatrixRotationQuaternion(quatern);
			offset = XMVectorLerp(
				XMLoadFloat3(&rOffset), XMLoadFloat3(&motit->offset), t
			);
		}
		else {
			//��Ԃ̕K�v�Ȃ�
			quatern = rQuaternion;
			rotmat =
				XMMatrixRotationQuaternion(rQuaternion);
			offset = XMLoadFloat3(&rOffset);
		}
		auto& pos = node.startPos;
		//��]
		auto mat = XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* rotmat
			* XMMatrixTranslation(pos.x, pos.y, pos.z);
		//�ړ�
		boneMatrices_[node.boneIdx] = mat *
			XMMatrixTranslationFromVector(offset);

		//�J�ڂ���Ȃ�O�̃��[�V�������o���Ă���
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
	//�Z���^�[�{�[������S�g�̃{�[���ɂ����Ă���
	RecursiveMatrixMultiply(&(model_->boneNodeTable_["�Z���^�["]), XMMatrixIdentity());
	IKSolve(frameNo);

	if (frameNo >= 0 && isTransiting) {
		isTransiting = false;//�t���[���ԍ������ɂȂ�����J�ڏI��
		forMotionData_.clear();
		forMotionData_ = forMot;//�J�ڏI�����̃��[�V�����ɍX�V

	}
	//�J�ڂ̎��s
	if (nextAnimeNodeID != -1) {
		elapsedTime_ = 0;
		forMotionData_.clear();
		forMotionData_ = forMot;
		if (nowAnimeNodeID_ != nextAnimeNodeID) {
			//���[�v����Ȃ��đJ�ڂȂ�
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
	//IK�Ώ̃{�[����������
	auto anime = animeNodes_[nowAnimeNodeID_].anime;
	auto rit = std::find_if(anime->ikEnableData.rbegin(),
		anime->ikEnableData.rend(),
		[frameNo](const VMDIKEnable& val) {
			return val.frameNo <= frameNo;
		});
	for (auto& ik : model_->ikData_) {
		if (rit != anime->ikEnableData.rend()) {
			//IK�{�[����IKEnable�f�[�^��T��
			auto ikEnableIt =
				rit->ikEnableTable.find(model_->boneNameArray_[ik.boneIdx]);
			if (ikEnableIt != rit->ikEnableTable.end()) {
				if (!ikEnableIt->second) {
					//IK�͖���
					continue;
				}
			}
		}
		//IK���L���Ȃ̂�IK����������
		auto childNodeNum = ik.nodeIdxes.size();

		switch (childNodeNum)
		{
		case 0:
			//IK���Ȃ肽���Ȃ�
			assert(0);
			continue;
		case 1://��_��IK
			SolveLookAt(ik);
			break;
		case 2://3�_��ik
			SolveCosine(ik);
			break;
		default://����ȏ�
			SolveCCDIK(ik);
			break;
		}
	}
}

//LookAt�ɂ��IK�̉���
void
MeshComponent::SolveLookAt(const PMDIK& ik) {
	//�m�[�h����̏ꍇ
	auto root = model_->boneNodeAddressArray_[ik.nodeIdxes[0]];
	auto target = model_->boneNodeAddressArray_[ik.targetIdx];
	auto rpos1 = root->startPos;
	auto tpos1 = target->startPos;
	//����������̈ʒu
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
	//���ꂼ��̃{�[���x�N�g��
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

//�]���藝�ɂ��IK�̉���
void
MeshComponent::SolveCosine(const PMDIK& ik) {
	//IK�̍\���m�[�h
	std::vector<Vector3> pos;
	//�{�[���̒���
	std::array<float, 2> edgeLens;

	//�ڕW�n�_�̃{�[��
	auto& target = model_->boneNodeAddressArray_[ik.boneIdx];
	auto tpos1 = target->startPos;
	auto tpos2 = Vector3(
		XMVector3Transform(
			XMLoadFloat3(&tpos1),
			boneMatrices_[ik.boneIdx]
		)
	);

	//�\���m�[�h�̏ꏊ���o����
		//���[�̃m�[�h
	auto endNode =
		model_->boneNodeAddressArray_[ik.targetIdx];
	pos.push_back(endNode->startPos);
	for (auto& idx : ik.nodeIdxes) {
		auto& boneNode =
			model_->boneNodeAddressArray_[idx];
		pos.emplace_back(boneNode->startPos);
	}
	//���[�g����̕��я��ɂ���
	std::reverse(pos.begin(), pos.end());

	//���ꂼ��̃{�[���̒���
	edgeLens[0] = (pos[1] - pos[0]).Magnitude();
	edgeLens[1] = (pos[2] - pos[1]).Magnitude();

	//�{�[���̈ʒu�ɍs���K�p����
	pos[0] = Vector3(XMVector3Transform(
		XMLoadFloat3(&pos[0]),
		boneMatrices_[ik.nodeIdxes[1]]
	));
	pos[2] = Vector3(XMVector3Transform(
		XMLoadFloat3(&pos[2]),
		boneMatrices_[ik.boneIdx]
	));

	//�]���藝�ɂ�蒆�ԃm�[�h�̈ʒu���v�Z����
	// �Ӄx�N�g��
	auto Avec = pos[2] - pos[0];
	//3�ӂ̒���
	float A = Avec.Magnitude();
	float B = edgeLens[0];
	float C = edgeLens[1];

	float ab = (A * A + B * B - C * C) / (2.0f * A * B);
	float bc = (B * B + C * C - A * A) / (2.0f * B * C);
	//�덷�̂��߁Aclamp����
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


	Vector3 axis;//��]�̎������߂�
	//�G�̉�]�Ȃ��΂�x��
	if (std::find(model_->kneeIdxes_.begin(), model_->kneeIdxes_.end(),
		ik.nodeIdxes[0]) == model_->kneeIdxes_.end()) {
		//��]�̕��ʂɐ����Ȃ��������߂�
		auto StoT = tpos1 - pos[0];
		auto StoE = pos[2] - pos[0];
		axis = Cross(StoT, StoE);
		axis.Normalize();
	}
	else {
		//�G�Ȃ̂ŉ�]��x��
		axis = Vector3(1, 0, 0);
	}

	//�s������
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


//CCDIK�ɂ��IK�̉���
void
MeshComponent::SolveCCDIK(const PMDIK& ik) {

	//IK�̍\���_
	std::vector<Vector3> pos;
	//�^�[�Q�b�g�����߂�
	auto targetNode = model_->boneNodeAddressArray_[ik.boneIdx];
	auto tpos1 = targetNode->startPos;

	//�����ł�IK�{�[���݂̂��l���đ��ΓI�Ȋp�x���o���̂Őe�̍��W�ϊ��𖳌��ɂ��Ă���
	auto parentmat = boneMatrices_[targetNode->ikParentBone];
	XMVECTOR det;
	auto invPmat = XMMatrixInverse(&det, parentmat);
	auto tpos2 = Vector3(XMVector3Transform(
		XMLoadFloat3(&tpos1),
		boneMatrices_[ik.boneIdx] * invPmat
	));

	//���[�̏ꏊ�����߂�
	auto endPos = model_->boneNodeAddressArray_[ik.targetIdx]->startPos;
	//���ꂼ��̃m�[�h�̏ꏊ
	for (auto idx : ik.nodeIdxes) {
		pos.push_back(model_->boneNodeAddressArray_[idx]->startPos);
	}

	//���ꂼ��̃m�[�h�ɂ�����s��ׂ���
	std::vector<XMMATRIX> mats(pos.size());
	std::fill(mats.begin(), mats.end(), XMMatrixIdentity());

	//��x�ɋȂ���p�x�̐���
	auto lim = ik.limit * XM_PI;
	//�덷
	constexpr float eps = 0.0005f;
	for (int i = 0; i < ik.iterations; i++) {
		if ((tpos2 - endPos).Magnitude() <= eps) {
			//�덷�͈͓̔�
			break;
		}
		//target�ɋ߂Â���
		for (int bidx = 0; bidx < pos.size(); bidx++) {
			//StartPosToTarget
			auto StoT = tpos2 - pos[bidx];
			//StartPosToEndPos
			auto StoE = endPos - pos[bidx];
			StoT.Normalize();
			StoE.Normalize();
			if ((StoT - StoE).Magnitude() <= eps) {
				//�덷�͈͓̔�
				continue;
			}
			//��]������T��E���߂Â���
			auto axis = Cross(StoE, StoT);
			axis.Normalize();
			float angle = XMVector3AngleBetweenVectors(
				XMLoadFloat3(&StoE),
				XMLoadFloat3(&StoT)
			).m128_f32[0];
			//�p�x�����ɂЂ�������Ȃ���
			angle = min(angle, lim);

			//�s��̍쐬
			auto rot = XMMatrixRotationAxis(
				XMLoadFloat3(&axis), angle);
			auto invpos = -pos[bidx];
			auto mat =
				XMMatrixTranslationFromVector(XMLoadFloat3(&invpos))
				* rot
				* XMMatrixTranslationFromVector(XMLoadFloat3(&pos[bidx]));
			mats[bidx] *= mat;

			//���[���ɂ����̍s���K�p����
			for (int idx = bidx - 1; idx >= 0; --idx) {
				pos[idx] = Vector3(XMVector3Transform(
					XMLoadFloat3(&pos[idx]), mat
				));
			}
			endPos = Vector3(XMVector3Transform(
				XMLoadFloat3(&endPos), mat
			));

			if ((endPos - tpos2).Magnitude() <= eps) {
				//�덷�͈͓̔�
				break;
			}
		}
	}
	//�s���K�p
	int idx = 0;
	for (auto& cidx : ik.nodeIdxes) {
		boneMatrices_[cidx] = mats[idx];
		idx++;
	}
	//ik�̃��[�g�m�[�h���疖�[�܂ň�U�Ƃ�̂������e�s��������Ă���
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
	//local���R�s�[���ĉ�],�ړ�,�X�P�[��������
	worldAABB_ = localAABB_;
	worldAABB_.Scale(scaMat);
	worldAABB_.Rotate(rotMat);
	worldAABB_.Move(pos);
}