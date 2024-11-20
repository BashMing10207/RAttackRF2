#include "pch.h"
#include "Item.h"

void Item::Init()
{
}

void Item::Update()
{
}

void Item::Render()
{
	switch (GetItemType())
	{
	case Item::Sword:
		break;
	case Item::Bow:
		break;
	case Item::Fire:
		break;
	default:
		break;
	}
}
