#pragma once
#include "XMFLOAT_Helper.h"
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <Windows.h>
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <array>
#include <unordered_map>
#include <DirectXTex.h>

using namespace DirectX;

class ModelLoader;
struct Model;
struct Material;
struct AnimationData;
class Sprite;
class Dx12Wrapper
{

	//ComPtr�ŊǗ�����
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;



public:
	static void Create();
	static void Destroy();
	static Dx12Wrapper* Instance();
	bool Init(HWND);
	ID3D12Device* Dev()const;//device�̎擾
	ID3D12GraphicsCommandList* CommandList()const;//commandList�̎擾
	//3D�v���W�F�N�V�����s���Ԃ�
	const XMMATRIX& Projection3D()const { return mappedSceneData_->proj; }
	//�n�_����Ԃ�
	const Vector3& EyePos()const { return eyePos_; }
	const Vector3& EyeRot()const { return eyeRot_; }

	//�J��������̐ݒ�
	void SetCameraSetting(const Vector3& eyePos, const Vector3& eyeRot, 
		float fov=XM_PIDIV4,
		const Vector3& up = Vector3(0, 1, 0));
	//��������̐ݒ�
	void SetDirectionalLightSetting(
		const Vector3& dir, const Vector3& col
		);

	//�_�����̒ǉ�
	void AddPointLight(class PointLightComponent*);
	//�폜
	void RemovePointLight(PointLightComponent*);

	//�r�l�b�g�̃I���I�t
	void SetVignette(bool v);
	//�V�[���ɉe������萔��ݒ肷��
	void SetSceneData();
	void SetSceneData2D();
	void SetBoardData();

	//backbuffer�ւ̏������ݏ���
	void BeginDrawToFinalRenderTarget();
	// backbuffer�ւ̏������݂̌�n��
	void Flip();

	//�}���`�p�X�����_�����O�p
		//�{�[�h���\�[�X�ɏ�������
	//�{�[�h�̃C���f�b�N�X�Ŏw��
	void BeginDrawToBoardsAndZBuff(std::list<unsigned int> boardIdx, bool useDepth);
	void BeginDrawToBoards(std::list<unsigned int> boardIdx, bool useDepth);
	void DrawBoard(unsigned int idx, unsigned int piplineIdx);
	void EndDrawToBoards(std::list<unsigned int> boardIdx);
	void EndDrawToBoardsAndZBuff(std::list<unsigned int> boardIdx);

	//�f�v�X�݂̂̕`��
	void BeginDrawToDepth();

	//Z-Buffer��p�����f�B�ӂ��[�ǃ����_�����O
	void DrawDeferred(unsigned int Boardidx);

	//�n�b�V���}�b�v�ɂ���΂�����B�Ȃ����loadmodel
	std::shared_ptr<Model> GetModelByPath(const char* path);
	//�n�b�V���}�b�v�ɂ���΂�����B�Ȃ����createsprite
	std::shared_ptr<Sprite> GetSpriteByPath(const char* path);
	//�n�b�V���}�b�v�ɂ���΂�����B�Ȃ����createsprite
	std::shared_ptr<AnimationData> GetAnimeByPath(const char* path);

	//�C���[�W���\�[�X�𓾂�B
//��x�ǂ݂��񂾂��̂̓n�b�V���}�b�v����R�s�[����
	ComPtr<ID3D12Resource>
		GetImageResourceByPath(const char*);

	//���_�z�񂩂烂�f�����擾(����name������΂����Ԃ�)
	std::shared_ptr<Model> GetModelByVertices(const std::vector<VertexData>& vertices,
		const std::vector<Index>& indecies, const std::vector<Material>& materials,
		const char* name);

	//���l�Ƀp�X����V�F�[�_�[�𓾂�
	ComPtr<ID3DBlob>
		GetVertexShaderResourceByPath(const char* path, const char* entryPoint);
	ComPtr<ID3DBlob>
		GetPixelShaderResourceByPath(const char* path, const char* entryPoint);

	//imgui�̕`��
	void MakeImguiWindow();
	void DrawImgui();
private:
	Dx12Wrapper();
	~Dx12Wrapper();
	static Dx12Wrapper* instance;

	struct SceneData
	{
		XMMATRIX invview;//�r���[�̋t�s��
		XMMATRIX view;//�r���[�s��
		XMMATRIX invproj;//�t�v���W�F�N�V�����s��
		XMMATRIX proj;//�v���W�F�N�V�����s��
		XMFLOAT4 lightCol;//���̐F
		XMFLOAT4 lightDir;//���̕���
		XMFLOAT4 eye;//���_���W
		XMMATRIX lightView;//�����猩���r���[�s��
	};
	struct SceneData2D
	{
		XMMATRIX proj;//�v���W�F�N�V�����s��
	};
	struct BoardData
	{
		int isVignette;
		float contrast;
	};

	Vector3 eyePos_;//���_���W
	Vector3 eyeRot_;//�����̌���
	Vector3 lightCol_;//���̐F
	Vector3 lightDir;//���̕���
	XMFLOAT4 planeVec;//���ʂ̕������̌W��
	Vector3 up_;//�J�����̏�����x�N�g��
	float fov_;//��p
	float contrast_;//�R���g���X�g(�V�O���C�h�Ȑ��̌W��)
	Vector3 bgCol_;//�w�i�F

	int fenceVal_;

