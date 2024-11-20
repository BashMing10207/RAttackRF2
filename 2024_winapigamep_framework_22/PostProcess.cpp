#include "pch.h"
#include <vector>
#include <thread>
#include <immintrin.h> // AVX2�� ���� ���
#include "PostProcess.h"

struct RGBColor {
    int r, g, b;

    RGBColor(int red = 0, int green = 0, int blue = 0) : r(red), g(green), b(blue) {}

    RGBColor operator+(const RGBColor& other) const {
        return RGBColor(r + other.r, g + other.g, b + other.b);
    }

    RGBColor operator/(int divisor) const {
        return RGBColor(r / divisor, g / divisor, b / divisor);
    }

    RGBColor operator*(int divisor) const {
        return RGBColor(r * divisor, g * divisor, b * divisor);
    }

    RGBColor ColorLerp(const RGBColor& other, float t) const {
        return RGBColor(
            static_cast<int>(r * (1 - t) + other.r * t),
            static_cast<int>(g * (1 - t) + other.g * t),
            static_cast<int>(b * (1 - t) + other.b * t)
        );
    }

    COLORREF ToCOLORREF() const {
        return RGB(r, g, b);
    }
};

// SIMD�� Ȱ���� box blur ó��
void SIMD_BoxBlur_Processing(BYTE* bitmapData, int width, int height, int blurSize, int rowBytes, int startRow, int endRow) {
    for (int y = startRow; y < endRow; ++y) {
        for (int x = 0; x < width; ++x) {

            int left = max(x - blurSize, 0);
            int right = min(x + blurSize, width - 1);
            int top = max(y - blurSize, 0);
            int bottom = min(y + blurSize, height - 1);

            int area = (right - left + 1) * (bottom - top + 1);

            // �ڽ� �� ���� ���
            RGBColor sum(0, 0, 0);
            for (int i = top; i <= bottom; ++i) {
                for (int j = left; j <= right; ++j) {
                    int idx = i * rowBytes + j * 3;
                    sum.r += bitmapData[idx + 2];
                    sum.g += bitmapData[idx + 1];
                    sum.b += bitmapData[idx];
                }
            }

            RGBColor avgColor = sum / area;

            // ���� ����� ��� ������ ����
            int idx = y * rowBytes + x * 3;
            RGBColor currentColor(bitmapData[idx + 2], bitmapData[idx + 1], bitmapData[idx]); // Correct RGB order
            RGBColor finalColor = avgColor.ColorLerp(currentColor, 0.5f);

            // ����� ��Ʈ�ʿ� ���� (RGB ������� ����)
            bitmapData[idx + 2] = finalColor.r;
            bitmapData[idx + 1] = finalColor.g;
            bitmapData[idx] = finalColor.b;
        }
    }
}

void SIMD_BoxBloom_Processing(BYTE* bitmapData, int width, int height, int blurSize, int rowBytes, int startRow, int endRow, int threshold,float intensity,float lerp) {
    
    for (int y = startRow; y < endRow; ++y) {
        for (int x = 0; x < width; ++x) {

           /* if (threshold >= bitmapData[y * rowBytes + x * 3] || threshold >= bitmapData[y * rowBytes + x * 3 + 1] ||
                threshold >= bitmapData[y * rowBytes + x * 3 + 2])
                continue;*/
            
            int left = max(x - blurSize, 0);
            int right = min(x + blurSize, width - 1);
            int top = max(y - blurSize, 0);
            int bottom = min(y + blurSize, height - 1);

            int area = (right - left + 1) * (bottom - top + 1);

            // �ڽ� �� ���� ���
            RGBColor sum(1,1,1);
            for (int i = top; i <= bottom; ++i) {
                for (int j = left; j <= right; ++j) {
                    int idx = i * rowBytes + j * 3;
                    sum.r += bitmapData[idx + 2];
                    sum.g += bitmapData[idx + 1];
                    sum.b += bitmapData[idx];
                }
            }

            RGBColor avgColor = sum * intensity / area;
            if (threshold >= avgColor.r || threshold >=sum.g ||threshold >= sum.b)
                continue;

            // ���� ����� ��� ������ ����
            int idx = y * rowBytes + x * 3;
            RGBColor currentColor(bitmapData[idx + 2], bitmapData[idx + 1], bitmapData[idx]); // Correct RGB order
            RGBColor finalColor = avgColor .ColorLerp(currentColor, lerp);

            // ����� ��Ʈ�ʿ� ���� (RGB ������� ����)
            bitmapData[idx + 2] = min(finalColor.r,254);
            bitmapData[idx + 1] = min(finalColor.g,254);
            bitmapData[idx] = min(finalColor.b,254);
        }
    }
}

