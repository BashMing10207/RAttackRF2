#include "pch.h"
#include "MapManager.h"

void MapManager::Render(HDC _hdc)
{
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			Rectangle(_hdc, (50 * i) + i * 10, (50 * j) + j * 10, (50 * i) + i * 10 + 50, (50 * j) + j * 10 + 50);
		}
	}
}
