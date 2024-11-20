#pragma once
#include "Object.h"
class Stone :
	public Object
{
public:
	Stone();
	~Stone();
public:
	void Update() override;
	void Render(HDC _hdc) override;
public:
	virtual void EnterCollision(Collider* _other);
	virtual void StayCollision(Collider* _other);
	virtual void ExitCollision(Collider* _other);
private:
	int m_hp;
};


