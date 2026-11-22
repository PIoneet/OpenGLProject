#pragma once

#include "Common.h"

// ============================================================
//  Screenshot.h  -  화면을 BMP 로 저장
//
//  보고서의 "실행 결과 스크린샷" 을 바로 뽑기 위한 기능.
//  외부 이미지 라이브러리 없이 쓸 수 있도록 24비트 BMP 로 저장한다.
//  (BMP 는 행 순서가 아래에서 위 -> glReadPixels 결과와 그대로 맞는다)
//
//  반드시 glfwSwapBuffers 호출 "전" 에 불러야 한다. (백버퍼를 읽는다)
// ============================================================

bool SaveScreenshotBMP(const std::string& path, int width, int height);

// 저장 경로를 shot_0001.bmp, shot_0002.bmp ... 로 자동 증가시키며 저장한다.
bool SaveScreenshotAuto(int width, int height, std::string& outPath);
