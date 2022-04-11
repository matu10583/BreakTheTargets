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

	//ComPtrで管理する
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;



public:
	static void Create();
	static void Destroy();
	static Dx12Wrapper* Instance();
	bool Init(HWND);
	ID3D12Device* Dev()const;//deviceの取得
	ID3D12GraphicsCommandList* CommandList()const;//commandListの取得
	//3Dプロジェクション行列を返す
	const XMMATRIX& Projection3D()const { return mappedSceneData_->proj; }
	//始点情報を返す
	const Vector3& EyePos()const { return eyePos_; }
	const Vector3& EyeRot()const { return eyeRot_; }

	//カメラ周りの設定
	void SetCameraSetting(const Vector3& eyePos, const Vector3& eyeRot, 
		float fov=XM_PIDIV4,
		const Vector3& up = Vector3(0, 1, 0));
	//光源周りの設定
	void SetDirectionalLightSetting(
		const Vector3& dir, const Vector3& col
		);

	//点光源の追加
	void AddPointLight(class PointLightComponent*);
	//削除
	void RemovePointLight(PointLightComponent*);

	//ビネットのオンオフ
	void SetVignette(bool v);
	//シーンに影響する定数を設定する
	void SetSceneData();
	void SetSceneData2D();
	void SetBoardData();

	//backbufferへの書き込み準備
	void BeginDrawToFinalRenderTarget();
	// backbufferへの書き込みの後始末
	void Flip();

	//マルチパスレンダリング用
		//ボードリソースに書き込む
	//ボードのインデックスで指定
	void BeginDrawToBoardsAndZBuff(std::list<unsigned int> boardIdx, bool useDepth);
	void BeginDrawToBoards(std::list<unsigned int> boardIdx, bool useDepth);
	void DrawBoard(unsigned int idx, unsigned int piplineIdx);
	void EndDrawToBoards(std::list<unsigned int> boardIdx);
	void EndDrawToBoardsAndZBuff(std::list<unsigned int> boardIdx);

	//デプスのみの描画
	void BeginDrawToDepth();

	//Z-Bufferを用いたディふぁーどレンダリング
	void DrawDeferred(unsigned int Boardidx);

	//ハッシュマップにあればそれを。なければloadmodel
	std::shared_ptr<Model> GetModelByPath(const char* path);
	//ハッシュマップにあればそれを。なければcreatesprite
	std::shared_ptr<Sprite> GetSpriteByPath(const char* path);
	//ハッシュマップにあればそれを。なければcreatesprite
	std::shared_ptr<AnimationData> GetAnimeByPath(const char* path);

	//イメージリソースを得る。
//一度読みこんだものはハッシュマップからコピーする
	ComPtr<ID3D12Resource>
		GetImageResourceByPath(const char*);

	//頂点配列からモデルを取得(同じnameがあればそれを返す)
	std::shared_ptr<Model> GetModelByVertices(const std::vector<VertexData>& vertices,
		const std::vector<Index>& indecies, const std::vector<Material>& materials,
		const char* name);

	//同様にパスからシェーダーを得る
	ComPtr<ID3DBlob>
		GetVertexShaderResourceByPath(const char* path, const char* entryPoint);
	ComPtr<ID3DBlob>
		GetPixelShaderResourceByPath(const char* path, const char* entryPoint);

	//imguiの描画
	void MakeImguiWindow();
	void DrawImgui();
