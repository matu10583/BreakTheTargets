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
	void AddPMDMeshComponent(MeshComponent*);//meshcomponentからしか呼ばない
	void AddVertMeshComponent(MeshComponent*);//meshcomponentからしか呼ばない
	void AddSpriteComponent(class SpriteComponent*);//spritecomponentからしか呼ばない
	void RemoveMeshComponent(MeshComponent*);//meshcomponentからしか呼ばない
	void RemoveSpriteComponent(class SpriteComponent*);//spritecomponentからしか呼ばない
private:
	Renderer();
	~Renderer();
	std::vector<MeshComponent*> pmdMeshComponents;//pmdから読み込んだモデル
	std::vector<MeshComponent*> vertMeshComponents;//頂点配列だけから生成したモデル
	std::vector<SpriteComponent*> spriteComponents;//画像表示コンポーネント

	std::vector<MeshComponent*> pendPmdMeshComponents;
	std::vector<MeshComponent*> pendVertMeshComponents;
	std::vector<SpriteComponent*> pendSpriteComponents;

	Dx12Wrapper* dx12_;

	static Renderer* instance;

	//pipeline初期化
	HRESULT CreateGraphicsPipelineForPMD_Depth();
	HRESULT CreateGraphicsPipelineForVertices();
	HRESULT CreateGraphicsPipelineForSprite();
	//rootsignature初期化
	HRESULT CreateRootSignatureForPMD();
	HRESULT CreateRootSignatureForVertices();
	HRESULT CreateRootSignatureForSprite();

	//描画関数
	void DrawModel(const struct ViewFrustum& vf);
	void DrawVert(const struct ViewFrustum& vf);
	void DrawSprite();
	void DrawDepth();

	//ここにグラフィックスパイプラインやシグネチャーを置く
	ComPtr<ID3D12PipelineState> pipelineForPMD_ = nullptr;
	ComPtr<ID3D12RootSignature> rootSignatureForPMD_ = nullptr;
	ComPtr<ID3D12PipelineState> pipelineForVert_ = nullptr;
	ComPtr<ID3D12RootSignature> rootSignatureForVert_ = nullptr;
	ComPtr<ID3D12PipelineState> pipelineForSprite_ = nullptr;
	ComPtr<ID3D12RootSignature> rootSignatureForSprite_ = nullptr;
	ComPtr<ID3D12PipelineState> pipelineForPMDDepth_ = nullptr;
	ComPtr<ID3D12PipelineState> pipelineForVertDepth_ = nullptr;

};

