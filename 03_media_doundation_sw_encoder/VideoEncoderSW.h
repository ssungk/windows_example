#ifndef _WINDOWS_EXAMPLE_VIDEO_ENCODER_SW_H_
#define _WINDOWS_EXAMPLE_VIDEO_ENCODER_SW_H_

#include <iostream>
#include <vector>
#include <atlbase.h>
#include <mfapi.h>
#include <mftransform.h>
#include <codecapi.h>
#include <strmif.h>
#include <d3d11.h>

class VideoEncoderSW
{
public:
  VideoEncoderSW(int width, int height);
  virtual ~VideoEncoderSW();
  std::vector<uint8_t> Encode(std::vector<uint8_t> buffer);

private:
  void init();
  void findEncoder();
  void setEncoderOption();
  void setInputType();
  void setOutputType();

private:
  int width_, height_;
  CComPtr<IMFTransform> mft_;

};
#endif