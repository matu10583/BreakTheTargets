#include "ModelLoader.h"
#include "Model.h"
#include "Sprite.h"
#include "Dx12Wrapper.h"
#include <string>
#include <assert.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#include <algorithm>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace {
	//モデルのパスとテクスチャのパスから合成パスを得る
	std::string GetTexturePathFromModelAndTexPath(const std::string& modelPath, const char* texPath) {
		unsigned int pathIndex1 = modelPath.rfind('/');
		unsigned int pathIndex2 = modelPath.rfind('\\');
		auto pathIndex = max(pathIndex1, pathIndex2);
		auto folderPath = modelPath.substr(0, pathIndex + 1);
		return folderPath + texPath;
	}

	//ファイル名から拡張子を取得する
	std::string
		GetExtension(const std::string& path) {
		unsigned int idx = path.rfind('.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	std::wstring
		GetExtension(const std::wstring& path) {
		unsigned int idx = path.rfind(L'.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	//テクスチャのパスをセパレータ文字で分離する
	std::pair<std::string, std::string>
		SplitFileName(const std::string& path, const char splitter = '*') {
		int idx = path.find(splitter);
		std::pair<std::string, std::string> ret;
		ret.first = path.substr(0, idx);
		ret.second = path.substr(idx + 1, path.length() - idx - 1);
		return ret;
	}

	size_t
		AllignToMultipleOf256(size_t size) {
		return (size + 0xff) & ~0xff;
	}
}
ModelLoader::ModelLoader() {
	dx12_ = Dx12Wrapper::Instance();
}

bool
ModelLoader::LoadPMDModel(const char* fileName, Model* model) {
	//TODO:PMDファイルのパースと読み込みの実装
	//PMDファイルを読み込み
	FILE* pmdFile;
	std::string strModelPath = fileName;
	fopen_s(&pmdFile, strModelPath.c_str(), "rb");
	if (pmdFile == nullptr) {
		assert(0);
		fclose(pmdFile);
		return false;
	}

	//ヘッダー読み込み
	struct PMDHeader {
		float version;
		char model_name[20];
		char comment[256];
	};
	PMDHeader pmdheader = {};
	char signature[3];
	fread(signature, sizeof(signature), 1, pmdFile);
	fread(&pmdheader, sizeof(PMDHeader), 1, pmdFile);

	//頂点数
	unsigned int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, pmdFile);

#pragma pack(1)//読み込みのために1バイトパッキング
	//PMDマテリアル構造体
	struct PMDMaterial
	{
		Vector3 diffuse;//拡散色
		float dAlpha;//ディフューズのα値
		float specularity;//スペキュラの強さ（乗算）
		Vector3 specular;//スペキュラ色
		Vector3 ambient;//アンビエント色
		unsigned char toonIdx;//トゥーン番号
		unsigned char edgeFlg;//マテリアル毎の輪郭線フラグ
		unsigned int indicesNum;//マテリアルが割り当てられるインデックス数
		char texFilePath[20];//テクスチャファイル名
	};

	struct PMDVertex
	{
		float pos[3]; // x, y, z // 座標
		float normal_vec[3]; // nx, ny, nz // 法線ベクトル
		float uv[2]; // u, v // UV座標 // MMDは頂点UV
		WORD bone_num[2]; // ボーン番号1、番号2 // モデル変形(頂点移動)時に影響
		BYTE bone_weight; // ボーン1に与える影響度 // min:0 max:100 // ボーン2への影響度は、(100 - bone_weight)
		BYTE edge_flag; // 0:通常、1:エッジ無効 // エッジ(輪郭)が有効の場合
	};

#pragma pack()//1バイトパッキング終わり



	constexpr unsigned int pmdvertex_size = sizeof(PMDVertex);//頂点一つ当たりのサイズ
	std::vector<PMDVertex> vertices(vertNum);//頂点用バッファ
	fread(vertices.data(), pmdvertex_size, vertices.size(), pmdFile);//頂点読み込み

	unsigned int indicesNum;//全体のインデックス数
	fread(&indicesNum, sizeof(indicesNum), 1, pmdFile);
	
	//全体のインデックス数の記録
	model->indicesNum_ = indicesNum;

	//頂点バッファ作製(MapでUPLOADする
	auto vheapprop =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto vresdesc =
		CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(vertices[0]));
	auto result = dx12_->Dev()->CreateCommittedResource(
		&vheapprop,
		D3D12_HEAP_FLAG_NONE,
		&vresdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(model->vb_.ReleaseAndGetAddressOf())
	);

	//Map用変数
	unsigned char* vertMap = nullptr;
	result = model->vb_->Map(0, nullptr, (void**)&vertMap);
	//頂点配列を保存
	model->vertices_.resize(vertNum);
	DWORD i = 0;
	for (auto& v : vertices) {
		model->vertices_[i] = Vector3(v.pos[0], v.pos[1], v.pos[2]);
		*(PMDVertex*)vertMap = v;
		vertMap += pmdvertex_size;
		i++;
	}
	model->vb_->Unmap(0, nullptr);

	//vertex buffer viewの作製
	//バッファの仮想アドレス
	model->vbView_.BufferLocation = model->vb_->GetGPUVirtualAddress();
	//全サイズ
	model->vbView_.SizeInBytes = vertices.size() * pmdvertex_size;
	//一頂点あたりのサイズ
	model->vbView_.StrideInBytes = pmdvertex_size;

	//indicesの読み込み
	std::vector<unsigned short>indices(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, pmdFile);

	//indices用のバッファ（UPLOAD）作製
	auto iresdesc =
		CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(indices[0]));
	auto iheapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	result = dx12_->Dev()->CreateCommittedResource(
		&iheapprop,
		D3D12_HEAP_FLAG_NONE,
		&iresdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(model->ib_.ReleaseAndGetAddressOf())
	);

	//Map用変数
	unsigned short* mappedIdx = nullptr;
	model->ib_->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	model->ib_->Unmap(0, nullptr);

	//index buffer viewの作製
	model->ibView_.BufferLocation = model->ib_->GetGPUVirtualAddress();
	model->ibView_.Format = DXGI_FORMAT_R16_UINT;
	model->ibView_.SizeInBytes = indices.size() * sizeof(indices[0]);

	//マテリアルの数
	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, pmdFile);

	//vectorのサイズを決める
	model->toonImgs_.resize(materialNum);
	model->sphImgs_.resize(materialNum);
	model->spaImgs_.resize(materialNum);
	model->textureImgs_.resize(materialNum);
	model->materialInfos_.resize(materialNum);
	model->normalImgs_.resize(materialNum);
	model->aoImgs_.resize(materialNum);

	//マテリアル情報取得
	std::vector<PMDMaterial> pmdMaterials(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(pmdMaterials[0]), 1, pmdFile);


	for (int i = 0; i < pmdMaterials.size(); ++i) {
		//マテリアルの情報を記録
		auto& materialInfo = model->materialInfos_[i];
		materialInfo.indicesNum = pmdMaterials[i].indicesNum;
		materialInfo.material.diffuse = pmdMaterials[i].diffuse;
		materialInfo.material.alpha = pmdMaterials[i].dAlpha;
		materialInfo.material.specular = pmdMaterials[i].specular;
		materialInfo.material.specularity = pmdMaterials[i].specularity;
		materialInfo.material.ambient = pmdMaterials[i].ambient;
		materialInfo.additional.toonIdx = pmdMaterials[i].toonIdx;
		materialInfo.additional.texPath = pmdMaterials[i].texFilePath;
		materialInfo.additional.edgeFlg = pmdMaterials[i].edgeFlg;
		//AOと法線マップは使わない
		materialInfo.additional.normalPath = "";
		materialInfo.additional.aoPath = "";

		//トゥーンリソースの取得
		char toonFilePath[32];
		sprintf_s(toonFilePath, "toon/toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
		model->toonImgs_[i] = dx12_->GetImageResourceByPath(toonFilePath);

		//テクスチャの指定がなければ空にする
		//（透明だとおかしいので後でなにかしらを入れる）
		if (strlen(pmdMaterials[i].texFilePath) == 0) {
			model->textureImgs_[i] = nullptr;
			continue;
		}

		std::string texFileName = pmdMaterials[i].texFilePath;
		std::string sphFileName = "";
		std::string spaFileName = "";
		//*で画像とスフィアマップが分けられているので探す
		//例）body.bmp*eye.spa
		if (count(texFileName.begin(), texFileName.end(), '*') > 0) {
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first) == "sph") {
				texFileName = namepair.second;
				sphFileName = namepair.first;
			}
			else if (GetExtension(namepair.first) == "spa") {
				texFileName = namepair.second;
				spaFileName = namepair.first;
			}
			else {
				texFileName = namepair.first;
				if (GetExtension(namepair.second) == "sph") {
					sphFileName = namepair.second;
				}
				else if (GetExtension(namepair.second) == "spa") {
					spaFileName = namepair.second;
				}
			}
		}
		else {
			if (GetExtension(pmdMaterials[i].texFilePath) == "sph") {
				sphFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else if (GetExtension(pmdMaterials[i].texFilePath) == "spa") {
				spaFileName = pmdMaterials[i].texFilePath;
				texFileName = "";
			}
			else {
				texFileName = pmdMaterials[i].texFilePath;
			}
		}
		//モデルとテクスチャパスから相対パスを得る
		if (texFileName != "") {
			auto texFilePath = GetTexturePathFromModelAndTexPath(strModelPath, texFileName.c_str());
			model->textureImgs_[i] = dx12_->GetImageResourceByPath(texFilePath.c_str());
		}
		if (sphFileName != "") {
			auto sphFilePath = GetTexturePathFromModelAndTexPath(strModelPath, sphFileName.c_str());
			model->sphImgs_[i] = dx12_->GetImageResourceByPath(sphFilePath.c_str());
		}
		if (spaFileName != "") {
			auto spaFilePath = GetTexturePathFromModelAndTexPath(strModelPath, spaFileName.c_str());
			model->spaImgs_[i] = dx12_->GetImageResourceByPath(spaFilePath.c_str());
		}
	}

	//マテリアルデータをバッファにコピー
	result = CreateMaterialData(model);
	if (FAILED(result)) {
		assert(0);
		return false;
	}
	
	//マテリアルのビューを作る
	result = CreateMaterialAndTextureView(model);
	if (FAILED(result)) {
		assert(0);
		return false;
	}

	//TODO:ボーンとIKの読み込みを実装する
	//ボーン情報の読み込み
	//ボーンの数
	unsigned short bonenum = 0;
	fread(&bonenum, sizeof(bonenum), 1, pmdFile);

	//bone情報
