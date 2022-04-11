#include "Dx12Wrapper.h"
#include "Game.h"
#include "Model.h"
#include "Sprite.h"
#include "ModelLoader.h"
#include "PointLightComponent.h"
#include <string>
#include <vector>
#include <Windows.h>
#include <assert.h>
#include <d3dcompiler.h>
#include <utility>
#include <filesystem>
#include <shlobj.h>
#include<iostream>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib, "DirectXTex.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

Dx12Wrapper* Dx12Wrapper::instance;
constexpr uint32_t shadowMapSize = 1024;

namespace {
	//���f���̃p�X�ƃe�N�X�`���̃p�X���獇���p�X�𓾂�
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath) {
		unsigned int pathIndex1 = modelPath.rfind('/');
		unsigned int pathIndex2 = modelPath.rfind('\\');
		auto pathIndex = max(pathIndex1, pathIndex2);
		auto folderPath = modelPath.substr(0, pathIndex + 1);
		return folderPath + texPath;
	}

	//�t�@�C��������g���q���擾����
	std::string
		GetExtension(const std::string& path) {
		unsigned int idx = path.rfind('.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	std::wstring
		GetExtension(const std::wstring& path) {
		int idx = path.rfind(L'.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}


	//string(�}���`�o�C�g������)����wstring(���C�h������)�𓾂�
	std::wstring
		GetWideStringFromString(const std::string& str) {
		//�����񐔂𓾂�
		auto num1 = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), -1, nullptr, 0);

		std::wstring wstr;
		wstr.resize(num1);//����ꂽ�����񐔂Ń��T�C�Y

		//�m�ۍς݂�wstr�ɕϊ���������R�s�[
		auto num2 = MultiByteToWideChar(CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), -1, &wstr[0], num1);

		assert(num1 == num2);//�ꉞ�`�F�b�N
		return wstr;
	}

	//HRESULT�^���猋�ʁibool�j�ƃG���[���(ID3DBlob)�����o��
	bool
		CheckResult(HRESULT result, ID3DBlob* errBlob) {
		if (FAILED(result)) {
			//�G���[�`�F�b�N
			if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
				
				OutputDebugStringA("File Not Found.");
				return false;
			}
			else {
				std::string errstr;
				errstr.resize(errBlob->GetBufferSize());
				std::copy_n((char*)errBlob->GetBufferPointer(), errBlob->GetBufferSize(),
					errstr.begin());
				errstr += "\n";
				OutputDebugStringA(errstr.c_str());
			}
			return SUCCEEDED(result);
		}
		return SUCCEEDED(result);
	}

	size_t
		AllignToMultipleOf256(size_t size) {
		return (size + 0xff) & ~0xff;
	}

	void EnableDebugLayer() {
		ComPtr<ID3D12Debug> debugLayer = nullptr;
		auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));
		debugLayer->EnableDebugLayer();
	}


	//pix on windows�ɂ��f�o�b�O
	static std::wstring GetLatestWinPixGpuCapturerPath_Cpp17()
	{
		LPWSTR programFilesPath = nullptr;
		SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);
		
		std::filesystem::path pixInstallationPath = programFilesPath;
		pixInstallationPath /= "Microsoft PIX";

		std::wstring newestVersionFound;

		for (auto const& directory_entry : std::filesystem::directory_iterator(pixInstallationPath))
		{
			if (directory_entry.is_directory())
			{
				if (newestVersionFound.empty() || newestVersionFound < directory_entry.path().filename().c_str())
				{
					newestVersionFound = directory_entry.path().filename().c_str();
				}
			}
		}

		if (newestVersionFound.empty())
		{
			// TODO: Error, no PIX installation found
		}

		return pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
	}
}

void
Dx12Wrapper::Create() {
	assert(!instance && "�V���O���g���ł���I");
	instance = new Dx12Wrapper();
}

void
Dx12Wrapper::Destroy() {
#ifdef _DEBUG//imgui�������
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif // _DEBUG



	delete instance;
	instance = nullptr;
}

Dx12Wrapper*
Dx12Wrapper::Instance() {
	return instance;
}

bool
Dx12Wrapper::Init(HWND hwnd) {
#ifdef _DEBUG
	EnableDebugLayer();
#endif // _DEBUG



	//pmd�֘A�@�\���l�߂��N���X
	modelLoader_.reset(new ModelLoader());
	CreateTextureLoadTable();
#ifdef _DEBUG
	if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
	{
		LoadLibrary(GetLatestWinPixGpuCapturerPath_Cpp17().c_str());
	}

#endif // _DEBUG


	if (FAILED(InitDXGIDevice())) {
		assert(0);
		return false;
	}
	if (FAILED(InitCommand())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateSwapChain(hwnd))) {
		assert(0);
		return false;
	}
	if (FAILED(CreateFinalRendererTarget())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateSceneBufferView())) {
		assert(0);
		return false;
	}
	if (FAILED(Create2DSceneBufferView())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateBoardConstBufferView())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateBoardResources())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateBoardVertex())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateBoardRTVs())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateBoardSRVs())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateBoardRootSignature())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateBoardGraphicsPipeline())) {
		assert(0);
		return false;
	}
	if (FAILED(CreateDepthStencilView())) {
		assert(0);
		return false;
	}
#ifdef _DEBUG
	if (!InitImgui()) {
		assert(0);
		return false;
	}
#endif // _DEBUG


	//�t�F���X�̍쐬
	if (FAILED(dev_->CreateFence(
		fenceVal_,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf())))) {
		assert(0);
		return false;
	}
	
	//�f�t�H���g�e�N�X�`���̍쐬
	if (!(modelLoader_->CreateDefaultTextures())) {
		assert(0);
		return false;
	}
	
	return true;
}

ID3D12Device*
Dx12Wrapper::Dev()const {
	return dev_.Get();
}

ID3D12GraphicsCommandList*
Dx12Wrapper::CommandList()const {
	return cmdList_.Get();
}

std::shared_ptr<Model>
Dx12Wrapper::GetModelByPath(const char* path) {
	auto result = modelMap_.find(path);
	if (result != modelMap_.end()) {
		//�e�[�u�����ɂ�������}�b�v���̃��f����Ԃ�
		return modelMap_[path];
	}
	else {
		modelMap_[path] = std::make_shared<Model>();
		auto result = LoadModel(path, modelMap_[path].get());
		if (!result) {
			assert(0);
			return nullptr;
		}
		return modelMap_[path];
	}
}

bool
Dx12Wrapper::LoadModel(const char* path, Model* model) {
	auto ext = GetExtension(path);
	if (ext == "pmd") {
		return modelLoader_->LoadPMDModel(path, model);
	}
	else if (ext == "pmx") {
		assert(0);
		return modelLoader_->LoadPMXModel(path, model);
	}
	else {
		assert(0);
		return false;
	}
}

