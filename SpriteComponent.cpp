#include "SpriteComponent.h"
#include "Sprite.h"
#include "Renderer.h"
#include "Dx12Wrapper.h"
#include "Object.h"
#include "Game.h"

SpriteComponent::SpriteComponent(Object* owner, const char* fileName, int updateOrder) :
	Component(owner, updateOrder),
scale_(1, 1),
pivot_(0, 0),
rot_(0),
clipSize_(1, 1),
clipLeftTop_(0, 0),
pos_(0, 0, 0),
offset_(0, 0),
dx12_(nullptr),
mappedTransMatrix_(nullptr),
sprite_(nullptr),
transformBuff_(nullptr),
transformHeap_(nullptr),
isDraw(true)
{
	Renderer::Instance()->AddSpriteComponent(this);
	//model�̏��𓾂�
	dx12_ = Dx12Wrapper::Instance();
	sprite_ = dx12_->GetSpriteByPath(fileName);
	//�ʒu���p�̃��\�[�X�����
	auto result = CreateTransformResourceAndView();
	if (FAILED(result)) {
		assert(0);
	}
}

SpriteComponent::~SpriteComponent() {
	Renderer::Instance()->RemoveSpriteComponent(this);
}

void
SpriteComponent::Update(float deltaTime) {
	//�ړ��A��]���̍X�V
	mappedTransMatrix_->world =
		XMMatrixScaling(scale_.x, scale_.y, 1.0f) *
		XMMatrixRotationZ(rot_) *
		XMMatrixTranslation(pos_.x, pos_.y, pos_.z);
	mappedTransMatrix_->pivot = pivot_;
	mappedTransMatrix_->clipSize = clipSize_;
	mappedTransMatrix_->clipLeftTop[0] = clipLeftTop_;
	mappedTransMatrix_->offset = offset_;
}

void
SpriteComponent::Draw() {
	if (owner_->GetState() != Object::Active ||
		GetState() != Component::CActive ||
		!isDraw) {
		return;
	}
	auto cmdList = dx12_->CommandList();
	//vb,ib�̐ݒ�
	cmdList->IASetVertexBuffers(0, 1, &(sprite_->VBView()));

	//�ʒu���transbuffer��descHeap�̐ݒ�
	ID3D12DescriptorHeap* transHeaps[] = {
		transformHeap_.Get()
	};
	cmdList->SetDescriptorHeaps(1, transHeaps);
	//1��rootparameter�ƕR�Â�
	cmdList->SetGraphicsRootDescriptorTable(
		1, transformHeap_->GetGPUDescriptorHandleForHeapStart());

	//texture��descheap�̐ݒ�
	ID3D12DescriptorHeap* texHeaps[] = {
		sprite_->TextureHeap()
	};
	cmdList->SetDescriptorHeaps(1, texHeaps);
	//2��rootparameter�ƕR�Â�
	cmdList->SetGraphicsRootDescriptorTable(
		2, sprite_->TextureHeap()->GetGPUDescriptorHandleForHeapStart());

	cmdList->DrawInstanced(4, 1, 0, 0);


}

HRESULT
SpriteComponent::CreateTransformResourceAndView() {
	//Create buffer
//256�̔{���ɍ��킹��
	auto buffSize = (sizeof(TransformData) + 0xff) & ~0xff;
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
		(void**)&mappedTransMatrix_
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	//�����l

	mappedTransMatrix_->world = XMMatrixIdentity();
	mappedTransMatrix_->pivot = pivot_;
	mappedTransMatrix_->clipSize = clipSize_;
	std::fill(mappedTransMatrix_->clipLeftTop,
		&mappedTransMatrix_->clipLeftTop[MAX_CLIP_SIZE - 1], Vector2::Zero());
	mappedTransMatrix_->offset = offset_;
	mappedTransMatrix_->spSize = Vector2::Zero();

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

const Vector2
SpriteComponent::GetImgSize()const {
	auto ret = sprite_->GetSize();
	return ret;
}

void 
SpriteComponent::SetScaleRatioToWindowSize(const Vector2& ratio) {
	auto wsize = Game::Instance()->GetWindowSize();
	auto spSize = sprite_->GetSize();
	float kx = wsize.cx / spSize.x;
	scale_.x = kx * ratio.x;
	float ky = wsize.cy / spSize.y;
	scale_.y = ky * ratio.y;
}
void 
SpriteComponent::SetScaleRatioToWindowSizeX(float ratio) {
	auto wsize = Game::Instance()->GetWindowSize();
	auto spSize = sprite_->GetSize();
	float kx = wsize.cx / spSize.x;
	scale_.x = kx * ratio;
	scale_.y = scale_.x;
}
void 
SpriteComponent::SetScaleRatioToWindowSizeY(float ratio) {
	auto wsize = Game::Instance()->GetWindowSize();
	auto spSize = sprite_->GetSize();
	float ky = wsize.cy / spSize.y;
	scale_.y = ky * ratio;
	scale_.x = scale_.y;
}


void 
SpriteComponent::SetPosRatioToWindowSize(const Vector3& p) {
	auto wsize = Game::Instance()->GetWindowSize();
	pos_ = Vector3(
		wsize.cx * p.x, wsize.cy * p.y, p.z
	);
}