#pragma pack(1)
	struct PMDBone {
		char boneName[20];//ボーン名
		unsigned short parentNo;//親ボーン番号
		unsigned short nextNo;//先端のボーン番号

		unsigned char type;//ボーン種別(とりあえず人体のモーションだけなので回転だけ)
		//Todo:回転以外のボーンにも対応する
		unsigned short ikBoneNo;//IKボーン番号
		XMFLOAT3 pos;//ボーンの基準点の座標
	};
#pragma pack()

	std::vector<PMDBone> pmdBones(bonenum);
	fread(
		pmdBones.data(), sizeof(PMDBone), pmdBones.size(), pmdFile);

	std::vector<std::string> boneNames(bonenum);
	model->boneNameArray_.resize(bonenum);
	model->boneNodeAddressArray_.resize(bonenum);
	model->kneeIdxes_.clear();

	//ノードを作る
	for (int i = 0; i < bonenum; i++) {
		auto& pb = pmdBones[i];
		boneNames[i] = pb.boneName;
		auto& node = model->boneNodeTable_[pb.boneName];
		node.boneIdx = i;
		node.startPos = pb.pos;

		model->boneNameArray_[i] = pb.boneName;
		if (model->boneNameArray_[i].find("ひざ") != std::string::npos) {
			//pmdの使用としてひざの文字列を含むボーンが角度制限を持つので覚える
			model->kneeIdxes_.emplace_back(i);
		}
		model->boneNodeAddressArray_[i] = &node;
	}
	//ツリーの構築
	for (auto& pb : pmdBones) {
		if (pb.parentNo >= pmdBones.size()) {
			//無効な番号なので親はない
			continue;
		}
		//このボーンを親の子ベクタに追加する
		auto pName = boneNames[pb.parentNo];
		model->boneNodeTable_[pName].children.emplace_back(
			&model->boneNodeTable_[pb.boneName]
		);
		model->boneNodeTable_[pb.boneName].ikParentBone = pb.parentNo;
	}

	//ikデータの読み込み
	uint16_t iknum = 0;
	fread(&iknum, sizeof(iknum), 1, pmdFile);

	model->ikData_.resize(iknum);

	//ikデータをとってくる
	for (auto& ik : model->ikData_) {
		fread(&ik.boneIdx, sizeof(ik.boneIdx), 1, pmdFile);
		fread(&ik.targetIdx, sizeof(ik.targetIdx), 1, pmdFile);
		uint8_t chainLen = 0;
		fread(&chainLen, sizeof(chainLen), 1, pmdFile);
		fread(&ik.iterations, sizeof(ik.iterations), 1, pmdFile);
		fread(&ik.limit, sizeof(ik.limit), 1, pmdFile);
		if (!chainLen) {
			//IKチェーンなし
			continue;
		}
		ik.nodeIdxes.resize(chainLen);
		fread(ik.nodeIdxes.data(), sizeof(ik.nodeIdxes[0]), chainLen, pmdFile);
	}

	fclose(pmdFile);
	return true;
}

