#include "pch.h"
#include <cmath>
#include "CollisionManager.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Object.h"
#include "Collider.h"
#include"RigidBody.h"
#include "TimeManager.h"
void CollisionManager::Update()
{
	for (UINT Row = 0; Row < (UINT)LAYER::END; ++Row)
	{
		for (UINT Col = Row; Col < (UINT)LAYER::END; ++Col)
		{
			if (m_arrLayer[Row] & (1 << Col))
			{
				//int a = 0;
				CollisionLayerUpdate((LAYER)Row, (LAYER)Col);
			}
		}
	}
}

void CollisionManager::CheckLayer(LAYER _left, LAYER _right)
{
	// 작은쪽을 행으로 씁시다.
	// 작은 값을 LAYER의 행으로, 큰 값을 열
	UINT Row = (UINT)_left;
	UINT Col = (UINT)_right;
	//Row = min(Row, Col);
	if (Row > Col)
		std::swap(Row, Col);
	//m_arrLayer[Row];
	//Col;

	//// 비트 연산
	// 체크가 되어있다면
	if (m_arrLayer[Row] & (1 << Col))
	{
		// 체크 풀기
		m_arrLayer[Row] &= ~(1 << Col);
	}
	// 체크가 안되어있다면r
	else
	{
		m_arrLayer[Row] |= (1 << Col);
	}
	int a = 0;
}

void CollisionManager::CheckReset()
{
	// 메모리 초기화
	memset(m_arrLayer, 0, sizeof(UINT) * (UINT)LAYER::END);
}

void CollisionManager::CollisionLayerUpdate(LAYER _left, LAYER _right)
{
	std::shared_ptr<Scene> pCurrentScene = GET_SINGLE(SceneManager)->GetCurrentScene();
	const vector<Object*>& vecLeftLayer = pCurrentScene->GetLayerObjects(_left);
	const vector<Object*>& vecRightLayer = pCurrentScene->GetLayerObjects(_right);
	map<ULONGLONG, bool>::iterator iter;
	for (size_t i = 0; i < vecLeftLayer.size(); ++i)
	{
		Collider* pLeftCollider = vecLeftLayer[i]->GetComponent<Collider>();
		// 충돌체 없는 경우
		if (nullptr == pLeftCollider)
			continue;
		for (size_t j = 0; j < vecRightLayer.size(); j++)
		{
			Collider* pRightCollider = vecRightLayer[j]->GetComponent<Collider>();
			// 충돌체가 없거나, 자기자신과의 충돌인 경우
			if (nullptr == pRightCollider || vecLeftLayer[i] == vecRightLayer[j])
				continue;

			COLLIDER_ID colliderID; // 두 충돌체로만 만들 수 있는 ID
 			colliderID.left_ID = pLeftCollider->GetID();
			colliderID.right_ID = pRightCollider->GetID();

			iter = m_mapCollisionInfo.find(colliderID.ID);
			// 이전 프레임 충돌한 적 없다.
			if (iter == m_mapCollisionInfo.end())
			{
				// 충돌 정보가 미등록된 상태인 경우 등록(충돌하지 않았다로)
				m_mapCollisionInfo.insert({ colliderID.ID, false });
				//m_mapCollisionInfo[colliderID.ID] = false;
				iter = m_mapCollisionInfo.find(colliderID.ID);
			}

			/*if (_left == LAYER::STATIC ? _right == LAYER::STATIC ? 
				IsCollision(pLeftCollider, pRightCollider) : IsCollisionMinscope(pRightCollider,pLeftCollider) != Vec2(0, 0)
				:(_right==LAYER::STATIC ? IsCollisionMinscope(pLeftCollider,pRightCollider)
				:IsCollisionCircle(pLeftCollider,pRightCollider)) != Vec2(0,0))*/
			bool isCollide = false;
			Vec2 dir = Vec2(0, 0);

			if (_left != LAYER::STATIC)
			{
				if (_right == LAYER::STATIC)
				{
					dir = IsCollisionMinscope(pLeftCollider, pRightCollider);
					isCollide = (dir == Vec2(0, 0));
				}
				else 
				{
					dir = IsCollisionCircle(pLeftCollider, pRightCollider);
					isCollide = dir != Vec2(0, 0);
				}
			}
			else if(_right == LAYER::STATIC)
			{
				isCollide = IsCollision(pLeftCollider, pRightCollider);
			}

			if(isCollide)
			{
				pLeftCollider->dir = dir;
				pRightCollider->dir = dir*-1;
				// 이전에도 충돌중
				if (iter->second)
				{
					if (vecLeftLayer[i]->GetIsDead() || vecRightLayer[j]->GetIsDead())
					{
						pLeftCollider->ExitCollision(pRightCollider);
						pRightCollider->ExitCollision(pLeftCollider);
						iter->second = false;
					}
					else
					{
						pLeftCollider->StayCollision(pRightCollider);
						pRightCollider->StayCollision(pLeftCollider);
					}
				}
				else // 이전에 충돌 x
				{
					if (!vecLeftLayer[i]->GetIsDead() && !vecRightLayer[j]->GetIsDead())
					{
						pLeftCollider->EnterCollision(pRightCollider);
						pRightCollider->EnterCollision(pLeftCollider);
						iter->second = true;


					}
				}
			}
			else // 충돌 안하네?
			{
				if (iter->second) // 근데 이전에 충돌중
				{
					pLeftCollider->ExitCollision(pRightCollider);
					pRightCollider->ExitCollision(pLeftCollider);
					iter->second = false;
				}
			}
		}
	}
}

