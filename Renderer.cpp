#include "Renderer.h"
#include "MeshComponent.h"
#include "SpriteComponent.h"
#include "Dx12Wrapper.h"
#include "Collider.h"
#include <tchar.h>
#include <assert.h>
#include <d3dx12.h>


Renderer* Renderer::instance;

bool 
Renderer::Init() {
	
	auto result = CreateRootSignatureForPMD();
	if (FAILED(result)) {
		assert(0);
		return false;
	}
	result = CreateGraphicsPipelineForPMD_Depth();
	if (FAILED(result)) {
		assert(0);
		return false;
	}

	result = CreateRootSignatureForVertices();
	if (FAILED(result)) {
		assert(0);
		return false;
	}
	result = CreateGraphicsPipelineForVertices();
	if (FAILED(result)) {
		assert(0);
		return false;
	}

	result = CreateRootSignatureForSprite();
	if (FAILED(result)) {
		assert(0);
		return false;
	}
	result = CreateGraphicsPipelineForSprite();
	if (FAILED(result)) {
		assert(0);
		return false;
	}
	
	return true;
}

void
Renderer::Draw() {
	//����p�����������Ă���
	ViewFrustum vf(dx12_->Projection3D(),
		dx12_->EyePos(), dx12_->EyeRot());

	//Depth�݂̂̕`��
	dx12_->BeginDrawToDepth();
	DrawDepth();

	//���Ԃ̃{�[�h�ԍ��ɕ`����
	std::list<unsigned int> boardIdxes;
	
	//board[0]�ւ̕`��
	boardIdxes.emplace_back(0);
	dx12_->BeginDrawToBoardsAndZBuff(boardIdxes, true);

	DrawModel(vf);
	DrawVert(vf);

	dx12_->EndDrawToBoardsAndZBuff(boardIdxes);

	//board1�ւ̕`��(deferred)
	boardIdxes.clear();
	boardIdxes.emplace_back(1);
	dx12_->BeginDrawToBoards(boardIdxes, false);

	dx12_->DrawDeferred(0);

	dx12_->EndDrawToBoards(boardIdxes);

	//board2�ւ̕`��
	boardIdxes.clear();
	boardIdxes.emplace_back(2);
	dx12_->BeginDrawToBoards(boardIdxes, false);

	dx12_->DrawBoard(1, 0);

	dx12_->EndDrawToBoards(boardIdxes);

	//buckbuffer�ւ̕`�揀��
	dx12_->BeginDrawToFinalRenderTarget();

	dx12_->DrawBoard(2, 1);
	DrawSprite();


#ifdef _DEBUG
	//imgui�̕`��
	dx12_->DrawImgui();
#endif // _DEBUG


	//�`�悪���ׂďI������̂ŉ�ʐ؂�ւ�
	dx12_->Flip();

	//�ǉ����̃I�u�W�F�N�g��ǉ�
	for (auto& p_obj : pendPmdMeshComponents) {
		pmdMeshComponents.emplace_back(p_obj);
	}
	pendPmdMeshComponents.clear();

	for (auto& p_obj : pendVertMeshComponents) {
		vertMeshComponents.emplace_back(p_obj);
	}
	pendVertMeshComponents.clear();

	for (auto& p_obj : pendSpriteComponents) {
		spriteComponents.emplace_back(p_obj);
	}
	pendSpriteComponents.clear();
	
}

void
Renderer::AddPMDMeshComponent(MeshComponent* mesh) {
	pendPmdMeshComponents.emplace_back(mesh);
}

void
Renderer::AddVertMeshComponent(MeshComponent* mesh) {
	pendVertMeshComponents.emplace_back(mesh);
}

void
Renderer::AddSpriteComponent(SpriteComponent* spr) {
	pendSpriteComponents.emplace_back(spr);
}

void
Renderer::RemoveMeshComponent(MeshComponent* mesh) {
	auto ppidx = std::find(pmdMeshComponents.begin(), pmdMeshComponents.end(), mesh);
	if (ppidx != pmdMeshComponents.end()) {
		pmdMeshComponents.erase(ppidx);
		return;
	}

	auto vvidx = std::find(vertMeshComponents.begin(), vertMeshComponents.end(), mesh);
	if (vvidx != vertMeshComponents.end()) {
		vertMeshComponents.erase(vvidx);
		return;
	}

	auto pidx = std::find(pendPmdMeshComponents.begin(), pendPmdMeshComponents.end(), mesh);
	if (pidx != pendPmdMeshComponents.end()) {
		pendPmdMeshComponents.erase(pidx);
		return;
	}

	auto vidx = std::find(pendVertMeshComponents.begin(), pendVertMeshComponents.end(), mesh);
	if (vidx != pendVertMeshComponents.end()) {
		pendVertMeshComponents.erase(vidx);
		return;
	}
	assert("�����meshcomponent�͂Ȃ���");
}

