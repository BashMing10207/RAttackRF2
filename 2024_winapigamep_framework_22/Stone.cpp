#include "pch.h"
#include "Stone.h"
#include "Collider.h"
#include "RigidBody.h"

Stone::Stone()
{
	this->AddComponent<RigidBody>();
	this->AddComponent<Collider>();
}

Stone::~Stone()
{

}

void Stone::Update()
{

}

void Stone::Render(HDC _hdc)
{
	Vec2 vPos = GetPos();
	Vec2 vSize = GetSize();
	ELLIPSE_RENDER(_hdc, vPos.x, vPos.y
		, vSize.x, vSize.y);
	ComponentRender(_hdc);
}

void Stone::EnterCollision(Collider* _other)
{
	GetComponent<RigidBody>()->AddForce(_other->dir);
}

void Stone::StayCollision(Collider* _other)
{
	
}

void Stone::ExitCollision(Collider* _other)
{

}
