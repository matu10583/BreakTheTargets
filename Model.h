#pragma once
#include "XMFLOAT_Helper.h"
#include <wrl.h>
#include <vector>
#include <d3d12.h>
#include<DirectXMath.h>
#include <string>
#include <unordered_map>

//-----------------------------------
//�ȉ��\����
		//�V�F�[�_���ɓ�������}�e���A���f�[�^
struct MaterialForHlsl {
	Vector3 diffuse; //�f�B�t���[�Y�F
	float alpha; // �f�B�t���[�Y��
	Vector3 specular; //�X�y�L�����F
	float specularity;//�X�y�L�����̋���(��Z�l)
	Vector3 ambient; //�A���r�G���g�F
};
//����ȊO�̃}�e���A���f�[�^
struct AdditionalMaterial {
	std::string texPath;//�e�N�X�`���t�@�C���p�X
	std::string normalPath;//�@���}�b�v�̃e�N�X�`���̃p�X
	std::string aoPath;//ao�}�b�v�̃e�N�X�`���̃p�X
	int toonIdx; //�g�D�[���ԍ�
	bool edgeFlg;//�}�e���A�����̗֊s���t���O
};
//�܂Ƃ߂�����
struct Material {
	unsigned int indicesNum;//�C���f�b�N�X��
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
	uint32_t boneIdx;//�{�[���C���f�b�N�X
	uint32_t boneType;//�{�[�����
	uint32_t ikParentBone;//�e�{�[��
	Vector3 startPos;//�{�[����_
	Vector3 endPos;//�{�[����[�X
	std::vector<BoneNode*> children;//�q
};

struct PMDIK {
	uint16_t boneIdx;	//IK�Ώۂ̃{�[��������
	uint16_t targetIdx;	//�^�[�Q�b�g�ɋ߂Â��邽�߂̃{�[���̃C���f�b�N�X
	uint16_t iterations;	//�n�D����
	float limit;	//��񓖂���̉�]����
	std::vector<uint16_t> nodeIdxes;	//�m�[�h�ԍ�
};



//---------------------------------------------------

struct Model
{
	template<typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	//�S�̂̃C���f�b�N�X��
	unsigned int indicesNum_;
	//���_�z��
	std::vector<Vector3> vertices_;
	//�}�e���A���̏��
	std::vector<Material> materialInfos_;
	//���ꂼ��̃}�e���A���̃r���[��u��heap
	ComPtr<ID3D12DescriptorHeap> materialHeap_;

	//�e�N�X�`���摜�̃��\�[�X
	std::vector<
		ComPtr<ID3D12Resource>
	> textureImgs_;
	//sph�摜
	std::vector<
		ComPtr<ID3D12Resource>
	> sphImgs_;
	//spa�摜
	std::vector<
		ComPtr<ID3D12Resource>
	> spaImgs_;
	//�@���}�b�v�摜
	std::vector<
		ComPtr<ID3D12Resource>
	> normalImgs_;
	//AO�}�b�v�摜
	std::vector<
		ComPtr<ID3D12Resource>
	> aoImgs_;

	//toon�V�F�[�f�B���O�摜
	std::vector<
		ComPtr<ID3D12Resource>
	> toonImgs_;
	//�}�e���A���̒萔
	ComPtr<ID3D12Resource> materialParams_;


	//vertex buffer
	ComPtr<ID3D12Resource> vb_;
	D3D12_VERTEX_BUFFER_VIEW vbView_;
	//index buffer
	ComPtr<ID3D12Resource> ib_;
	D3D12_INDEX_BUFFER_VIEW ibView_;

	//�{�[�����
	//�m�[�h�𖼑O���猟���ł���悤�ɂ���
	std::unordered_map<std::string, BoneNode> boneNodeTable_;
	//���O�ƃC���f�b�N�X�̑Ή�
	std::vector<std::string> boneNameArray_;
	//�C���f�b�N�X�ƃm�[�h�̑Ή�
	std::vector<BoneNode*> boneNodeAddressArray_;

	std::vector<PMDIK> ikData_;
	//���O��"�Ђ�"���܂ރm�[�h�̃C���f�b�N�X
	std::vector<uint32_t> kneeIdxes_;

};

//VMD�p�\����
	//IK on/off
struct VMDIKEnable {
	uint32_t frameNo;//�t���[���ԍ�
	std::unordered_map<std::string, bool>
		ikEnableTable;//���O�ƃt���O�̃}�b�v
};

struct KeyFrame {
	unsigned int frameNo;//���t���[���ڂ�
	XMVECTOR quaternion;//�N�I�[�^�j�I��
	Vector3 offset;
	Vector2 p1, p2;//�x�W�F�Ȑ��̃R���g���[���|�C���g
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
	unsigned int duration;//�A�j���[�V�����̍ő�̂ӂ�[�ނ���
	std::vector<VMDIKEnable> ikEnableData;
};