void
Renderer::RemoveSpriteComponent(SpriteComponent* spr) {
	auto pidx = std::find(pendSpriteComponents.begin(), pendSpriteComponents.end(), spr);
	if (pidx != pendSpriteComponents.end()) {
		pendSpriteComponents.erase(pidx);
		return;
	}

	auto idx = std::find(spriteComponents.begin(), spriteComponents.end(), spr);
	if (idx != spriteComponents.end()) {
		spriteComponents.erase(idx);
		return;
	}
	assert("�����pendSpritecomponent�͂Ȃ���");
}


void 
Renderer::Create() {
	assert(!instance && "�V���O���g���ł���I");
	instance = new Renderer();
}

void 
Renderer::Destroy() {
	delete instance;
	instance = nullptr;
}

Renderer*
Renderer::Instance() {
	return instance;
}

Renderer::Renderer() {
	dx12_ = Dx12Wrapper::Instance();
}

Renderer::~Renderer() {

}


void
Renderer::DrawModel(const struct ViewFrustum& vf) {

	//�O�p�`�̃��X�g��`�悷��
	dx12_->CommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);
	//���f���p�̃p�C�v���C���Ɓ�V�O�l�`����������
	dx12_->CommandList()->SetPipelineState(
		pipelineForPMD_.Get()
	);
	dx12_->CommandList()->SetGraphicsRootSignature(
		rootSignatureForPMD_.Get()
	);

	//���f����`�悷��̂ŃV�[���f�[�^��ݒ肷��
	dx12_->SetSceneData();
	for (auto pmdmc : pmdMeshComponents) {
		//������ɓ����Ă�Ȃ烂�f���̕`��
		if (Collide::Intersect(vf,
			pmdmc->GetWorldAABB())) {
			pmdmc->DrawPMD(false);
		}
	}
}

void
Renderer::DrawVert(const struct ViewFrustum& vf) {

	//�O�p�`�̃��X�g��`�悷��
	dx12_->CommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);
	//���_�z��p�̃p�C�v���C���Ɓ�V�O�l�`����������
	dx12_->CommandList()->SetPipelineState(
		pipelineForVert_.Get()
	);
	dx12_->CommandList()->SetGraphicsRootSignature(
		rootSignatureForVert_.Get()
	);

	//���f����`�悷��̂ŃV�[���f�[�^��ݒ肷��
	dx12_->SetSceneData();
	for (auto vertmc : vertMeshComponents) {
		//���f���̕`��
		if (Collide::Intersect(vf,
			vertmc->GetWorldAABB())) {
			vertmc->DrawVert(false);
		}
	}
}

void
Renderer::DrawSprite() {

	//�O�p�`�̃X�g���b�v��`�悷��
	dx12_->CommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
	);
	//���f���p�̃p�C�v���C���Ɓ�V�O�l�`����������
	dx12_->CommandList()->SetPipelineState(
		pipelineForSprite_.Get()
	);
	dx12_->CommandList()->SetGraphicsRootSignature(
		rootSignatureForSprite_.Get()
	);

	//Sprite��`�悷��̂�2d�V�[���f�[�^��ݒ肷��
	dx12_->SetSceneData2D();
	for (auto sc : spriteComponents) {
		//Sprite�̕`��
		sc->Draw();
	}

}

void
Renderer::DrawDepth() {
	auto cmdlist = dx12_->CommandList();
	//�O�p�`�̃X�g���b�v��`�悷��
	dx12_->CommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);
	cmdlist->SetPipelineState(pipelineForPMDDepth_.Get());
	cmdlist->SetGraphicsRootSignature(rootSignatureForPMD_.Get());

	//���f����`�悷��̂ŃV�[���f�[�^��ݒ肷��
	dx12_->SetSceneData();
	for (auto pmdmc : pmdMeshComponents) {
		//���f���̕`��
		pmdmc->DrawPMD(true);
	}

	//�O�p�`�̃X�g���b�v��`�悷��
	dx12_->CommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);
	cmdlist->SetPipelineState(pipelineForVertDepth_.Get());
	cmdlist->SetGraphicsRootSignature(rootSignatureForVert_.Get());

	//���f����`�悷��̂ŃV�[���f�[�^��ݒ肷��
	dx12_->SetSceneData();

	for (auto vmc : vertMeshComponents) {
		//���f���̕`��
		vmc->DrawVert(true);
	}
}