std::shared_ptr<AnimationData>
Dx12Wrapper::GetAnimeByPath(const char* path) {
	auto result = animeVMDMap_.find(path);
	if (result != animeVMDMap_.end()) {
		//�e�[�u�����ɂ�������}�b�v���̃��f����Ԃ�
		return animeVMDMap_[path];
	}
	else {
		animeVMDMap_[path] = std::make_shared<AnimationData>();
		auto result = modelLoader_->LoadAnimation(path, animeVMDMap_[path].get());
		if (!result) {
			assert(0);
			return nullptr;
		}
		return animeVMDMap_[path];
	}
}

std::shared_ptr<Sprite>
Dx12Wrapper::GetSpriteByPath(const char* path) {
	auto result = spriteMap_.find(path);
	if (result != spriteMap_.end()) {
		//�e�[�u�����ɂ�������}�b�v���̃X�v���C�g��Ԃ�
		return spriteMap_[path];
	}
	else {
		spriteMap_[path] = std::make_shared<Sprite>();
		auto result = modelLoader_->LoadSprite(path, spriteMap_[path].get());
		if (!result) {
			assert(0);
			return nullptr;
		}
		return spriteMap_[path];
	}
}

std::shared_ptr<Model>
Dx12Wrapper::GetModelByVertices(const std::vector<VertexData>& vertices,
	const std::vector<Index>& indecies, const std::vector<Material>& materials,
	const char* name) {

	if (name == nullptr) {//���O�̎w�肪�Ȃ����
		auto model = std::make_shared<Model>();
		auto result = modelLoader_->CreateVerticesModel(vertices, indecies, materials, model.get());
		return model;
	}

	auto result = modelMap_.find(name);
	if (result != modelMap_.end()) {
		//�e�[�u�����ɂ�������}�b�v���̃��f����Ԃ�
		return modelMap_[name];
	}
	else {
		modelMap_[name] = std::make_shared<Model>();
		auto res = modelLoader_->CreateVerticesModel(
			vertices, indecies, materials, modelMap_[name].get());
		if (!res) {
			assert(0);
			return nullptr;
		}
		return modelMap_[name];
	}
}

void
Dx12Wrapper::SetCameraSetting(const Vector3& eye, const Vector3& eyeRot, 
	float fov, const Vector3& up) {
	eyePos_ = eye;
	eyeRot_ = eyeRot;
	fov_ = fov;
	up_ = up;
}

void
Dx12Wrapper::SetDirectionalLightSetting(
	const Vector3& dir, const Vector3& col) {
	//���̏��̑��M
	mappedSceneData_->lightDir = dir;
	mappedSceneData_->lightCol = col;
}

void
Dx12Wrapper::SetVignette(bool v) {
	mappedBoardData_->isVignette = v;
}


void 
Dx12Wrapper::SetSceneData() {
	DXGI_SWAP_CHAIN_DESC1 desc1 = {};
	auto result = swapchain_->GetDesc1(&desc1);

	//view�s��̌v�Z
	auto eyepos = XMLoadFloat3(&eyePos_);
	XMMATRIX rotmat = XMMatrixRotationRollPitchYaw(
		eyeRot_.x, eyeRot_.y, eyeRot_.z);
	auto unitZ = Vector3(0, 0, 1);
	auto unitZvec = XMLoadFloat3(&unitZ);
	auto forVec = XMVector3Transform(unitZvec, rotmat);

	auto targetpos = XMVectorAdd(eyepos, forVec);
	auto upvec = XMLoadFloat3(&up_);
	auto view = XMMatrixLookAtLH(eyepos, targetpos, upvec);
	mappedSceneData_->view = view;		
	XMVECTOR det;
	auto invview = XMMatrixInverse(&det, view);
	mappedSceneData_->invview = invview;

	mappedSceneData_->eye = eyePos_;

	float ratio = static_cast<float>(desc1.Width) / static_cast<float>(desc1.Height);
	//�v���W�F�������s��̌v�Z
	auto fov_ = XM_PIDIV4;
	auto proj = XMMatrixPerspectiveFovLH(
		fov_,
		ratio,
		0.1f,//�j�A�\�N���b�v
		1000.0f//�t�@�[�N���b�v
	);
	mappedSceneData_->proj = proj;


	auto invproj = XMMatrixInverse(&det, proj);
	mappedSceneData_->invproj = invproj;

	auto lpos = eyePos_ - lightDir * 500;
	auto lview = XMMatrixLookAtLH(
		XMLoadFloat3(&lpos),
		XMLoadFloat3(&lightDir),
		upvec);

	mappedSceneData_->lightView =
		XMMatrixTranslation(-eyePos_.x, -eyePos_.y, -eyePos_.z) *
		lview *
		XMMatrixOrthographicLH(300, 300, 1.0f, 1000.0f);
	mappedSceneData_->lightDir = lightDir;

	//���݂̃V�[�����Z�b�g
	ID3D12DescriptorHeap* sceneheaps[] = {
		sceneCbvHeap_.Get()
	};
	cmdList_->SetDescriptorHeaps(1, sceneheaps);
	cmdList_->SetGraphicsRootDescriptorTable(
		0, sceneCbvHeap_->GetGPUDescriptorHandleForHeapStart());

	cmdList_->SetDescriptorHeaps(1,
		depthSrvHeap_.GetAddressOf());
	auto handle = depthSrvHeap_->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += dev_->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);//2�߂̃��C�g�f�v�X���Z�b�g
	cmdList_->SetGraphicsRootDescriptorTable(3,
		handle);
}

void
Dx12Wrapper::SetSceneData2D() {
	DXGI_SWAP_CHAIN_DESC1 desc1 = {};
	auto result = swapchain_->GetDesc1(&desc1);
	auto w = desc1.Width;
	auto h = desc1.Height;
	//�v���W�F�������s��̌v�Z
	// ����Ă����X�N���[�����W���V�F�[�_�[�悤���W�ɕϊ�
	//���_���W��-1~1�Ȃ̂Ńs�N�Z��������ɕϊ�����
	// (+-2/size)��������0~2(0~-2)�ɂ������̂�+-1�ړ�������
	// �X�N���[�����W�ł�y���͔��]������
	//��)�X�N���[���T�C�Y(500, 1000)��̓_(200, 200)
	//�c�F(200/500)*2-1 =-0.2
	XMFLOAT4X4 scalemat = XMFLOAT4X4(
		2.0f / w, 0, 0, 0,
		0, -2.0f / h, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
	XMFLOAT4X4 movemat = XMFLOAT4X4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		-1, 1, 0, 1
	);
	mappedSceneData2D_->proj = 
		XMLoadFloat4x4(&scalemat)*
		XMLoadFloat4x4(&movemat);
	

	//���݂̃V�[�����Z�b�g
	ID3D12DescriptorHeap* sceneheaps[] = {
		scene2DCbvHeap_.Get()
	};
	cmdList_->SetDescriptorHeaps(1, sceneheaps);
	cmdList_->SetGraphicsRootDescriptorTable(
		0, scene2DCbvHeap_->GetGPUDescriptorHandleForHeapStart());
}

void
Dx12Wrapper::SetBoardData() {
	mappedBoardData_->contrast = contrast_;
	//���݂�cbv���Z�b�g
	ID3D12DescriptorHeap* boardheaps[] = {
		boardCbvHeap_.Get()
	};
	cmdList_->SetDescriptorHeaps(1, boardheaps);
	cmdList_->SetGraphicsRootDescriptorTable(
		1, boardCbvHeap_->GetGPUDescriptorHandleForHeapStart());
}