void Bloom(HDC hdc, int blurSize, int threshold,float intensity,float lerp)
//hdc: hdc, blurSize:���� ũ��, threshold: ��� �ּ� ���, ���, �帲(�������� ����ȿ��)
{
    int width = SCREEN_WIDTH;
    int height = SCREEN_HEIGHT;

    // ���� �̹��� ���� �����͸� �����ͼ� ����
    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(BITMAPINFO));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;

    void* pBits;
    HBITMAP hBitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (!hBitmap) return;

    HDC hMemDC = CreateCompatibleDC(hdc);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
    BitBlt(hMemDC, 0, 0, width, height, hdc, 0, 0, SRCCOPY);

    BYTE* bitmapData = static_cast<BYTE*>(pBits);
    int rowBytes = ((width * 3 + 3) & ~3); // �� ���� ũ�� ��� (3����Ʈ ������ ����)

    // ��Ƽ�������� ���� ó��
    int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    int chunkHeight = height / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int startRow = i * chunkHeight;
        int endRow = (i == numThreads - 1) ? height : (i + 1) * chunkHeight;
        threads.push_back(std::thread(SIMD_BoxBloom_Processing, bitmapData, width, height, blurSize,
            rowBytes, startRow, endRow,threshold,intensity,lerp));
    }

    for (auto& t : threads) {
        t.join();
    }

    // ����� hdc�� ����
    BitBlt(hdc, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);

    // �޸� ����
    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
}

// ��Ʈ���� ó���ϴ� �Լ�
void Blur(HDC hdc, int blurSize) {
    int width = SCREEN_WIDTH;
    int height = SCREEN_HEIGHT;

    // ���� �̹��� ���� �����͸� �����ͼ� ����
    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(BITMAPINFO));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;

    void* pBits;
    HBITMAP hBitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (!hBitmap) return;

    HDC hMemDC = CreateCompatibleDC(hdc);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
    BitBlt(hMemDC, 0, 0, width, height, hdc, 0, 0, SRCCOPY);

    BYTE* bitmapData = static_cast<BYTE*>(pBits);
    int rowBytes = ((width * 3 + 3) & ~3); // �� ���� ũ�� ��� (3����Ʈ ������ ����)

    // ��Ƽ�������� ���� ó��
    int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;
    int chunkHeight = height / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int startRow = i * chunkHeight;
        int endRow = (i == numThreads - 1) ? height : (i + 1) * chunkHeight;
        threads.push_back(std::thread(SIMD_BoxBlur_Processing, bitmapData, width, height, blurSize, rowBytes, startRow, endRow));
    }

    for (auto& t : threads) {
        t.join();
    }

    // ����� hdc�� ����
    BitBlt(hdc, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);

    // �޸� ����
    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
}







//
//int Lerp(int a, int b, float t)
//{
//    return (int)(a * (1 - t) + b * (t));
//}
//
//
//struct RGBColor {
//    int r, g, b;
//
//    RGBColor(int red, int green, int blue) : r(red), g(green), b(blue) {}
//    RGBColor() : r(0), g(0), b(0) {}
//
//    RGBColor operator+(const RGBColor& other) const {
//        return RGBColor(r + other.r, g + other.g, b + other.b);
//    }
//
//    RGBColor operator-(const RGBColor& other) const {
//        return RGBColor(r - other.r, g - other.g, b - other.b);
//    }
//
//    RGBColor operator/(int divisor) const {
//        return RGBColor(r / divisor, g / divisor, b / divisor);
//    }
//};
//
//void Blur(HDC hdc, int blurSize) {
//    int width = SCREEN_WIDTH;
//    int height = SCREEN_HEIGHT;
//
//    // BITMAPINFO ����
//    BITMAPINFO bi;
//    ZeroMemory(&bi, sizeof(BITMAPINFO));
//    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//    bi.bmiHeader.biWidth = width;
//    bi.bmiHeader.biHeight = -height;  // DIB�� ���� ������ ���� ������ ����
//    bi.bmiHeader.biPlanes = 1;
//    bi.bmiHeader.biBitCount = 24;     // 24��Ʈ ���� (RGB)
//    bi.bmiHeader.biCompression = BI_RGB;
//
//    // DIBSection ����
//    void* pBits;
//    HBITMAP hBitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
//    if (!hBitmap) return;
//
//    // ���� hdc�� �����͸� DIBSection���� ����
//    HDC hMemDC = CreateCompatibleDC(hdc);
//    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
//    BitBlt(hMemDC, 0, 0, width, height, hdc, 0, 0, SRCCOPY);
//
//    // �ȼ� ������ ������ ���� �����ͷ� ��ȯ
//    BYTE* bitmapData = static_cast<BYTE*>(pBits);
//    int rowBytes = ((width * 3 + 3) & ~3); // �� ���� ����Ʈ �� (4�� ����� ����)
//
//    // R, G, B ä�ο� ���� ������ ���� ���̺� �迭 ����
//    std::vector<int> sumTableR((width + 1) * (height + 1), 0);
//    std::vector<int> sumTableG((width + 1) * (height + 1), 0);
//    std::vector<int> sumTableB((width + 1) * (height + 1), 0);
//
//    // ���� ���̺� ���� (����ȭ)
//#pragma omp parallel for
//    for (int y = 1; y <= height; ++y) {
//        for (int x = 1; x <= width; ++x) {
//            int index = (y - 1) * rowBytes + (x - 1) * 3;
//            int r = bitmapData[index + 2];
//            int g = bitmapData[index + 1];
//            int b = bitmapData[index];
//
//            int idx = y * (width + 1) + x;
//            sumTableR[idx] = r + sumTableR[idx - 1] + sumTableR[idx - (width + 1)] - sumTableR[idx - (width + 2)];
//            sumTableG[idx] = g + sumTableG[idx - 1] + sumTableG[idx - (width + 1)] - sumTableG[idx - (width + 2)];
//            sumTableB[idx] = b + sumTableB[idx - 1] + sumTableB[idx - (width + 1)] - sumTableB[idx - (width + 2)];
//        }
//    }
//
//    // �� ó�� (����ȭ)
//#pragma omp parallel for
//    for (int y = 0; y < height; ++y) {
//        for (int x = 0; x < width; ++x) {
//            int left = max(x - blurSize, 0);
//            int right = min(x + blurSize, width - 1);
//            int top = max(y - blurSize, 0);
//            int bottom = min(y + blurSize, height - 1);
//
//            int area = (right - left + 1) * (bottom - top + 1);
//
//            int idx1 = (bottom + 1) * (width + 1) + (right + 1);
//            int idx2 = top * (width + 1) + (right + 1);
//            int idx3 = (bottom + 1) * (width + 1) + left;
//            int idx4 = top * (width + 1) + left;
//
//            int r = (sumTableR[idx1] - sumTableR[idx2] - sumTableR[idx3] + sumTableR[idx4]) / area;
//            int g = (sumTableG[idx1] - sumTableG[idx2] - sumTableG[idx3] + sumTableG[idx4]) / area;
//            int b = (sumTableB[idx1] - sumTableB[idx2] - sumTableB[idx3] + sumTableB[idx4]) / area;
//
//            // ���� ����� ����(50% ���� ����)
//            int origIndex = y * rowBytes + x * 3;
//            bitmapData[origIndex + 2] = static_cast<BYTE>((r + bitmapData[origIndex + 2]) / 2);
//            bitmapData[origIndex + 1] = static_cast<BYTE>((g + bitmapData[origIndex + 1]) / 2);
//            bitmapData[origIndex] = static_cast<BYTE>((b + bitmapData[origIndex]) / 2);
//        }
//    }
//
//    // ����� hdc�� ����
//    BitBlt(hdc, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);
//
//    // �޸� ����
//    SelectObject(hMemDC, hOldBitmap);
//    DeleteObject(hBitmap);
//    DeleteDC(hMemDC);
//}


