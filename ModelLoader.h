#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include "XMFLOAT_Helper.h"

struct Model;
struct Material;
struct AnimationData;
class Sprite;
class Dx12Wrapper;
class ModelLoader
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;
public:
	//ファイルからモデルを読み込む
	ModelLoader();
	bool LoadPMDModel(const char* fileName, Model* mesh);
	bool LoadPMXModel(const char* fileName, Model* mesh);

	//頂点配列からモデルを作る
	bool CreateVerticesModel(const std::vector<VertexData>& vertices,
		const std::vector<Index>& indecies, const std::vector<Material>& material,
		Model* model);
	//画像スプライトのモデルを作る
	bool LoadSprite(const char* fileName, Sprite* sprite);
	//アニメーションのデータを作る
	bool LoadAnimation(const char* fileName, AnimationData* anime);
	//デフォルトで用意するテクスチャを作る
	bool CreateDefaultTextures();
private:
	Dx12Wrapper* dx12_;

	//バッファを作りマテリアルデータをコピー
	HRESULT CreateMaterialData(Model*);
	//マテリアルとテクスチャ群のビューを作る
	HRESULT CreateMaterialAndTextureView(Model*);

	//デフォルトのテクスチャ用
	ComPtr<ID3D12Resource> whiteTex_ = nullptr;
	ComPtr<ID3D12Resource> blackTex_ = nullptr;
	ComPtr<ID3D12Resource> gradTex_ = nullptr;
	ComPtr<ID3D12Resource> normalTex_ = nullptr;
	//TODO:これらの関数の実装
	//WriteBackタイプのテクスチャ―ようリソースを作る
	ComPtr<ID3D12Resource>
		CreateWriteBackTexture(size_t width, size_t height);
	ComPtr<ID3D12Resource>
		CreateWhiteTexture();//白テクスチャの生成
	ComPtr<ID3D12Resource>
		CreateBlackTexture();//黒テクスチャの生成
	ComPtr<ID3D12Resource>
		CreateGrayGradationTexture();//グレーグラデーションテクスチャの生成
	ComPtr<ID3D12Resource>
		CreateNormalTexture();//デフォルトの法線マップテクスチャの生成
};

