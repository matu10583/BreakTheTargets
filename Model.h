#pragma once
#include "XMFLOAT_Helper.h"
#include <wrl.h>
#include <vector>
#include <d3d12.h>
#include<DirectXMath.h>
#include <string>
#include <unordered_map>

//-----------------------------------
//以下構造体
		//シェーダ側に投げられるマテリアルデータ
struct MaterialForHlsl {
	Vector3 diffuse; //ディフューズ色
	float alpha; // ディフューズα
	Vector3 specular; //スペキュラ色
	float specularity;//スペキュラの強さ(乗算値)
	Vector3 ambient; //アンビエント色
};
//それ以外のマテリアルデータ
struct AdditionalMaterial {
	std::string texPath;//テクスチャファイルパス
	std::string normalPath;//法線マップのテクスチャのパス
	std::string aoPath;//aoマップのテクスチャのパス
	int toonIdx; //トゥーン番号
	bool edgeFlg;//マテリアル毎の輪郭線フラグ
};
//まとめたもの
struct Material {
	unsigned int indicesNum;//インデックス数
	MaterialForHlsl material;
	AdditionalMaterial additional;
	Material():
	Material(0, Vector3(0, 0, 0), 0)
	{};
	Material(unsigned int indicesNumber, const Vector3& diffuse, float alpha) {
		indicesNum = indicesNumber;
		material.diffuse = diffuse;
		material.alpha = alpha;
		material.specular = Vector3(1, 1, 1);
		material.specularity = 15;
		material.ambient = Vector3(0.2f, 0.2f, 0.2f);
		additional.texPath = "";
		additional.normalPath = "";
		additional.aoPath = "";
		additional.toonIdx = 255;
		additional.edgeFlg = false;
	}
};


struct BoneNode {
	uint32_t boneIdx;//ボーンインデックス
	uint32_t boneType;//ボーン種別
	uint32_t ikParentBone;//親ボーン
	Vector3 startPos;//ボーン基準点
	Vector3 endPos;//ボーン先端店
	std::vector<BoneNode*> children;//子
};

struct PMDIK {
	uint16_t boneIdx;	//IK対象のボーンを示す
	uint16_t targetIdx;	//ターゲットに近づけるためのボーンのインデックス
	uint16_t iterations;	//嗜好お回数
	float limit;	//一回当たりの回転制限
	std::vector<uint16_t> nodeIdxes;	//ノード番号
};



//---------------------------------------------------

struct Model
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//全体のインデックス数
	unsigned int indicesNum_;
	//頂点配列
	std::vector<Vector3> vertices_;
	//マテリアルの情報
	std::vector<Material> materialInfos_;
	//それぞれのマテリアルのビューを置くheap
	ComPtr<ID3D12DescriptorHeap> materialHeap_;

	//テクスチャ画像のリソース
	std::vector<
		ComPtr<ID3D12Resource>
	> textureImgs_;
	//sph画像
	std::vector<
		ComPtr<ID3D12Resource>
	> sphImgs_;
	//spa画像
	std::vector<
		ComPtr<ID3D12Resource>
	> spaImgs_;
	//法線マップ画像
	std::vector<
		ComPtr<ID3D12Resource>
	> normalImgs_;
	//AOマップ画像
	std::vector<
		ComPtr<ID3D12Resource>
	> aoImgs_;

	//toonシェーディング画像
	std::vector<
		ComPtr<ID3D12Resource>
	> toonImgs_;
	//マテリアルの定数
	ComPtr<ID3D12Resource> materialParams_;


	//vertex buffer
	ComPtr<ID3D12Resource> vb_;
	D3D12_VERTEX_BUFFER_VIEW vbView_;
	//index buffer
	ComPtr<ID3D12Resource> ib_;
	D3D12_INDEX_BUFFER_VIEW ibView_;

	//ボーン情報
	//ノードを名前から検索できるようにする
	std::unordered_map<std::string, BoneNode> boneNodeTable_;
	//名前とインデックスの対応
	std::vector<std::string> boneNameArray_;
	//インデックスとノードの対応
	std::vector<BoneNode*> boneNodeAddressArray_;

	std::vector<PMDIK> ikData_;
	//名前に"ひざ"を含むノードのインデックス
	std::vector<uint32_t> kneeIdxes_;

};

//VMD用構造体
	//IK on/off
struct VMDIKEnable {
	uint32_t frameNo;//フレーム番号
	std::unordered_map<std::string, bool>
		ikEnableTable;//名前とフラグのマップ
};

struct KeyFrame {
	unsigned int frameNo;//何フレーム目か
	XMVECTOR quaternion;//クオータニオン
	Vector3 offset;
	Vector2 p1, p2;//ベジェ曲線のコントロールポイント
	KeyFrame(unsigned int fno, XMVECTOR& q,
		Vector3& ofst,
		const Vector2& ip1, const Vector2& ip2)
		:frameNo(fno), quaternion(q),
		offset(ofst),
		p1(ip1), p2(ip2)
	{};
	KeyFrame(){}
};

struct AnimationData
{
	std::unordered_map<std::string,
	std::vector<KeyFrame>>
		motiondata;
	unsigned int duration;//アニメーションの最大のふれーむすう
	std::vector<VMDIKEnable> ikEnableData;
};