HRESULT
Renderer::CreateRootSignatureForPMD() {
	//�����W
	CD3DX12_DESCRIPTOR_RANGE descRanges[5] = {};
	//�萔�Fb0�i�r���[�v���W�F�N�V�����s��j
	descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//�萔�Fb1�i���W�ϊ��s��j
	descRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	//�萔�Fb2�i�}�e���A���萔�����j
	descRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	//�e�N�X�`���Ft0~t5�i�e�N�X�`���Aspa,sph,�g�D�[��, normal, ao�j
	descRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0);
	//�e�N�X�`���Ft6�ilightDepth�j
	descRanges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);

	//���[�g�p�����[�^�[
	CD3DX12_ROOT_PARAMETER rootParams[4] = {};
	rootParams[0].InitAsDescriptorTable(1, &descRanges[0]);//�r���[�v���_�N�V�����s��
	rootParams[1].InitAsDescriptorTable(1, &descRanges[1]);//���W�ϊ��s��
	rootParams[2].InitAsDescriptorTable(2, &descRanges[2]);//�}�e���A������
	rootParams[3].InitAsDescriptorTable(1, &descRanges[4]);//lightDepth

	//�T���v���[
	CD3DX12_STATIC_SAMPLER_DESC samplerDescs[3] = {};
	samplerDescs[0].Init(0);//s0
	samplerDescs[1].Init(1, D3D12_FILTER_ANISOTROPIC, 
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerDescs[2].Init(2,
		D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,//��r�����j�A���
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerDescs[2].ComparisonFunc =
		D3D12_COMPARISON_FUNC_LESS_EQUAL;//��r���@��<=
	samplerDescs[2].MaxAnisotropy = 1;//�[�x�X�΂�L��

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(
		4, rootParams,
		3, samplerDescs,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errblob = nullptr;
	auto result = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&rootSigBlob,
		&errblob
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	result = dx12_->Dev()->CreateRootSignature(
		0,
		rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(rootSignatureForPMD_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	return result;
}

HRESULT
Renderer::CreateGraphicsPipelineForPMD_Depth() {
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "BONE_NO",0,DXGI_FORMAT_R16G16_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "WEIGHT",0,DXGI_FORMAT_R8_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
	};

	ComPtr<ID3DBlob> vsBlob = 
		dx12_->GetVertexShaderResourceByPath("BasicVertexShader.hlsl", "BasicVS");
	ComPtr<ID3DBlob> psBlob = 
		dx12_->GetPixelShaderResourceByPath("BasicPixelShader.hlsl", "BasicPS");


	D3D12_GRAPHICS_PIPELINE_STATE_DESC plsDesc = {};
	plsDesc.pRootSignature = rootSignatureForPMD_.Get();
	plsDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	plsDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

	plsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//�T���v���}�X�N

	auto bldesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	bldesc.AlphaToCoverageEnable = true;
	bldesc.IndependentBlendEnable = true;
	bldesc.RenderTarget[0].BlendEnable = true;
	bldesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;//���Z
	bldesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//(1-a)
	bldesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;//a
	bldesc.RenderTarget[1].BlendEnable = false;
	//src*a+dest*(1-a)
	plsDesc.BlendState = bldesc;

	plsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	plsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�

	plsDesc.DepthStencilState.DepthEnable = true;//�[�x�o�b�t�@���g��
	plsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//�S�ď�������
	plsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//�����������̗p
	plsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	plsDesc.DepthStencilState.StencilEnable = false;

	plsDesc.InputLayout.pInputElementDescs = inputLayout;//���C�A�E�g�擪�A�h���X
	plsDesc.InputLayout.NumElements = _countof(inputLayout);//���C�A�E�g�z��

	plsDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
	plsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��

	plsDesc.NumRenderTargets = 2;//normal�o�b�t�@���o��
	plsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA
	plsDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA

	plsDesc.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
	plsDesc.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�
	plsDesc.NodeMask = 0;
	plsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	auto result = dx12_->Dev()->CreateGraphicsPipelineState(
		&plsDesc, 
		IID_PPV_ARGS(pipelineForPMD_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
	}

	//CreateDepthPipeline
	ComPtr<ID3DBlob> DvsBlob = dx12_->GetVertexShaderResourceByPath(
		"BasicVertexShader.hlsl",
		"DepthBasicVS"
	);
	plsDesc.VS = CD3DX12_SHADER_BYTECODE(DvsBlob.Get());
	plsDesc.PS.BytecodeLength = 0;//�s�N�Z���V�F�[�_�[�͂���Ȃ�
	plsDesc.PS.pShaderBytecode = nullptr;
	plsDesc.NumRenderTargets = 0;//�`�悵�Ȃ�
	plsDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	plsDesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;

	plsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	result = dx12_->Dev()->CreateGraphicsPipelineState(
		&plsDesc,
		IID_PPV_ARGS(pipelineForPMDDepth_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
	}
	return result;
}

HRESULT
Renderer::CreateGraphicsPipelineForVertices() {
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION",
		0,
		DXGI_FORMAT_R32G32B32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0},
		{ "NORMAL",
		0,
		DXGI_FORMAT_R32G32B32_FLOAT,
		0,D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0 },
		{"TEXCOORD",
		0,
		DXGI_FORMAT_R32G32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0},
	};

	ComPtr<ID3DBlob> vsBlob =
		dx12_->GetVertexShaderResourceByPath("BasicVertexShader.hlsl", "VertVS");
	ComPtr<ID3DBlob> psBlob =
		dx12_->GetPixelShaderResourceByPath("BasicPixelShader.hlsl", "VertPS");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC plsDesc = {};
	plsDesc.pRootSignature = rootSignatureForVert_.Get();
	plsDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	plsDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

	plsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//�T���v���}�X�N


	auto bldesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	bldesc.AlphaToCoverageEnable = true;
	bldesc.IndependentBlendEnable = true;
	bldesc.RenderTarget[1].BlendEnable = true;
	bldesc.RenderTarget[0].BlendEnable = true;
	bldesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;//���Z
	bldesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//(1-a)
	bldesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;//a
	//src*a+dest*(1-a)
	plsDesc.BlendState = bldesc;

	plsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	plsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	plsDesc.DepthStencilState.DepthEnable = true;//�[�x�o�b�t�@���g��
	plsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//�S�ď�������
	plsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//�����������̗p
	plsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	plsDesc.DepthStencilState.StencilEnable = false;

	plsDesc.InputLayout.pInputElementDescs = inputLayout;//���C�A�E�g�擪�A�h���X
	plsDesc.InputLayout.NumElements = _countof(inputLayout);//���C�A�E�g�z��

	plsDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
	plsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��

	plsDesc.NumRenderTargets = 2;//normal�o�b�t�@���o��
	plsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA
	plsDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA

	plsDesc.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
	plsDesc.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�
	plsDesc.NodeMask = 0;
	plsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	auto result = dx12_->Dev()->CreateGraphicsPipelineState(
		&plsDesc,
		IID_PPV_ARGS(pipelineForVert_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}

	//CreateDepthPipeline
	ComPtr<ID3DBlob> DvsBlob = dx12_->GetVertexShaderResourceByPath(
		"BasicVertexShader.hlsl",
		"DepthVertVS"
	);
	plsDesc.VS = CD3DX12_SHADER_BYTECODE(DvsBlob.Get());
	plsDesc.PS.BytecodeLength = 0;//�s�N�Z���V�F�[�_�[�͂���Ȃ�
	plsDesc.PS.pShaderBytecode = nullptr;
	plsDesc.NumRenderTargets = 0;//�`�悵�Ȃ�
	plsDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	plsDesc.RTVFormats[1] = DXGI_FORMAT_UNKNOWN;
	plsDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	result = dx12_->Dev()->CreateGraphicsPipelineState(
		&plsDesc,
		IID_PPV_ARGS(pipelineForVertDepth_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
		return result;
	}
	return result;
}

HRESULT
Renderer::CreateRootSignatureForVertices() {
	//�����W
	CD3DX12_DESCRIPTOR_RANGE ranges[5] = {};
	//�萔�Fb0�i�r���[�v���W�F�N�V�����s��j
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//�萔�Fb1�i���W�ϊ��s��j
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	//�萔�Fb2�i�}�e���A���萔�����j
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	//�e�N�X�`���Ft0~t5�i�e�N�X�`���Aspa,sph,�g�D�[��, normal, ao�j
	ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0);
	//�e�N�X�`���Ft6�ilightDepth�j
	ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);

	//root parameter
	CD3DX12_ROOT_PARAMETER rp[4];
	rp[0].InitAsDescriptorTable(1, &ranges[0]);
	rp[1].InitAsDescriptorTable(1, &ranges[1]);
	rp[2].InitAsDescriptorTable(2, &ranges[2]);
	rp[3].InitAsDescriptorTable(1, &ranges[4]);

	//�T���v���[
	CD3DX12_STATIC_SAMPLER_DESC samplerDescs[3] = {};
	samplerDescs[0].Init(0);//s0
	samplerDescs[1].Init(1, D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerDescs[2].Init(2,
		D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,//��r�����j�A���
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerDescs[2].ComparisonFunc =
		D3D12_COMPARISON_FUNC_LESS_EQUAL;//��r���@��<=
	samplerDescs[2].MaxAnisotropy = 1;//�[�x�X�΂�L��

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(
		4, rp,
		3, samplerDescs,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errblob = nullptr;
	auto result = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&rootSigBlob,
		&errblob
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	result = dx12_->Dev()->CreateRootSignature(
		0,
		rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(rootSignatureForVert_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	return result;
}



HRESULT
Renderer::CreateRootSignatureForSprite() {
	//�����W
	CD3DX12_DESCRIPTOR_RANGE descRanges[3] = {};
	//�萔�Fb0�i�v���W�F�N�V�����s��j
	descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//�萔�Fb1�i���W�ϊ��s��j
	descRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	//�e�N�X�`���Ft0�i�e�N�X�`���j
	descRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	//���[�g�p�����[�^�[
	CD3DX12_ROOT_PARAMETER rootParams[3] = {};
	rootParams[0].InitAsDescriptorTable(1, &descRanges[0]);//�r���[�v���_�N�V�����s��
	rootParams[1].InitAsDescriptorTable(1, &descRanges[1]);//���W�ϊ��s��
	rootParams[2].InitAsDescriptorTable(1, &descRanges[2]);//�e�N�X�`��

	//�T���v���[
	CD3DX12_STATIC_SAMPLER_DESC samplerDescs[1] = {};
	samplerDescs[0].Init(0);//s0

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init(
		3, rootParams,
		1, samplerDescs,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errblob = nullptr;
	auto result = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&rootSigBlob,
		&errblob
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}

	result = dx12_->Dev()->CreateRootSignature(
		0,
		rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(rootSignatureForSprite_.ReleaseAndGetAddressOf())
	);
	if (FAILED(result)) {
		assert(0);
		return result;
	}
	return result;
}

HRESULT
Renderer::CreateGraphicsPipelineForSprite() {
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
		{ "TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 },
};

	ComPtr<ID3DBlob> vsBlob =
		dx12_->GetVertexShaderResourceByPath("SpriteVertexShader.hlsl", "VS");
	ComPtr<ID3DBlob> psBlob =
		dx12_->GetPixelShaderResourceByPath("SpritePixelShader.hlsl", "PS");

	D3D12_GRAPHICS_PIPELINE_STATE_DESC plsDesc = {};
	plsDesc.pRootSignature = rootSignatureForSprite_.Get();
	plsDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	plsDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

	plsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//�T���v���}�X�N


	auto bl = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	bl.AlphaToCoverageEnable = true;
	bl.IndependentBlendEnable = false;
	bl.RenderTarget[0].BlendEnable = true;	
	bl.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;//���Z
	bl.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//(1-a)
	bl.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;//a
	plsDesc.BlendState = bl;


	plsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	plsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�

	plsDesc.DepthStencilState.DepthEnable = true;//�[�x�o�b�t�@���g��
	plsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//�S�ď�������
	plsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//�����������̗p
	plsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	plsDesc.DepthStencilState.StencilEnable = false;

	plsDesc.InputLayout.pInputElementDescs = inputLayout;//���C�A�E�g�擪�A�h���X
	plsDesc.InputLayout.NumElements = _countof(inputLayout);//���C�A�E�g�z��

	plsDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//�X�g���b�v���̃J�b�g�Ȃ�
	plsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//�O�p�`�ō\��

	plsDesc.NumRenderTargets = 1;
	plsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0�`1�ɐ��K�����ꂽRGBA

	plsDesc.SampleDesc.Count = 1;//�T���v�����O��1�s�N�Z���ɂ��P
	plsDesc.SampleDesc.Quality = 0;//�N�I���e�B�͍Œ�
	plsDesc.NodeMask = 0;
	plsDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	auto result = dx12_->Dev()->CreateGraphicsPipelineState(
		&plsDesc,
		IID_PPV_ARGS(pipelineForSprite_.ReleaseAndGetAddressOf()));
	if (FAILED(result)) {
		assert(SUCCEEDED(result));
	}
	return result;
}