Dx12Wrapper::Dx12Wrapper():
fenceVal_(1),
mappedSceneData_(nullptr),
eyePos_(0, 10, 10),
eyeRot_(0, 0, 0),
lightCol_(0.3f, 0.3f,0.3f),
lightDir(0, -1, 0),
planeVec(0, 1, 0, 0),
fov_(XM_PIDIV4),
mappedSceneData2D_(nullptr),
bgCol_(0, 0, 0),
contrast_(5),
up_(0, 1, 0)
{
}

Dx12Wrapper::~Dx12Wrapper(){

}
HRESULT
Dx12Wrapper::InitDXGIDevice() {
	//create dxgiFactory
	auto facflg = 0;
#ifdef _DEBUG
	facflg |= DXGI_CREATE_FACTORY_DEBUG;
#endif // DEBUG
	
	auto result = CreateDXGIFactory2(facflg,
		IID_PPV_ARGS(dxgiFactory_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) return result;
	
	//�f�o�C�X�̏�����
	//�t�B�[�`���[���x���Ǝg���O���{�����߂�
	D3D_FEATURE_LEVEL levs[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	std::vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0;
		dxgiFactory_->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND;
		++i) {
		adapters.push_back(tmpAdapter);
	}
	
	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adptdesc = {};
		adpt->GetDesc(&adptdesc);
		std::wstring strDesc = adptdesc.Description;
		//TODO: �g���O���{��I������I�v�V����������
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}
	result = S_FALSE;
	D3D_FEATURE_LEVEL fLev;//�g�p�����t�B�[�`���[���x��
	for (auto l : levs) {
		if (SUCCEEDED(D3D12CreateDevice(
			tmpAdapter,//�g���A�_�v�^�[
			l,//�t�B�[�`���[���x��
			IID_PPV_ARGS(dev_.ReleaseAndGetAddressOf())
		))) {
			fLev = l;
			result = S_OK;
			break;
		}
	}
	for (auto adpt : adapters) {//HACK:ComPtr���g���Ă��܂����Ƃ�肽��
		adpt->Release();
	}
	return result;
}

HRESULT
Dx12Wrapper::InitCommand() {
	auto result = dev_->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(cmdAllocator_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) return result;
	result = dev_->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdAllocator_.Get(),
		nullptr,
		IID_PPV_ARGS(cmdList_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) return result;

	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};
	cmdQueueDesc.Flags =
		D3D12_COMMAND_QUEUE_FLAG_NONE;//GPU�^�C���A�E�g�Ȃ�
	cmdQueueDesc.NodeMask = 0;//GPU�͈��
	cmdQueueDesc.Priority = 
		D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;//�D��x�Ȃ�
	cmdQueueDesc.Type =
		D3D12_COMMAND_LIST_TYPE_DIRECT;
	result = dev_->CreateCommandQueue(
		&cmdQueueDesc,
		IID_PPV_ARGS(cmdQueue_.ReleaseAndGetAddressOf())
	);
	return result;
}

HRESULT
Dx12Wrapper::CreateSwapChain(HWND hwnd) {
	RECT rc = {};
	GetWindowRect(hwnd, &rc);

	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	SIZE winSize = Game::Instance()->GetWindowSize();
	swapchainDesc.Width = winSize.cx;
	swapchainDesc.Height = winSize.cy;
	swapchainDesc.Format =
		DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage =
		DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;
	//�X�P�[�����O�\
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
	//�t���b�v��͂����ɔj��
	swapchainDesc.SwapEffect =
		DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//Alpha�͎w��Ȃ�
	swapchainDesc.AlphaMode =
		DXGI_ALPHA_MODE_UNSPECIFIED;
	//�E�B���h�E�A�t���X�N���[���̐؂�ւ��\
	swapchainDesc.Flags =
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	auto result = dxgiFactory_->CreateSwapChainForHwnd(
		cmdQueue_.Get(),
		hwnd,
		&swapchainDesc,
		nullptr,
		nullptr,
		(IDXGISwapChain1**)swapchain_.ReleaseAndGetAddressOf()
	);
	return result;
}

HRESULT
Dx12Wrapper::CreateFinalRendererTarget() {
	//Desc Heap for RTV
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;//RTV
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDesc.NodeMask = 0;//one GPU
	heapDesc.NumDescriptors = 2;//�X���b�v�`�F�[���̕\���̓��

	auto result = dev_->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(&rtvHeap_)
	);
	if (FAILED(result)) return result;

	//swapchain��heap��R�Â���
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = swapchain_->GetDesc(&swcDesc);
	backBuffers_.resize(swcDesc.BufferCount);//�o�b�t�@�[�̐����킹

	D3D12_CPU_DESCRIPTOR_HANDLE handle = 
		rtvHeap_->GetCPUDescriptorHandleForHeapStart();

	//srgbRTV�ݒ�
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format =
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension =
		D3D12_RTV_DIMENSION_TEXTURE2D;
	for (unsigned int i = 0; i < swcDesc.BufferCount; ++i) {
		result = swapchain_->
			GetBuffer(i, IID_PPV_ARGS(&backBuffers_[i]));
		if (FAILED(result)) return result;
		rtvDesc.Format = 
			backBuffers_[i]->GetDesc().Format;
		dev_->CreateRenderTargetView(
			backBuffers_[i].Get(),
			&rtvDesc,
			handle
		);
		handle.ptr += dev_->GetDescriptorHandleIncrementSize(//����i�߂�
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);
	}

	//��ʐݒ�
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	result = swapchain_->GetDesc1(&desc);
	viewport_.reset(
		new CD3DX12_VIEWPORT(backBuffers_[0].Get())
	);
	rect_.reset(
		new CD3DX12_RECT(0, 0, desc.Width, desc.Height)
	);
	return result;
}

HRESULT
Dx12Wrapper::CreateSceneBufferView() {
	auto heapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc =
		CD3DX12_RESOURCE_DESC::Buffer(
			AllignToMultipleOf256(sizeof(SceneData))//�����𑁂����邽�߂�256�̔{���ɍ��킹��

		);
	//CreateConstBuffer
	auto result = dev_->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(sceneConstBuffer_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) return result;

	result = sceneConstBuffer_->Map(
		0, nullptr,
		(void**)&mappedSceneData_
	);//�o�b�t�@�[�Ƀ}�b�v����

	//scene cbv�p��heap�쐻
	D3D12_DESCRIPTOR_HEAP_DESC descHeapdesc = {};
	descHeapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//�V�F�[�_�[�Ŏg��
	descHeapdesc.NodeMask = 0;//gpu��1��
	descHeapdesc.NumDescriptors = 1;//scene cbv
	descHeapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//cbv
	result = dev_->CreateDescriptorHeap(
		&descHeapdesc,
		IID_PPV_ARGS(sceneCbvHeap_.ReleaseAndGetAddressOf())
	);

	//scene cbv�쐻
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = sceneConstBuffer_->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = sceneConstBuffer_->GetDesc().Width;//height��1�Ȃ̂�width���T�C�Y
	dev_->CreateConstantBufferView(
		&cbvDesc,
		sceneCbvHeap_->GetCPUDescriptorHandleForHeapStart()
	);

	return result;
}