//
//struct RGBColor {
//    int r, g, b;
//
//    RGBColor(int red, int green, int blue) : r(red), g(green), b(blue) {}
//    RGBColor() : r(0), g(0), b(0) {}
//
//    RGBColor operator+(const RGBColor& other) const {
//        return RGBColor(r + other.r, g + other.g, b + other.b);
//    }
//
//    RGBColor operator-(const RGBColor& other) const {
//        return RGBColor(r - other.r, g - other.g, b - other.b);
//    }
//
//    RGBColor operator/(int divisor) const {
//        return RGBColor(r / divisor, g / divisor, b / divisor);
//    }
//};
//
//void Blur(HDC hdc, int blurSize) {
//    int width = SCREEN_WIDTH;
//    int height = SCREEN_HEIGHT;
//
//    // BITMAPINFO ����
//    BITMAPINFO bi;
//    ZeroMemory(&bi, sizeof(BITMAPINFO));
//    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//    bi.bmiHeader.biWidth = width;
//    bi.bmiHeader.biHeight = -height;  // DIB�� ���� ������ ���� ������ ����
//    bi.bmiHeader.biPlanes = 1;
//    bi.bmiHeader.biBitCount = 24;     // 24��Ʈ ���� (RGB)
//    bi.bmiHeader.biCompression = BI_RGB;
//
//    // DIBSection ����
//    void* pBits;
//    HBITMAP hBitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
//    if (!hBitmap) return;
//
//    // ���� hdc�� �����͸� DIBSection���� ����
//    HDC hMemDC = CreateCompatibleDC(hdc);
//    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
//    BitBlt(hMemDC, 0, 0, width, height, hdc, 0, 0, SRCCOPY);
//
//    // �ȼ� ������ ������ ���� �����ͷ� ��ȯ
//    BYTE* bitmapData = static_cast<BYTE*>(pBits);
//    int rowBytes = ((width * 3 + 3) & ~3); // �� ���� ����Ʈ �� (4�� ����� ����)
//
//    // ���� ���̺��� 1���� �迭�� ����
//    std::vector<int> sumTableR((width + 1) * (height + 1), 0);
//    std::vector<int> sumTableG((width + 1) * (height + 1), 0);
//    std::vector<int> sumTableB((width + 1) * (height + 1), 0);
//
//    // ���� ���̺� ���� (���� ������, ������ ��Ȯ�� ����)
//    for (int y = 1; y <= height; ++y) {
//        for (int x = 1; x <= width; ++x) {
//            int index = (y - 1) * rowBytes + (x - 1) * 3;
//            int r = bitmapData[index + 2];
//            int g = bitmapData[index + 1];
//            int b = bitmapData[index];
//
//            int idx = y * (width + 1) + x;
//            sumTableR[idx] = r + sumTableR[idx - 1] + sumTableR[idx - (width + 1)] - sumTableR[idx - (width + 2)];
//            sumTableG[idx] = g + sumTableG[idx - 1] + sumTableG[idx - (width + 1)] - sumTableG[idx - (width + 2)];
//            sumTableB[idx] = b + sumTableB[idx - 1] + sumTableB[idx - (width + 1)] - sumTableB[idx - (width + 2)];
//        }
//    }
//
//    // �� ó�� (����ȭ ����)
//#pragma omp parallel for
//    for (int y = 0; y < height; ++y) {
//        for (int x = 0; x < width; ++x) {
//            int left = max(x - blurSize, 0);
//            int right = min(x + blurSize, width - 1);
//            int top = max(y - blurSize, 0);
//            int bottom = min(y + blurSize, height - 1);
//
//            int area = (right - left + 1) * (bottom - top + 1);
//
//            int idx1 = (bottom + 1) * (width + 1) + (right + 1);
//            int idx2 = top * (width + 1) + (right + 1);
//            int idx3 = (bottom + 1) * (width + 1) + left;
//            int idx4 = top * (width + 1) + left;
//
//            int r = (sumTableR[idx1] - sumTableR[idx2] - sumTableR[idx3] + sumTableR[idx4]) / area;
//            int g = (sumTableG[idx1] - sumTableG[idx2] - sumTableG[idx3] + sumTableG[idx4]) / area;
//            int b = (sumTableB[idx1] - sumTableB[idx2] - sumTableB[idx3] + sumTableB[idx4]) / area;
//
//            // ���� ����� ����(50% ���� ����)
//            int origIndex = y * rowBytes + x * 3;
//            bitmapData[origIndex + 2] = static_cast<BYTE>((r + bitmapData[origIndex + 2]) / 2);
//            bitmapData[origIndex + 1] = static_cast<BYTE>((g + bitmapData[origIndex + 1]) / 2);
//            bitmapData[origIndex] = static_cast<BYTE>((b + bitmapData[origIndex]) / 2);
//        }
//    }
//
//    // ����� hdc�� ����
//    BitBlt(hdc, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);
//
//    // �޸� ����
//    SelectObject(hMemDC, hOldBitmap);
//    DeleteObject(hBitmap);
//    DeleteDC(hMemDC);
//}


