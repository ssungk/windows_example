#include "VideoDecoderSW.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "Mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

VideoDecoderSW::VideoDecoderSW(int width, int height) :
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

VideoDecoderSW::~VideoDecoderSW()
{
  HRESULT hr = MFShutdown();
  if (FAILED(hr))
  {
    printf("MFShutdown failed\n");
    std::terminate();
  }
}

std::vector<uint8_t> VideoDecoderSW::Decode(std::vector<uint8_t> ibuffer)
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

  memcpy(p , ibuffer.data() , ibuffer.size());

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

  CComPtr<IMFSample> output_sample;
  hr = MFCreateSample(&output_sample);
  if (FAILED(hr))
  {
    printf("MFCreateSample failed");
  }

  // Buffer ����
  hr = MFCreateMemoryBuffer(width_ * height_ * 4, &media_obuf);
  if (FAILED(hr))
  {
    printf("MFCreateMemoryBuffer failed");
  }

  // Sample�� Buffer �߰�
  hr = output_sample->AddBuffer(media_obuf);
  if (FAILED(hr))
  {
    printf("sample_->AddBuffer failed");
  }

  DWORD status = 0;
  MFT_OUTPUT_DATA_BUFFER output_data = { 0, output_sample, 0, nullptr };
  hr = mft_->ProcessOutput(0, 1, &output_data, &status);
  if (FAILED(hr))
  {
    printf("mft->ProcessOutput failed");
  }

  p = NULL;
  DWORD length = 0;
  hr = media_obuf->Lock(&p,NULL, &length);
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


void VideoDecoderSW::init()
{
  // ���ڴ��� input Ÿ���� ���� �����ʿ� ����!!
  findDecoder();
  setDecoderOption();
  setInputType();
  setOutputType();  
}

void VideoDecoderSW::findDecoder()
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

void VideoDecoderSW::setDecoderOption()
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

void VideoDecoderSW::setInputType()
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

void VideoDecoderSW::setOutputType()
{
  HRESULT hr = S_OK;
  GUID id = { 0 };
  CComPtr<IMFMediaType> meida_type;

  for (int i = 0; !IsEqualGUID(id, MFVideoFormat_I420); ++i)
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

  hr = meida_type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_I420);
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

  hr = mft_->SetOutputType(0, meida_type, 0);
  if (FAILED(hr))
  {
    printf("mft_->SetOutputType failed");
    std::terminate();
  }
}