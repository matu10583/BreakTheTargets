#pragma once
class Object;


class Component
{

public:
	enum CState
	{
		CActive,
		CInActive
	};

	Component(Object* owner, int updateOrder = 100);
	virtual ~Component();

	virtual void Update(float deltaTime);
	virtual void ProcessInput(const struct InputState& inputState);
	int GetUpdateOrder();

	Object& GetOwner()const;

	void SetState(CState cs) { state_ = cs; }
	const CState GetState() { return state_; }

protected:
	Object* owner_;
	int updateOrder_;
	CState state_;
};