HRESULT 
Dx12Wrapper::Create2DSceneBufferView() {
	auto heapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc =
		CD3DX12_RESOURCE_DESC::Buffer(
			AllignToMultipleOf256(sizeof(SceneData2D))//�����𑁂����邽�߂�256�̔{���ɍ��킹��

		);
	//CreateConstBuffer
	auto result = dev_->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(scene2DConstBuffer_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) return result;

	result = scene2DConstBuffer_->Map(
		0, nullptr,
		(void**)&mappedSceneData2D_
	);//�o�b�t�@�[�Ƀ}�b�v����

	//scene cbv�p��heap�쐻
	D3D12_DESCRIPTOR_HEAP_DESC descHeapdesc = {};
	descHeapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//�V�F�[�_�[�Ŏg��
	descHeapdesc.NodeMask = 0;//gpu��1��
	descHeapdesc.NumDescriptors = 1;//scene cbv
	descHeapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//cbv
	result = dev_->CreateDescriptorHeap(
		&descHeapdesc,
		IID_PPV_ARGS(scene2DCbvHeap_.ReleaseAndGetAddressOf())
	);

	//scene cbv�쐻
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = scene2DConstBuffer_->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = scene2DConstBuffer_->GetDesc().Width;//height��1�Ȃ̂�width���T�C�Y
	dev_->CreateConstantBufferView(
		&cbvDesc,
		scene2DCbvHeap_->GetCPUDescriptorHandleForHeapStart()
	);

	return result;
}

HRESULT
Dx12Wrapper::CreateBoardConstBufferView() {
	auto heapProp =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resDesc =
		CD3DX12_RESOURCE_DESC::Buffer(
			AllignToMultipleOf256(sizeof(SceneData2D))//�����𑁂����邽�߂�256�̔{���ɍ��킹��

		);
	//CreateConstBuffer
	auto result = dev_->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(boardConstBuff_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) return result;

	result = boardConstBuff_->Map(
		0, nullptr,
		(void**)&mappedBoardData_
	);//�o�b�t�@�[�Ƀ}�b�v����

	//scene cbv�p��heap�쐻
	D3D12_DESCRIPTOR_HEAP_DESC descHeapdesc = {};
	descHeapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;//�V�F�[�_�[�Ŏg��
	descHeapdesc.NodeMask = 0;//gpu��1��
	descHeapdesc.NumDescriptors = 1;//board cbv
	descHeapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;//cbv
	result = dev_->CreateDescriptorHeap(
		&descHeapdesc,
		IID_PPV_ARGS(boardCbvHeap_.ReleaseAndGetAddressOf())
	);

	//scene cbv�쐻
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = boardConstBuff_->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = boardConstBuff_->GetDesc().Width;//height��1�Ȃ̂�width���T�C�Y
	dev_->CreateConstantBufferView(
		&cbvDesc,
		boardCbvHeap_->GetCPUDescriptorHandleForHeapStart()
	);

	return result;
}

HRESULT
Dx12Wrapper::CreateBoardResources() {
	auto resDesc = backBuffers_[0]->GetDesc();//�G�𒣂�t���邾���Ȃ̂�backbuffers�̐ݒ���ė��p
	auto heapprop =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	float clvCol[4] = { bgCol_.x, bgCol_.y, bgCol_.z, 1.0f };//�N���A�F
	auto clearVal =
		CD3DX12_CLEAR_VALUE(
			DXGI_FORMAT_R8G8B8A8_UNORM,
			clvCol
		);
	for (auto& board : boardResources_) {
		auto result = dev_->CreateCommittedResource(
			&heapprop,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,//�ŏ���sr
			&clearVal,
			IID_PPV_ARGS(board.ReleaseAndGetAddressOf())
		);
		if (FAILED(result)) return result;
	}

	//zbuff
	for (auto& zbuff : zbuffResources_) {
		auto result = dev_->CreateCommittedResource(
			&heapprop,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,//�ŏ���sr
			&clearVal,
			IID_PPV_ARGS(zbuff.ReleaseAndGetAddressOf())
		);
		if (FAILED(result)) return result;
	}

	return S_OK;
}

HRESULT
Dx12Wrapper::CreateBoardRTVs() {
	//create rtv heap
	auto descHeapdesc = rtvHeap_->GetDesc();//backbuffer��rtv�ƂقƂ�Ǔ���
	descHeapdesc.NumDescriptors = boardResources_.size();
	auto result = dev_->CreateDescriptorHeap(
		&descHeapdesc,
		IID_PPV_ARGS(boardRtvHeap_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) return result;

	//create rtv
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	auto handle = boardRtvHeap_->GetCPUDescriptorHandleForHeapStart();
	for (auto& board : boardResources_) {
		dev_->CreateRenderTargetView(
			board.Get(),
			&rtvDesc,
			handle
		);
		handle.ptr += dev_->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);//��i�߂�
	}

	//zbuff
	descHeapdesc.NumDescriptors = zbuffResources_.size();
	result = dev_->CreateDescriptorHeap(
		&descHeapdesc,
		IID_PPV_ARGS(zbuffRtvHeap_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) return result;

	//create rtv
	handle = zbuffRtvHeap_->GetCPUDescriptorHandleForHeapStart();
	for (auto& zbuff : zbuffResources_) {
		dev_->CreateRenderTargetView(
			zbuff.Get(),
			&rtvDesc,
			handle
		);
		handle.ptr += dev_->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);//��i�߂�
	}
	return result;
}

