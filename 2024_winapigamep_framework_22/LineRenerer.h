#pragma once
#include "Component.h"
class LineRenerer :
    public Component
{
public:
    LineRenerer();
    ~LineRenerer();

    void StartDrawing(Vec2 startPos);  // �� �׸��� ����
    void Update(Vec2 endPos);          // �� �׸��� ������Ʈ
    void StopDrawing();                // �� �׸��� ����
    void Render(HDC _hdc) override;    // �� �׸��� ������

private:
    bool m_isDrawing;  // ���� �׸��� �ִ�?
    Vec2 m_startPos;   // ���� ���� ��ġ
    Vec2 m_endPos;     // ���� �� ��ġ(���콺�� ��ġ)
};