bool
ModelLoader::CreateVerticesModel(const std::vector<VertexData>& vertices,
	const std::vector<Index>& indices, const std::vector<Material>& material,
	Model* model) {

	//リソース作製
	auto heapprop =
		CD3DX12_HEAP_PROPERTIES(
			D3D12_HEAP_TYPE_UPLOAD
		);
	auto resdesc = 
		CD3DX12_RESOURCE_DESC::Buffer(vertices.size()*sizeof(VertexData));

	auto result = dx12_->Dev()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(model->vb_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return false;
	}
	//vertex viewの作製
	model->vbView_.BufferLocation = model->vb_->GetGPUVirtualAddress();
	model->vbView_.SizeInBytes = vertices.size()*sizeof(VertexData);
	model->vbView_.StrideInBytes = sizeof(VertexData);
	//頂点は記録
	for (auto& vd: vertices) {
		model->vertices_.emplace_back(vd.pos);
	}

	//頂点情報の転送
	VertexData* mappedVertex = nullptr;
	model->vb_->Map(0, nullptr,
		(void**)&mappedVertex);
	std::copy(vertices.begin(), vertices.end(), mappedVertex);
	model->vb_->Unmap(0, nullptr);

	//index bufferの作製
	if (indices.empty()) {
		return true;
	}
	resdesc =
		CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(Index));

	result = dx12_->Dev()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(model->ib_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return false;
	}
	//index viewの作製
	model->ibView_.BufferLocation = model->ib_->GetGPUVirtualAddress();
	model->ibView_.SizeInBytes = indices.size() * sizeof(Index);
	model->ibView_.Format = DXGI_FORMAT_R16_UINT;
	//インデックス数の記録
	model->indicesNum_ = indices.size() * 3;

	//インデックス情報の転送
	Index* mappedIndex = nullptr;
	model->ib_->Map(0, nullptr,
		(void**)&mappedIndex);
	std::copy(indices.begin(), indices.end(), mappedIndex);
	model->ib_->Unmap(0, nullptr);

	//マテリアル
	model->materialInfos_ = material;
	unsigned int materialNum = material.size();
	//vectorのサイズを決める
	model->toonImgs_.resize(materialNum);
	model->sphImgs_.resize(materialNum);
	model->spaImgs_.resize(materialNum);
	model->textureImgs_.resize(materialNum);
	model->normalImgs_.resize(materialNum);
	model->aoImgs_.resize(materialNum);
	
	for (int i = 0; i < material.size(); ++i) {

		//トゥーンリソースの取得
		char toonFilePath[32];
		sprintf_s(toonFilePath, "toon/toon%02d.bmp", material[i].additional.toonIdx + 1);
		model->toonImgs_[i] = dx12_->GetImageResourceByPath(toonFilePath);

		//テクスチャの指定がなければ空にする
		//（透明だとおかしいので後でなにかしらを入れる）
		if (strlen(material[i].additional.texPath.c_str()) == 0) {
			model->textureImgs_[i] = nullptr;
			continue;
		}

		std::string texFileName = material[i].additional.texPath;
		std::string sphFileName = "";
		std::string spaFileName = "";
		std::string normalFileName = material[i].additional.normalPath;
		std::string aoFileName = material[i].additional.aoPath;
		//*で画像とスフィアマップが分けられているので探す
		//例）body.bmp*eye.spa
		if (count(texFileName.begin(), texFileName.end(), '*') > 0) {
			auto namepair = SplitFileName(texFileName);
			if (GetExtension(namepair.first) == "sph") {
				texFileName = namepair.second;
				sphFileName = namepair.first;
			}
			else if (GetExtension(namepair.first) == "spa") {
				texFileName = namepair.second;
				spaFileName = namepair.first;
			}
			else {
				texFileName = namepair.first;
				if (GetExtension(namepair.second) == "sph") {
					sphFileName = namepair.second;
				}
				else if (GetExtension(namepair.second) == "spa") {
					spaFileName = namepair.second;
				}
			}
		}
		else {
			if (GetExtension(material[i].additional.texPath) == "sph") {
				sphFileName = material[i].additional.texPath;
				texFileName = "";
			}
			else if (GetExtension(material[i].additional.texPath) == "spa") {
				spaFileName = material[i].additional.texPath;
				texFileName = "";
			}
			else {
				texFileName = material[i].additional.texPath;
			}
		}
		//imgパスととテクスチャパスから相対パスを得る
		if (texFileName != "") {
			auto texFilePath = "img/" + texFileName;
			model->textureImgs_[i] = dx12_->GetImageResourceByPath(texFilePath.c_str());
		}
		if (sphFileName != "") {
			auto sphFilePath = "img/" + sphFileName;
			model->sphImgs_[i] = dx12_->GetImageResourceByPath(sphFilePath.c_str());
		}
		if (spaFileName != "") {
			auto spaFilePath = "img/" + spaFileName;
			model->spaImgs_[i] = dx12_->GetImageResourceByPath(spaFilePath.c_str());
		}
		if (normalFileName != "") {
			auto normalFilePath = "img/" + normalFileName;
			model->normalImgs_[i] = dx12_->GetImageResourceByPath(normalFilePath.c_str());
		}
		if (aoFileName != "") {
			auto aoFilePath = "img/" + aoFileName;
			model->aoImgs_[i] = dx12_->GetImageResourceByPath(aoFilePath.c_str());
		}

	}

	//マテリアルデータをバッファにコピー
	result = CreateMaterialData(model);
	if (FAILED(result)) {
		assert(0);
		return false;
	}

	//マテリアルのビューを作る
	result = CreateMaterialAndTextureView(model);
	if (FAILED(result)) {
		assert(0);
		return false;
	}


	return true;
}