HRESULT
Dx12Wrapper::CreateBoardSRVs() {
	//create heap desc
	auto heapDesc = rtvHeap_->GetDesc();//�ݒ���ė��p
	heapDesc.NumDescriptors = boardResources_.size();
	heapDesc.Type =
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags =
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	auto result = dev_->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(boardSrvHeap_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) return result;

	//create srv
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	auto handle = 
		boardSrvHeap_->GetCPUDescriptorHandleForHeapStart();
	for (auto& board : boardResources_) {
		dev_->CreateShaderResourceView(
			board.Get(),
			&srvDesc,
			handle
		);
		handle.ptr +=
			dev_->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
	}

	//zbuff
	heapDesc.NumDescriptors = zbuffResources_.size();
	result = dev_->CreateDescriptorHeap(
		&heapDesc,
		IID_PPV_ARGS(zbuffSrvHeap_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) return result;

	//create srv
	handle = zbuffSrvHeap_->GetCPUDescriptorHandleForHeapStart();
	for (auto& zbuff : zbuffResources_) {
		dev_->CreateShaderResourceView(
			zbuff.Get(),
			&srvDesc,
			handle
		);
		handle.ptr +=
			dev_->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
	}

	return result;
}

HRESULT
Dx12Wrapper::CreateBoardVertex() {
	VertexData2D vd[4] = {
		{{-1, -1, 0.1f}, {0, 1}},
		{{-1, 1, 0.1f}, {0, 0}},
		{{1, -1, 0.1f}, {1, 1}},
		{{1, 1, 0.1f}, {1, 0}},
	};

	auto heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resdesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(vd));
	auto result = dev_->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(boardVertBuff_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	VertexData2D* mappedVD = nullptr;
	boardVertBuff_->Map(0, nullptr, (void**)&mappedVD);
	std::copy(std::begin(vd), std::end(vd), mappedVD);
	boardVertBuff_->Unmap(0, nullptr);

	boardVBView_.BufferLocation = boardVertBuff_->GetGPUVirtualAddress();
	boardVBView_.SizeInBytes = sizeof(vd);
	boardVBView_.StrideInBytes = sizeof(VertexData2D);

	return result;
}

HRESULT
Dx12Wrapper::CreateBoardRootSignature() {
	//range
	CD3DX12_DESCRIPTOR_RANGE range[2] = {};
	range[0].Init(//�{�[�h�e�N�X�`��
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,//t
		1,
		0//0~
	);
	range[1].Init(//�{�[�h�֘A�̒萔
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,//c
		1,
		0//0~
	);

	//root parameter
	CD3DX12_ROOT_PARAMETER rp[2] = {};
	rp[0].InitAsDescriptorTable(//t
		1,
		&range[0],
		D3D12_SHADER_VISIBILITY_PIXEL
	);
	rp[1].InitAsDescriptorTable(//c
		1,
		&range[1],
		D3D12_SHADER_VISIBILITY_ALL
	);

	//sampler
	auto sampler = CD3DX12_STATIC_SAMPLER_DESC(0);

	//rsdesc
	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = 2;
	rsDesc.NumStaticSamplers = 1;
	rsDesc.pParameters = rp;
	rsDesc.pStaticSamplers = &sampler;

	ComPtr<ID3DBlob> rsblob;
	ComPtr<ID3DBlob> errblob;

	auto result = D3D12SerializeRootSignature(
		&rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		rsblob.ReleaseAndGetAddressOf(),
		errblob.ReleaseAndGetAddressOf()
	);
	if (!CheckResult(result, errblob.Get())) {
		return result;
	}

	result = dev_->CreateRootSignature(
		0,
		rsblob->GetBufferPointer(),
		rsblob->GetBufferSize(),
		IID_PPV_ARGS(boardRS_.ReleaseAndGetAddressOf())
	);

	//zbuff
	CD3DX12_DESCRIPTOR_RANGE ran[5];
	ran[0].Init(//color,
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,//t
		1,
		0//0~
	);
	ran[1].Init(// normal
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,//t
		1,
		1//1~
	);
	ran[2].Init(//depth
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,//t
		1,
		2//2~
	);
	ran[3].Init(//�V�[���֘A�̒萔
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,//c
		1,
		0//0~
	);
	ran[4].Init(//�V�[���֘A�̒萔
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,//c
		1,
		1//1~
	);

	//root parameter
	CD3DX12_ROOT_PARAMETER root[5];
	root[0].InitAsDescriptorTable(//t
		1,
		&ran[0],
		D3D12_SHADER_VISIBILITY_PIXEL
	);
	root[1].InitAsDescriptorTable(//t
		1,
		&ran[1],
		D3D12_SHADER_VISIBILITY_PIXEL
	);
	root[2].InitAsDescriptorTable(//t
		1,
		&ran[2],
		D3D12_SHADER_VISIBILITY_PIXEL
	);
	root[3].InitAsDescriptorTable(//c
		1,
		&ran[3],
		D3D12_SHADER_VISIBILITY_ALL
	);
	root[4].InitAsDescriptorTable(//c
		1,
		&ran[4],
		D3D12_SHADER_VISIBILITY_ALL
	);

	rsDesc.NumParameters = 5;
	rsDesc.pParameters = root;

	result = D3D12SerializeRootSignature(
		&rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		rsblob.ReleaseAndGetAddressOf(),
		errblob.ReleaseAndGetAddressOf()
	);
	if (!CheckResult(result, errblob.Get())) {
		return result;
	}

	result = dev_->CreateRootSignature(
		0,
		rsblob->GetBufferPointer(),
		rsblob->GetBufferSize(),
		IID_PPV_ARGS(deferredRS_.ReleaseAndGetAddressOf())
	);

	return result;
}



HRESULT
Dx12Wrapper::CreateBoardGraphicsPipeline() {
	D3D12_INPUT_ELEMENT_DESC layout[2] = {
	{"POSITION",//Semantic name
	0,//Semantic index
	DXGI_FORMAT_R32G32B32_FLOAT,//Format
	0,//input slot
	D3D12_APPEND_ALIGNED_ELEMENT,//Alligned byte offset
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,//input slot class
	0//instance data step rate
	},
	{"TEXCOORD",
	0,
	DXGI_FORMAT_R32G32_FLOAT,
	0,
	D3D12_APPEND_ALIGNED_ELEMENT,
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
	0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
	gpsDesc.InputLayout.NumElements = _countof(layout);
	gpsDesc.InputLayout.pInputElementDescs = layout;

	ComPtr<ID3DBlob> vsblob;
	ComPtr<ID3DBlob> psblob;
	ComPtr<ID3DBlob> errblob;

	vsblob = GetVertexShaderResourceByPath(
		"BoardVertexShader.hlsl",
		"vs"
	);

	psblob = GetPixelShaderResourceByPath(
		"BoardPixelShader.hlsl",
		"ps0"
	);


	gpsDesc.VS =
		CD3DX12_SHADER_BYTECODE(vsblob.Get());
	gpsDesc.PS =
		CD3DX12_SHADER_BYTECODE(psblob.Get());
	gpsDesc.BlendState =
		CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	gpsDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsDesc.NumRenderTargets = 1;
	gpsDesc.RTVFormats[0] =
		DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsDesc.RasterizerState =
		CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	gpsDesc.SampleMask =
		D3D12_DEFAULT_SAMPLE_MASK;
	gpsDesc.SampleDesc.Count = 1;
	gpsDesc.SampleDesc.Quality = 0;
	gpsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	gpsDesc.pRootSignature = boardRS_.Get();
	//board1�p�C�v���C��
	boardPipelines_.resize(2);
	auto result = dev_->CreateGraphicsPipelineState(
		&gpsDesc, IID_PPV_ARGS(boardPipelines_[0].ReleaseAndGetAddressOf()));

	//board2�p�C�v���C��
	psblob = GetPixelShaderResourceByPath(
		"BoardPixelShader.hlsl",
		"ps1"
	);


	gpsDesc.PS =
		CD3DX12_SHADER_BYTECODE(psblob.Get());

	boardPipelines_.resize(2);
	result = dev_->CreateGraphicsPipelineState(
		&gpsDesc, IID_PPV_ARGS(boardPipelines_[1].ReleaseAndGetAddressOf()));

	if (!CheckResult(result, errblob.Get())) return result;

	//deferred
	psblob = GetPixelShaderResourceByPath(
		"ZBuffPixelShader.hlsl",
		"ZBuffPS"
	);


	vsblob = GetVertexShaderResourceByPath(
		"ZBuffVertexShader.hlsl",
		"ZBuffVS"
	);



	gpsDesc.pRootSignature = deferredRS_.Get();
	gpsDesc.PS =
		CD3DX12_SHADER_BYTECODE(psblob.Get());
	gpsDesc.VS =
		CD3DX12_SHADER_BYTECODE(vsblob.Get());

	auto bldesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	bldesc.AlphaToCoverageEnable = true;
	bldesc.IndependentBlendEnable = false;
	bldesc.RenderTarget[0].BlendEnable = true;
	bldesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;//���Z
	bldesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;//���̂܂�
	bldesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;//a

	gpsDesc.BlendState = bldesc;

	result = dev_->CreateGraphicsPipelineState(
		&gpsDesc, IID_PPV_ARGS(deferredPipeline_.ReleaseAndGetAddressOf()));

	if (!CheckResult(result, errblob.Get())) return result;

	return result;
}

bool
Dx12Wrapper::InitImgui() {

	//imgui�p��descheap�����
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Flags =
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = 0;
	desc.NumDescriptors = 1;//imgui
	desc.Type =
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = dev_->CreateDescriptorHeap(
		&desc,
		IID_PPV_ARGS(heapForImgui_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		return false;
	}

	//imgui������
	if (ImGui::CreateContext() == nullptr) {
		assert(0);
		return false;
	}
	//win�p�̏�����
	bool res = ImGui_ImplWin32_Init(Game::Instance()->GetHWND());
	if (!res) {
		assert(0);
		return false;
	}
	//dx12�p�̏�����
	auto dx12 = Dx12Wrapper::Instance();
	res = ImGui_ImplDX12_Init(
		dx12->Dev(),//dx12�f�o�C�X
		3,//frames_in_flight(���_�A�C���f�b�N�X�o�b�t�@�[�̐��H�T���v����3)
		DXGI_FORMAT_R8G8B8A8_UNORM,//�������ݐ�̃t�H�[�}�b�g
		heapForImgui_.Get(),//imgui�p��descheap
		heapForImgui_->GetCPUDescriptorHandleForHeapStart(),
		heapForImgui_->GetGPUDescriptorHandleForHeapStart()
	);

	return res;
}

HRESULT
Dx12Wrapper::CreateDepthStencilView() {
	//create depth buffer
	DXGI_SWAP_CHAIN_DESC1 desc = {};
	auto result = swapchain_->GetDesc1(&desc);

	auto resdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32_TYPELESS,
		desc.Width, desc.Height,
		1,//array size
		1,//mip levels
		1,//sample count
		0,//sample quality
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	auto heapprop = CD3DX12_HEAP_PROPERTIES(
		D3D12_HEAP_TYPE_DEFAULT
	);
	CD3DX12_CLEAR_VALUE depthClearValue(DXGI_FORMAT_D32_FLOAT, 1.0f, 0);

	result = dev_->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(depthBuffer_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) return result;

	//create LightdepthBuffer
	resdesc.Width = shadowMapSize;
	resdesc.Height = shadowMapSize;

	result = dev_->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(lightdepthBuffer_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) return result;

	//create dsv heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeapdesc = {};
	descHeapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descHeapdesc.NodeMask = 0;
	descHeapdesc.NumDescriptors = 2;
	descHeapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

	result = dev_->CreateDescriptorHeap(
		&descHeapdesc,
		IID_PPV_ARGS(dsvHeap_.ReleaseAndGetAddressOf())
	);

	//create dsv
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	auto handle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
	dev_->CreateDepthStencilView(
		depthBuffer_.Get(),
		&dsvDesc,
		handle
	);

	handle.ptr += dev_->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	);

	dev_->CreateDepthStencilView(
		lightdepthBuffer_.Get(),
		&dsvDesc,
		handle
	);

	//TODO:�V���h�E�}�b�v�̎��͂�����srv��ǉ�����
		//create depth srv heap
	descHeapdesc = {};
	descHeapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapdesc.NodeMask = 0;
	descHeapdesc.NumDescriptors = 2;
	descHeapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = dev_->CreateDescriptorHeap(
		&descHeapdesc,
		IID_PPV_ARGS(depthSrvHeap_.ReleaseAndGetAddressOf())
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	handle = depthSrvHeap_->GetCPUDescriptorHandleForHeapStart();
	dev_->CreateShaderResourceView(
		depthBuffer_.Get(),
		&srvDesc, handle
	);
	handle.ptr += dev_->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	dev_->CreateShaderResourceView(
		lightdepthBuffer_.Get(),
		&srvDesc,
		handle
	);

	return result;
}


Microsoft::WRL::ComPtr<
	ID3D12Resource>
Dx12Wrapper::GetImageResourceByPath(const char* path) {
	auto it = imageTable_.find(path);
	if (it != imageTable_.end()) {
		//�e�[�u���ɓ��ɂ������烍�[�h����̂ł͂Ȃ��}�b�v���̃��\�[�X��Ԃ�
		return imageTable_[path];
	}
	else {
		return CreateImageResourceFromFile(path);
	}
}

ComPtr<ID3DBlob>
Dx12Wrapper::GetVertexShaderResourceByPath(const char* path, const char* entryPoint) {
//�V�F�[�_�[��ǂݍ���
	ComPtr<ID3DBlob> vsblob = nullptr;
	ComPtr<ID3DBlob> errblob = nullptr;
	auto wpath = GetWideStringFromString(path);
	auto result = D3DCompileFromFile(
		wpath.c_str(),
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint, "vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &vsblob, &errblob
	);
	if (!CheckResult(result, errblob.Get())) {
		assert(0);
		return nullptr;
	}
	return vsblob;
	
}
ComPtr<ID3DBlob>
Dx12Wrapper::GetPixelShaderResourceByPath(const char* path, const char* entryPoint) {
	auto pathAndEntry = std::string(path) + entryPoint;
//�V�F�[�_�[��ǂݍ���
	ComPtr<ID3DBlob> psblob = nullptr;
	ComPtr<ID3DBlob> errblob = nullptr;
	auto wpath = GetWideStringFromString(path);
	auto result = D3DCompileFromFile(
		wpath.c_str(),
		nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint, "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0, &psblob, &errblob
	);
	if (!CheckResult(result, errblob.Get())) {
		assert(0);
		return nullptr;
	}
	return psblob;

}


ComPtr<ID3D12Resource>
Dx12Wrapper::CreateImageResourceFromFile(const char* path) {
	std::string texPath = path;

	//�摜�̓ǂݍ���
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	//�ǂݍ��݊֐��̓s����wstring�ɕϊ�����
	auto wtexpath = GetWideStringFromString(texPath);
	//�g���q
	auto ext = GetExtension(texPath);
	//�摜�ǂݍ���
	auto result = loadImageLambdaTable_[ext](
		wtexpath,
		&metadata,
		scratchImg
		);
	if (FAILED(result)) {
		//�����ȃt�@�C���p�X�����Ă�ꍇ������̂�
		//�~�߂Ȃ�
		return nullptr;
	}
	//���f�[�^���o
	auto img = scratchImg.GetImage(0, 0, 0);

	//write to subresource�œ]��
	auto heapprop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0
	);
	auto resdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		metadata.format,
		metadata.width, metadata.height,
		metadata.arraySize,
		metadata.mipLevels
	);

	//�o�b�t�@�쐻
	ComPtr<ID3D12Resource> imgbuff = nullptr;
	result = dev_->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,//texture
		nullptr,
		IID_PPV_ARGS(imgbuff.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return nullptr;
	}
	//�摜�f�[�^�]��
	result = imgbuff->WriteToSubresource(
		0,
		nullptr,//�S�̈�
		img->pixels,//���f�[�^�A�h���X
		img->rowPitch,//1���C���̃T�C�Y
		img->slicePitch//�S�T�C�Y
	);
	if (FAILED(result)) {
		assert(0);
		return nullptr;
	}

	return imgbuff;
}

void
Dx12Wrapper::CreateTextureLoadTable() {
	loadImageLambdaTable_["sph"] = 
		loadImageLambdaTable_["spa"] = 
		loadImageLambdaTable_["bmp"] = 
		loadImageLambdaTable_["png"] = 
		loadImageLambdaTable_["jpg"] = 
		[](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT {
		return LoadFromWICFile(path.c_str(), WIC_FLAGS_NONE, meta, img);
	};

	loadImageLambdaTable_["tga"] = 
		[](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT {
		return LoadFromTGAFile(path.c_str(), meta, img);
	};

	loadImageLambdaTable_["dds"] = 
		[](const std::wstring& path, TexMetadata* meta, ScratchImage& img)->HRESULT {
		return LoadFromDDSFile(path.c_str(), DDS_FLAGS_NONE, meta, img);
	};
}

//�`��p�֐�

void
Dx12Wrapper::BeginDrawToFinalRenderTarget() {
	//���݌��ɗ��Ă�o�b�t�@�̃C���f�b�N�X���擾
	auto bbidx = swapchain_->GetCurrentBackBufferIndex();
	//RenderTarget�ɕς���
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffers_[bbidx].Get(),
		D3D12_RESOURCE_STATE_PRESENT,//�O�̏��
		D3D12_RESOURCE_STATE_RENDER_TARGET//��̏��
	);
	cmdList_->ResourceBarrier(1, &barrier);

	//rtv�̐ݒ�
	auto rtvH = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbidx * dev_->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	);
	//�[�x�o�b�t�@�̎w��
	auto dsvH = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

	cmdList_->OMSetRenderTargets(1, &rtvH, false, &dsvH);

	//��ʃN���A
	float clCol[] = { bgCol_.x, bgCol_.y, bgCol_.z, 1.0f };
	cmdList_->ClearRenderTargetView(
		rtvH, clCol, 0, nullptr
	);

	//�r���[�|�[�g�A�����[�ꂭ�Ƃ̐ݒ�
	cmdList_->RSSetViewports(1, viewport_.get());
	cmdList_->RSSetScissorRects(1, rect_.get());
}

void
Dx12Wrapper::Flip() {
	auto bbidx = swapchain_->GetCurrentBackBufferIndex();
	//�\���p�ɏ�Ԃ�߂�
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffers_[bbidx].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	cmdList_->ResourceBarrier(1, &barrier);

	//���ߏI��
	cmdList_->Close();

	//���ߎ��s
	ID3D12CommandList* cmdlists[] = { cmdList_.Get() };
	cmdQueue_->ExecuteCommandLists(1, cmdlists);

	//�������I���܂őҋ@
	cmdQueue_->Signal(fence_.Get(), ++fenceVal_);
	if (fence_->GetCompletedValue() < fenceVal_) {
		auto event = CreateEvent(nullptr, false, false, nullptr);
		fence_->SetEventOnCompletion(fenceVal_, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}

	cmdAllocator_->Reset();
	cmdList_->Reset(cmdAllocator_.Get(), nullptr);

	auto result = swapchain_->Present(0, 0);
	if (FAILED(result)) {
		assert(0);
	}
}

void
Dx12Wrapper::BeginDrawToPath(Dx12Wrapper::DrawToPathSetting& dp) {
	//�����_�[�^�[�Q�b�g�ɂ���
	for (auto& rt : dp.renderTargets) {
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			rt,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,//�O�̏��
			D3D12_RESOURCE_STATE_RENDER_TARGET//��̏��
		);
		cmdList_->ResourceBarrier(1, &barrier);
	}

	//rtv�̐ݒ�
	//�[�x�o�b�t�@�̎w��
	if (dp.dsvDescHeap == nullptr) {
		cmdList_->OMSetRenderTargets(
			dp.rtvHandles.size(), &(dp.rtvHandles[0]), false, nullptr);
	}
	else {
		auto dsvH = dp.dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
		cmdList_->ClearDepthStencilView(dsvH, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		cmdList_->OMSetRenderTargets(
			dp.rtvHandles.size(), &(dp.rtvHandles[0]), false, &dsvH);
	}
	

	//��ʃN���A
	float clCol[] = { bgCol_.x, bgCol_.y, bgCol_.z, 1.0f };
	for (auto& rtvH : dp.rtvHandles) {
		cmdList_->ClearRenderTargetView(
			rtvH, clCol, 0, nullptr
		);
	}

	//�r���[�|�[�g�A�����[�ꂭ�Ƃ̐ݒ�
	cmdList_->RSSetViewports(1, viewport_.get());
	cmdList_->RSSetScissorRects(1, rect_.get());
}

void
Dx12Wrapper::EndDrawToPath(DrawToPathSetting& dp) {
	//�s�N�Z���V�F�[�_�[�ɖ߂�
	for (auto& rt : dp.renderTargets) {
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			rt,
			D3D12_RESOURCE_STATE_RENDER_TARGET,//��̏��
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE//�O�̏��
		);
		cmdList_->ResourceBarrier(1, &barrier);
	}
}


void
Dx12Wrapper::BeginDrawToBoards(std::list<unsigned int>  boardIdx, bool useDepth) {
	DrawToPathSetting dpSetting;
	dpSetting.dsvDescHeap = nullptr;
	if (useDepth) {
		dpSetting.dsvDescHeap = dsvHeap_.Get();
	}
	for (auto idx :boardIdx) {
		dpSetting.renderTargets.emplace_back(boardResources_[idx].Get());
		auto incSize = dev_->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv;
		rtv.InitOffsetted(
			boardRtvHeap_->GetCPUDescriptorHandleForHeapStart(),
			incSize * idx);
		dpSetting.rtvHandles.emplace_back(rtv);
	}
	BeginDrawToPath(dpSetting);
}

void
Dx12Wrapper::DrawBoard(unsigned int idx, unsigned int pIdx) {
	cmdList_->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
	);
	cmdList_->SetGraphicsRootSignature(boardRS_.Get());
	cmdList_->SetPipelineState(boardPipelines_[pIdx].Get());

	cmdList_->SetDescriptorHeaps(1, boardSrvHeap_.GetAddressOf());
	auto handle = boardSrvHeap_->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += dev_->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	) * idx;
	cmdList_->SetGraphicsRootDescriptorTable(0, handle);

	//cbv�̐ݒ�
	SetBoardData();

	cmdList_->IASetVertexBuffers(0, 1, &boardVBView_);
	cmdList_->DrawInstanced(4, 1, 0, 0);
}


