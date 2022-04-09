#include "VideoDecoderHW.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

VideoDecoderHW::VideoDecoderHW(int width, int height) :
  width_(width),
  height_(height)
{
  HRESULT hr = MFStartup(MF_VERSION);
  if (FAILED(hr))
  {
    printf("MFStartup failed\n");
    std::terminate();
  }

  init();
}

VideoDecoderHW::~VideoDecoderHW()
{
  HRESULT hr = MFShutdown();
  if (FAILED(hr))
  {
    printf("MFShutdown failed\n");
    std::terminate();
  }
}

std::vector<uint8_t> VideoDecoderHW::Decode(std::vector<uint8_t> ibuffer)
{
  std::vector<uint8_t> obuffer(width_ * height_ * 3 / 2);

  CComPtr<IMFMediaBuffer> media_ibuf, media_obuf;
  HRESULT hr = MFCreateMemoryBuffer(width_ * height_ * 4, &media_ibuf);
  if (FAILED(hr))
  {
    printf("MFCreateMemoryBuffer failed");
    std::terminate();
  }

  CComPtr<IMFSample> input_sample_;
  hr = MFCreateSample(&input_sample_);
  if (FAILED(hr))
  {
    printf("MFCreateSample failed");
  }

  input_sample_->SetSampleTime(0);
  input_sample_->SetSampleDuration(0);

  hr = input_sample_->AddBuffer(media_ibuf);
  if (FAILED(hr))
  {
    printf("input_sample_->AddBuffer(buffer) failed");
    std::terminate();
  }

  BYTE* p = NULL;
  hr = media_ibuf->Lock(&p, NULL, NULL);
  if (FAILED(hr))
  {
    printf(" media_buf->Lock failed");
  }

  memcpy(p, ibuffer.data(), ibuffer.size());

  hr = media_ibuf->SetCurrentLength(static_cast<DWORD>(ibuffer.size()));
  if (FAILED(hr))
  {
    printf("media_buf->SetCurrentLength failed");
  }

  hr = media_ibuf->Unlock();
  if (FAILED(hr))
  {
    printf("media_buf->Unlock failed");
  }

  hr = mft_->ProcessInput(0, input_sample_, 0);
  if (FAILED(hr))
  {
    printf("mft->ProcessInput");
  }

   DWORD status = 0;
  MFT_OUTPUT_DATA_BUFFER output_data = { 0, nullptr, 0, nullptr };

OUTPUT:
  hr = mft_->ProcessOutput(0, 1, &output_data, &status);
  if (FAILED(hr))
  {
    if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
    {
      setOutputType();
      goto OUTPUT;
    }
    printf("mft->ProcessOutput failed");
  }

  CComPtr<IMFSample> sample;
  sample.Attach(output_data.pSample);

  hr = sample->GetBufferByIndex(0, &media_obuf);
  if (FAILED(hr))
  {
    printf("sample->GetBufferByIndex failed");
    std::terminate();
  }

  p = NULL;
  DWORD length = 0;
  hr = media_obuf->Lock(&p, NULL, &length);
  if (FAILED(hr))
  {
    printf(" media_buf->Lock failed");
  }

  obuffer.resize(length);
  memcpy(obuffer.data(), p, length);

  hr = media_obuf->Unlock();
  if (FAILED(hr))
  {
    printf("media_buf->Unlock failed");
  }

  return obuffer;
}

void VideoDecoderHW::init()
{
  // 인코더와 달리 input 타입이 먼저 설정됨에 주의!!
  findEncoder();
  initDirectX();
  setDecoderOption();
  setInputType();
  setOutputType();
  setReady();
}

void VideoDecoderHW::findEncoder()
{
  CComHeapPtr<IMFActivate*> activate;
  UINT32 count = 0;
  MFT_REGISTER_TYPE_INFO intput_type, output_type;
  intput_type.guidMajorType = MFMediaType_Video;
  intput_type.guidSubtype = MFVideoFormat_H264;
  output_type.guidMajorType = MFMediaType_Video;
  output_type.guidSubtype = MFVideoFormat_NV12;
  UINT32 flags = MFT_ENUM_FLAG_SYNCMFT;

  HRESULT hr = MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER, flags, &intput_type, &output_type, &activate, &count);
  if (FAILED(hr) || !count)
  {
    printf("MFTEnumEx failed : %d", count);
    std::terminate();
  }

  hr = activate[0]->ActivateObject(IID_PPV_ARGS(&mft_));
  if (FAILED(hr))
  {
    printf("ActivateObject failed");
  }

  for (UINT32 i = 0; i < count; i++)
  {
    activate[i]->Release();
  }
}

