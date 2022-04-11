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
	//�t�@�C�����烂�f����ǂݍ���
	ModelLoader();
	bool LoadPMDModel(const char* fileName, Model* mesh);
	bool LoadPMXModel(const char* fileName, Model* mesh);

	//���_�z�񂩂烂�f�������
	bool CreateVerticesModel(const std::vector<VertexData>& vertices,
		const std::vector<Index>& indecies, const std::vector<Material>& material,
		Model* model);
	//�摜�X�v���C�g�̃��f�������
	bool LoadSprite(const char* fileName, Sprite* sprite);
	//�A�j���[�V�����̃f�[�^�����
	bool LoadAnimation(const char* fileName, AnimationData* anime);
	//�f�t�H���g�ŗp�ӂ���e�N�X�`�������
	bool CreateDefaultTextures();
private:
	Dx12Wrapper* dx12_;

	//�o�b�t�@�����}�e���A���f�[�^���R�s�[
	HRESULT CreateMaterialData(Model*);
	//�}�e���A���ƃe�N�X�`���Q�̃r���[�����
	HRESULT CreateMaterialAndTextureView(Model*);

	//�f�t�H���g�̃e�N�X�`���p
	ComPtr<ID3D12Resource> whiteTex_ = nullptr;
	ComPtr<ID3D12Resource> blackTex_ = nullptr;
	ComPtr<ID3D12Resource> gradTex_ = nullptr;
	ComPtr<ID3D12Resource> normalTex_ = nullptr;
	//TODO:�����̊֐��̎���
	//WriteBack�^�C�v�̃e�N�X�`���\�悤���\�[�X�����
	ComPtr<ID3D12Resource>
		CreateWriteBackTexture(size_t width, size_t height);
	ComPtr<ID3D12Resource>
		CreateWhiteTexture();//���e�N�X�`���̐���
	ComPtr<ID3D12Resource>
		CreateBlackTexture();//���e�N�X�`���̐���
	ComPtr<ID3D12Resource>
		CreateGrayGradationTexture();//�O���[�O���f�[�V�����e�N�X�`���̐���
	ComPtr<ID3D12Resource>
		CreateNormalTexture();//�f�t�H���g�̖@���}�b�v�e�N�X�`���̐���
};

