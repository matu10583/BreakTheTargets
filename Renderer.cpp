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
	//判定用視錘台を作っておく
	ViewFrustum vf(dx12_->Projection3D(),
		dx12_->EyePos(), dx12_->EyeRot());

	//Depthのみの描画
	dx12_->BeginDrawToDepth();
	DrawDepth();

	//何番のボード番号に描くか
	std::list<unsigned int> boardIdxes;
	
	//board[0]への描画
	boardIdxes.emplace_back(0);
	dx12_->BeginDrawToBoardsAndZBuff(boardIdxes, true);

	DrawModel(vf);
	DrawVert(vf);

	dx12_->EndDrawToBoardsAndZBuff(boardIdxes);

	//board1への描画(deferred)
	boardIdxes.clear();
	boardIdxes.emplace_back(1);
	dx12_->BeginDrawToBoards(boardIdxes, false);

	dx12_->DrawDeferred(0);

	dx12_->EndDrawToBoards(boardIdxes);

	//board2への描画
	boardIdxes.clear();
	boardIdxes.emplace_back(2);
	dx12_->BeginDrawToBoards(boardIdxes, false);

	dx12_->DrawBoard(1, 0);

	dx12_->EndDrawToBoards(boardIdxes);

	//buckbufferへの描画準備
	dx12_->BeginDrawToFinalRenderTarget();

	dx12_->DrawBoard(2, 1);
	DrawSprite();


#ifdef _DEBUG
	//imguiの描画
	dx12_->DrawImgui();
#endif // _DEBUG


	//描画がすべて終わったので画面切り替え
	dx12_->Flip();

	//追加町のオブジェクトを追加
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
	assert("そんなmeshcomponentはないよ");
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
	assert("そんなpendSpritecomponentはないよ");
}


void 
Renderer::Create() {
	assert(!instance && "シングルトンですよ！");
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

	//三角形のリストを描画する
	dx12_->CommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);
	//モデル用のパイプラインと√シグネチャをせっと
	dx12_->CommandList()->SetPipelineState(
		pipelineForPMD_.Get()
	);
	dx12_->CommandList()->SetGraphicsRootSignature(
		rootSignatureForPMD_.Get()
	);

	//モデルを描画するのでシーンデータを設定する
	dx12_->SetSceneData();
	for (auto pmdmc : pmdMeshComponents) {
		//視錘台に入ってるならモデルの描画
		if (Collide::Intersect(vf,
			pmdmc->GetWorldAABB())) {
			pmdmc->DrawPMD(false);
		}
	}
}

void
Renderer::DrawVert(const struct ViewFrustum& vf) {

	//三角形のリストを描画する
	dx12_->CommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);
	//頂点配列用のパイプラインと√シグネチャをせっと
	dx12_->CommandList()->SetPipelineState(
		pipelineForVert_.Get()
	);
	dx12_->CommandList()->SetGraphicsRootSignature(
		rootSignatureForVert_.Get()
	);

	//モデルを描画するのでシーンデータを設定する
	dx12_->SetSceneData();
	for (auto vertmc : vertMeshComponents) {
		//モデルの描画
		if (Collide::Intersect(vf,
			vertmc->GetWorldAABB())) {
			vertmc->DrawVert(false);
		}
	}
}

void
Renderer::DrawSprite() {

	//三角形のストリップを描画する
	dx12_->CommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
	);
	//モデル用のパイプラインと√シグネチャをせっと
	dx12_->CommandList()->SetPipelineState(
		pipelineForSprite_.Get()
	);
	dx12_->CommandList()->SetGraphicsRootSignature(
		rootSignatureForSprite_.Get()
	);

	//Spriteを描画するので2dシーンデータを設定する
	dx12_->SetSceneData2D();
	for (auto sc : spriteComponents) {
		//Spriteの描画
		sc->Draw();
	}

}

