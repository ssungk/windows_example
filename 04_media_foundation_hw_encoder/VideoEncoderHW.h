#ifndef _WINDOWS_EXAMPLE_VIDEO_ENCODER_HW_H_
#define _WINDOWS_EXAMPLE_VIDEO_ENCODER_HW_H_

#include <iostream>
#include <vector>
#include <future>
#include <atlbase.h>
#include <mfapi.h>
#include <mftransform.h>
#include <codecapi.h>
#include <strmif.h>
#include <d3d11.h>

#include "AsyncCallback.h"

class VideoEncoderHW : public AsyncCallback
{
public:
  VideoEncoderHW(int width, int height);
  virtual ~VideoEncoderHW();
  std::vector<uint8_t> Encode(std::vector<uint8_t> buffer);

public:
  STDMETHODIMP Invoke(IMFAsyncResult* result) override;

private:
  void init();
  void findEncoder();
  void initDirectX();
  void setEncoderOption();
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