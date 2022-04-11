
#include "Game.h"
#include "Object.h"
#include "Dx12Wrapper.h"
#include <algorithm>
#include <assert.h>
#include <tchar.h>
#include <numbers>
#include <iostream>
#include <Windows.h>
#include "Renderer.h"
#include "AudioSystem.h"
#include "InputSystem.h"
#include "PhysicsSystem.h"
#include "XMFLOAT_Helper.h"
#include "TitleSceneObject.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#pragma comment(lib, "winmm.lib")

const unsigned MIN_DELTA_MILISECONDS = 16;//62.6fps
const unsigned MAX_DELTA_MILISECONDS = 50;//20fps



Game* Game::instance_;

Game::Game(unsigned int h, unsigned int w):isRunning_(true),
tickCount_(0),
WINDOWWIDTH_(w),
WINDOWHEIGHT_(h),
hwnd_(nullptr),
windowClass_({}),
sceneObject_(nullptr),
nextScene_(nullptr),
stopUpdateGameObjects_(false)
{
}

Game::~Game() {
	delete sceneObject_;
	AudioSystem::Destroy();
	Renderer::Destroy();
	InputSystem::Destroy();
	PhysicsSystem::Destroy();
	Dx12Wrapper::Destroy();
	CoUninitialize();
}

Game*
Game::Instance() {
	return instance_;
}

void 
Game::Create(unsigned int h, unsigned int w) {
	//ゲームの作製
	assert(!instance_ && "二個作るな");
	instance_ = new Game(h, w);
}

bool 
Game::Init() {
	//Com起動！
	auto result = CoInitializeEx(
		0,
		COINIT_MULTITHREADED
	);
	if (FAILED(result)) {
		assert(0);
		return false;
	}

	CreateGameWindow(hwnd_, windowClass_);
	//入力システムの初期化
	InputSystem::Create();
	InputSystem::Instance()->Init();
	
	//TODO:dx12の初期化
	Dx12Wrapper::Create();
	if (!(Dx12Wrapper::Instance()->Init(hwnd_))) {
		assert(0);
		return false;
	}
	
	//Rendererの初期化
	Renderer::Create();
	if (!(Renderer::Instance()->Init())) {
		assert(0);
		return false;
	}
	//PhysicsSystemの初期化
	PhysicsSystem::Create();
	
	//AudioSystemの初期化
	AudioSystem::Create();
	if (!AudioSystem::Instance()->Init()) {
		assert(0);
		return false;
	}

	//シーンの生成(最初はタイトル)
	sceneObject_ = new TitleSceneObject(this);


	return true;
}

void 
Game::RunLoop() {//main loop
	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT) {
			break;
		}
		if (!isRunning_) {
			//修了時にウィンドウが残っていたらComPtrの解放エラーが
			//出るのでウィンドウを閉じてから終了する。
			PostMessage(hwnd_, WM_CLOSE, 0, 0);
		}
#ifdef _DEBUG
		//imgui描画前処理
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
#endif // _DEBUG


		ShowWindow(hwnd_, SW_SHOW);
		ProcessInput();
		UpdateGame();
		GenerateOutput();


	}
}

void
Game::Shutdown() {//終了処理

	//ウィンドウクラスの解除
	UnregisterClass(windowClass_.lpszClassName, windowClass_.hInstance);
	delete instance_;
}
#ifdef _DEBUG
float gDeltaTimes[10];
int fCount = 0;
#endif // _DEBUG


void
Game::ProcessInput(){//入力処理
	//入力の取得
	auto inputSystem = InputSystem::Instance();
	inputSystem->Update();
	const InputState& inputState = inputSystem->GetInputState();

	nextScene_ = sceneObject_->ProcessInput(inputState);

	if (nextScene_ == nullptr) {//終わる
		isRunning_ = false;
	}
	else if (sceneObject_ != nextScene_) {
		//シーンが遷移しているなら今のシーンは解放する
		delete sceneObject_;
		sceneObject_ = nextScene_;
	}
}

