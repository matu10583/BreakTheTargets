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
		unsigned int idx = path.rfind(L'.');
		return path.substr(idx + 1, path.length() - idx - 1);
	}

	//�e�N�X�`���̃p�X���Z�p���[�^�����ŕ�������
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
	//TODO:PMD�t�@�C���̃p�[�X�Ɠǂݍ��݂̎���
	//PMD�t�@�C����ǂݍ���
	FILE* pmdFile;
	std::string strModelPath = fileName;
	fopen_s(&pmdFile, strModelPath.c_str(), "rb");
	if (pmdFile == nullptr) {
		assert(0);
		fclose(pmdFile);
		return false;
	}

	//�w�b�_�[�ǂݍ���
	struct PMDHeader {
		float version;
		char model_name[20];
		char comment[256];
	};
	PMDHeader pmdheader = {};
	char signature[3];
	fread(signature, sizeof(signature), 1, pmdFile);
	fread(&pmdheader, sizeof(PMDHeader), 1, pmdFile);

	//���_��
	unsigned int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, pmdFile);

#pragma pack(1)//�ǂݍ��݂̂��߂�1�o�C�g�p�b�L���O
	//PMD�}�e���A���\����
	struct PMDMaterial
	{
		Vector3 diffuse;//�g�U�F
		float dAlpha;//�f�B�t���[�Y�̃��l
		float specularity;//�X�y�L�����̋����i��Z�j
		Vector3 specular;//�X�y�L�����F
		Vector3 ambient;//�A���r�G���g�F
		unsigned char toonIdx;//�g�D�[���ԍ�
		unsigned char edgeFlg;//�}�e���A�����̗֊s���t���O
		unsigned int indicesNum;//�}�e���A�������蓖�Ă���C���f�b�N�X��
		char texFilePath[20];//�e�N�X�`���t�@�C����
	};

	struct PMDVertex
	{
		float pos[3]; // x, y, z // ���W
		float normal_vec[3]; // nx, ny, nz // �@���x�N�g��
		float uv[2]; // u, v // UV���W // MMD�͒��_UV
		WORD bone_num[2]; // �{�[���ԍ�1�A�ԍ�2 // ���f���ό`(���_�ړ�)���ɉe��
		BYTE bone_weight; // �{�[��1�ɗ^����e���x // min:0 max:100 // �{�[��2�ւ̉e���x�́A(100 - bone_weight)
		BYTE edge_flag; // 0:�ʏ�A1:�G�b�W���� // �G�b�W(�֊s)���L���̏ꍇ
	};

