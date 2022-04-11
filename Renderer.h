#pragma once
#include <vector>
#include <wrl.h>
#include <d3d12.h>

class MeshComponent;
class Dx12Wrapper;

class Renderer
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
	static void Create();
	static void Destroy();
	static Renderer* Instance();



	bool Init();
	void Draw();
	void AddPMDMeshComponent(MeshComponent*);//meshcomponent���炵���Ă΂Ȃ�
	void AddVertMeshComponent(MeshComponent*);//meshcomponent���炵���Ă΂Ȃ�
	void AddSpriteComponent(class SpriteComponent*);//spritecomponent���炵���Ă΂Ȃ�
	void RemoveMeshComponent(MeshComponent*);//meshcomponent���炵���Ă΂Ȃ�
	void RemoveSpriteComponent(class SpriteComponent*);//spritecomponent���炵���Ă΂Ȃ�
private:
	Renderer();
	~Renderer();
	std::vector<MeshComponent*> pmdMeshComponents;//pmd����ǂݍ��񂾃��f��
	std::vector<MeshComponent*> vertMeshComponents;//���_�z�񂾂����琶���������f��
	std::vector<SpriteComponent*> spriteComponents;//�摜�\���R���|�[�l���g

	std::vector<MeshComponent*> pendPmdMeshComponents;
	std::vector<MeshComponent*> pendVertMeshComponents;
	std::vector<SpriteComponent*> pendSpriteComponents;

	Dx12Wrapper* dx12_;

	static Renderer* instance;

	//pipeline������
	HRESULT CreateGraphicsPipelineForPMD_Depth();
	HRESULT CreateGraphicsPipelineForVertices();
	HRESULT CreateGraphicsPipelineForSprite();
	//rootsignature������
	HRESULT CreateRootSignatureForPMD();
	HRESULT CreateRootSignatureForVertices();
	HRESULT CreateRootSignatureForSprite();

	//�`��֐�
	void DrawModel(const struct ViewFrustum& vf);
	void DrawVert(const struct ViewFrustum& vf);
	void DrawSprite();
	void DrawDepth();

	//�����ɃO���t�B�b�N�X�p�C�v���C����V�O�l�`���[��u��
	ComPtr<ID3D12PipelineState> pipelineForPMD_ = nullptr;
	ComPtr<ID3D12RootSignature> rootSignatureForPMD_ = nullptr;
	ComPtr<ID3D12PipelineState> pipelineForVert_ = nullptr;
	ComPtr<ID3D12RootSignature> rootSignatureForVert_ = nullptr;
	ComPtr<ID3D12PipelineState> pipelineForSprite_ = nullptr;
	ComPtr<ID3D12RootSignature> rootSignatureForSprite_ = nullptr;
	ComPtr<ID3D12PipelineState> pipelineForPMDDepth_ = nullptr;
	ComPtr<ID3D12PipelineState> pipelineForVertDepth_ = nullptr;

};