void 
Game::UpdateGame() {
	//フレームレート設定
	DWORD endTime = tickCount_ + MIN_DELTA_MILISECONDS;
	//早く終わりすぎたら休む
	while (timeGetTime()<endTime)
	{
		Sleep(1);
	}
	//前のループとの差(秒)
	float deltaTime = (timeGetTime() - tickCount_) / 1000.0f;
#ifdef _DEBUG
	fCount++;
	for (int i = 0; i <9; i++) {
		gDeltaTimes[i] = gDeltaTimes[i+1];
	}
	gDeltaTimes[9] = deltaTime;
	if((fCount%60)==0){
		float sum = 0;
		for (int i = 0; i < 10; i++) {
			sum += gDeltaTimes[i];
		}
		sum /= 10.0f;
		std::cout << "\r" << "AveFrameRate: " << 1.0f / sum << std::string(20, ' ');
		fCount = 0;
	}

#endif // _DEBUG

	//今のTickカウントを取得(ミリ秒)
	tickCount_ = timeGetTime();
	if (deltaTime > (MAX_DELTA_MILISECONDS / 1000.0f)) {
		//遅すぎてものが瞬間移動しないように上限を決める
		deltaTime = MAX_DELTA_MILISECONDS / 1000.0f;
	}

	nextScene_ = sceneObject_->Update(deltaTime);
	if (!stopUpdateGameObjects_) {
		PhysicsSystem::Instance()->Update(deltaTime);
	}
	AudioSystem::Instance()->Update(deltaTime);

	if (nextScene_ == nullptr) {//終わる
		isRunning_ = false;
	}
	else if (sceneObject_ != nextScene_) {
		//シーンが遷移しているなら今のシーンは解放する
		delete sceneObject_;
		sceneObject_ = nextScene_;
	}

}

void
Game::GenerateOutput() {//出力処理

#ifdef _DEBUG

	//imguiの描画
	Dx12Wrapper::Instance()->MakeImguiWindow();
#endif // _DEBUG

	Renderer* renderer = Renderer::Instance();
	//描画
	renderer->Draw();
}


//imguiようwindouプロシージャ
extern LRESULT
ImGui_ImplWin32_WndProcHandler(
	HWND, UINT, WPARAM, LPARAM);

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//ウィンドウが破棄されたら
		PostQuitMessage(0);
		return 0;
	}
#ifdef _DEBUG
	ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
#endif // _DEBUG

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void
Game::CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass) {
	HINSTANCE hInst = GetModuleHandle(nullptr);
	//ウィンドウクラス生成
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;//コールバック関数の指定
	windowClass.lpszClassName = _T("TPS");//アプリケーションクラス名
	windowClass.hInstance = GetModuleHandle(0);
	RegisterClassEx(&windowClass);//アプリケーションクラス

	RECT wrc = { 0, 0, (LONG)WINDOWWIDTH_, (LONG)WINDOWHEIGHT_ };//ウィンドウサイズを決める
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	//ウィンドウオブジェクトの生成
	hwnd = CreateWindow(windowClass.lpszClassName,//クラス名指定
		_T("TPS就職作品_岡田成広史"),//タイトルバーの文字
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,//表示X座標
		CW_USEDEFAULT,//表示Y座標
		wrc.right - wrc.left,//ウィンドウ幅
		wrc.bottom - wrc.top,//ウィンドウ高
		nullptr,//親ウィンドウ
		nullptr,//メニュー
		windowClass.hInstance,//呼び出しアプリケーションハンドル
		nullptr);//追加パラメータ
}

SIZE 
Game::GetWindowSize() {
	RECT rect;
	GetClientRect(hwnd_, &rect);
	SIZE size;
	size.cx = rect.right - rect.left;
	size.cy = rect.bottom - rect.top;
	return size;
}

HWND&
Game::GetHWND() {
	return hwnd_;
}
