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
	NONE,//������ĂȂ�
	PRESSED,//���̃t���[���ŉ����ꂽ
	RELEASED,//���̃t���[���ŗ����ꂽ
	HELD//��������������Ă���
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
	//�{�^���̏�Ԃ�Ԃ�
	ButtonState GetButtonState(BYTE VK_CODE)const;
	//�}�b�s���O���ꂽ�{�^���̏�Ԃ�Ԃ�
	ButtonState GetMappedButtonState(Action action)const;
	//�Ƃ肠����������Ă��邩�ǂ���
	bool IsOn(BYTE VK_CODE)const;
	//MappedVer.
	bool IsMappedButtonOn(Action action)const;

private:
	BYTE prevState_[VK_SIZE];
	BYTE currState_[VK_SIZE];
	//�{�^���̃}�b�s���O
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
	//�E�B���h�E�����W
	Vector2 centerP_;
	Vector2 currP_;

};


struct InputState
{
	//�L�[�{�[�h�Ǘ��N���X
	KeyboardState kb;
	//�}�E�X�Ǘ��N���X
	MouseState ms;
};


class InputSystem
{
public:
	//�V���O���g��
	static void Create();
	static InputSystem* Instance();
	static void Destroy();

	bool Init();

	//���͏�Ԃ̍X�V
	void Update();

	//�L�[�R���t�B�O�̕ύX
	void SetButtonActionMap(Action action, BYTE VK_CORD);

	//���͏�Ԃ𑗂�
	const InputState& GetInputState()const;
private:
	InputSystem(){};
	~InputSystem() {};
	static InputSystem* instance_;

	InputState inputState_;
};