//struct RGBColor {
//    int r, g, b;
//
//    RGBColor(int red, int green, int blue) : r(red), g(green), b(blue) {}
//    RGBColor() : r(0), g(0), b(0) {}
//
//    RGBColor operator+(const RGBColor& other) const {
//        return RGBColor(r + other.r, g + other.g, b + other.b);
//    }
//
//    RGBColor operator-(const RGBColor& other) const {
//        return RGBColor(r - other.r, g - other.g, b - other.b);
//    }
//
//    RGBColor operator/(int divisor) const {
//        return RGBColor(r / divisor, g / divisor, b / divisor);
//    }
//};
//
//void Blur(HDC hdc, int blurSize) {
//    int width = SCREEN_WIDTH;
//    int height = SCREEN_HEIGHT;
//
//    // BITMAPINFO ����
//    BITMAPINFO bi;
//    ZeroMemory(&bi, sizeof(BITMAPINFO));
//    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//    bi.bmiHeader.biWidth = width;
//    bi.bmiHeader.biHeight = -height;  // DIB�� ���� ������ ���� ������ ����
//    bi.bmiHeader.biPlanes = 1;
//    bi.bmiHeader.biBitCount = 24;     // 24��Ʈ ���� (RGB)
//    bi.bmiHeader.biCompression = BI_RGB;
//
//    // DIBSection ����
//    void* pBits;
//    HBITMAP hBitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
//    if (!hBitmap) return;
//
//    // ���� hdc�� �����͸� DIBSection���� ����
//    HDC hMemDC = CreateCompatibleDC(hdc);
//    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
//    BitBlt(hMemDC, 0, 0, width, height, hdc, 0, 0, SRCCOPY);
//
//    // �ȼ� ������ ������ ���� �����ͷ� ��ȯ
//    BYTE* bitmapData = static_cast<BYTE*>(pBits);
//    int rowBytes = ((width * 3 + 3) & ~3); // �� ���� ����Ʈ �� (4�� ����� ����)
//
//    // ���� ���̺��� 1���� �迭�� ����
//    std::vector<int> sumTableR((width + 1) * (height + 1), 0);
//    std::vector<int> sumTableG((width + 1) * (height + 1), 0);
//    std::vector<int> sumTableB((width + 1) * (height + 1), 0);
//
//    // ���� ���̺� ���� (���� ������)
//    for (int y = 1; y <= height; ++y) {
//        for (int x = 1; x <= width; ++x) {
//            int index = (y - 1) * rowBytes + (x - 1) * 3;
//            int r = bitmapData[index + 2];
//            int g = bitmapData[index + 1];
//            int b = bitmapData[index];
//
//            int idx = y * (width + 1) + x;
//            sumTableR[idx] = r + sumTableR[idx - 1] + sumTableR[idx - (width + 1)] - sumTableR[idx - (width + 2)];
//            sumTableG[idx] = g + sumTableG[idx - 1] + sumTableG[idx - (width + 1)] - sumTableG[idx - (width + 2)];
//            sumTableB[idx] = b + sumTableB[idx - 1] + sumTableB[idx - (width + 1)] - sumTableB[idx - (width + 2)];
//        }
//    }
//
//    // �� ó�� (����ȭ ����)
//#pragma omp parallel for
//    for (int y = 0; y < height; ++y) {
//        for (int x = 0; x < width; ++x) {
//            int left = max(x - blurSize, 0);
//            int right = min(x + blurSize, width - 1);
//            int top = max(y - blurSize, 0);
//            int bottom = min(y + blurSize, height - 1);
//
//            int area = (right - left + 1) * (bottom - top + 1);
//
//            int idx1 = (bottom + 1) * (width + 1) + (right + 1);
//            int idx2 = top * (width + 1) + (right + 1);
//            int idx3 = (bottom + 1) * (width + 1) + left;
//            int idx4 = top * (width + 1) + left;
//
//            int r = (sumTableR[idx1] - sumTableR[idx2] - sumTableR[idx3] + sumTableR[idx4]) / area;
//            int g = (sumTableG[idx1] - sumTableG[idx2] - sumTableG[idx3] + sumTableG[idx4]) / area;
//            int b = (sumTableB[idx1] - sumTableB[idx2] - sumTableB[idx3] + sumTableB[idx4]) / area;
//
//            // ���� ����� ����(50% ���� ����)
//            int origIndex = y * rowBytes + x * 3;
//            bitmapData[origIndex + 2] = static_cast<BYTE>((r + bitmapData[origIndex + 2]) / 2);
//            bitmapData[origIndex + 1] = static_cast<BYTE>((g + bitmapData[origIndex + 1]) / 2);
//            bitmapData[origIndex] = static_cast<BYTE>((b + bitmapData[origIndex]) / 2);
//        }
//    }
//
//    // ����� hdc�� ����
//    BitBlt(hdc, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);
//
//    // �޸� ����
//    SelectObject(hMemDC, hOldBitmap);
//    DeleteObject(hBitmap);
//    DeleteDC(hMemDC);
//}


