#include "PointLightComponent.h"
#include "Dx12Wrapper.h"
#include "Object.h"

PointLightComponent::PointLightComponent(Object* owner, int updateOrder):
Component(owner, updateOrder)
{
	dx12_ = Dx12Wrapper::Instance();
	dx12_->AddPointLight(this);
	auto result = CreateLightResourceAndView();
	if (FAILED(result)) {
		assert(0);
	}
}

PointLightComponent::~PointLightComponent() {
	Dx12Wrapper::Instance()->RemovePointLight(this);
}

void
PointLightComponent::SetLightBuffer()
{
	auto pos = owner_->GetWorldPos();
	mappedLightData_->lightCol = color_;
	mappedLightData_->lightPos = pos;
	mappedLightData_->lightScale = scale_;
	mappedLightData_->radius = radius_;
	mappedLightData_->range = range_;

	//現在のシーンをセット
	ID3D12DescriptorHeap* sceneheaps[] = {
		lightHeap_.Get()
	};
	dx12_->CommandList()->SetDescriptorHeaps(1, sceneheaps);
	dx12_->CommandList()->SetGraphicsRootDescriptorTable(
		4, lightHeap_->GetGPUDescriptorHandleForHeapStart());
}

HRESULT 
PointLightComponent::CreateLightResourceAndView() {
	//Create buffer
//256の倍数に合わせる
//サイズはボーンの行列と位置と回転の行列分
	auto buffSize = (
		sizeof(LightData) + 0xff) & ~0xff;
	auto heapprop =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resdesc = CD3DX12_RESOURCE_DESC::Buffer(buffSize);

	auto result = dx12_->Dev()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(lightBuff_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	//マップ
	result = lightBuff_->Map(
		0, nullptr,
		(void**)&mappedLightData_
	);
	if (FAILED(result)) {
		assert(0);
		return result;
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
		IID_PPV_ARGS(lightHeap_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = lightBuff_->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = buffSize;
	dx12_->Dev()->CreateConstantBufferView(
		&cbvDesc,
		lightHeap_->GetCPUDescriptorHandleForHeapStart()
	);
	return S_OK;
}