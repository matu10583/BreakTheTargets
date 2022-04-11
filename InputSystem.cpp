#include "InputSystem.h"
#include "Game.h"


//KeyBoard
KeyboardState::KeyboardState()
{
}

bool
KeyboardState::IsOn(BYTE VK_CODE) const {
	return currState_[VK_CODE] & SIG_BIT;
}

ButtonState
KeyboardState::GetButtonState(BYTE VK_CODE)const {
	if (currState_[VK_CODE] & SIG_BIT) {
		//��������Ă���
		if (prevState_[VK_CODE] & SIG_BIT) {
			//�O��������Ă�
			return HELD;
		}
		else {
			//�O�͉�����ĂȂ�����
			return PRESSED;
		}
	}
	else {
		//��������ĂȂ�
		if (prevState_[VK_CODE] & SIG_BIT) {
			//�O������Ă�
			return RELEASED;
		}
		else {
			//�O�͉�����ĂȂ�����
			return NONE;
		}
	}
}

bool
KeyboardState::IsMappedButtonOn(Action action)const {
	return IsOn(buttonMap_.at(action));
}

ButtonState
KeyboardState::GetMappedButtonState(Action action)const {
	return GetButtonState(buttonMap_.at(action));
}

//Mouse
MouseState::MouseState()
{
}

Vector2
MouseState::GetMoveVec()const {
	return currP_ - centerP_;
}



//InputSysem

InputSystem* InputSystem::instance_;

void
InputSystem::Create() {
	assert(!instance_ && "�����");
	instance_ = new InputSystem();
}

void
InputSystem::Destroy() {
	delete instance_;
}

InputSystem*
InputSystem::Instance() {
	return instance_;
}

bool
InputSystem::Init() {
	//KeyBoard
	//���͏󋵂����Z�b�g
	memset(inputState_.kb.prevState_, 0x00, VK_SIZE);
	memset(inputState_.kb.currState_, 0x00, VK_SIZE);
	//�f�t�H���g�̃L�[�R���t�B�O��ݒ�
	{
		SetButtonActionMap(MOVE_JUMP, VK_SPACE);
		SetButtonActionMap(SHOOT_BULLET, VK_LBUTTON);
		SetButtonActionMap(MOVE_FORWAD, 'W');
		SetButtonActionMap(MOVE_LEFT, 'A');
		SetButtonActionMap(MOVE_BACK, 'S');
		SetButtonActionMap(MOVE_RIGHT, 'D');
		SetButtonActionMap(READY_SHOOT, VK_RBUTTON);
	}

	//Mouse
	//�}�E�X�J�[�\���̓E�B���h�E���S�ɌŒ肷��
	Game* game = Game::Instance();
	RECT rect;
	GetWindowRect(game->GetHWND(), &rect);
	int xPos = (rect.left + rect.right) / 2;
	int yPos = (rect.top + rect.bottom) / 2;
	SetCursorPos(xPos, yPos);
#ifndef _DEBUG
	ShowCursor(false);
	ClipCursor(&rect);
#endif // _DEBUG

	inputState_.ms.centerP_ = Vector2(xPos, yPos);

	return true;
}
#ifdef _DEBUG
bool gFreeCursor = false;
#endif // _DEBUG


void
InputSystem::Update() {
	//KeyBoard
	//�O�t���[���̏�Ԃ�ۑ�
	memcpy_s(inputState_.kb.prevState_, VK_SIZE,
		inputState_.kb.currState_, VK_SIZE);
	//���̃L�[�{�[�h�̏�Ԃ��擾
	for (int i = 0; i < VK_SIZE; i++) {
		inputState_.kb.currState_[i] = GetAsyncKeyState(i)>>16;
	}


	//Mouse
	POINT cursorP;
	GetCursorPos(&cursorP);
	//���݈ʒu���L�^
	inputState_.ms.currP_ = Vector2(
		cursorP.x, cursorP.y
	);
	
	//�}�E�X�J�[�\���̓E�B���h�E���S�ɌŒ肷��
	Game* game = Game::Instance();
	RECT rect;
	GetWindowRect(game->GetHWND(), &rect);
	int xPos = (rect.left + rect.right) / 2;
	int yPos = (rect.top + rect.bottom) / 2;
#ifndef _DEBUG
	SetCursorPos(xPos, yPos);
#endif // _DEBUG

	//�E�B���h�E�̒��S���W���擾
	inputState_.ms.centerP_ = Vector2(xPos, yPos);

#ifdef _DEBUG
	auto cst = inputState_.kb.GetButtonState('C');
	if (cst==PRESSED) {
		gFreeCursor = !gFreeCursor;
	}

	if (gFreeCursor) {
		inputState_.ms.currP_ = inputState_.ms.centerP_;
	}
	else {
		SetCursorPos(xPos, yPos);
	}
#endif // _DEBUG

}

void 
InputSystem::SetButtonActionMap(Action action, BYTE VK_CODE) {
	inputState_.kb.buttonMap_[action] = VK_CODE;
}

const InputState&
InputSystem::GetInputState()const {
	return inputState_;
}