//struct RGBColor {
//    int r, g, b;
//
//    RGBColor(int red, int green, int blue) : r(red), g(green), b(blue) {}
//    RGBColor() : r(0), g(0), b(0) {}
//
//    RGBColor operator+(const RGBColor& other) const {
//        return RGBColor(r + other.r, g + other.g, b + other.b);
//    }
//
//    RGBColor operator-(const RGBColor& other) const {
//        return RGBColor(r - other.r, g - other.g, b - other.b);
//    }
//
//    RGBColor operator/(int divisor) const {
//        return RGBColor(r / divisor, g / divisor, b / divisor);
//    }
//
//    RGBColor ColorLerp(const RGBColor& other, float t) const {
//        return RGBColor(
//            static_cast<int>(r * (1 - t) + other.r * t),
//            static_cast<int>(g * (1 - t) + other.g * t),
//            static_cast<int>(b * (1 - t) + other.b * t)
//        );
//    }
//};
//
//void Blur(HDC hdc, int blurSize) {
//    int width = SCREEN_WIDTH;
//    int height = SCREEN_HEIGHT;
//
//    // HBITMAP ��ü ��������
//    HBITMAP hBitmap = (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP);
//    if (!hBitmap) return;
//
//    // DIB ���� �� �޸� �Ҵ�
//    BITMAPINFOHEADER biHeader;
//    biHeader.biSize = sizeof(BITMAPINFOHEADER);
//    biHeader.biWidth = width;
//    biHeader.biHeight = -height;
//    biHeader.biPlanes = 1;
//    biHeader.biBitCount = 24;
//    biHeader.biCompression = BI_RGB;
//    biHeader.biSizeImage = width * height * 3;
//
//    std::vector<BYTE> bitmapData(width * height * 3);
//    GetDIBits(hdc, hBitmap, 0, height, bitmapData.data(), (BITMAPINFO*)&biHeader, DIB_RGB_COLORS);
//
//    // ���� �����͸� RGBColor �������� ����
//    std::vector<std::vector<RGBColor>> colors(width, std::vector<RGBColor>(height));
//#pragma omp parallel for collapse(2)
//    for (int j = 0; j < height; ++j) {
//        for (int i = 0; i < width; ++i) {
//            int index = (j * width + i) * 3;
//            colors[i][j] = RGBColor(bitmapData[index + 2], bitmapData[index + 1], bitmapData[index]);
//        }
//    }
//
//    // ���� ���̺� ����
//    std::vector<std::vector<RGBColor>> sumTable(width + 1, std::vector<RGBColor>(height + 1));
//    for (int i = 1; i <= width; ++i) {
//        for (int j = 1; j <= height; ++j) {
//            sumTable[i][j] = colors[i - 1][j - 1] + sumTable[i - 1][j] + sumTable[i][j - 1] - sumTable[i - 1][j - 1];
//        }
//    }
//
//    // �� ó��
//#pragma omp parallel for collapse(2)
//    for (int j = 0; j < height; ++j) {
//        for (int i = 0; i < width; ++i) {
//            int left = max(i - blurSize, 0);
//            int right = min(i + blurSize, width - 1);
//            int top = max(j - blurSize, 0);
//            int bottom = min(j + blurSize, height - 1);
//
//            int area = (right - left + 1) * (bottom - top + 1);
//
//            RGBColor sum = sumTable[right + 1][bottom + 1]
//                - sumTable[left][bottom + 1]
//                - sumTable[right + 1][top]
//                + sumTable[left][top];
//            RGBColor avgColor = sum / area;
//            RGBColor finalColor = avgColor.ColorLerp(colors[i][j], 0.5f);
//
//            int index = (j * width + i) * 3;
//            bitmapData[index] = static_cast<BYTE>(finalColor.b);
//            bitmapData[index + 1] = static_cast<BYTE>(finalColor.g);
//            bitmapData[index + 2] = static_cast<BYTE>(finalColor.r);
//        }
//    }
//
//    // ����� ȭ�鿡 �ݿ�
//    SetDIBits(hdc, hBitmap, 0, height, bitmapData.data(), (BITMAPINFO*)&biHeader, DIB_RGB_COLORS);
//}




