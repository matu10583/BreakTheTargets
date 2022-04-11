#pragma once
#include "XMFLOAT_Helper.h"
#include <wrl.h>
#include <vector>
#include <d3d12.h>
#include<DirectXMath.h>
#include <string>
#include <unordered_map>

class Sprite
{
	friend class ModelLoader;
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;


public:
	Sprite();

	//getter
	const D3D12_VERTEX_BUFFER_VIEW&
		VBView()const { return vbView_; }
	const D3D12_INDEX_BUFFER_VIEW&
		IBView()const { return ibView_; }
	ID3D12DescriptorHeap*
		TextureHeap()const { return texHeap_.Get(); }
	const Vector2 GetSize()const;

private:
	//頂点配列
	std::vector<Vector3> vertices_;
	//テクスチャのビューを置くheap
	ComPtr<ID3D12DescriptorHeap> texHeap_;

	//テクスチャ画像のリソース
	ComPtr<ID3D12Resource> textureImg_;


	//vertex buffer
	ComPtr<ID3D12Resource> vb_;
	D3D12_VERTEX_BUFFER_VIEW vbView_;
	//index buffer
	ComPtr<ID3D12Resource> ib_;
	D3D12_INDEX_BUFFER_VIEW ibView_;
};