void
Dx12Wrapper::EndDrawToBoards(std::list<unsigned int>  boardIdx) {
	DrawToPathSetting dpSetting;
	dpSetting.dsvDescHeap = nullptr;

	for (auto idx :boardIdx) {
		dpSetting.renderTargets.emplace_back(boardResources_[idx].Get());
		auto incSize = dev_->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv;
		rtv.InitOffsetted(
			boardRtvHeap_->GetCPUDescriptorHandleForHeapStart(),
			incSize * idx);
		dpSetting.rtvHandles.emplace_back(rtv);
	}
	EndDrawToPath(dpSetting);
}

void Dx12Wrapper::BeginDrawToDepth() {
	auto handle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += dev_->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	);//2�߂̃f�v�X
	cmdList_->OMSetRenderTargets(0, nullptr, false, &handle);
	cmdList_->ClearDepthStencilView(handle,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	D3D12_VIEWPORT vp = CD3DX12_VIEWPORT(0.0f, 0.0f, shadowMapSize, shadowMapSize);
	cmdList_->RSSetViewports(1, &vp);//�r���[�|�[�g

	CD3DX12_RECT rc(0, 0, shadowMapSize, shadowMapSize);
	cmdList_->RSSetScissorRects(1, &rc);//�V�U�[(�؂蔲��)��`
}