private:
	Dx12Wrapper();
	~Dx12Wrapper();
	static Dx12Wrapper* instance;

	struct SceneData
	{
		XMMATRIX invview;//ビューの逆行列
		XMMATRIX view;//ビュー行列
		XMMATRIX invproj;//逆プロジェクション行列
		XMMATRIX proj;//プロジェクション行列
		XMFLOAT4 lightCol;//光の色
		XMFLOAT4 lightDir;//光の方向
		XMFLOAT4 eye;//視点座標
		XMMATRIX lightView;//光から見たビュー行列
	};
	struct SceneData2D
	{
		XMMATRIX proj;//プロジェクション行列
	};
	struct BoardData
	{
		int isVignette;
		float contrast;
	};

	Vector3 eyePos_;//視点座標
	Vector3 eyeRot_;//視線の向き
	Vector3 lightCol_;//光の色
	Vector3 lightDir;//光の方向
	XMFLOAT4 planeVec;//平面の方程式の係数
	Vector3 up_;//カメラの上向きベクトル
	float fov_;//画角
	float contrast_;//コントラスト(シグモイド曲線の係数)
	Vector3 bgCol_;//背景色

	int fenceVal_;

	ComPtr<IDXGIFactory6> dxgiFactory_;
	ComPtr<ID3D12Device> dev_;
	ComPtr<IDXGISwapChain4> swapchain_;
	ComPtr<ID3D12CommandAllocator> cmdAllocator_;
	ComPtr<ID3D12GraphicsCommandList> cmdList_;
	ComPtr<ID3D12CommandQueue> cmdQueue_;


	ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	std::vector<ComPtr<ID3D12Resource>> backBuffers_;
	ComPtr<ID3D12Resource> sceneConstBuffer_;//SceneData用
	ComPtr<ID3D12DescriptorHeap> sceneCbvHeap_;
	ComPtr<ID3D12Resource> scene2DConstBuffer_;//SceneData2D用
	ComPtr<ID3D12DescriptorHeap> scene2DCbvHeap_;
	//絵を投影するボード。マルチパスレンダリング用に複数に対応する
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

	ComPtr<ID3D12Resource> depthBuffer_;//depth用
	ComPtr<ID3D12Resource> lightdepthBuffer_;//光源から見たdepth用
	ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	ComPtr<ID3D12DescriptorHeap> depthSrvHeap_;//depthを画像としてみるよう

	ComPtr<ID3D12DescriptorHeap> heapForImgui_;//imgui用

	ComPtr<ID3D12Fence> fence_;

	std::unique_ptr<D3D12_VIEWPORT> viewport_;
	std::unique_ptr<D3D12_RECT> rect_;
	SceneData* mappedSceneData_;//カメラ周りの変数を送るためのマップ用変数
	SceneData2D* mappedSceneData2D_;
	BoardData* mappedBoardData_;//ポストエフェクト周りのデータ用
	std::unique_ptr<ModelLoader> modelLoader_;//PMDファイルを読むための機能

	HRESULT InitDXGIDevice();
	HRESULT InitCommand();
	HRESULT CreateSwapChain(HWND hwnd);
	HRESULT CreateFinalRendererTarget();//最終的なレンダーターゲットの生成
	HRESULT CreateSceneBufferView();//シーンで使う定数のバッファーを作る
	HRESULT Create2DSceneBufferView();//2Dの描画の際に使うシーン周りの定数バッファを作る
	HRESULT CreateBoardConstBufferView();//シーンで使う定数のバッファーを作る
	//投影するための板を生成.マルチパスレンダリング用に複数に対応する
	HRESULT CreateBoardResources();
	HRESULT CreateBoardVertex();
	HRESULT CreateBoardRTVs();
	HRESULT CreateBoardSRVs();
	HRESULT CreateBoardRootSignature();
	HRESULT CreateBoardGraphicsPipeline();
	HRESULT CreateDepthStencilView();	//深度バッファ作成

	//Imguiの初期化
	bool InitImgui();

	struct DrawToPathSetting
	{
		std::vector<ID3D12Resource*> renderTargets;
		std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvHandles;
		ID3D12DescriptorHeap* dsvDescHeap;
	};
	//途中のパスに書き込む準備
	void BeginDrawToPath(DrawToPathSetting&);
	//後始末
	void EndDrawToPath(DrawToPathSetting&);

	//TODO:pmxも余裕があれば
	bool LoadModel(const char* path, Model* model);


	ComPtr<ID3D12Resource>
		CreateImageResourceFromFile(const char*);

	using ImageLoadLambda = std::function<
		HRESULT(const std::wstring& path,
			TexMetadata*,
			ScratchImage&)>;
	//画像ロード関数テーブル
	std::unordered_map<std::string, ImageLoadLambda>
		loadImageLambdaTable_;
	void CreateTextureLoadTable();

	//イメージテーブル
	std::unordered_map<std::string,
		ComPtr<ID3D12Resource>>
		imageTable_;

	//PMDから読み込んだModelのテーブル
	std::unordered_map<std::string,
		std::shared_ptr<Model>>
		modelMap_;

	//VMDから読み込んだModelのテーブル
	std::unordered_map<std::string,
		std::shared_ptr<AnimationData>>
		animeVMDMap_;

	//スプライトテーブル
	std::unordered_map<std::string,
		std::shared_ptr<Sprite>>
		spriteMap_;

	//点光源配列
	std::vector<class PointLightComponent*>
		pointLights_;


};

