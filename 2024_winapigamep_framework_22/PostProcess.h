#pragma once
#include "pch.h"
class PostProcess
{
	
};

void PostProcess(HDC hdc);

void Bloom(HDC hdc, int blurSize,int threshold,float intersity,float lerp);

void Blur(HDC hdc,int blurSize);