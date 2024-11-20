#pragma once

class Item
{
public:
	enum ItemType
	{
		Sword,
		Bow,
		Fire
	};
public:
	int sword;
	int bow;
	int fire;

	ItemType _itemType;

protected:
	ItemType GetItemType() { return _itemType; }

public:
	void Init();
	void Update();
	void Render();
};