//
//struct RGBColor {
//public:
//    int r, g, b;
//
//    RGBColor(int red, int green, int blue) : r(red), g(green), b(blue) {}
//    RGBColor() : r(0), g(0), b(0) {}
//
//    RGBColor operator+(const RGBColor& other) const {
//        return RGBColor(r + other.r, g + other.g, b + other.b);
//    }
//
//    RGBColor operator-(const RGBColor& other) const {
//        return RGBColor(r - other.r, g - other.g, b - other.b);
//    }
//
//    RGBColor operator/(int divisor) const {
//        return RGBColor(r / divisor, g / divisor, b / divisor);
//    }
//
//    RGBColor ColorLerp(const RGBColor& other, float t) const {
//        return RGBColor(
//            static_cast<int>(r * (1 - t) + other.r * t),
//            static_cast<int>(g * (1 - t) + other.g * t),
//            static_cast<int>(b * (1 - t) + other.b * t)
//        );
//    }
//
//    COLORREF ToCOLORREF() const {
//        return RGB(r, g, b);
//    }
//};
//
//void Blur(HDC hdc, int blurSize) {
//    int width = SCREEN_WIDTH;
//    int height = SCREEN_HEIGHT;
//
//    // �̹��� ������ DIB�� ��������
//    BITMAPINFOHEADER biHeader;
//    biHeader.biSize = sizeof(BITMAPINFOHEADER);
//    biHeader.biWidth = width;
//    biHeader.biHeight = -height; // ������ ��ܿ��� ����
//    biHeader.biPlanes = 1;
//    biHeader.biBitCount = 24; // 24 ��Ʈ �÷�
//    biHeader.biCompression = BI_RGB;
//    biHeader.biSizeImage = width * height * 3;
//    biHeader.biXPelsPerMeter = 0;
//    biHeader.biYPelsPerMeter = 0;
//    biHeader.biClrUsed = 0;
//    biHeader.biClrImportant = 0;
//
//    // �޸� �Ҵ�
//    std::vector<BYTE> bitmapData(width * height * 3);
//
//    // �̹��� ���� ��������
//    GetDIBits(hdc, (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP), 0, height, bitmapData.data(), (BITMAPINFO*)&biHeader, DIB_RGB_COLORS);
//
//    // ���� ������ ó��
//    std::vector<std::vector<RGBColor>> colors(width, std::vector<RGBColor>(height));
//    for (int i = 0; i < width; ++i) {
//        for (int j = 0; j < height; ++j) {
//            int index = (j * width + i) * 3;
//            BYTE b = bitmapData[index];
//            BYTE g = bitmapData[index + 1];
//            BYTE r = bitmapData[index + 2];
//            colors[i][j] = RGBColor(r, g, b);
//        }
//    }
//
//    // ���� �̹��� ��� (������ ���̺� ����)
//    std::vector<std::vector<RGBColor>> sumTable(width + 1, std::vector<RGBColor>(height + 1));
//    for (int i = 1; i <= width; ++i) {
//        for (int j = 1; j <= height; ++j) {
//            sumTable[i][j] = colors[i - 1][j - 1]
//                + sumTable[i - 1][j]
//                + sumTable[i][j - 1]
//                - sumTable[i - 1][j - 1];
//        }
//    }
//
//    // �� ó��
//    for (int i = 0; i < width; ++i) {
//        for (int j = 0; j < height; ++j) {
//            int left = max(i - blurSize, 0);
//            int right = min(i + blurSize, width - 1);
//            int top = max(j - blurSize, 0);
//            int bottom = min(j + blurSize, height - 1);
//
//            int area = (right - left + 1) * (bottom - top + 1);
//
//            // ������ ��� ���
//            RGBColor sum = sumTable[right + 1][bottom + 1]
//                - sumTable[left][bottom + 1]
//                - sumTable[right + 1][top]
//                + sumTable[left][top];
//            RGBColor avgColor = sum / area;
//
//            // ���� ����� ��� ������ ����
//            RGBColor finalColor = avgColor.ColorLerp(colors[i][j], 0.5f);
//
//            // ������ ������ �޸� ���ۿ� ����
//            int index = (j * width + i) * 3;
//            bitmapData[index] = (BYTE)finalColor.b;
//            bitmapData[index + 1] = (BYTE)finalColor.g;
//            bitmapData[index + 2] = (BYTE)finalColor.r;
//        }
//    }
//
//    // ������ �̹��� �����͸� ȭ�鿡 �ݿ�
//    SetDIBits(hdc, (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP), 0, height, bitmapData.data(), (BITMAPINFO*)&biHeader, DIB_RGB_COLORS);
//}



