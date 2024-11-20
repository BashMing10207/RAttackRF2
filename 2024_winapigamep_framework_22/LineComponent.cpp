#include "pch.h"
#include "LineComponent.h"

LineComponent::LineComponent()
    : m_isDrawing(false), m_startPos(Vec2()), m_endPos(Vec2())
{
}

LineComponent::~LineComponent()
{
}

void LineComponent::StartDrawing(Vec2 startPos)
{
    m_isDrawing = true;
    m_startPos = startPos;
}

void LineComponent::Update(Vec2 endPos)
{
    if (m_isDrawing)
    {
        m_endPos = endPos;  // �� ��ġ ������Ʈ
    }
}

void LineComponent::StopDrawing()
{
    m_isDrawing = false;  // �� �׸��� ����
}

void LineComponent::Render(HDC _hdc)
{
    if (m_isDrawing)
    {
        MoveToEx(_hdc, m_startPos.x, m_startPos.y, NULL);
        LineTo(_hdc, m_endPos.x, m_endPos.y);  // ���� �׸���
    }
}