//PMDしか機能を装備してないのでPMDにできない機能は読み飛ばす
//PMDに変換できることがわかったので実装しない
bool 
ModelLoader::LoadPMXModel(const char* fileName, Model* model) {
	FILE* pmxFile;
	std::string strModelPath = fileName;
	fopen_s(&pmxFile, strModelPath.c_str(), "rb");
	if (pmxFile == nullptr) {
		assert(0);
		fclose(pmxFile);
		return false;
	}
	float ver;
	fseek(pmxFile, 4, SEEK_CUR);
	fread(&ver, sizeof(ver), 1, pmxFile);
	if (ver - 2.1f > -FLT_EPSILON) {
		fclose(pmxFile);
		assert(0);
		return false;//2.0だけ読む
	}

	byte Hlength;
	fread(&Hlength, sizeof(Hlength), 1, pmxFile);
	if (Hlength != 8) {
		fclose(pmxFile);
		assert(0);
		return false;//2.0だけ読む
	}
	std::array<byte, 8> Hdata;
	fread(&Hdata, sizeof(Hdata[0]), 8, pmxFile);
	enum MAGIC_NUMBER
	{
		ENCODE,
		ADD_UV,
		VERTEX_INDEX_SIZE,
		TEXTURE_INDEX_SIZE,
		MATERIAL_INDEX_SIZE,
		BONE_INDEX_SIZE,
		MORPH_INDEX_SIZE,
		PHYSICS_INDEX_SIZE
	};

	unsigned Infolen;
	for (int i = 0; i < 4; i++) {
		fread(&Infolen, sizeof(Infolen), 1, pmxFile);
		fseek(pmxFile, Infolen, SEEK_CUR);
	}

	//頂点
	int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, pmxFile);
	
