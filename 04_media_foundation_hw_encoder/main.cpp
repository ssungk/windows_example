#include "VideoEncoderHW.h"

#define FRAME_COUNT 300

int main()
{
  // �ָ���� �ػ�(�������� ù��° ������ �޸� ���� �����ó�� ����)
  int width = GetSystemMetrics(SM_CXSCREEN);
  int height = GetSystemMetrics(SM_CYSCREEN);

  VideoEncoderHW enc(width, height);

  FILE *fr, *fw;
  fopen_s(&fr, "../screen0.nv12", "rb");
  fopen_s(&fw, "../screen0_hw.h264", "wb");

  std::vector<uint8_t> buf(width * height * 3 / 2);

  for (int i = 0; i < FRAME_COUNT; i++)
  {
    printf("[%003d] meida foundation SW Encode frame\n", i);
    fread(buf.data(), width * height * 3 / 2, 1, fr);

    auto buffer = enc.Encode(buf);

    fwrite(buffer.data(), buffer.size(), 1, fw);
  }

  return 0;
}