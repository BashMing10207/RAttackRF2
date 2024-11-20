#pragma once
#include "Component.h"

class LineComponent : public Component
{
public:
    LineComponent();
    ~LineComponent();

    void StartDrawing(Vec2 startPos);  // 선 그리기 시작
    void Update(Vec2 endPos);          // 선 그리기 업데이트
    void StopDrawing();                // 선 그리기 종료
    void Render(HDC _hdc) override;    // 선 그리기 렌더링

private:
    bool m_isDrawing;  // 선을 그리고 있니?
    Vec2 m_startPos;   // 선의 시작 위치
    Vec2 m_endPos;     // 선의 끝 위치(마우스의 위치)
};