#pragma pack(1)//読み込みのために1バイトパッキング
	//PMDマテリアル構造体
	struct PMDMaterial
	{
		Vector3 diffuse;//拡散色
		float dAlpha;//ディフューズのα値
		float specularity;//スペキュラの強さ（乗算）
		Vector3 specular;//スペキュラ色
		Vector3 ambient;//アンビエント色
		unsigned char toonIdx;//トゥーン番号
		unsigned char edgeFlg;//マテリアル毎の輪郭線フラグ
		unsigned int indicesNum;//マテリアルが割り当てられるインデックス数
		char texFilePath[20];//テクスチャファイル名
	};

	struct PMDVertex
	{
		float pos[3]; // x, y, z // 座標
		float normal_vec[3]; // nx, ny, nz // 法線ベクトル
		float uv[2]; // u, v // UV座標 // MMDは頂点UV
		WORD bone_num[2]; // ボーン番号1、番号2 // モデル変形(頂点移動)時に影響
		BYTE bone_weight; // ボーン1に与える影響度 // min:0 max:100 // ボーン2への影響度は、(100 - bone_weight)
		BYTE edge_flag; // 0:通常、1:エッジ無効 // エッジ(輪郭)が有効の場合
	};

#pragma pack()//1バイトパッキング終わり

	std::vector<PMDVertex> vertices(vertNum);//頂点用バッファ

	//頂点のボーン変形方式によるそれぞれのデータサイズ
	auto bsize = Hdata[BONE_INDEX_SIZE];
	std::array<int, 4> WeightSize = {
		bsize,//BDEF1
		bsize * 2 + 4,//BDEF2
		bsize * 4 + 4 * 4,//BDEF4
		bsize * 2 + 4 + 12 * 3//SDEF
	};

	for (auto& v : vertices) {
		fread(v.pos, sizeof(float), 3 + 3 + 2, pmxFile);
		fseek(pmxFile, 16 * Hdata[ADD_UV], SEEK_CUR);//追加UV
		byte w;//ウェイト方式
		fread(&w, sizeof(w), 1, pmxFile);
		fseek(pmxFile, WeightSize[w], SEEK_CUR);
	}
	//TODO:実装
	return true;
}

bool
ModelLoader::LoadSprite(const char* fileName, Sprite* sprite) {
	//画像のロード
	sprite->textureImg_ = dx12_->GetImageResourceByPath(fileName);
	auto resdesc = sprite->textureImg_->GetDesc();
	auto w = resdesc.Width;
	auto h = resdesc.Height;
	//スクリーン座標
	std::vector<VertexData2D> vd{
		{Vector3(0.0f, h, 0.0f), Vector2(0.0f, 1.0f)},//左下
		{Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f)},//左上
		{Vector3(w, h, 0.0f), Vector2(1.0f, 1.0f)},//右下
		{Vector3(w, 0.0f, 0.0f), Vector2(1.0f, 0.0f)},//右上
	};
	//頂点配列の記録
	sprite->vertices_.resize(4);
	for (int i = 0; i < 4; i++) {
		sprite->vertices_[i] = vd[i].pos;
	}
	//頂点情報の作成

	//リソース作製
	auto heapprop =
		CD3DX12_HEAP_PROPERTIES(
			D3D12_HEAP_TYPE_UPLOAD
		);
	resdesc =
		CD3DX12_RESOURCE_DESC::Buffer(vd.size() * sizeof(VertexData2D));

	auto result = dx12_->Dev()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(sprite->vb_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return false;
	}
	//vertex viewの作製
	sprite->vbView_.BufferLocation = sprite->vb_->GetGPUVirtualAddress();
	sprite->vbView_.SizeInBytes = vd.size() * sizeof(VertexData2D);
	sprite->vbView_.StrideInBytes = sizeof(VertexData2D);

	//頂点情報の転送
	VertexData2D* mappedVertex = nullptr;
	sprite->vb_->Map(0, nullptr,
		(void**)&mappedVertex);
	std::copy(vd.begin(), vd.end(), mappedVertex);
	sprite->vb_->Unmap(0, nullptr);

	//テクスチャビュー作製
	//Descriptor Heap Desc作製
	D3D12_DESCRIPTOR_HEAP_DESC descheapdesc = {};
	descheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descheapdesc.NodeMask = 0;
	descheapdesc.NumDescriptors = 1;
	descheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = dx12_->Dev()->CreateDescriptorHeap(
		&descheapdesc,
		IID_PPV_ARGS(sprite->texHeap_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return false;
	}
	//画像バッファのビューデスク
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	CD3DX12_CPU_DESCRIPTOR_HANDLE texHeapHandle(
		sprite->texHeap_->GetCPUDescriptorHandleForHeapStart());

	srvDesc.Format = sprite->textureImg_->GetDesc().Format;
	dx12_->Dev()->CreateShaderResourceView(
		sprite->textureImg_.Get(),
		&srvDesc,
		texHeapHandle
	);

	return true;
}