bool CollisionManager::IsCollision(Collider* _left, Collider* _right)
{
	Vec2 vLeftPos = _left->GetLatedUpatedPos();
	Vec2 vRightPos = _right->GetLatedUpatedPos();
	Vec2 vLeftSize = _left->GetSize();
	Vec2 vRightSize = _right->GetSize();

	RECT leftRt = RECT_MAKE(vLeftPos.x, vLeftPos.y, vLeftSize.x, vLeftSize.y);
	RECT rightRt = RECT_MAKE(vRightPos.x, vRightPos.y, vRightSize.x, vRightSize.y);
	RECT rt;
	return ::IntersectRect(&rt, &leftRt, &rightRt);
	/*if (abs(vRightPos.x - vLeftPos.x) < (vLeftSize.x + vRightSize.x) / 2.f
		&& abs(vRightPos.y - vLeftPos.y) < (vLeftSize.y + vRightSize.y) / 2.f)
	{
		return true;
	}

	return false;*/
}

Vec2 CollisionManager::IsCollisionMinscope(Collider* _circle, Collider* _ractangle)
{
	Vec2 circlePos = _circle->GetLatedUpatedPos();
	Vec2 vRightPos = _ractangle->GetLatedUpatedPos();
	Vec2 circleSize = _circle->GetSize();
	Vec2 vRightSize = _ractangle->GetSize();
	vRightSize += circleSize;

	if (!(vRightPos.x + vRightSize.x / 2 >= circlePos.x,
		vRightPos.y + vRightSize.y / 2 >= circlePos.y,
		vRightPos.x - vRightSize.x / 2 <= circlePos.x,
		vRightPos.y - vRightSize.y / 2 <= circlePos.y))
		return Vec2(0, 0);
	//if(vRightPos.x  
	//Vec2::A2BLineAndPoint(Vec2(vRightPos.x + vRightSize.x / 2, vRightPos.y),
		//Vec2(vRightPos.x, vRightPos.y+vRightSize.y/2))
}

Vec2 CollisionManager::IsCollisionCircle(Collider* _left, Collider* _right)
{
	Vec2 vleftPos = _left->GetLatedUpatedPos();
	Vec2 vRightPos = _right->GetLatedUpatedPos();
	Vec2 vLeftSize = _left->GetSize();
	Vec2 vRightSize = _right->GetSize();

	Vec2 dir = (vleftPos - vRightPos);
	float radius = vLeftSize.x + vRightSize.x;

	float distance = dir.magnitude();
	
	return dir.Normalize() * max(radius - distance, 0);
}



