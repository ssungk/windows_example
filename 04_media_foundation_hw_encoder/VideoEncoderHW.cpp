#include "VideoEncoderHW.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

VideoEncoderHW::VideoEncoderHW(int width, int height) :
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

VideoEncoderHW::~VideoEncoderHW()
{
  HRESULT hr = MFShutdown();
  if (FAILED(hr))
  {
    printf("MFShutdown failed\n");
    std::terminate();
  }
}

std::vector<uint8_t> VideoEncoderHW::Encode(std::vector<uint8_t> ibuffer)
{
  std::vector<uint8_t> obuffer(width_ * height_* 3 / 2);

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
    std::terminate();
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
    std::terminate();
  }

  memcpy(p , ibuffer.data() , ibuffer.size());

  hr = media_ibuf->SetCurrentLength(static_cast<DWORD>(ibuffer.size()));
  if (FAILED(hr))
  {
    printf("media_buf->SetCurrentLength failed");
    std::terminate();
  }

  hr = media_ibuf->Unlock();
  if (FAILED(hr))
  {
    printf("media_buf->Unlock failed");
    std::terminate();
  }

  std::promise<bool> promise[2];
  bool ret = promise_[0].get_future().get();
  promise_[0] = std::move(promise[0]);

  hr = mft_->ProcessInput(0, input_sample_, 0);
  if (FAILED(hr))
  {
    printf("mft->ProcessInput");
    std::terminate();
  }

  ret = promise_[1].get_future().get();
  promise_[1] = std::move(promise[1]);

  DWORD status = 0;
  MFT_OUTPUT_DATA_BUFFER output_data = { 0, nullptr, 0, nullptr };

OUTPUT:
  hr = mft_->ProcessOutput(0, 1, &output_data, &status);
  if (FAILED(hr))
  {
    if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
    {
      setOutputType();

      bool ret = promise_[1].get_future().get();
      std::promise<bool> promise;
      promise_[1] = std::move(promise);
      goto OUTPUT;
    }
    printf("mft->ProcessOutput failed");
    std::terminate();
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
  hr = media_obuf->Lock(&p,NULL, &length);
  if (FAILED(hr))
  {
    printf("media_buf->Lock failed");
    std::terminate();
  }

  obuffer.resize(length);
  memcpy(obuffer.data(), p, length);

  hr = media_obuf->Unlock();
  if (FAILED(hr))
  {
    printf("media_buf->Unlock failed");
    std::terminate();
  }

  return obuffer;
}

STDMETHODIMP VideoEncoderHW::Invoke(IMFAsyncResult* result)
{
  // 이부분은 다른 Thread에서 동작됨, 주의할것!!!
  CComPtr<IMFMediaEvent> event;
  MediaEventType type;

  HRESULT hr = eg_->EndGetEvent(result, &event);
  if (FAILED(hr))
  {
    printf("eg_->EndGetEvent failed");
    std::terminate();
  }

  hr = event->GetType(&type);
  if (FAILED(hr))
  {
    printf("event->GetType failed");
    std::terminate();
  }

  switch (type)
  {
  case METransformNeedInput:
    printf("METransfoonNeedInput\n");
    promise_[0].set_value(true);
    break;
  case METransformHaveOutput:
    printf("METransformHaveOutput\n");
    promise_[1].set_value(true);
    break;
  case METransformDrainComplete:
  {
    printf("METransformDrainComplete\n");
    // 비동기식 mft 일경우에만 Shutdown 호출해야됨(MFShutdown는 항상 호출)
    //CComPtr<IMFShutdown> shutdown;
    //HRESULT hr = mft_->QueryInterface(IID_PPV_ARGS(&shutdown));
    //CHECK_HR(hr, "encoder->QueryInterface(IID_PPV_ARGS(&shutdown) failed");

    //hr = shutdown->Shutdown();
    //CHECK_HR(hr, "shutdown->Shutdown()");
    //Release();
  }
  return hr;
  case METransformMarker:                   printf("METransformMarker\n");                       break;
  case METransformInputStreamStateChanged:  printf("METransformInputStreamStateChanged\n");      break;
  default:                                  printf("default\n");                                 break;
  }

  hr = eg_->BeginGetEvent(this, nullptr);
  //CHECK_HR(hr, "event_generator_->BeginGetEvent");

  return hr;
}

void VideoEncoderHW::init()
{
  // 초기화중에 output 타입이 먼저 설정됨에 주의!!
  findEncoder();
  initDirectX();
  setEncoderOption();
  setOutputType();
  setInputType();
  setReady();
}

