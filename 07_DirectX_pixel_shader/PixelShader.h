#ifndef _WINDOWS_EXAMPLE_PIXEL_SHADER_H_
#define _WINDOWS_EXAMPLE_PIXEL_SHADER_H_

#include <iostream>
#include <vector>
#include <atlbase.h>
#include <d3d11.h>

class PixelShader
{
public:
  PixelShader(int width, int height);
  virtual ~PixelShader();

private:
  void init();

private:
  int width_, height_;

};
#endif