bool 
ModelLoader::LoadAnimation(const char* fileName, AnimationData* anime) {
	//
	FILE* vmdFile;
	fopen_s(&vmdFile, fileName, "rb");
	if (vmdFile == nullptr) {
		assert(0);
		fclose(vmdFile);
		return false;
	}

	//ヘッダーを飛ばす
	fseek(vmdFile, 50, SEEK_SET);

	//モーションデータの数
	unsigned int motionNum = 0;
	fread(&motionNum, sizeof(motionNum), 1, vmdFile);

	//データロード用構造体
	struct VMDMotion {
		char boneName[15];//ボーン名
		//ここにパディングが入る.
		unsigned int frameNo;//フレーム番号
		Vector3 location;//位置
		XMFLOAT4 quaternion;//クォータニオン
		unsigned char bezier[64];//ベジェ補間
	};
	std::vector<VMDMotion> vmdMotionData(motionNum);

	for (auto& mo : vmdMotionData) {
		fread(mo.boneName, sizeof(mo.boneName), 1, vmdFile);
		fread(&mo.frameNo,
			sizeof(mo.frameNo) +
			sizeof(mo.location) +
			sizeof(mo.quaternion) +
			sizeof(mo.bezier),
			1, vmdFile);
	}

	//データを整形して読み込む
	for (auto& mo : vmdMotionData) {
		auto quat = XMLoadFloat4(&mo.quaternion);
		//ベジェのコントロールポイント座標を正規化して求める
		//ボーンは回転だけなので回転のポイントだけを取得
		auto p1 = Vector2((float)mo.bezier[3] / 127.0f,
			(float)mo.bezier[7] / 127.0f);
		auto p2 = Vector2((float)mo.bezier[11] / 127.0f,
			(float)mo.bezier[15] / 127.0f);
		anime->motiondata[mo.boneName].emplace_back(
			KeyFrame(mo.frameNo, quat, mo.location, p1, p2)
		);
		anime->duration = std::max<unsigned int>(anime->duration,
			mo.frameNo);
	}

	//モーションデータを整列させる
	for (auto& mo : anime->motiondata) {
		std::sort(mo.second.begin(),
			mo.second.end(),
			[](const KeyFrame& lval, const KeyFrame& rval) {
				return (lval.frameNo <= rval.frameNo);
			}
		);
	}


	//以下のデータはIKEnableまで使わないがseekもできないので四読
#pragma pack(1)
	//表情データ
	struct VMDMorph
	{
		char name[15];//名前
		uint32_t frameNo;//フレーム番号
		float weight;//ウェイト(0~1)
	};
#pragma pack()

	uint32_t morphCount = 0;
	fread(&morphCount, sizeof(morphCount), 1, vmdFile);
	std::vector<VMDMorph> morphs(morphCount);
	fread(morphs.data(), sizeof(VMDMorph), morphCount, vmdFile);

#pragma pack(1)
	//カメラ
	struct VMDCamera
	{
		uint32_t frameNo;//フレーム番号
		float distance;//距離
		XMFLOAT3 pos;//座標
		XMFLOAT3 eulerAngle;//オイラー角
		uint8_t Interpolation[24];
		uint32_t fov;//視野角
		uint8_t persFlg;//パースフラグ
	};
#pragma pack()

	uint32_t vmdCameraCount = 0;
	fread(&vmdCameraCount, sizeof(vmdCameraCount), 1, vmdFile);

	std::vector<VMDCamera> cameraData(vmdCameraCount);
	fread(cameraData.data(), sizeof(VMDCamera), vmdCameraCount, vmdFile);

	struct VMDLight
	{
		uint32_t frameNo;//フレーム番号
		XMFLOAT3 rgb;//ライト色
		XMFLOAT3 vec;//高専ベクトル（並行光線）
	};

	uint32_t vmdLightCount = 0;
	fread(&vmdLightCount, sizeof(vmdLightCount), 1, vmdFile);

	std::vector<VMDLight> lights(vmdLightCount);
	fread(lights.data(), sizeof(VMDLight), vmdLightCount, vmdFile);

#pragma pack(1)
	//セルフ影データ
	struct VMDSelfShadow
	{
		uint32_t frameNo;//フレーム番号
		uint8_t mode;//影モード(0:none, 1:mode1, 2:mode2)
		float distance;//距離
	};
#pragma pack()

	uint32_t selfShadowCount = 0;
	fread(&selfShadowCount, sizeof(selfShadowCount), 1, vmdFile);

	std::vector<VMDSelfShadow>
		selfShadowData(selfShadowCount);
	fread(selfShadowData.data(), sizeof(VMDSelfShadow), selfShadowCount, vmdFile);

	//IKをフレームごとに切るかどうかのデータ
	uint32_t ikSwitchCount = 0;
	fread(&ikSwitchCount, sizeof(ikSwitchCount), 1, vmdFile);

	anime->ikEnableData.resize(ikSwitchCount);
	for (auto& ikEnable : anime->ikEnableData) {
		fread(&ikEnable.frameNo, sizeof(uint32_t), 1, vmdFile);
		//可視フラグ。使わない
		uint8_t visibleFlg = 0;
		fread(&visibleFlg, sizeof(visibleFlg), 1, vmdFile);

		//対象ボーン数
		uint32_t ikBoneCount = 0;
		fread(&ikBoneCount, sizeof(ikBoneCount), 1, vmdFile);

		//ikenableデータ読み込み
		for (unsigned int i = 0; i < ikBoneCount; i++) {
			char ikBoneName[20];
			fread(ikBoneName, _countof(ikBoneName), 1, vmdFile);//名前
			uint8_t flg = 0;
			fread(&flg, sizeof(flg), 1, vmdFile);//フラグ
			ikEnable.ikEnableTable[ikBoneName] = flg;
		}
	}

	fclose(vmdFile);

	return true;
}

