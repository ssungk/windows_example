#include "VideoDecoderSW.h"

#define FRAME_COUNT 300

int main()
{
  // 주모니터 해상도(본예제는 첫번째 예제와 달리 다중 모니터처리 안함)
  int width = GetSystemMetrics(SM_CXSCREEN);
  int height = GetSystemMetrics(SM_CYSCREEN);

  VideoDecoderSW dec(width, height);

  FILE *fr, *frs, *fw;
  fopen_s(&fr, "../screen0.h264", "rb");
  fopen_s(&frs, "../screen0.size", "rb");
  fopen_s(&fw, "../screen0_decode_sw.yuv", "wb");

  std::vector<uint8_t> buf(width * height * 3 / 2);

  for (int i = 0; i < FRAME_COUNT; i++)
  {
    printf("[%003d] meida foundation SW decode frame\n", i);

    uint32_t size = 0;
    fread(&size, sizeof(size), 1, frs);
    
    buf.resize(size);
    fread(buf.data(), size, 1, fr);

    auto buffer = dec.Decode(buf);

    fwrite(buffer.data(), buffer.size(), 1, fw);
  }

  return 0;
}