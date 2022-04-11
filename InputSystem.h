#pragma once
#include "XMFLOAT_Helper.h"
#include <Windows.h>
#include <DirectXMath.h>
#include <unordered_map>

const unsigned VK_SIZE = 256;
const BYTE SIG_BIT = 0x80;
using namespace DirectX;



enum ButtonState
{
	NONE,//押されてない
	PRESSED,//このフレームで押された
	RELEASED,//このフレームで離された
	HELD//引き続き押されている
};

enum Action
{
	MOVE_JUMP,
	SHOOT_BULLET,
	MOVE_FORWAD,
	MOVE_BACK,
	MOVE_RIGHT,
	MOVE_LEFT,
	READY_SHOOT
};

class KeyboardState
{
public:
	friend class InputSystem;
	KeyboardState();
	//ボタンの状態を返す
	ButtonState GetButtonState(BYTE VK_CODE)const;
	//マッピングされたボタンの状態を返す
	ButtonState GetMappedButtonState(Action action)const;
	//とりあえず押されているかどうか
	bool IsOn(BYTE VK_CODE)const;
	//MappedVer.
	bool IsMappedButtonOn(Action action)const;

private:
	BYTE prevState_[VK_SIZE];
	BYTE currState_[VK_SIZE];
	//ボタンのマッピング
	std::unordered_map<Action, BYTE>
		buttonMap_;


};

class MouseState
{
public:
	friend class InputSystem;
	MouseState();
	Vector2 GetMoveVec()const;

private:
	//ウィンドウ内座標
	Vector2 centerP_;
	Vector2 currP_;

};


struct InputState
{
	//キーボード管理クラス
	KeyboardState kb;
	//マウス管理クラス
	MouseState ms;
};


class InputSystem
{
public:
	//シングルトン
	static void Create();
	static InputSystem* Instance();
	static void Destroy();

	bool Init();

	//入力状態の更新
	void Update();

	//キーコンフィグの変更
	void SetButtonActionMap(Action action, BYTE VK_CORD);

	//入力状態を送る
	const InputState& GetInputState()const;
private:
	InputSystem(){};
	~InputSystem() {};
	static InputSystem* instance_;

	InputState inputState_;
};