#include "VideoProcessor.h"

#define FRAME_COUNT 300

int main()
{
  // 주모니터 해상도(본예제는 첫번째 예제와 달리 다중 모니터처리 안함)
  int width = GetSystemMetrics(SM_CXSCREEN);
  int height = GetSystemMetrics(SM_CYSCREEN);

  VideoProcessor vp(width, height);

  FILE *fr, *fw;
  fopen_s(&fr, "../screen0.bgr", "rb");
  fopen_s(&fw, "../screen0.nv12", "wb");

  std::vector<uint8_t> buf(width * height * 4);

  for (int i = 0; i < FRAME_COUNT; i++)
  {
    printf("[%d]\n", i);
    fread(buf.data(), width * height * 4, 1, fr);

    auto buffer = vp.Process(buf);

    fwrite(buffer.data(), buffer.size(), 1, fw);
  }

  return 0;
}