//int Lerp(int a, int b, float t)
//{
//    return (int)(a * (1 - t) + b * (t));
//}
//
//struct RGBColor {
//public:
//    int r, g, b;
//
//    RGBColor(int red, int green, int blue) : r(red), g(green), b(blue) {}
//    RGBColor() : r(0), g(0), b(0) {}
//
//    RGBColor operator+(const RGBColor& other) const {
//        return RGBColor(r + other.r, g + other.g, b + other.b);
//    }
//
//    RGBColor operator-(const RGBColor& other) const {
//        return RGBColor(r - other.r, g - other.g, b - other.b);
//    }
//
//    RGBColor operator/(int divisor) const {
//        return RGBColor(r / divisor, g / divisor, b / divisor);
//    }
//
//    RGBColor ColorLerp(const RGBColor& other, float t) const {
//        return RGBColor(
//            static_cast<int>(r * (1 - t) + other.r * t),
//            static_cast<int>(g * (1 - t) + other.g * t),
//            static_cast<int>(b * (1 - t) + other.b * t)
//        );
//    }
//
//    COLORREF ToCOLORREF() const {
//        return RGB(r, g, b);
//    }
//};
//
//void Blur(HDC hdc, int blurSize) {
//    int width = SCREEN_WIDTH;
//    int height = SCREEN_HEIGHT;
//
//    // ���� �̹��� ���� �����͸� �����ͼ� ����
//    std::vector<std::vector<RGBColor>> colors(width, std::vector<RGBColor>(height));
//    for (int i = 0; i < width; ++i) {
//        for (int j = 0; j < height; ++j) {
//            COLORREF color = GetPixel(hdc, i, j);
//            colors[i][j] = RGBColor(GetRValue(color), GetGValue(color), GetBValue(color));
//        }
//    }
//
//    // ���� �̹��� ��� (������ ���̺� ����)
//    std::vector<std::vector<RGBColor>> sumTable(width + 1, std::vector<RGBColor>(height + 1));
//    for (int i = 1; i <= width; ++i) {
//        for (int j = 1; j <= height; ++j) {
//            sumTable[i][j] = colors[i - 1][j - 1]
//                + sumTable[i - 1][j]
//                + sumTable[i][j - 1]
//                - sumTable[i - 1][j - 1];
//        }
//    }
//
//    // �� ó��
//    for (int i = 0; i < width; ++i) {
//        for (int j = 0; j < height; ++j) {
//            int left = max(i - blurSize, 0);
//            int right = min(i + blurSize, width - 1);
//            int top = max(j - blurSize, 0);
//            int bottom = min(j + blurSize, height - 1);
//
//            int area = (right - left + 1) * (bottom - top + 1);
//
//            // ������ ��� ���
//            RGBColor sum = sumTable[right + 1][bottom + 1]
//                - sumTable[left][bottom + 1]
//                - sumTable[right + 1][top]
//                + sumTable[left][top];
//            RGBColor avgColor = sum / area;
//
//            // ���� ����� ��� ������ ����
//            RGBColor finalColor = avgColor.ColorLerp(colors[i][j], 0.5f);
//
//            SetPixel(hdc, i, j, finalColor.ToCOLORREF());
//        }
//    }
//}




//float Lerp(float a, float b, float t)
//{
//    return (a * (1 - t) + b * (t));
//}


//struct RGBColor {
//public:
//    int r, g, b;
//
//    RGBColor(int red, int green, int blue) : r(red), g(green), b(blue) {}
//    RGBColor() : r(0), g(0), b(0) {}
//
//    RGBColor operator+(const RGBColor other) const {
//        return RGBColor(r + other.r, g + other.g, b + other.b);
//    }
//
//    RGBColor operator-(const RGBColor other) const {
//        return RGBColor(r - other.r, g - other.g, b - other.b);
//    }
//
//    RGBColor operator/(int divisor) const {
//        return RGBColor(r / divisor, g / divisor, b / divisor);
//    }
//
//    RGBColor ColorLerp(RGBColor other, float t) const {
//        return RGBColor(
//            Lerp(r, other.r, t),
//            Lerp(g, other.g, t),
//            Lerp(b, other.b, t)
//        );
//    }
//
//    COLORREF RGB2COLOREF() const {
//        return RGB(r, g, b);
//    }
//};
//
//void Blur(HDC hdc, int blurSize) {
//    int width = SCREEN_WIDTH;
//    int height = SCREEN_HEIGHT;
//    std::vector<std::vector<RGBColor>> colors(width, std::vector<RGBColor>(height));
//    std::vector<std::vector<RGBColor>> sumTable(width + 1, std::vector<RGBColor>(height + 1));
//
//    // 1. ���� �̹��� ���� �����͸� �����ͼ� ����
//    for (int i = 0; i < width; ++i) {
//        for (int j = 0; j < height; ++j) {
//            COLORREF color = GetPixel(hdc, i, j);
//            colors[i][j] = RGBColor(GetRValue(color), GetGValue(color), GetBValue(color));
//        }
//    }
//
//    // 2. ���� �̹��� ���
//    for (int i = 1; i <= width; ++i) {
//        for (int j = 1; j <= height; ++j) {
//            sumTable[i][j] = colors[i - 1][j - 1] + sumTable[i - 1][j] + sumTable[i][j - 1] - sumTable[i - 1][j - 1];
//        }
//    }
//
//    // 3. �� ó��
//    for (int i = 0; i < width; ++i) {
//        for (int j = 0; j < height; ++j) {
//            int left = max(i - blurSize, 0);
//            int right = min(i + blurSize, width - 1);
//            int top = max(j - blurSize, 0);
//            int bottom = min(j + blurSize, height - 1);
//
//            int area = (right - left + 1) * (bottom - top + 1);
//
//            RGBColor sum = sumTable[right + 1][bottom + 1] - sumTable[left][bottom + 1] - sumTable[right + 1][top] + sumTable[left][top];
//            RGBColor avgColor = sum / area;
//            RGBColor finalColor = avgColor.ColorLerp(colors[i][j], 0.5f);
//
//            SetPixel(hdc, i, j, finalColor.RGB2COLOREF());
//        }
//    }
//}



