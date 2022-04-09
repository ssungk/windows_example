#ifndef _WINDOWS_EXAMPLE_VIDEO_DECODER_HW_H_
#define _WINDOWS_EXAMPLE_VIDEO_DECODER_HW_H_

#include <iostream>
#include <vector>
#include <future>
#include <atlbase.h>
#include <mfapi.h>
#include <mftransform.h>
#include <mferror.h>
#include <codecapi.h>
#include <strmif.h>
#include <d3d11.h>

class VideoDecoderHW
{
public:
  VideoDecoderHW(int width, int height);
  virtual ~VideoDecoderHW();
  std::vector<uint8_t> Decode(std::vector<uint8_t> buffer);

private:
  void init();
  void findEncoder();
  void initDirectX();
  void setDecoderOption();
  void setInputType();
  void setOutputType();
  void setReady();

private:
  int width_, height_;
  CComPtr<IMFTransform> mft_;
  CComPtr<ID3D11Device> device_;
  CComPtr<ID3D11DeviceContext> context_;
  CComPtr<IMFMediaEventGenerator> eg_;
  std::promise<bool> promise_[2];

};
#endif