void
Dx12Wrapper::BeginDrawToBoardsAndZBuff(//zbuff�ɂ��o�͂���
	std::list<unsigned int> boardIdx, bool useDepth) {
	DrawToPathSetting dpSetting;
	dpSetting.dsvDescHeap = nullptr;
	if (useDepth) {
		dpSetting.dsvDescHeap = dsvHeap_.Get();
	}
	for (auto idx : boardIdx) {
		dpSetting.renderTargets.emplace_back(boardResources_[idx].Get());
		auto incSize = dev_->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv;
		rtv.InitOffsetted(
			boardRtvHeap_->GetCPUDescriptorHandleForHeapStart(),
			incSize * idx);
		dpSetting.rtvHandles.emplace_back(rtv);
	}

	int idx = 0;
	for (auto& zbuf : zbuffResources_) {
		dpSetting.renderTargets.emplace_back(zbuf.Get());
		auto incSize = dev_->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv;
		rtv.InitOffsetted(
			zbuffRtvHeap_->GetCPUDescriptorHandleForHeapStart(),
			incSize * idx);
		dpSetting.rtvHandles.emplace_back(rtv);
		idx++;
	}
	
	BeginDrawToPath(dpSetting);
}

void 
Dx12Wrapper::EndDrawToBoardsAndZBuff(std::list<unsigned int> boardIdx) {
	DrawToPathSetting dpSetting;
	dpSetting.dsvDescHeap = nullptr;
	for (auto idx : boardIdx) {
		dpSetting.renderTargets.emplace_back(boardResources_[idx].Get());
		auto incSize = dev_->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv;
		rtv.InitOffsetted(
			boardRtvHeap_->GetCPUDescriptorHandleForHeapStart(),
			incSize * idx);
		dpSetting.rtvHandles.emplace_back(rtv);
	}

	int idx = 0;
	for (auto& zbuf : zbuffResources_) {
		dpSetting.renderTargets.emplace_back(zbuf.Get());
		auto incSize = dev_->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv;
		rtv.InitOffsetted(
			zbuffRtvHeap_->GetCPUDescriptorHandleForHeapStart(),
			incSize * idx);
		dpSetting.rtvHandles.emplace_back(rtv);
		idx++;
	}

	EndDrawToPath(dpSetting);
}