void VideoEncoderHW::findEncoder()
{
  CComHeapPtr<IMFActivate*> activate;
  UINT32 count = 0;
  MFT_REGISTER_TYPE_INFO intput_type, output_type;
  intput_type.guidMajorType = MFMediaType_Video;
  intput_type.guidSubtype = MFVideoFormat_NV12;
  output_type.guidMajorType = MFMediaType_Video;
  output_type.guidSubtype = MFVideoFormat_H264;
  UINT32 flags = MFT_ENUM_FLAG_HARDWARE;

  HRESULT hr = MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER, flags, &intput_type, &output_type, &activate, &count);
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

void VideoEncoderHW::initDirectX()
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

void VideoEncoderHW::setEncoderOption()
{
  // 인코더 옵션 설정
  CComPtr<ICodecAPI> codec;
  HRESULT hr = mft_->QueryInterface(IID_PPV_ARGS(&codec));
  //CHECK_HR(hr, "encoder->QueryInterface(IID_PPV_ARGS(&codec) failed");

  // rate control mode
  VARIANT var;
  var.vt = VT_UI4;
  var.ulVal = eAVEncCommonRateControlMode_Quality;
  hr = codec->SetValue(&CODECAPI_AVEncCommonRateControlMode, &var);
  //CHECK_HR(hr, "CODECAPI_AVEncCommonRateControlMode");

  // rate control value
  var.vt = VT_UI4;
  var.ulVal = 70;
  hr = codec->SetValue(&CODECAPI_AVEncCommonQuality, &var);
  //CHECK_HR(hr, "CODECAPI_AVEncCommonQuality");

  // Quality and Speed 트레이드 오프 값, 0에 가까울수록 속도가 빠름, 100에 가까울수록 품질이 좋음
  var.vt = VT_UI4;
  var.ulVal = 0;
  hr = codec->SetValue(&CODECAPI_AVEncCommonQualityVsSpeed, &var);
  //CHECK_HR(hr, "CODECAPI_AVEncCommonQualityVsSpeed");

  // GOP 사이즈
  var.vt = VT_UI4;
  var.ulVal = 256;
  hr = codec->SetValue(&CODECAPI_AVEncMPVGOPSize, &var);
  //CHECK_HR(hr, "CODECAPI_AVEncMPVGOPSize");

  // low latency mode
  var.vt = VT_BOOL;
  var.boolVal = VARIANT_TRUE;
  hr = codec->SetValue(&CODECAPI_AVLowLatencyMode, &var);
  //CHECK_HR(hr, "CODECAPI_AVLowLatencyMode");
}

void VideoEncoderHW::setInputType()
{
  HRESULT hr = S_OK;
  GUID id = { 0 };
  CComPtr<IMFMediaType> meida_type;
  for (int i = 0; !IsEqualGUID(id, MFVideoFormat_NV12); ++i)
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

  hr = meida_type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
  if (FAILED(hr))
  {
    printf("meida_type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive) failed");
    std::terminate();
  }

  hr = mft_->SetInputType(0, meida_type, 0);
  if (FAILED(hr))
  {
    printf("mft_->SetInputType failed");
    std::terminate();
  }
}

void VideoEncoderHW::setOutputType()
{
  HRESULT hr = S_OK;
  GUID id = { 0 };
  CComPtr<IMFMediaType> meida_type;

  for (int i = 0; !IsEqualGUID(id, MFVideoFormat_H264); ++i)
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

  hr = meida_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
  if (FAILED(hr))
  {
    printf(" meida_type->SetGUID(MF_MT_SUBTYPE) failed");
    std::terminate();
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

  hr = meida_type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
  if (FAILED(hr))
  {
    printf("SetUINT32 failed");
    std::terminate();
  }

  hr = meida_type->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_High);
  if (FAILED(hr))
  {
    printf("SetUINT32 failed");
    std::terminate();
  }

  hr = mft_->SetOutputType(0, meida_type, 0);
  if (FAILED(hr))
  {
    printf("mft_->SetOutputType failed");
    std::terminate();
  }
}

void VideoEncoderHW::setReady()
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


  hr = mft_->QueryInterface(&eg_);
  if (SUCCEEDED(hr))
  {
    hr = eg_->BeginGetEvent(this, nullptr);
    if (FAILED(hr))
    {
      printf("eventGenerator->BeginGetEvent failed");
      std::terminate();
    }
  }
}