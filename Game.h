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

	//ウィンドウ周り
	WNDCLASSEX windowClass_;
	HWND hwnd_;
	const unsigned int WINDOWWIDTH_;
	const unsigned int WINDOWHEIGHT_;

	bool isRunning_;
	DWORD tickCount_;

	//今のシーン
	class SceneObject* sceneObject_;
	//次のシーン
	SceneObject* nextScene_;

	void ProcessInput();
	void UpdateGame();
	void GenerateOutput();
	void CreateGameWindow(HWND& hwnd, WNDCLASSEX& windowClass);

	bool stopUpdateGameObjects_;//ゲームオブジェクトの更新を一旦止める
	//(SceneObject::UpdateScene,SceneInputは動く)
};

