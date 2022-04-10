#include "PixelShader.h"

#pragma comment(lib, "d3d11.lib")

PixelShader::PixelShader(int width, int height) :
  width_(width),
  height_(height)
{
  init();
}

PixelShader::~PixelShader()
{

}

void PixelShader::init()
{
  //findEncoder();
  //setEncoderOption();
  //setOutputType();
  //setInputType();  
}