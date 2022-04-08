#ifndef _WINDOWS_EXAMPLE_VIDEO_DECODER_SW_H_
#define _WINDOWS_EXAMPLE_VIDEO_DECODER_SW_H_

#include <iostream>
#include <vector>
#include <atlbase.h>
#include <mfapi.h>
#include <mftransform.h>
#include <codecapi.h>
#include <strmif.h>
#include <d3d11.h>

class VideoDecoderSW
{
public:
  VideoDecoderSW(int width, int height);
  virtual ~VideoDecoderSW();
  std::vector<uint8_t> Decode(std::vector<uint8_t> buffer);

private:
  void init();
  void findDecoder();
  void setDecoderOption();
  void setInputType();
  void setOutputType();

private:
  int width_, height_;
  CComPtr<IMFTransform> mft_;

};
#endif