//struct RGBColor
//{
//public:
//    int r, g, b;
//
//    RGBColor(int red, int green, int blue)
//    {
//        r = red;
//        g = green;
//        b = blue;
//    }
//    RGBColor()
//    {
//
//    }
//    RGBColor operator+(const RGBColor other) const {
//        return RGBColor(r + other.r, g + other.g, b + other.b);
//    }
//    RGBColor operator-(const RGBColor other) const {
//        return RGBColor(r - other.r, g - other.g, b - other.b);
//    }
//    RGBColor operator*(const RGBColor other) const {
//        return RGBColor(r * other.r, g * other.g, b * other.b);
//    }
//    RGBColor operator/(const RGBColor other) const {
//        return RGBColor(r / other.r, g / other.g, b / other.b);
//    }
//
//    RGBColor ColorLerp(RGBColor other,float t)
//    {
//        return RGBColor(
//            Lerp(r, other.r, t),
//            Lerp(g, other.g, t),
//            Lerp(b, other.b, t)
//        );
//    }
//
//    COLORREF RGB2COLOREF() 
//    {
//        return RGB(r, g, b);
//    }
//};
//
//void PostProcess(HDC hdc)
//{
//}
//
//void Bloom(HDC hdc)
//{
//}
//
//
//
//std::pair<int, int> getDrawingAreaDimensions(HDC context)
//{
//    // Drawing area is a bitmap
//    {
//        BITMAP header;
//        ZeroMemory(&header, sizeof(BITMAP));
//
//        HGDIOBJ bmp = GetCurrentObject(context, OBJ_BITMAP);
//        GetObject(bmp, sizeof(BITMAP), &header);
//        const int width = header.bmWidth;
//        const int height = header.bmHeight;
//        if (width > 1 && height > 1) {
//            return { width, height };
//        }
//    }
//
//    // Drawing area is a printer page
//    const int width = GetDeviceCaps(context, HORZRES);
//    const int height = GetDeviceCaps(context, VERTRES);
//    return { width, height };
//}
//RGBColor colors[SCREEN_WIDTH][SCREEN_HEIGHT];
//
//void Blur(HDC hdc, int blurSize)
//{
//
//
//    std::pair<int, int> hdcSize;// = getDrawingAreaDimensions(hdc);
//    hdcSize.first = SCREEN_WIDTH;
//    hdcSize.second = SCREEN_HEIGHT;
//
//    for (int i = 0; i < hdcSize.first; i++)
//    {
//        for (int j = 0; j < hdcSize.second; j++)
//        {
//            COLORREF color = GetPixel(hdc, i, j);
//            colors[i][j] = RGBColor(GetRValue(color), GetGValue(color), GetBValue(color));
//        }
//
//        for (int j = 0; j < hdcSize.second; j++)
//        {
//            RGBColor midColor(0,0,0);
//            int cnt = 0;
//            for (int k = -blurSize; k < blurSize; k++)
//            {
//                for (int l = -blurSize; l < blurSize; l++)
//                {
//                    if (i + k > 0 && j + l > 0 && i + k < hdcSize.first && j + l < hdcSize.second)
//                    {
//                        midColor = midColor + colors[i + k][j + l];
//                        cnt++;
//                    }
//                }
//            }
//            if (cnt != 0)
//            {
//                midColor.r /= cnt;
//                midColor.g /= cnt;
//                midColor.b /= cnt;
//            }
//
//            for (int k = -blurSize; k < blurSize; k++)
//            {
//                for (int l = -blurSize; l < blurSize; l++)
//                {
//                    if (i + k > 0 && j + l > 0 && i + k < hdcSize.first && j + l < hdcSize.second)
//                    {
//                        RGBColor color;
//                        color = midColor.ColorLerp(colors[i + k][j + l], 0.5f);
//                        SetPixel(hdc, i + k, j + l, color.RGB2COLOREF());
//                    }
//                }
//            }
//        }
//    }
//}