void
Dx12Wrapper::DrawImgui() {

	ImGui::Render();

	auto dx12 = Dx12Wrapper::Instance();
	dx12->CommandList()->SetDescriptorHeaps(
		1, heapForImgui_.GetAddressOf()
	);
	//�`��
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dx12->CommandList());


}

void
Dx12Wrapper::MakeImguiWindow() {


	//�E�B���h�E��`
	ImGui::Begin("Dx12Wrapper");//�E�B���h�E��
	ImGui::SetWindowSize(
		ImVec2(400, 500),//�E�B���h�E�T�C�Y
		ImGuiCond_::ImGuiCond_FirstUseEver
	);
	//imgui�ݒ�
	ImGui::SliderFloat("Contrast", &contrast_, 0.0f, 15.0f);

	ImGui::ColorPicker3("BgCol", &bgCol_.x,
		ImGuiColorEditFlags_::ImGuiColorEditFlags_InputRGB);


	ImGui::End();
}

//�_�����̒ǉ�
void 
Dx12Wrapper::AddPointLight(PointLightComponent* pc) {
	pointLights_.push_back(pc);
}
//�폜
void 
Dx12Wrapper::RemovePointLight(PointLightComponent* pc) {
	auto it = std::find(pointLights_.begin(), pointLights_.end(), pc);
	if (it == pointLights_.end()) {
		assert(0 && "����ȃR���|�[�l���g�͂Ȃ���");
		return;
	}
	pointLights_.erase(it);
}

void
Dx12Wrapper::DrawDeferred(unsigned idx) {
	cmdList_->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
	);

	//���ʂɃJ���[�o�b�t�@��`�悷��
	DrawBoard(idx, 1);

	//���̏�Ƀ��u�����f�B���O���Ă�
	// 
	cmdList_->SetGraphicsRootSignature(deferredRS_.Get());
	cmdList_->SetPipelineState(deferredPipeline_.Get());
	//�J���[�o�b�t�@
	cmdList_->SetDescriptorHeaps(1, boardSrvHeap_.GetAddressOf());
	auto handle = boardSrvHeap_->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += dev_->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	) * idx;
	cmdList_->SetGraphicsRootDescriptorTable(0, handle);

	//z�o�b�t�@
	cmdList_->SetDescriptorHeaps(1, zbuffSrvHeap_.GetAddressOf());
	handle = zbuffSrvHeap_->GetGPUDescriptorHandleForHeapStart();
	cmdList_->SetGraphicsRootDescriptorTable(1, handle);

	cmdList_->SetDescriptorHeaps(1,
		depthSrvHeap_.GetAddressOf());
	handle = depthSrvHeap_->GetGPUDescriptorHandleForHeapStart();
	//1�߂̒ʏ�f�v�X���Z�b�g
	cmdList_->SetGraphicsRootDescriptorTable(2,
		handle);

	//���݂̃V�[�����Z�b�g
	ID3D12DescriptorHeap* sceneheaps[] = {
		sceneCbvHeap_.Get()
	};
	cmdList_->SetDescriptorHeaps(1, sceneheaps);
	cmdList_->SetGraphicsRootDescriptorTable(
		3, sceneCbvHeap_->GetGPUDescriptorHandleForHeapStart());

	cmdList_->IASetVertexBuffers(0, 1, &boardVBView_);

	for (auto& pc : pointLights_) {//���ꂼ��̌����ɂ��Čv�Z
		pc->SetLightBuffer();
		cmdList_->DrawInstanced(4, 1, 0, 0);
	}

}