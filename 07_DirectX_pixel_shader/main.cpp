#include "PixelShader.h"

#define FRAME_COUNT 300

int main()
{
  // �ָ���� �ػ�(�������� ù��° ������ �޸� ���� �����ó�� ����)
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