void
Renderer::DrawDepth() {
	auto cmdlist = dx12_->CommandList();
	//三角形のストリップを描画する
	dx12_->CommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);
	cmdlist->SetPipelineState(pipelineForPMDDepth_.Get());
	cmdlist->SetGraphicsRootSignature(rootSignatureForPMD_.Get());

	//モデルを描画するのでシーンデータを設定する
	dx12_->SetSceneData();
	for (auto pmdmc : pmdMeshComponents) {
		//モデルの描画
		pmdmc->DrawPMD(true);
	}

	//三角形のストリップを描画する
	dx12_->CommandList()->IASetPrimitiveTopology(
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
	);
	cmdlist->SetPipelineState(pipelineForVertDepth_.Get());
	cmdlist->SetGraphicsRootSignature(rootSignatureForVert_.Get());

	//モデルを描画するのでシーンデータを設定する
	dx12_->SetSceneData();

	for (auto vmc : vertMeshComponents) {
		//モデルの描画
		vmc->DrawVert(true);
	}
}

HRESULT
Renderer::CreateRootSignatureForPMD() {
	//レンジ
	CD3DX12_DESCRIPTOR_RANGE descRanges[5] = {};
	//定数：b0（ビュープロジェクション行列）
	descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//定数：b1（座標変換行列）
	descRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	//定数：b2（マテリアル定数部分）
	descRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	//テクスチャ：t0~t5（テクスチャ、spa,sph,トゥーン, normal, ao）
	descRanges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0);
	//テクスチャ：t6（lightDepth）
	descRanges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);

	//ルートパラメーター
	CD3DX12_ROOT_PARAMETER rootParams[4] = {};
	rootParams[0].InitAsDescriptorTable(1, &descRanges[0]);//ビュープロダクション行列
	rootParams[1].InitAsDescriptorTable(1, &descRanges[1]);//座標変換行列
	rootParams[2].InitAsDescriptorTable(2, &descRanges[2]);//マテリアル周り
	rootParams[3].InitAsDescriptorTable(1, &descRanges[4]);//lightDepth

	//サンプラー
	CD3DX12_STATIC_SAMPLER_DESC samplerDescs[3] = {};
	samplerDescs[0].Init(0);//s0
	samplerDescs[1].Init(1, D3D12_FILTER_ANISOTROPIC, 
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerDescs[2].Init(2,
		D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,//比較をリニア補間
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerDescs[2].ComparisonFunc =
		D3D12_COMPARISON_FUNC_LESS_EQUAL;//比較方法は<=
	samplerDescs[2].MaxAnisotropy = 1;//深度傾斜を有効

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

	plsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//サンプルマスク

	auto bldesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	bldesc.AlphaToCoverageEnable = true;
	bldesc.IndependentBlendEnable = true;
	bldesc.RenderTarget[0].BlendEnable = true;
	bldesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;//加算
	bldesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//(1-a)
	bldesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;//a
	bldesc.RenderTarget[1].BlendEnable = false;
	//src*a+dest*(1-a)
	plsDesc.BlendState = bldesc;

	plsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	plsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない

	plsDesc.DepthStencilState.DepthEnable = true;//深度バッファを使う
	plsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//全て書き込み
	plsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//小さい方を採用
	plsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	plsDesc.DepthStencilState.StencilEnable = false;

	plsDesc.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
	plsDesc.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列数

	plsDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	plsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	plsDesc.NumRenderTargets = 2;//normalバッファも出力
	plsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA
	plsDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	plsDesc.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	plsDesc.SampleDesc.Quality = 0;//クオリティは最低
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
	plsDesc.PS.BytecodeLength = 0;//ピクセルシェーダーはいらない
	plsDesc.PS.pShaderBytecode = nullptr;
	plsDesc.NumRenderTargets = 0;//描画しない
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

	plsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//サンプルマスク


	auto bldesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	bldesc.AlphaToCoverageEnable = true;
	bldesc.IndependentBlendEnable = true;
	bldesc.RenderTarget[1].BlendEnable = true;
	bldesc.RenderTarget[0].BlendEnable = true;
	bldesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;//加算
	bldesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//(1-a)
	bldesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;//a
	//src*a+dest*(1-a)
	plsDesc.BlendState = bldesc;

	plsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	plsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	plsDesc.DepthStencilState.DepthEnable = true;//深度バッファを使う
	plsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//全て書き込み
	plsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//小さい方を採用
	plsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	plsDesc.DepthStencilState.StencilEnable = false;

	plsDesc.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
	plsDesc.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列数

	plsDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	plsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	plsDesc.NumRenderTargets = 2;//normalバッファも出力
	plsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA
	plsDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	plsDesc.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	plsDesc.SampleDesc.Quality = 0;//クオリティは最低
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
	plsDesc.PS.BytecodeLength = 0;//ピクセルシェーダーはいらない
	plsDesc.PS.pShaderBytecode = nullptr;
	plsDesc.NumRenderTargets = 0;//描画しない
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
	//レンジ
	CD3DX12_DESCRIPTOR_RANGE ranges[5] = {};
	//定数：b0（ビュープロジェクション行列）
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//定数：b1（座標変換行列）
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	//定数：b2（マテリアル定数部分）
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	//テクスチャ：t0~t5（テクスチャ、spa,sph,トゥーン, normal, ao）
	ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0);
	//テクスチャ：t6（lightDepth）
	ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 6);

	//root parameter
	CD3DX12_ROOT_PARAMETER rp[4];
	rp[0].InitAsDescriptorTable(1, &ranges[0]);
	rp[1].InitAsDescriptorTable(1, &ranges[1]);
	rp[2].InitAsDescriptorTable(2, &ranges[2]);
	rp[3].InitAsDescriptorTable(1, &ranges[4]);

	//サンプラー
	CD3DX12_STATIC_SAMPLER_DESC samplerDescs[3] = {};
	samplerDescs[0].Init(0);//s0
	samplerDescs[1].Init(1, D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerDescs[2].Init(2,
		D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,//比較をリニア補間
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerDescs[2].ComparisonFunc =
		D3D12_COMPARISON_FUNC_LESS_EQUAL;//比較方法は<=
	samplerDescs[2].MaxAnisotropy = 1;//深度傾斜を有効

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
	//レンジ
	CD3DX12_DESCRIPTOR_RANGE descRanges[3] = {};
	//定数：b0（プロジェクション行列）
	descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	//定数：b1（座標変換行列）
	descRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	//テクスチャ：t0（テクスチャ）
	descRanges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	//ルートパラメーター
	CD3DX12_ROOT_PARAMETER rootParams[3] = {};
	rootParams[0].InitAsDescriptorTable(1, &descRanges[0]);//ビュープロダクション行列
	rootParams[1].InitAsDescriptorTable(1, &descRanges[1]);//座標変換行列
	rootParams[2].InitAsDescriptorTable(1, &descRanges[2]);//テクスチャ

	//サンプラー
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

	plsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//サンプルマスク


	auto bl = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	bl.AlphaToCoverageEnable = true;
	bl.IndependentBlendEnable = false;
	bl.RenderTarget[0].BlendEnable = true;	
	bl.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;//加算
	bl.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;//(1-a)
	bl.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;//a
	plsDesc.BlendState = bl;


	plsDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	plsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//カリングしない

	plsDesc.DepthStencilState.DepthEnable = true;//深度バッファを使う
	plsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//全て書き込み
	plsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//小さい方を採用
	plsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	plsDesc.DepthStencilState.StencilEnable = false;

	plsDesc.InputLayout.pInputElementDescs = inputLayout;//レイアウト先頭アドレス
	plsDesc.InputLayout.NumElements = _countof(inputLayout);//レイアウト配列数

	plsDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;//ストリップ時のカットなし
	plsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;//三角形で構成

	plsDesc.NumRenderTargets = 1;
	plsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;//0～1に正規化されたRGBA

	plsDesc.SampleDesc.Count = 1;//サンプリングは1ピクセルにつき１
	plsDesc.SampleDesc.Quality = 0;//クオリティは最低
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