HRESULT
ModelLoader::CreateMaterialData(Model* model) {
	//マテリアル定数バッファの作成
	auto matBuffSize = AllignToMultipleOf256(
		sizeof(model->materialInfos_[0].material)
	);

	auto heapprop =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto resdesc =
		CD3DX12_RESOURCE_DESC::Buffer(matBuffSize * model->materialInfos_.size());

	auto result = dx12_->Dev()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(model->materialParams_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	//マテリアルデータをマップ、コピー
	char* mappedMat = nullptr;
	result = model->materialParams_->Map(
		0,
		nullptr,
		(void**)&mappedMat
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	for (auto& matInfo : model->materialInfos_) {
		*(MaterialForHlsl*)mappedMat = matInfo.material;
		mappedMat += matBuffSize;//次の位置へ
	}
	model->materialParams_->Unmap(0, nullptr);

	return S_OK;
}

HRESULT//マテリアルの定数バッファと画像バッファ４つのビューをまとめて作る
ModelLoader::CreateMaterialAndTextureView(Model* model) {
	//Descriptor Heap Desc作製
	D3D12_DESCRIPTOR_HEAP_DESC descheapdesc = {};
	descheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descheapdesc.NodeMask = 0;
	//マテリアルの数だけ
	//(7の内訳はmaterial,spa,sph,texture,toon,normal,ao)
	descheapdesc.NumDescriptors = model->materialInfos_.size() * 7;
	descheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	auto result = dx12_->Dev()->CreateDescriptorHeap(
		&descheapdesc,
		IID_PPV_ARGS(model->materialHeap_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	//View作製
	//マテリアルの定数バッファのビューデスク
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = model->materialParams_->GetGPUVirtualAddress();
	auto matBuffSize = AllignToMultipleOf256(
		sizeof(MaterialForHlsl)
	);
	cbvDesc.SizeInBytes = matBuffSize;

	//マテリアルの画像バッファのビューデスク
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	CD3DX12_CPU_DESCRIPTOR_HANDLE matHeapHandle(
		model->materialHeap_->GetCPUDescriptorHandleForHeapStart());
	auto incSize = dx12_->Dev()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	//ループでアドレスを進めながらViewを作っていく
	for (int i = 0; i < model->materialInfos_.size(); ++i) {
		//MaterialのCBVを作製
		dx12_->Dev()->CreateConstantBufferView(&cbvDesc, matHeapHandle);
		matHeapHandle.ptr += incSize;//次の場所に移動する
		cbvDesc.BufferLocation += matBuffSize;//次の要素に移動する
		//tex
		if (model->textureImgs_[i] == nullptr) {
			//テクスチャが空だと見た目が可笑しくなるので
			//真っ白な画像をテクスチャにする
			srvDesc.Format = whiteTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				whiteTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		else {
			//テクスチャがあるのでそれを使う
			srvDesc.Format = model->textureImgs_[i]->GetDesc().Format;
			auto texRes = model->textureImgs_[i].Get();
			dx12_->Dev()->CreateShaderResourceView(
				texRes,
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//次に進める
		//sph
		if (model->sphImgs_[i] == nullptr) {
			//テクスチャが空だと見た目が可笑しくなるので
			//真っ白な画像をテクスチャにする
			srvDesc.Format = whiteTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				whiteTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		else {
			//テクスチャがあるのでそれを使う
			srvDesc.Format = model->sphImgs_[i]->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				model->sphImgs_[i].Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//次に進める
		//spa
		if (model->spaImgs_[i] == nullptr) {
			//テクスチャが空だと見た目が可笑しくなるので
			//真っ黒な画像をテクスチャにする
			srvDesc.Format = blackTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				blackTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		else {
			//テクスチャがあるのでそれを使う
			srvDesc.Format = model->spaImgs_[i]->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				model->spaImgs_[i].Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//次に進める
		//toon
		if (model->toonImgs_[i] == nullptr) {
			//テクスチャが空だと見た目が可笑しくなるので
			//グラデーション画像をテクスチャにする
			srvDesc.Format = gradTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				gradTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		
		else {
			//テクスチャがあるのでそれを使う
			srvDesc.Format = model->toonImgs_[i]->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				model->toonImgs_[i].Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//次に進める
		//normal
		if (model->normalImgs_[i] == nullptr) {
			//テクスチャが空だと見た目が可笑しくなるので
			//グラデーション画像をテクスチャにする
			srvDesc.Format = normalTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				normalTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}

		else {
			//テクスチャがあるのでそれを使う
			srvDesc.Format = model->normalImgs_[i]->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				model->normalImgs_[i].Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//次に進める
		//ao
		if (model->aoImgs_[i] == nullptr) {
			//テクスチャが空だと見た目が可笑しくなるので
			//白画像をテクスチャにする
			srvDesc.Format = whiteTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				whiteTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}

		else {
			//テクスチャがあるのでそれを使う
			srvDesc.Format = model->aoImgs_[i]->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				model->aoImgs_[i].Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//次に進める

	}
	return result;
}

ComPtr<ID3D12Resource>
ModelLoader::CreateWriteBackTexture(size_t width, size_t height) {
	ComPtr<ID3D12Resource> texbuff = nullptr;
	auto resdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		width,
		height
	);
	auto heapprop = CD3DX12_HEAP_PROPERTIES(
		D3D12_CPU_PAGE_PROPERTY_WRITE_BACK,
		D3D12_MEMORY_POOL_L0
	);
	auto result = dx12_->Dev()->CreateCommittedResource(
		&heapprop,
		D3D12_HEAP_FLAG_NONE,
		&resdesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(&texbuff)
	);
	if (FAILED(result)) {
		assert(0);
		return nullptr;
	}
	return texbuff;
}

ComPtr<ID3D12Resource>
ModelLoader::CreateWhiteTexture() {
	//一辺の長さ
	const size_t s = 4;
	auto whitebuff = CreateWriteBackTexture(s, s);
	//白のデータ(255,255,255,255)をs*s個分
	std::vector<unsigned char> wdata(s * s * 4);
	std::fill(wdata.begin(), wdata.end(), 0xff);

	//転送
	auto result = whitebuff->WriteToSubresource(
		0,
		nullptr,
		wdata.data(),
		4 * s,
		wdata.size()
	);
	if (FAILED(result)) {
		assert(0);
		return nullptr;
	}
	return whitebuff;
}

ComPtr<ID3D12Resource>
ModelLoader::CreateBlackTexture() {
	//一辺の長さ
	const size_t s = 4;
	auto blackbuff = CreateWriteBackTexture(s, s);
	//黒のデータ(0,0,0,0)をs*s個分
	std::vector<unsigned char> wdata(s * s * 4);
	std::fill(wdata.begin(), wdata.end(), 0x00);

	//転送
	auto result = blackbuff->WriteToSubresource(
		0,
		nullptr,
		wdata.data(),
		4 * s,
		wdata.size()
	);
	if (FAILED(result)) {
		assert(0);
		return nullptr;
	}
	return blackbuff;
}

ComPtr<ID3D12Resource>
ModelLoader::CreateGrayGradationTexture() {
	//縦で上が白くて下が黒いグラデーションテクスチャを作る
	const size_t w = 4;
	const size_t h = 256;
	auto ggradbuff = CreateWriteBackTexture(w, h);
	//ジョジョに暗くしていく.4byteごとに埋めるのでunsigned int
	std::vector<unsigned int> ggraddata(w * h);
	auto it = ggraddata.begin();
	unsigned char c = 0xff;
	//4個づつ埋めていく
	for (; it != ggraddata.end(); it += w) {
		//実際の並びはRGBAではなくARGBなので
		//Aを頭に置く
		auto col = (0xff << 24) | RGB(c, c, c);
		std::fill(it, it + w, col);
		--c;
	}

	//転送
	auto result = ggradbuff->WriteToSubresource(
		0,
		nullptr,
		ggraddata.data(),
		4 * w,
		ggraddata.size()
	);
	if (FAILED(result)) {
		assert(0);
		return nullptr;
	}
	return ggradbuff;
}

//z方向だけ1の法線マップ
ComPtr<ID3D12Resource>
ModelLoader::CreateNormalTexture() {
	//一辺の長さ
	const size_t s = 4;
	auto normalbuff = CreateWriteBackTexture(s, s);
	//データ(0.5,0.5,1.0,1.0)をs*s個分
	std::vector<unsigned int> ndata(s * s);
	auto it = ndata.begin();
	//実際の並びはRGBAではなくARGBなので
	//Aを頭に置く
	unsigned int col = (0xff << 24) | RGB(0x7F, 0x7F, 0xff);
	std::fill(ndata.begin(), ndata.end(), col);


	//転送
	auto result = normalbuff->WriteToSubresource(
		0,
		nullptr,
		ndata.data(),
		4 * s,
		ndata.size()
	);
	if (FAILED(result)) {
		assert(0);
		return nullptr;
	}
	return normalbuff;
}

bool
ModelLoader::CreateDefaultTextures() {
	whiteTex_ = CreateWhiteTexture();
	blackTex_ = CreateBlackTexture();
	gradTex_ = CreateGrayGradationTexture();
	normalTex_ = CreateNormalTexture();
	if (!whiteTex_ || !blackTex_ || !gradTex_ || !normalTex_) {
		assert(0);
		return false;
	}
	return true;
}
