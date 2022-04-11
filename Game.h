#pragma once
#include <vector>
#include <Windows.h>
class Object;
class Dx12Wrapper;

class Game
{
public:
	static Game* Instance();
	static void Create(unsigned int h, unsigned int w);

	void RunLoop();
	void Shutdown();
	bool Init();
	SIZE GetWindowSize();
	HWND& GetHWND();

	bool GetStopUpdateGameObjects()const {
		return stopUpdateGameObjects_;
	}
	void SetStopUpdateGameObjects(bool flag) {
		stopUpdateGameObjects_ = flag;
	}

private:
	//singleton
	Game(unsigned int h, unsigned int w);
	Game(const Game&);
	~Game();
	static Game* instance_;

	//�E�B���h�E����
	WNDCLASSEX windowClass_;
	HWND hwnd_;
	const unsigned int WINDOWWIDTH_;
	const unsigned int WINDOWHEIGHT_;

	bool isRunning_;
	DWORD tickCount_;

	//���̃V�[��
	class SceneObject* sceneObject_;
	//���̃V�[��
	SceneObject* nextScene_;

	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();
	void CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass);

	bool stopUpdateGameObjects_;//�Q�[���I�u�W�F�N�g�̍X�V����U�~�߂�
	//(SceneObject::UpdateScene,SceneInput�͓���)
};

