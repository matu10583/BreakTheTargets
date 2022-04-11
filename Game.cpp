
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
	//�Q�[���̍쐻
	assert(!instance_ && "�����");
	instance_ = new Game(h, w);
}

bool 
Game::Init() {
	//Com�N���I
	auto result = CoInitializeEx(
		0,
		COINIT_MULTITHREADED
	);
	if (FAILED(result)) {
		assert(0);
		return false;
	}

	CreateGameWindow(hwnd_, windowClass_);
	//���̓V�X�e���̏�����
	InputSystem::Create();
	InputSystem::Instance()->Init();
	
	//TODO:dx12�̏�����
	Dx12Wrapper::Create();
	if (!(Dx12Wrapper::Instance()->Init(hwnd_))) {
		assert(0);
		return false;
	}
	
	//Renderer�̏�����
	Renderer::Create();
	if (!(Renderer::Instance()->Init())) {
		assert(0);
		return false;
	}
	//PhysicsSystem�̏�����
	PhysicsSystem::Create();
	
	//AudioSystem�̏�����
	AudioSystem::Create();
	if (!AudioSystem::Instance()->Init()) {
		assert(0);
		return false;
	}

	//�V�[���̐���(�ŏ��̓^�C�g��)
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
			//�C�����ɃE�B���h�E���c���Ă�����ComPtr�̉���G���[��
			//�o��̂ŃE�B���h�E����Ă���I������B
			PostMessage(hwnd_, WM_CLOSE, 0, 0);
		}
#ifdef _DEBUG
		//imgui�`��O����
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
Game::Shutdown() {//�I������

	//�E�B���h�E�N���X�̉���
	UnregisterClass(windowClass_.lpszClassName, windowClass_.hInstance);
	delete instance_;
}
#ifdef _DEBUG
float gDeltaTimes[10];
int fCount = 0;
#endif // _DEBUG


void
Game::ProcessInput(){//���͏���
	//���͂̎擾
	auto inputSystem = InputSystem::Instance();
	inputSystem->Update();
	const InputState& inputState = inputSystem->GetInputState();

	nextScene_ = sceneObject_->ProcessInput(inputState);

	if (nextScene_ == nullptr) {//�I���
		isRunning_ = false;
	}
	else if (sceneObject_ != nextScene_) {
		//�V�[�����J�ڂ��Ă���Ȃ獡�̃V�[���͉������
		delete sceneObject_;
		sceneObject_ = nextScene_;
	}
}

void 
Game::UpdateGame() {
	//�t���[�����[�g�ݒ�
	DWORD endTime = tickCount_ + MIN_DELTA_MILISECONDS;
	//�����I��肷������x��
	while (timeGetTime()<endTime)
	{
		Sleep(1);
	}
	//�O�̃��[�v�Ƃ̍�(�b)
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

	//����Tick�J�E���g���擾(�~���b)
	tickCount_ = timeGetTime();
	if (deltaTime > (MAX_DELTA_MILISECONDS / 1000.0f)) {
		//�x�����Ă��̂��u�Ԉړ����Ȃ��悤�ɏ�������߂�
		deltaTime = MAX_DELTA_MILISECONDS / 1000.0f;
	}

	nextScene_ = sceneObject_->Update(deltaTime);
	if (!stopUpdateGameObjects_) {
		PhysicsSystem::Instance()->Update(deltaTime);
	}
	AudioSystem::Instance()->Update(deltaTime);

	if (nextScene_ == nullptr) {//�I���
		isRunning_ = false;
	}
	else if (sceneObject_ != nextScene_) {
		//�V�[�����J�ڂ��Ă���Ȃ獡�̃V�[���͉������
		delete sceneObject_;
		sceneObject_ = nextScene_;
	}

}

void
Game::GenerateOutput() {//�o�͏���

#ifdef _DEBUG

	//imgui�̕`��
	Dx12Wrapper::Instance()->MakeImguiWindow();
#endif // _DEBUG

	Renderer* renderer = Renderer::Instance();
	//�`��
	renderer->Draw();
}


//imgui�悤windou�v���V�[�W��
extern LRESULT
ImGui_ImplWin32_WndProcHandler(
	HWND, UINT, WPARAM, LPARAM);

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	if (msg == WM_DESTROY) {//�E�B���h�E���j�����ꂽ��
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
	//�E�B���h�E�N���X����
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.lpfnWndProc = (WNDPROC)WindowProcedure;//�R�[���o�b�N�֐��̎w��
	windowClass.lpszClassName = _T("TPS");//�A�v���P�[�V�����N���X��
	windowClass.hInstance = GetModuleHandle(0);
	RegisterClassEx(&windowClass);//�A�v���P�[�V�����N���X

	RECT wrc = { 0, 0, (LONG)WINDOWWIDTH_, (LONG)WINDOWHEIGHT_ };//�E�B���h�E�T�C�Y�����߂�
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);
	//�E�B���h�E�I�u�W�F�N�g�̐���
	hwnd = CreateWindow(windowClass.lpszClassName,//�N���X���w��
		_T("TPS�A�E��i_���c���L�j"),//�^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,//�\��X���W
		CW_USEDEFAULT,//�\��Y���W
		wrc.right - wrc.left,//�E�B���h�E��
		wrc.bottom - wrc.top,//�E�B���h�E��
		nullptr,//�e�E�B���h�E
		nullptr,//���j���[
		windowClass.hInstance,//�Ăяo���A�v���P�[�V�����n���h��
		nullptr);//�ǉ��p�����[�^
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
