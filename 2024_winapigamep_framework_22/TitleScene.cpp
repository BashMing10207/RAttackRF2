#include "pch.h"
#include "TitleScene.h"
#include "Object.h"
#include "Player.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "Enemy.h"
#include "CollisionManager.h"
#include "ResourceManager.h"
void TitleScene::Init()
{
	Object* pObj = new Enemy;
	pObj->SetPos({ SCREEN_WIDTH / 2.f,150.f });
	pObj->SetSize({ 100.f,100.f });
	pObj->SetName(L"Enemy");
	AddObject(pObj, LAYER::ENEMY);

	Object* pObj2 = new Enemy;
	pObj2->SetPos({(float)SCREEN_WIDTH,150.f });
	pObj2->SetSize({ 100.f,100.f });
	pObj2->SetName(L"Enemy");
	AddObject(pObj2, LAYER::ENEMY);


	Object* pPlayer6 = new Player;
	pPlayer6->SetPos({ SCREEN_WIDTH / 2.f,500.f });
	pPlayer6->SetSize({ 100.f,100.f });
	AddObject(pPlayer6, LAYER::PLAYER);
	GET_SINGLE(CollisionManager)->CheckLayer(LAYER::PROJECTILE, LAYER::ENEMY);
	//GET_SINGLE(CollisionManager)->CheckLayer(LAYER::PLAYER, LAYER::ENEMY);
	GET_SINGLE(ResourceManager)->LoadSound(L"BGM", L"Sound\\Retro_bgm.wav", true);
	GET_SINGLE(ResourceManager)->Play(L"BGM");
}

void TitleScene::Update()
{
	Scene::Update();
	if (GET_KEYDOWN(KEY_TYPE::ENTER))
		GET_SINGLE(SceneManager)->LoadScene(L"GameScene");
}