#pragma pack()//1�o�C�g�p�b�L���O�I���



	constexpr unsigned int pmdvertex_size = sizeof(PMDVertex);//���_�������̃T�C�Y
	std::vector<PMDVertex> vertices(vertNum);//���_�p�o�b�t�@
	fread(vertices.data(), pmdvertex_size, vertices.size(), pmdFile);//���_�ǂݍ���

	unsigned int indicesNum;//�S�̂̃C���f�b�N�X��
	fread(&indicesNum, sizeof(indicesNum), 1, pmdFile);
	
	//�S�̂̃C���f�b�N�X���̋L�^
	model->indicesNum_ = indicesNum;

	//���_�o�b�t�@�쐻(Map��UPLOAD����
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

	//Map�p�ϐ�
	unsigned char* vertMap = nullptr;
	result = model->vb_->Map(0, nullptr, (void**)&vertMap);
	//���_�z���ۑ�
	model->vertices_.resize(vertNum);
	DWORD i = 0;
	for (auto& v : vertices) {
		model->vertices_[i] = Vector3(v.pos[0], v.pos[1], v.pos[2]);
		*(PMDVertex*)vertMap = v;
		vertMap += pmdvertex_size;
		i++;
	}
	model->vb_->Unmap(0, nullptr);

	//vertex buffer view�̍쐻
	//�o�b�t�@�̉��z�A�h���X
	model->vbView_.BufferLocation = model->vb_->GetGPUVirtualAddress();
	//�S�T�C�Y
	model->vbView_.SizeInBytes = vertices.size() * pmdvertex_size;
	//�꒸�_������̃T�C�Y
	model->vbView_.StrideInBytes = pmdvertex_size;

	//indices�̓ǂݍ���
	std::vector<unsigned short>indices(indicesNum);
	fread(indices.data(), indices.size() * sizeof(indices[0]), 1, pmdFile);

	//indices�p�̃o�b�t�@�iUPLOAD�j�쐻
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

	//Map�p�ϐ�
	unsigned short* mappedIdx = nullptr;
	model->ib_->Map(0, nullptr, (void**)&mappedIdx);
	std::copy(indices.begin(), indices.end(), mappedIdx);
	model->ib_->Unmap(0, nullptr);

	//index buffer view�̍쐻
	model->ibView_.BufferLocation = model->ib_->GetGPUVirtualAddress();
	model->ibView_.Format = DXGI_FORMAT_R16_UINT;
	model->ibView_.SizeInBytes = indices.size() * sizeof(indices[0]);

	//�}�e���A���̐�
	unsigned int materialNum;
	fread(&materialNum, sizeof(materialNum), 1, pmdFile);

	//vector�̃T�C�Y�����߂�
	model->toonImgs_.resize(materialNum);
	model->sphImgs_.resize(materialNum);
	model->spaImgs_.resize(materialNum);
	model->textureImgs_.resize(materialNum);
	model->materialInfos_.resize(materialNum);
	model->normalImgs_.resize(materialNum);
	model->aoImgs_.resize(materialNum);

	//�}�e���A�����擾
	std::vector<PMDMaterial> pmdMaterials(materialNum);
	fread(pmdMaterials.data(), pmdMaterials.size() * sizeof(pmdMaterials[0]), 1, pmdFile);


	for (int i = 0; i < pmdMaterials.size(); ++i) {
		//�}�e���A���̏����L�^
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
		//AO�Ɩ@���}�b�v�͎g��Ȃ�
		materialInfo.additional.normalPath = "";
		materialInfo.additional.aoPath = "";

		//�g�D�[�����\�[�X�̎擾
		char toonFilePath[32];
		sprintf_s(toonFilePath, "toon/toon%02d.bmp", pmdMaterials[i].toonIdx + 1);
		model->toonImgs_[i] = dx12_->GetImageResourceByPath(toonFilePath);

		//�e�N�X�`���̎w�肪�Ȃ���΋�ɂ���
		//�i�������Ƃ��������̂Ō�łȂɂ����������j
		if (strlen(pmdMaterials[i].texFilePath) == 0) {
			model->textureImgs_[i] = nullptr;
			continue;
		}

		std::string texFileName = pmdMaterials[i].texFilePath;
		std::string sphFileName = "";
		std::string spaFileName = "";
		//*�ŉ摜�ƃX�t�B�A�}�b�v���������Ă���̂ŒT��
		//��jbody.bmp*eye.spa
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
		//���f���ƃe�N�X�`���p�X���瑊�΃p�X�𓾂�
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

	//�}�e���A���f�[�^���o�b�t�@�ɃR�s�[
	result = CreateMaterialData(model);
	if (FAILED(result)) {
		assert(0);
		return false;
	}
	
	//�}�e���A���̃r���[�����
	result = CreateMaterialAndTextureView(model);
	if (FAILED(result)) {
		assert(0);
		return false;
	}

	//TODO:�{�[����IK�̓ǂݍ��݂���������
	//�{�[�����̓ǂݍ���
	//�{�[���̐�
	unsigned short bonenum = 0;
	fread(&bonenum, sizeof(bonenum), 1, pmdFile);

	//bone���
#pragma pack(1)
	struct PMDBone {
		char boneName[20];//�{�[����
		unsigned short parentNo;//�e�{�[���ԍ�
		unsigned short nextNo;//��[�̃{�[���ԍ�

		unsigned char type;//�{�[�����(�Ƃ肠�����l�̂̃��[�V���������Ȃ̂ŉ�]����)
		//Todo:��]�ȊO�̃{�[���ɂ��Ή�����
		unsigned short ikBoneNo;//IK�{�[���ԍ�
		XMFLOAT3 pos;//�{�[���̊�_�̍��W
	};
#pragma pack()

	std::vector<PMDBone> pmdBones(bonenum);
	fread(
		pmdBones.data(), sizeof(PMDBone), pmdBones.size(), pmdFile);

	std::vector<std::string> boneNames(bonenum);
	model->boneNameArray_.resize(bonenum);
	model->boneNodeAddressArray_.resize(bonenum);
	model->kneeIdxes_.clear();

	//�m�[�h�����
	for (int i = 0; i < bonenum; i++) {
		auto& pb = pmdBones[i];
		boneNames[i] = pb.boneName;
		auto& node = model->boneNodeTable_[pb.boneName];
		node.boneIdx = i;
		node.startPos = pb.pos;

		model->boneNameArray_[i] = pb.boneName;
		if (model->boneNameArray_[i].find("�Ђ�") != std::string::npos) {
			//pmd�̎g�p�Ƃ��ĂЂ��̕�������܂ރ{�[�����p�x���������̂Ŋo����
			model->kneeIdxes_.emplace_back(i);
		}
		model->boneNodeAddressArray_[i] = &node;
	}
	//�c���[�̍\�z
	for (auto& pb : pmdBones) {
		if (pb.parentNo >= pmdBones.size()) {
			//�����Ȕԍ��Ȃ̂Őe�͂Ȃ�
			continue;
		}
		//���̃{�[����e�̎q�x�N�^�ɒǉ�����
		auto pName = boneNames[pb.parentNo];
		model->boneNodeTable_[pName].children.emplace_back(
			&model->boneNodeTable_[pb.boneName]
		);
		model->boneNodeTable_[pb.boneName].ikParentBone = pb.parentNo;
	}

	//ik�f�[�^�̓ǂݍ���
	uint16_t iknum = 0;
	fread(&iknum, sizeof(iknum), 1, pmdFile);

	model->ikData_.resize(iknum);

	//ik�f�[�^���Ƃ��Ă���
	for (auto& ik : model->ikData_) {
		fread(&ik.boneIdx, sizeof(ik.boneIdx), 1, pmdFile);
		fread(&ik.targetIdx, sizeof(ik.targetIdx), 1, pmdFile);
		uint8_t chainLen = 0;
		fread(&chainLen, sizeof(chainLen), 1, pmdFile);
		fread(&ik.iterations, sizeof(ik.iterations), 1, pmdFile);
		fread(&ik.limit, sizeof(ik.limit), 1, pmdFile);
		if (!chainLen) {
			//IK�`�F�[���Ȃ�
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

	//���\�[�X�쐻
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
	//vertex view�̍쐻
	model->vbView_.BufferLocation = model->vb_->GetGPUVirtualAddress();
	model->vbView_.SizeInBytes = vertices.size()*sizeof(VertexData);
	model->vbView_.StrideInBytes = sizeof(VertexData);
	//���_�͋L�^
	for (auto& vd: vertices) {
		model->vertices_.emplace_back(vd.pos);
	}

	//���_���̓]��
	VertexData* mappedVertex = nullptr;
	model->vb_->Map(0, nullptr,
		(void**)&mappedVertex);
	std::copy(vertices.begin(), vertices.end(), mappedVertex);
	model->vb_->Unmap(0, nullptr);

	//index buffer�̍쐻
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
	//index view�̍쐻
	model->ibView_.BufferLocation = model->ib_->GetGPUVirtualAddress();
	model->ibView_.SizeInBytes = indices.size() * sizeof(Index);
	model->ibView_.Format = DXGI_FORMAT_R16_UINT;
	//�C���f�b�N�X���̋L�^
	model->indicesNum_ = indices.size() * 3;

	//�C���f�b�N�X���̓]��
	Index* mappedIndex = nullptr;
	model->ib_->Map(0, nullptr,
		(void**)&mappedIndex);
	std::copy(indices.begin(), indices.end(), mappedIndex);
	model->ib_->Unmap(0, nullptr);

	//�}�e���A��
	model->materialInfos_ = material;
	unsigned int materialNum = material.size();
	//vector�̃T�C�Y�����߂�
	model->toonImgs_.resize(materialNum);
	model->sphImgs_.resize(materialNum);
	model->spaImgs_.resize(materialNum);
	model->textureImgs_.resize(materialNum);
	model->normalImgs_.resize(materialNum);
	model->aoImgs_.resize(materialNum);
	
	for (int i = 0; i < material.size(); ++i) {

		//�g�D�[�����\�[�X�̎擾
		char toonFilePath[32];
		sprintf_s(toonFilePath, "toon/toon%02d.bmp", material[i].additional.toonIdx + 1);
		model->toonImgs_[i] = dx12_->GetImageResourceByPath(toonFilePath);

		//�e�N�X�`���̎w�肪�Ȃ���΋�ɂ���
		//�i�������Ƃ��������̂Ō�łȂɂ����������j
		if (strlen(material[i].additional.texPath.c_str()) == 0) {
			model->textureImgs_[i] = nullptr;
			continue;
		}

		std::string texFileName = material[i].additional.texPath;
		std::string sphFileName = "";
		std::string spaFileName = "";
		std::string normalFileName = material[i].additional.normalPath;
		std::string aoFileName = material[i].additional.aoPath;
		//*�ŉ摜�ƃX�t�B�A�}�b�v���������Ă���̂ŒT��
		//��jbody.bmp*eye.spa
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
		//img�p�X�Ƃƃe�N�X�`���p�X���瑊�΃p�X�𓾂�
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

	//�}�e���A���f�[�^���o�b�t�@�ɃR�s�[
	result = CreateMaterialData(model);
	if (FAILED(result)) {
		assert(0);
		return false;
	}

	//�}�e���A���̃r���[�����
	result = CreateMaterialAndTextureView(model);
	if (FAILED(result)) {
		assert(0);
		return false;
	}


	return true;
}

//PMD�����@�\�𑕔����ĂȂ��̂�PMD�ɂł��Ȃ��@�\�͓ǂݔ�΂�
//PMD�ɕϊ��ł��邱�Ƃ��킩�����̂Ŏ������Ȃ�
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
		return false;//2.0�����ǂ�
	}

	byte Hlength;
	fread(&Hlength, sizeof(Hlength), 1, pmxFile);
	if (Hlength != 8) {
		fclose(pmxFile);
		assert(0);
		return false;//2.0�����ǂ�
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

	//���_
	int vertNum;
	fread(&vertNum, sizeof(vertNum), 1, pmxFile);
	
#pragma pack(1)//�ǂݍ��݂̂��߂�1�o�C�g�p�b�L���O
	//PMD�}�e���A���\����
	struct PMDMaterial
	{
		Vector3 diffuse;//�g�U�F
		float dAlpha;//�f�B�t���[�Y�̃��l
		float specularity;//�X�y�L�����̋����i��Z�j
		Vector3 specular;//�X�y�L�����F
		Vector3 ambient;//�A���r�G���g�F
		unsigned char toonIdx;//�g�D�[���ԍ�
		unsigned char edgeFlg;//�}�e���A�����̗֊s���t���O
		unsigned int indicesNum;//�}�e���A�������蓖�Ă���C���f�b�N�X��
		char texFilePath[20];//�e�N�X�`���t�@�C����
	};

	struct PMDVertex
	{
		float pos[3]; // x, y, z // ���W
		float normal_vec[3]; // nx, ny, nz // �@���x�N�g��
		float uv[2]; // u, v // UV���W // MMD�͒��_UV
		WORD bone_num[2]; // �{�[���ԍ�1�A�ԍ�2 // ���f���ό`(���_�ړ�)���ɉe��
		BYTE bone_weight; // �{�[��1�ɗ^����e���x // min:0 max:100 // �{�[��2�ւ̉e���x�́A(100 - bone_weight)
		BYTE edge_flag; // 0:�ʏ�A1:�G�b�W���� // �G�b�W(�֊s)���L���̏ꍇ
	};

#pragma pack()//1�o�C�g�p�b�L���O�I���

	std::vector<PMDVertex> vertices(vertNum);//���_�p�o�b�t�@

	//���_�̃{�[���ό`�����ɂ�邻�ꂼ��̃f�[�^�T�C�Y
	auto bsize = Hdata[BONE_INDEX_SIZE];
	std::array<int, 4> WeightSize = {
		bsize,//BDEF1
		bsize * 2 + 4,//BDEF2
		bsize * 4 + 4 * 4,//BDEF4
		bsize * 2 + 4 + 12 * 3//SDEF
	};

	for (auto& v : vertices) {
		fread(v.pos, sizeof(float), 3 + 3 + 2, pmxFile);
		fseek(pmxFile, 16 * Hdata[ADD_UV], SEEK_CUR);//�ǉ�UV
		byte w;//�E�F�C�g����
		fread(&w, sizeof(w), 1, pmxFile);
		fseek(pmxFile, WeightSize[w], SEEK_CUR);
	}
	//TODO:����
	return true;
}

bool
ModelLoader::LoadSprite(const char* fileName, Sprite* sprite) {
	//�摜�̃��[�h
	sprite->textureImg_ = dx12_->GetImageResourceByPath(fileName);
	auto resdesc = sprite->textureImg_->GetDesc();
	auto w = resdesc.Width;
	auto h = resdesc.Height;
	//�X�N���[�����W
	std::vector<VertexData2D> vd{
		{Vector3(0.0f, h, 0.0f), Vector2(0.0f, 1.0f)},//����
		{Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f)},//����
		{Vector3(w, h, 0.0f), Vector2(1.0f, 1.0f)},//�E��
		{Vector3(w, 0.0f, 0.0f), Vector2(1.0f, 0.0f)},//�E��
	};
	//���_�z��̋L�^
	sprite->vertices_.resize(4);
	for (int i = 0; i < 4; i++) {
		sprite->vertices_[i] = vd[i].pos;
	}
	//���_���̍쐬

	//���\�[�X�쐻
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
	//vertex view�̍쐻
	sprite->vbView_.BufferLocation = sprite->vb_->GetGPUVirtualAddress();
	sprite->vbView_.SizeInBytes = vd.size() * sizeof(VertexData2D);
	sprite->vbView_.StrideInBytes = sizeof(VertexData2D);

	//���_���̓]��
	VertexData2D* mappedVertex = nullptr;
	sprite->vb_->Map(0, nullptr,
		(void**)&mappedVertex);
	std::copy(vd.begin(), vd.end(), mappedVertex);
	sprite->vb_->Unmap(0, nullptr);

	//�e�N�X�`���r���[�쐻
	//Descriptor Heap Desc�쐻
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
	//�摜�o�b�t�@�̃r���[�f�X�N
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

	//�w�b�_�[���΂�
	fseek(vmdFile, 50, SEEK_SET);

	//���[�V�����f�[�^�̐�
	unsigned int motionNum = 0;
	fread(&motionNum, sizeof(motionNum), 1, vmdFile);

	//�f�[�^���[�h�p�\����
	struct VMDMotion {
		char boneName[15];//�{�[����
		//�����Ƀp�f�B���O������.
		unsigned int frameNo;//�t���[���ԍ�
		Vector3 location;//�ʒu
		XMFLOAT4 quaternion;//�N�H�[�^�j�I��
		unsigned char bezier[64];//�x�W�F���
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

	//�f�[�^�𐮌`���ēǂݍ���
	for (auto& mo : vmdMotionData) {
		auto quat = XMLoadFloat4(&mo.quaternion);
		//�x�W�F�̃R���g���[���|�C���g���W�𐳋K�����ċ��߂�
		//�{�[���͉�]�����Ȃ̂ŉ�]�̃|�C���g�������擾
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

	//���[�V�����f�[�^�𐮗񂳂���
	for (auto& mo : anime->motiondata) {
		std::sort(mo.second.begin(),
			mo.second.end(),
			[](const KeyFrame& lval, const KeyFrame& rval) {
				return (lval.frameNo <= rval.frameNo);
			}
		);
	}


	//�ȉ��̃f�[�^��IKEnable�܂Ŏg��Ȃ���seek���ł��Ȃ��̂Ŏl��
#pragma pack(1)
	//�\��f�[�^
	struct VMDMorph
	{
		char name[15];//���O
		uint32_t frameNo;//�t���[���ԍ�
		float weight;//�E�F�C�g(0~1)
	};
#pragma pack()

	uint32_t morphCount = 0;
	fread(&morphCount, sizeof(morphCount), 1, vmdFile);
	std::vector<VMDMorph> morphs(morphCount);
	fread(morphs.data(), sizeof(VMDMorph), morphCount, vmdFile);

#pragma pack(1)
	//�J����
	struct VMDCamera
	{
		uint32_t frameNo;//�t���[���ԍ�
		float distance;//����
		XMFLOAT3 pos;//���W
		XMFLOAT3 eulerAngle;//�I�C���[�p
		uint8_t Interpolation[24];
		uint32_t fov;//����p
		uint8_t persFlg;//�p�[�X�t���O
	};
#pragma pack()

	uint32_t vmdCameraCount = 0;
	fread(&vmdCameraCount, sizeof(vmdCameraCount), 1, vmdFile);

	std::vector<VMDCamera> cameraData(vmdCameraCount);
	fread(cameraData.data(), sizeof(VMDCamera), vmdCameraCount, vmdFile);

	struct VMDLight
	{
		uint32_t frameNo;//�t���[���ԍ�
		XMFLOAT3 rgb;//���C�g�F
		XMFLOAT3 vec;//����x�N�g���i���s�����j
	};

	uint32_t vmdLightCount = 0;
	fread(&vmdLightCount, sizeof(vmdLightCount), 1, vmdFile);

	std::vector<VMDLight> lights(vmdLightCount);
	fread(lights.data(), sizeof(VMDLight), vmdLightCount, vmdFile);

#pragma pack(1)
	//�Z���t�e�f�[�^
	struct VMDSelfShadow
	{
		uint32_t frameNo;//�t���[���ԍ�
		uint8_t mode;//�e���[�h(0:none, 1:mode1, 2:mode2)
		float distance;//����
	};
#pragma pack()

	uint32_t selfShadowCount = 0;
	fread(&selfShadowCount, sizeof(selfShadowCount), 1, vmdFile);

	std::vector<VMDSelfShadow>
		selfShadowData(selfShadowCount);
	fread(selfShadowData.data(), sizeof(VMDSelfShadow), selfShadowCount, vmdFile);

	//IK���t���[�����Ƃɐ؂邩�ǂ����̃f�[�^
	uint32_t ikSwitchCount = 0;
	fread(&ikSwitchCount, sizeof(ikSwitchCount), 1, vmdFile);

	anime->ikEnableData.resize(ikSwitchCount);
	for (auto& ikEnable : anime->ikEnableData) {
		fread(&ikEnable.frameNo, sizeof(uint32_t), 1, vmdFile);
		//���t���O�B�g��Ȃ�
		uint8_t visibleFlg = 0;
		fread(&visibleFlg, sizeof(visibleFlg), 1, vmdFile);

		//�Ώۃ{�[����
		uint32_t ikBoneCount = 0;
		fread(&ikBoneCount, sizeof(ikBoneCount), 1, vmdFile);

		//ikenable�f�[�^�ǂݍ���
		for (unsigned int i = 0; i < ikBoneCount; i++) {
			char ikBoneName[20];
			fread(ikBoneName, _countof(ikBoneName), 1, vmdFile);//���O
			uint8_t flg = 0;
			fread(&flg, sizeof(flg), 1, vmdFile);//�t���O
			ikEnable.ikEnableTable[ikBoneName] = flg;
		}
	}

	fclose(vmdFile);

	return true;
}

HRESULT
ModelLoader::CreateMaterialData(Model* model) {
	//�}�e���A���萔�o�b�t�@�̍쐬
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

	//�}�e���A���f�[�^���}�b�v�A�R�s�[
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
		mappedMat += matBuffSize;//���̈ʒu��
	}
	model->materialParams_->Unmap(0, nullptr);

	return S_OK;
}

HRESULT//�}�e���A���̒萔�o�b�t�@�Ɖ摜�o�b�t�@�S�̃r���[���܂Ƃ߂č��
ModelLoader::CreateMaterialAndTextureView(Model* model) {
	//Descriptor Heap Desc�쐻
	D3D12_DESCRIPTOR_HEAP_DESC descheapdesc = {};
	descheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descheapdesc.NodeMask = 0;
	//�}�e���A���̐�����
	//(7�̓����material,spa,sph,texture,toon,normal,ao)
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

	//View�쐻
	//�}�e���A���̒萔�o�b�t�@�̃r���[�f�X�N
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = model->materialParams_->GetGPUVirtualAddress();
	auto matBuffSize = AllignToMultipleOf256(
		sizeof(MaterialForHlsl)
	);
	cbvDesc.SizeInBytes = matBuffSize;

	//�}�e���A���̉摜�o�b�t�@�̃r���[�f�X�N
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	CD3DX12_CPU_DESCRIPTOR_HANDLE matHeapHandle(
		model->materialHeap_->GetCPUDescriptorHandleForHeapStart());
	auto incSize = dx12_->Dev()->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	//���[�v�ŃA�h���X��i�߂Ȃ���View������Ă���
	for (int i = 0; i < model->materialInfos_.size(); ++i) {
		//Material��CBV���쐻
		dx12_->Dev()->CreateConstantBufferView(&cbvDesc, matHeapHandle);
		matHeapHandle.ptr += incSize;//���̏ꏊ�Ɉړ�����
		cbvDesc.BufferLocation += matBuffSize;//���̗v�f�Ɉړ�����
		//tex
		if (model->textureImgs_[i] == nullptr) {
			//�e�N�X�`�����󂾂ƌ����ڂ��΂����Ȃ�̂�
			//�^�����ȉ摜���e�N�X�`���ɂ���
			srvDesc.Format = whiteTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				whiteTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		else {
			//�e�N�X�`��������̂ł�����g��
			srvDesc.Format = model->textureImgs_[i]->GetDesc().Format;
			auto texRes = model->textureImgs_[i].Get();
			dx12_->Dev()->CreateShaderResourceView(
				texRes,
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//���ɐi�߂�
		//sph
		if (model->sphImgs_[i] == nullptr) {
			//�e�N�X�`�����󂾂ƌ����ڂ��΂����Ȃ�̂�
			//�^�����ȉ摜���e�N�X�`���ɂ���
			srvDesc.Format = whiteTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				whiteTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		else {
			//�e�N�X�`��������̂ł�����g��
			srvDesc.Format = model->sphImgs_[i]->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				model->sphImgs_[i].Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//���ɐi�߂�
		//spa
		if (model->spaImgs_[i] == nullptr) {
			//�e�N�X�`�����󂾂ƌ����ڂ��΂����Ȃ�̂�
			//�^�����ȉ摜���e�N�X�`���ɂ���
			srvDesc.Format = blackTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				blackTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		else {
			//�e�N�X�`��������̂ł�����g��
			srvDesc.Format = model->spaImgs_[i]->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				model->spaImgs_[i].Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//���ɐi�߂�
		//toon
		if (model->toonImgs_[i] == nullptr) {
			//�e�N�X�`�����󂾂ƌ����ڂ��΂����Ȃ�̂�
			//�O���f�[�V�����摜���e�N�X�`���ɂ���
			srvDesc.Format = gradTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				gradTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		
		else {
			//�e�N�X�`��������̂ł�����g��
			srvDesc.Format = model->toonImgs_[i]->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				model->toonImgs_[i].Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//���ɐi�߂�
		//normal
		if (model->normalImgs_[i] == nullptr) {
			//�e�N�X�`�����󂾂ƌ����ڂ��΂����Ȃ�̂�
			//�O���f�[�V�����摜���e�N�X�`���ɂ���
			srvDesc.Format = normalTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				normalTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}

		else {
			//�e�N�X�`��������̂ł�����g��
			srvDesc.Format = model->normalImgs_[i]->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				model->normalImgs_[i].Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//���ɐi�߂�
		//ao
		if (model->aoImgs_[i] == nullptr) {
			//�e�N�X�`�����󂾂ƌ����ڂ��΂����Ȃ�̂�
			//���摜���e�N�X�`���ɂ���
			srvDesc.Format = whiteTex_->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				whiteTex_.Get(),
				&srvDesc,
				matHeapHandle
			);
		}

		else {
			//�e�N�X�`��������̂ł�����g��
			srvDesc.Format = model->aoImgs_[i]->GetDesc().Format;
			dx12_->Dev()->CreateShaderResourceView(
				model->aoImgs_[i].Get(),
				&srvDesc,
				matHeapHandle
			);
		}
		matHeapHandle.ptr += incSize;//���ɐi�߂�

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
	//��ӂ̒���
	const size_t s = 4;
	auto whitebuff = CreateWriteBackTexture(s, s);
	//���̃f�[�^(255,255,255,255)��s*s��
	std::vector<unsigned char> wdata(s * s * 4);
	std::fill(wdata.begin(), wdata.end(), 0xff);

	//�]��
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
	//��ӂ̒���
	const size_t s = 4;
	auto blackbuff = CreateWriteBackTexture(s, s);
	//���̃f�[�^(0,0,0,0)��s*s��
	std::vector<unsigned char> wdata(s * s * 4);
	std::fill(wdata.begin(), wdata.end(), 0x00);

	//�]��
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
	//�c�ŏオ�����ĉ��������O���f�[�V�����e�N�X�`�������
	const size_t w = 4;
	const size_t h = 256;
	auto ggradbuff = CreateWriteBackTexture(w, h);
	//�W���W���ɈÂ����Ă���.4byte���Ƃɖ��߂�̂�unsigned int
	std::vector<unsigned int> ggraddata(w * h);
	auto it = ggraddata.begin();
	unsigned char c = 0xff;
	//4�Â��߂Ă���
	for (; it != ggraddata.end(); it += w) {
		//���ۂ̕��т�RGBA�ł͂Ȃ�ARGB�Ȃ̂�
		//A�𓪂ɒu��
		auto col = (0xff << 24) | RGB(c, c, c);
		std::fill(it, it + w, col);
		--c;
	}

	//�]��
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

//z��������1�̖@���}�b�v
ComPtr<ID3D12Resource>
ModelLoader::CreateNormalTexture() {
	//��ӂ̒���
	const size_t s = 4;
	auto normalbuff = CreateWriteBackTexture(s, s);
	//�f�[�^(0.5,0.5,1.0,1.0)��s*s��
	std::vector<unsigned int> ndata(s * s);
	auto it = ndata.begin();
	//���ۂ̕��т�RGBA�ł͂Ȃ�ARGB�Ȃ̂�
	//A�𓪂ɒu��
	unsigned int col = (0xff << 24) | RGB(0x7F, 0x7F, 0xff);
	std::fill(ndata.begin(), ndata.end(), col);


	//�]��
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
