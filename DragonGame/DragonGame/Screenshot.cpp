#include "Screenshot.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace
{
    void PutU16(unsigned char* p, unsigned short v)
    {
        p[0] = static_cast<unsigned char>(v & 0xFF);
        p[1] = static_cast<unsigned char>((v >> 8) & 0xFF);
    }

    void PutU32(unsigned char* p, unsigned int v)
    {
        p[0] = static_cast<unsigned char>(v & 0xFF);
        p[1] = static_cast<unsigned char>((v >> 8) & 0xFF);
        p[2] = static_cast<unsigned char>((v >> 16) & 0xFF);
        p[3] = static_cast<unsigned char>((v >> 24) & 0xFF);
    }
}

bool SaveScreenshotBMP(const std::string& path, int width, int height)
{
    if (width <= 0 || height <= 0) return false;

    const int rowBytes = width * 3;
    const int padding = (4 - (rowBytes % 4)) % 4;      // BMP 는 행마다 4바이트 정렬
    const int paddedRow = rowBytes + padding;

    std::vector<unsigned char> pixels(static_cast<size_t>(rowBytes) * height);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    const unsigned int imageSize = static_cast<unsigned int>(paddedRow) * height;
    const unsigned int fileSize = 54 + imageSize;

    unsigned char header[54]{};
    header[0] = 'B'; header[1] = 'M';
    PutU32(header + 2, fileSize);
    PutU32(header + 10, 54);                  // 픽셀 데이터 시작 위치
    PutU32(header + 14, 40);                  // DIB 헤더 크기
    PutU32(header + 18, static_cast<unsigned int>(width));
    PutU32(header + 22, static_cast<unsigned int>(height));
    PutU16(header + 26, 1);                   // 플레인 수
    PutU16(header + 28, 24);                  // 비트 수
    PutU32(header + 34, imageSize);

    std::ofstream out(path, std::ios::binary);
    if (!out)
    {
        std::cerr << "스크린샷 저장 실패: " << path << std::endl;
        return false;
    }

    out.write(reinterpret_cast<const char*>(header), sizeof(header));

    std::vector<unsigned char> row(static_cast<size_t>(paddedRow), 0);
    for (int y = 0; y < height; ++y)
    {
        const unsigned char* src = pixels.data() + static_cast<size_t>(y) * rowBytes;

        // BMP 는 BGR 순서로 저장한다
        for (int x = 0; x < width; ++x)
        {
            row[static_cast<size_t>(x) * 3 + 0] = src[static_cast<size_t>(x) * 3 + 2];
            row[static_cast<size_t>(x) * 3 + 1] = src[static_cast<size_t>(x) * 3 + 1];
            row[static_cast<size_t>(x) * 3 + 2] = src[static_cast<size_t>(x) * 3 + 0];
        }

        out.write(reinterpret_cast<const char*>(row.data()), paddedRow);
    }

    return out.good();
}

bool SaveScreenshotAuto(int width, int height, std::string& outPath)
{
    static int counter = 0;
    ++counter;

    std::ostringstream ss;
    ss << "shot_" << std::setw(4) << std::setfill('0') << counter << ".bmp";
    outPath = ss.str();

    return SaveScreenshotBMP(outPath, width, height);
}
