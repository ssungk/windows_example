#include "PixelShader.h"

#define FRAME_COUNT 300

int main()
{
  // 주모니터 해상도(본예제는 첫번째 예제와 달리 다중 모니터처리 안함)
  int width = GetSystemMetrics(SM_CXSCREEN);
  int height = GetSystemMetrics(SM_CYSCREEN);

  PixelShader shader(width, height);

  FILE *fr, *fw;
  fopen_s(&fr, "../screen0_decode_sw.yuv", "rb");
  fopen_s(&fw, "../screen0_reconstruction.bgr", "wb");

  std::vector<uint8_t> buf(width * height * 3 / 2);

  for (int i = 0; i < FRAME_COUNT; i++)
  {
    printf("[%03d] Pixel Shadere frame\n", i);
    //fread(buf.data(), width * height * 3 / 2, 1, fr);

   

    //fwrite(buffer.data(), buffer.size(), 1, fw);
  }

  return 0;
}