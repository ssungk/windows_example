#ifndef _WINDOWS_EXAMPLE_VIDEO_PROCESSOR_H_
#define _WINDOWS_EXAMPLE_VIDEO_PROCESSOR_H_

#include <iostream>
#include <vector>
#include <atlbase.h>
#include <mfapi.h>
#include <mftransform.h> 
#include <d3d11.h>

class VideoProcessor
{
public:
  VideoProcessor(int width, int height);
  virtual ~VideoProcessor();
  std::vector<uint8_t> Process(std::vector<uint8_t> buffer);

private:
  void init();
  void findVideoProcessor();
  void setInputType();
  void setOutputType();

private:
  int width_, height_;
  CComPtr<IMFTransform> mft_;

};
#endif