void VideoDecoderHW::initDirectX()
{
  HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &device_, NULL, &context_);
  if (FAILED(hr))
  {
    printf("D3D11CreateDevice failed\n");
    std::terminate();
  }

  CComPtr<IMFAttributes> attributes;
  hr = mft_->GetAttributes(&attributes);
  if (FAILED(hr))
  {
    printf("mft_->GetAttributes failed\n");
    std::terminate();
  }

  // async를지원하는지 확인
  // 하드웨어 가속의 경우 "항상" asnyc함
  // 하드웨어가 지원하지 않아 SW구현으로 우회하려면 이 모든부분을 고려해서 코드 작성해야함
  uint32_t async = FALSE;
  hr = attributes->GetUINT32(MF_TRANSFORM_ASYNC, &async);
  if (SUCCEEDED(hr) && async)
  {
    hr = attributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, TRUE);
    if (FAILED(hr))
    {
      printf("attributes->GetUINT32(MF_TRANSFORM_ASYNC_UNLOCK) failed\n");
      std::terminate();
    }
  }

  UINT32 d3d11_aware = false;
  hr = attributes->GetUINT32(MF_SA_D3D11_AWARE, &d3d11_aware);
  if (SUCCEEDED(hr) && d3d11_aware)
  {
    UINT resetToken = 0;
    CComPtr<IMFDXGIDeviceManager> device_manager;
    hr = MFCreateDXGIDeviceManager(&resetToken, &device_manager);
    if (FAILED(hr))
    {
      printf("MFCreateDXGIDeviceManager failed\n");
      std::terminate();
    }

    hr = device_manager->ResetDevice(device_, resetToken);
    if (FAILED(hr))
    {
      printf("device_manager->ResetDevice failed\n");
      std::terminate();
    }

    hr = mft_->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, (ULONG_PTR)device_manager.p);
    if (FAILED(hr))
    {
      printf("mft_->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER failed\n");
      std::terminate();
    }
  }
}

void VideoDecoderHW::setDecoderOption()
{
  CComPtr<IMFAttributes> attributes;
  HRESULT hr = mft_->GetAttributes(&attributes);
  if (FAILED(hr))
  {
    printf("ft_->GetAttributes(&attributes); failed");
  }

  hr = attributes->SetUINT32(MF_LOW_LATENCY, TRUE);
  if (FAILED(hr))
  {
    printf("attributes->SetUINT32(MF_LOW_LATENCY failed");
  }
}

void VideoDecoderHW::setInputType()
{
  HRESULT hr = S_OK;
  GUID id = { 0 };
  CComPtr<IMFMediaType> meida_type;
  for (int i = 0; !IsEqualGUID(id, MFVideoFormat_H264); ++i)
  {
    meida_type = nullptr;

    hr = mft_->GetInputAvailableType(0, i, &meida_type);
    if (FAILED(hr))
    {
      printf("GetOutputAvailableType failed");
      std::terminate();
    }


    hr = meida_type->GetGUID(MF_MT_SUBTYPE, &id);
    if (FAILED(hr))
    {
      printf(" meida_type->SetGUID(MF_MT_SUBTYPE) failed");
      std::terminate();
    }
  }

  hr = MFSetAttributeSize(meida_type, MF_MT_FRAME_SIZE, width_, height_);
  if (FAILED(hr))
  {
    printf("MFSetAttributeSize failed");
    std::terminate();
  }

  hr = MFSetAttributeRatio(meida_type, MF_MT_FRAME_RATE, 30, 1);
  if (FAILED(hr))
  {
    printf("MFSetAttributeRatio failed");
    std::terminate();
  }


  hr = mft_->SetInputType(0, meida_type, 0);
  if (FAILED(hr))
  {
    printf("mft_->SetInputType failed");
    std::terminate();
  }
}

void VideoDecoderHW::setOutputType()
{
  HRESULT hr = S_OK;
  GUID id = { 0 };
  CComPtr<IMFMediaType> meida_type;

  for (int i = 0; !IsEqualGUID(id, MFVideoFormat_NV12); ++i)
  {
    meida_type = nullptr;

    hr = mft_->GetOutputAvailableType(0, i, &meida_type);
    if (FAILED(hr))
    {
      printf("mft_->GetOutputAvailableType failed");
      std::terminate();
    }

    hr = meida_type->GetGUID(MF_MT_SUBTYPE, &id);
    if (FAILED(hr))
    {
      printf("meida_type->GetGUID failed");
      std::terminate();
    }
  }

  hr = meida_type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
  if (FAILED(hr))
  {
    printf("meida_type->SetGUID(MF_MT_MAJOR_TYPE) failed");
    std::terminate();
  }

  hr = meida_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
  if (FAILED(hr))
  {
    printf(" meida_type->SetGUID(MF_MT_SUBTYPE) failed");
    std::terminate();
  }

  //hr = MFSetAttributeSize(meida_type, MF_MT_FRAME_SIZE, width_, height_);
  //if (FAILED(hr))
  //{
  //  printf("MFSetAttributeSize failed");
  //  std::terminate();
  //}

  hr = MFSetAttributeRatio(meida_type, MF_MT_FRAME_RATE, 30, 1);
  if (FAILED(hr))
  {
    printf("MFSetAttributeRatio failed");
    std::terminate();
  }

  hr = mft_->SetOutputType(0, meida_type, 0);
  if (FAILED(hr))
  {
    printf("mft_->SetOutputType failed");
    std::terminate();
  }
}

void VideoDecoderHW::setReady()
{
  // 선택사항 메세지.
  // 메세지를 보내면 최초 인코딩 할때 필요한 초기화를 미리 수행해놓기 때문에 응답속도가 빨라짐
  HRESULT hr = mft_->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
  if (FAILED(hr))
  {
    printf("MFT_MESSAGE_NOTIFY_BEGIN_STREAMING failed");
    std::terminate();
  }

  // Sync:선택, Async:필수
  hr = mft_->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
  if (FAILED(hr))
  {
    printf("MFT_MESSAGE_NOTIFY_START_OF_STREAM failed");
    std::terminate();
  }
}