	ComPtr<IDXGIFactory6> dxgiFactory_;
	ComPtr<ID3D12Device> dev_;
	ComPtr<IDXGISwapChain4> swapchain_;
	ComPtr<ID3D12CommandAllocator> cmdAllocator_;
	ComPtr<ID3D12GraphicsCommandList> cmdList_;
	ComPtr<ID3D12CommandQueue> cmdQueue_;


	ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	std::vector<ComPtr<ID3D12Resource>> backBuffers_;
	ComPtr<ID3D12Resource> sceneConstBuffer_;//SceneData�p
	ComPtr<ID3D12DescriptorHeap> sceneCbvHeap_;
	ComPtr<ID3D12Resource> scene2DConstBuffer_;//SceneData2D�p
	ComPtr<ID3D12DescriptorHeap> scene2DCbvHeap_;
	//�G�𓊉e����{�[�h�B�}���`�p�X�����_�����O�p�ɕ����ɑΉ�����
	std::array<ComPtr<ID3D12Resource>, 3>
		boardResources_;
	ComPtr<ID3D12Resource> boardConstBuff_;
	ComPtr<ID3D12DescriptorHeap> boardCbvHeap_;
	ComPtr<ID3D12Resource> boardVertBuff_;
	D3D12_VERTEX_BUFFER_VIEW boardVBView_;
	ComPtr<ID3D12DescriptorHeap> boardRtvHeap_;
	ComPtr<ID3D12DescriptorHeap> boardSrvHeap_;
	ComPtr<ID3D12RootSignature> boardRS_;
	std::vector<
		ComPtr<ID3D12PipelineState>> boardPipelines_;
	//Z-Buffer
	std::array<
		ComPtr<ID3D12Resource>, 1> zbuffResources_;
	ComPtr<ID3D12DescriptorHeap> zbuffRtvHeap_;
	ComPtr<ID3D12DescriptorHeap> zbuffSrvHeap_;
	ComPtr<ID3D12PipelineState> deferredPipeline_;
	ComPtr<ID3D12RootSignature> deferredRS_;

	ComPtr<ID3D12Resource> depthBuffer_;//depth�p
	ComPtr<ID3D12Resource> lightdepthBuffer_;//�������猩��depth�p
	ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	ComPtr<ID3D12DescriptorHeap> depthSrvHeap_;//depth���摜�Ƃ��Ă݂�悤

	ComPtr<ID3D12DescriptorHeap> heapForImgui_;//imgui�p

	ComPtr<ID3D12Fence> fence_;

	std::unique_ptr<D3D12_VIEWPORT> viewport_;
	std::unique_ptr<D3D12_RECT> rect_;
	SceneData* mappedSceneData_;//�J��������̕ϐ��𑗂邽�߂̃}�b�v�p�ϐ�
	SceneData2D* mappedSceneData2D_;
	BoardData* mappedBoardData_;//�|�X�g�G�t�F�N�g����̃f�[�^�p
	std::unique_ptr<ModelLoader> modelLoader_;//PMD�t�@�C����ǂނ��߂̋@�\

	HRESULT InitDXGIDevice();
	HRESULT InitCommand();
	HRESULT CreateSwapChain(HWND hwnd);
	HRESULT CreateFinalRendererTarget();//�ŏI�I�ȃ����_�[�^�[�Q�b�g�̐���
	HRESULT CreateSceneBufferView();//�V�[���Ŏg���萔�̃o�b�t�@�[�����
	HRESULT Create2DSceneBufferView();//2D�̕`��̍ۂɎg���V�[������̒萔�o�b�t�@�����
	HRESULT CreateBoardConstBufferView();//�V�[���Ŏg���萔�̃o�b�t�@�[�����
	//���e���邽�߂̔𐶐�.�}���`�p�X�����_�����O�p�ɕ����ɑΉ�����
	HRESULT CreateBoardResources();
	HRESULT CreateBoardVertex();
	HRESULT CreateBoardRTVs();
	HRESULT CreateBoardSRVs();
	HRESULT CreateBoardRootSignature();
	HRESULT CreateBoardGraphicsPipeline();
	HRESULT CreateDepthStencilView();	//�[�x�o�b�t�@�쐬

	//Imgui�̏�����
	bool InitImgui();

	struct DrawToPathSetting
	{
		std::vector<ID3D12Resource*> renderTargets;
		std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvHandles;
		ID3D12DescriptorHeap* dsvDescHeap;
	};
	//�r���̃p�X�ɏ������ޏ���
	void BeginDrawToPath(DrawToPathSetting&);
	//��n��
	void EndDrawToPath(DrawToPathSetting&);

	//TODO:pmx���]�T�������
	bool LoadModel(const char* path, Model* model);


	ComPtr<ID3D12Resource>
		CreateImageResourceFromFile(const char*);

	using ImageLoadLambda = std::function<
		HRESULT(const std::wstring& path,
			TexMetadata*,
			ScratchImage&)>;
	//�摜���[�h�֐��e�[�u��
	std::unordered_map<std::string, ImageLoadLambda>
		loadImageLambdaTable_;
	void CreateTextureLoadTable();

	//�C���[�W�e�[�u��
	std::unordered_map<std::string,
		ComPtr<ID3D12Resource>>
		imageTable_;

	//PMD����ǂݍ���Model�̃e�[�u��
	std::unordered_map<std::string,
		std::shared_ptr<Model>>
		modelMap_;

	//VMD����ǂݍ���Model�̃e�[�u��
	std::unordered_map<std::string,
		std::shared_ptr<AnimationData>>
		animeVMDMap_;

	//�X�v���C�g�e�[�u��
	std::unordered_map<std::string,
		std::shared_ptr<Sprite>>
		spriteMap_;

	//�_�����z��
	std::vector<class PointLightComponent*>
		pointLights_;


};

